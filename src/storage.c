#include <stdio.h>
#include <stdlib.h>
#include "storage.h"

static int find_row_index(FILE* file, int id);
static void write_row_at(FILE* file, int index, Row* row);

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

void write_row(FILE* file, Row* row)
{
    fseek(file, 0, SEEK_END);
    fwrite(row, sizeof(Row), 1, file);
    fflush(file);
}

int read_row(FILE* file, Row* row)
{
    return fread(row, sizeof(Row), 1, file);
}

int delete_row(FILE* file, int id)
{
    int index = find_row_index(file ,id);
    if (index == -1)
        return 0;  /* not found */
    
    Row row;
    fseek(file, (long)(index * sizeof(Row)), SEEK_SET);
    read_row(file, &row);
    
    row.is_deleted = 1;
    write_row_at(file, index, &row);
    
    return 1;  /* success */
}

int id_exists(FILE* file, int id)
{
    return find_row_index(file, id) != -1;
}



/**
 * ==================
 * Helper functions
 * ==================
 */

/* Generic row search helper */
static int find_row_index(FILE* file, int id)
{
    Row row;
    rewind(file);
    clearerr(file);
    int index = 0;
    
    while (read_row(file, &row))
    {
        if (row.id == id && row.is_deleted == 0)
            return index;
        index++;
    }
    return -1;  /* not found */
}


/* Write row at specific file position */
static void write_row_at(FILE* file, int index, Row* row)
{
    fseek(file, (long)(index * sizeof(Row)), SEEK_SET);
    fwrite(row, sizeof(Row), 1, file);
    fflush(file);
}