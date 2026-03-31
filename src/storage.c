#include <stdio.h>
#include <stdlib.h>
#include "storage.h"

static int find_row_index(FILE* file, int id, Row* out_row);

/*
 * File storage layer.
 * Handles opening the database file and binary row read/write operations.
 */

/* Use r+b to support in-place updates required by delete. */
FILE* db_open(const char* filename)
{
    FILE* file = fopen(filename, "r+b");
    if (file == NULL)
    {
        file = fopen(filename, "w+b");
        if (file == NULL)
        {
            printf("Unable to open file\n");
            exit(EXIT_FAILURE);
        }
    }
    return file;
}

void db_close(FILE* file)
{
    fclose(file);
}

int write_row(FILE* file, const Row* row)
{
    if (fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }

    if (fwrite(row, sizeof(Row), 1, file) != 1) {
        return 0;
    }

    return fflush(file) == 0;
}

int read_row(FILE* file, Row* row)
{
    return fread(row, sizeof(Row), 1, file) == 1;
}

int delete_row(FILE* file, int id)
{
    int index = -1;
    Row row;

    if (!find_active_row_by_id(file, id, &row, &index)) {
        return 0;  /* not found */
    }

    row.is_deleted = 1;
    return write_row_at(file, index, &row);
}

int id_exists(FILE* file, int id)
{
    return find_active_row_by_id(file, id, NULL, NULL);
}

int count_active_rows(FILE* file)
{
    Row row;
    int count = 0;

    rewind(file);
    clearerr(file);

    while (read_row(file, &row)) {
        if (!row.is_deleted) {
            count++;
        }
    }

    return count;
}

int find_active_row_by_id(FILE* file, int id, Row* out_row, int* out_index)
{
    Row row;
    int index = find_row_index(file, id, &row);

    if (index == -1)
        return 0;

    if (out_row != NULL) {
        *out_row = row;
    }

    if (out_index != NULL) {
        *out_index = index;
    }

    return 1;
}



/**
 * ==================
 * Helper functions
 * ==================
 */

/* Generic row search helper */
static int find_row_index(FILE* file, int id, Row* out_row)
{
    Row row;
    rewind(file);
    clearerr(file);
    int index = 0;
    
    while (read_row(file, &row))
    {
        if (row.id == id && row.is_deleted == 0) {
            if (out_row != NULL) {
                *out_row = row;
            }
            return index;
        }
        index++;
    }
    return -1;  /* not found */
}


/* Write row at specific file position */
int write_row_at(FILE* file, int index, const Row* row)
{
    if (fseek(file, (long)(index * sizeof(Row)), SEEK_SET) != 0) {
        return 0;
    }

    if (fwrite(row, sizeof(Row), 1, file) != 1) {
        return 0;
    }

    return fflush(file) == 0;
}