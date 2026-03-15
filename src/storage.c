#include <stdio.h>
#include "parser.h"
#include <stdlib.h>

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
    Row row;
    rewind(file);
    clearerr(file);
    int index = 0;
    
    while (read_row(file ,&row))
    {
        if (row.id == id && row.is_deleted == 0)
        {
            row.is_deleted = 1;
            fseek(file, (long)(index * sizeof(Row)), SEEK_SET);
            fwrite(&row, sizeof(Row), 1, file);
            fflush(file);
            return 1;
        }
        index++;
    }
    return 0;
}

int id_exists(FILE* file, int id)
{
    Row row;
    rewind(file);
    clearerr(file);
    while (read_row(file, &row))
    {
        if (row.is_deleted == 0 && row.id == id)
            return 1;
    }
    return 0;
}