#include <stdio.h>
#include <string.h>

#include "database.h"
#include "parser.h"
#include "storage.h"

#define MSG_NOT_FOUND "No row with id %d found\n"
#define MSG_UPDATED "Row %d updated\n"
#define MSG_IO_ERROR "I/O error while accessing database file\n"

static void print_row(const Row* row);
/* Execute validated statements against the persisted row store. */

void executeInsert(Table* table, const Statement* statement)
{
    Row row_to_write;

    if (count_active_rows(table) >= (int)MAX_ROWS) {
        printf("Error: table is full\n");
        return;
    }

    if (id_exists(table, statement->row.id)) {
        printf("Error: id %d already exists\n", statement->row.id);
        return;
    }

    row_to_write = statement->row;
    row_to_write.is_deleted = 0;
    if (!write_row(table, &row_to_write)) {
        printf(MSG_IO_ERROR);
    }
}

void executeSelect(Table* table)
{
    Row row;

    for (uint32_t i = 0; i < table->num_rows; i++) {
        if (!read_row(table, i, &row)) {
            printf(MSG_IO_ERROR);
            return;
        }
        if (!row.is_deleted) {
            print_row(&row);
        }
    }
}

void executeSelectOne(Table* table, int id)
{
    Row row;
    
    if (find_active_row_by_id(table, id, &row, NULL))
    {
        print_row(&row);
    } else {
        printf(MSG_NOT_FOUND, id);
    }
}

void executeDelete(Table* table, int id)
{
    if (delete_row(table, id))
    {
        printf("Row %d deleted\n", id);
    } else {
        printf(MSG_NOT_FOUND, id);
    }
}

void executeUpdate(Table* table, int id, const char* new_name, int new_age)
{
    Row row;
    uint32_t index;
    
    if (find_active_row_by_id(table, id, &row, &index))
    {
        strncpy(row.username, new_name, MAX_USERNAME - 1);
        row.username[MAX_USERNAME - 1] = '\0';
        row.age = new_age;

        if (write_row_at(table, index, &row)) {
            printf(MSG_UPDATED, id);
        } else {
            printf(MSG_IO_ERROR);
        }
    } else {
        printf(MSG_NOT_FOUND, id);
    }
}


/**
 * ========================
 * Helper Functions
 * ========================
 */

static void print_row(const Row* row)
{
    printf("%d %s %d\n", row->id, row->username, row->age);
}