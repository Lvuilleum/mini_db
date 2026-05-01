#include <stdio.h>
#include <string.h>
#include <math.h>
#include "database.h"
#include "parser.h"
#include "storage.h"

#define MSG_NOT_FOUND "No row with id %u found\n"
#define MSG_UPDATED "Row %u updated\n"
#define MSG_IO_ERROR "I/O error while accessing database file\n"

static void send_row_full(const Row* row, int output_fd);
static void send_row(const Row* row, int output_fd);
/* Execute validated statements against the persisted row store. */

void executeInsert(Table* table, const Statement* statement)
{
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
    Row row;
    float min_dist = INFINITY;
    uint32_t best_id = 0;
    int found = 0;

    for (uint32_t i = 0; i < table->num_rows; i++) {
        if (read_row(table, i, &row) && !row.is_deleted) {
            float dist = calculateDistance(query_vector, row.vector);

            if (dist < min_dist) {
                min_dist = dist;
                best_id = row.id;
                found = 1;
            }
        }
    }   
    if (found) {
        dprintf(conn_fd, "Closest match: ID %u (Distance: %.4f)\n", best_id, min_dist);
    } else {
        dprintf(conn_fd, "No data found.\n");
    }
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

static void send_row(const Row* row, int output_fd)
{
    printf("ID: %u | Vector: [", row->id);
    
    if (DIMENSION > 6) {
        for (int i = 0; i < 3; i++) {
            dprintf(output_fd, "%.2f%s", row->vector[i], ", ");
        }
        printf("... , ");
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