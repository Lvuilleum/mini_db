#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "database.h"
#include "parser.h"
#include "storage.h"

#define MSG_NOT_FOUND "No row with id %u found\n"
#define MSG_UPDATED "Row %u updated\n"
#define MSG_IO_ERROR "I/O error while accessing database file\n"
#define MATCH_SIZE 3
#define THREAD_NUMBER 1

static void send_row_full(const Row* row, int output_fd);
static void send_row(const Row* row, int output_fd);
static void* search_worker(void* arg);
/* Execute validated statements against the persisted row store. */

void executeInsert(Table* table, const Statement* statement)
{
    pthread_mutex_lock(&table->lock);
    Row row_to_write;

    if (count_active_rows(table) >= (int)MAX_ROWS) {
        printf("Error: table is full\n");
        return;
    }

    if (id_exists(table, statement->row.id)) {
        printf("Error: id %u already exists\n", statement->row.id);
        return;
    }

    row_to_write = statement->row;
    row_to_write.is_deleted = 0;

    if (!write_row(table, &row_to_write)) {
        printf(MSG_IO_ERROR);
    }
    pthread_mutex_unlock(&table->lock);
}

void executeSelect(Table* table, int output_fd)
{
    Row row;

    for (uint32_t i = 0; i < table->num_rows; i++) {
        if (!read_row(table, i, &row)) {
            dprintf(output_fd, MSG_IO_ERROR);
            return;
        }
        if (!row.is_deleted) {
            send_row(&row, output_fd);
        }
    }
}

void executeSelectOne(Table* table, uint32_t id, int output_fd)
{
    Row row;
    
    if (find_active_row_by_id(table, id, &row, NULL))
    {
        send_row_full(&row, output_fd);
    } else {
        dprintf(output_fd, MSG_NOT_FOUND, id);
    }
}

void executeDelete(Table* table, uint32_t id)
{
    if (delete_row(table, id))
    {
        printf("Row %u deleted\n", id);
    } else {
        printf(MSG_NOT_FOUND, id);
    }
}

void executeUpdate(Table* table, uint32_t id, const float* new_vector)
{
    Row row;
    uint32_t index;
    
    if (find_active_row_by_id(table, id, &row, &index))
    {
        memcpy(row.vector, new_vector, sizeof(float)* DIMENSION);

        if (write_row_at(table, index, &row)) {
            printf(MSG_UPDATED, id);
        } else {
            printf(MSG_IO_ERROR);
        }
    } else {
        printf(MSG_NOT_FOUND, id);
    }
}

void executeSearch(Table* table, const float* query_vector, int conn_fd) {
    printf("num_rows = %u, THREAD_NUMBER = %d\n", table->num_rows, THREAD_NUMBER);
    if (table->num_rows == 0) return;
    
    pthread_t threads[THREAD_NUMBER];
    search_worker_args_t args[THREAD_NUMBER];
    
    int advance = (table->num_rows + THREAD_NUMBER - 1) / THREAD_NUMBER;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < THREAD_NUMBER; i++) {
        args[i].table = table;
        args[i].query_vector = query_vector;
        args[i].start_row = i * advance;
        args[i].end_row = (i+1) * advance;
        if (args[i].end_row > table->num_rows) args[i].end_row = table->num_rows;

        if (pthread_create(&threads[i], NULL, search_worker, &args[i]) != 0) {
            perror("pthread_create");
        }
    }

    for (size_t i = 0; i < THREAD_NUMBER; i++) {
        pthread_join(threads[i], NULL);
    }

    Match final_results[MATCH_SIZE];
    for (int i = 0; i < MATCH_SIZE; i++) {
        final_results[i].distance = INFINITY;
        final_results[i].id = 0;
    }

    for (size_t i = 0; i < THREAD_NUMBER; i++) {
        for (size_t j = 0; j < MATCH_SIZE; j++) {
            float dist = args[i].local_top_distances[j];
            uint32_t id = args[i].local_top_ids[j];

            if (dist == INFINITY) continue;

            if (dist < final_results[MATCH_SIZE - 1].distance) {
                final_results[MATCH_SIZE - 1].distance = dist;
                final_results[MATCH_SIZE - 1].id = id;

                for (int k = MATCH_SIZE - 1; k > 0; k--) {
                    if (final_results[k].distance < final_results[k - 1].distance) {
                        Match temp = final_results[k];
                        final_results[k] = final_results[k - 1];
                        final_results[k - 1] = temp;
                    } else break;
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double diff = (end.tv_sec - start.tv_sec) * 1000.0 + 
                  (end.tv_nsec - start.tv_nsec) / 1000000.0;

    dprintf(conn_fd, "Search completed in %.3f ms (using %d threads)\n", diff, THREAD_NUMBER);
    dprintf(conn_fd, "--- Top Search Results ---\n");
    
    for (int i = 0; i < MATCH_SIZE; i++) {
        if (final_results[i].distance == INFINITY) continue;
        dprintf(conn_fd, "%d. ID %u (Distance: %.4f)\n", i + 1, final_results[i].id, final_results[i].distance);
    }
    dprintf(conn_fd, "<END>\n");
}


float calculateDistance(const float* v1, const float* v2) {
    float sum = 0.0;
    for (int i = 0; i < DIMENSION; i++) {
        float diff = v1[i] - v2[i];
        sum += diff*diff;
    }
    return sqrtf(sum);
}


/**
 * ========================
 * Helper Functions
 * ========================
 */


static void* search_worker(void* arg) {
    search_worker_args_t* args = (search_worker_args_t*)arg;
    Row row;

    for (size_t i = 0; i < MATCH_SIZE; i++) {
        args->local_top_distances[i] = INFINITY;
        args->local_top_ids[i] = 0;
    }

    for (uint32_t i = args->start_row; i < args->end_row; i++) {
        if (read_row(args->table, i, &row) && !row.is_deleted) {
            float dist = calculateDistance(args->query_vector, row.vector);

            if (dist < args->local_top_distances[MATCH_SIZE-1]) {
                args->local_top_distances[MATCH_SIZE-1] = dist;
                args->local_top_ids[MATCH_SIZE-1] = row.id;

                for (int j = MATCH_SIZE-1; j > 0; j--) {
                    if (args->local_top_distances[j] < args->local_top_distances[j-1]) {
                        float td = args->local_top_distances[j];
                        int ti = args->local_top_ids[j];

                        args->local_top_distances[j] = args->local_top_distances[j - 1];
                        args->local_top_ids[j] = args->local_top_ids[j - 1];

                        args->local_top_distances[j - 1] = td;
                        args->local_top_ids[j - 1] = ti;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    
    return NULL;
}

static void send_row(const Row* row, int output_fd)
{
    dprintf(output_fd, "ID: %u | Vector: [", row->id);
    
    if (DIMENSION > 6) {
        for (int i = 0; i < 3; i++) {
            dprintf(output_fd, "%.2f%s", row->vector[i], ", ");
        }
        dprintf(output_fd, "... , ");
        for (int i = DIMENSION - 3; i < DIMENSION; i++) {
            dprintf(output_fd, "%.2f%s", row->vector[i], (i == DIMENSION - 1) ? "" : ", ");
        }
    } else {
        for (int i = 0; i < DIMENSION; i++) {
            dprintf(output_fd, "%.2f%s", row->vector[i], (i == DIMENSION - 1) ? "" : ", ");
        }
    }
    dprintf(output_fd, "]\n");
}

static void send_row_full(const Row* row, int output_fd)
{
    dprintf(output_fd, "ID: %u | Vector : [", row->id);
    for (int i = 0; i < DIMENSION; i++) {
            dprintf(output_fd, "%.2f%s", row->vector[i], ", ");
    }
    dprintf(output_fd, "]\n");
}