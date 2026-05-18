#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "database.h"
#include "parser.h"
#include "storage.h"

/* Forward declarations */
static void send_row(const Row* row, int output_fd, int full_vector);
static void* search_worker(void* arg);
static void swap_matches(Match* array, int i);
static void insert_match_in_sorted_array(Match* array, float distance, uint32_t id);
static void merge_thread_results(Match* final_results, const Match* thread_results);

/* ============================================
 * PUBLIC DATABASE OPERATIONS
 * ============================================ */

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
            send_row(&row, output_fd, 0);
        }
    }
}

void executeSelectOne(Table* table, uint32_t id, int output_fd)
{
    Row row;
    
    if (find_active_row_by_id(table, id, &row, NULL)) {
        send_row(&row, output_fd, 1);
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

void executeSearch(Table* table, const float* query_vector, int conn_fd)
{
    if (table->num_rows == 0) {
        return;
    }

    /* Initialize thread arguments */
    pthread_t threads[THREAD_NUMBER];
    search_worker_args_t args[THREAD_NUMBER];
    int rows_per_thread = (table->num_rows + THREAD_NUMBER - 1) / THREAD_NUMBER;

    for (int i = 0; i < THREAD_NUMBER; i++) {
        args[i].table = table;
        args[i].query_vector = query_vector;
        args[i].start_row = i * rows_per_thread;
        args[i].end_row = (i + 1 < THREAD_NUMBER) ? (i + 1) * rows_per_thread : table->num_rows;

        if (pthread_create(&threads[i], NULL, search_worker, &args[i]) != 0) {
            perror("pthread_create");
        }
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < THREAD_NUMBER; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Merge results from all threads */
    Match final_results[MATCH_SIZE];
    for (int i = 0; i < MATCH_SIZE; i++) {
        final_results[i].distance = INFINITY;
        final_results[i].id = 0;
    }

    for (int i = 0; i < THREAD_NUMBER; i++) {
        merge_thread_results(final_results, args[i].local_result);
    }

    print_top_search(conn_fd, final_results);
}

/* ============================================
 * HELPER FUNCTIONS
 * ============================================ */

float calculateDistance(const float* v1, const float* v2)
{
    float sum = 0.0;
    for (int i = 0; i < DIMENSION; i++) {
        float diff = v1[i] - v2[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

/**
 * Send row to output with optional full vector display
 * full_vector: 1 for complete vector, 0 for abbreviated (first 3 and last 3 elements)
 */
static void send_row(const Row* row, int output_fd, int full_vector)
{
    dprintf(output_fd, "ID: %u | Vector: [", row->id);
    
    if (!full_vector && DIMENSION > 6) {
        for (int i = 0; i < 3; i++) {
            dprintf(output_fd, "%.2f, ", row->vector[i]);
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

/**
 * Insert a match into the sorted array, maintaining top MATCH_SIZE results
 */
static void insert_match_in_sorted_array(Match* array, float distance, uint32_t id)
{
    if (distance >= array[MATCH_SIZE - 1].distance) {
        return;
    }

    array[MATCH_SIZE - 1].distance = distance;
    array[MATCH_SIZE - 1].id = id;

    for (int i = MATCH_SIZE - 1; i > 0; i--) {
        if (array[i].distance < array[i - 1].distance) {
            swap_matches(array, i);
        } else {
            break;
        }
    }
}

/**
 * Swap two adjacent match entries in an array
 */
static void swap_matches(Match* array, int i)
{
    Match temp = array[i];
    array[i] = array[i - 1];
    array[i - 1] = temp;
}

/**
 * Merge thread results into final results array
 */
static void merge_thread_results(Match* final_results, const Match* thread_results)
{
    for (int i = 0; i < MATCH_SIZE; i++) {
        if (thread_results[i].distance != INFINITY) {
            insert_match_in_sorted_array(final_results, thread_results[i].distance, thread_results[i].id);
        }
    }
}

/**
 * Worker thread for parallel search
 */
static void* search_worker(void* arg)
{
    search_worker_args_t* args = (search_worker_args_t*)arg;
    Row row;

    /* Initialize local results */
    for (int i = 0; i < MATCH_SIZE; i++) {
        args->local_result[i].distance = INFINITY;
        args->local_result[i].id = 0;
    }

    /* Search assigned rows */
    for (uint32_t i = args->start_row; i < args->end_row; i++) {
        if (read_row(args->table, i, &row) && !row.is_deleted) {
            float dist = calculateDistance(args->query_vector, row.vector);
            insert_match_in_sorted_array(args->local_result, dist, row.id);
        }
    }

    return NULL;
}

/**
 * Print top search results
 */
void print_top_search(int conn_fd, const Match final_results[MATCH_SIZE])
{
    dprintf(conn_fd, MSG_TOP_SEARCH);
    for (int i = 0; i < MATCH_SIZE; i++) {
        if (final_results[i].distance == INFINITY) {
            continue;
        }
        dprintf(conn_fd, "%d. ID %u (Distance: %.4f)\n", i + 1, final_results[i].id, final_results[i].distance);
    }
    dprintf(conn_fd, RESPONSE_END_MARKER);
}
