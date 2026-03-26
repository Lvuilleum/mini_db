#include <stdio.h>
#include <string.h>

#include "database.h"
#include "parser.h"
#include "storage.h"

/* Execute validated statements against the persisted row store. */

void executeInsert(Table* table, Statement* statement, FILE* file)
{
    if (table->num_rows >= MAX_ROWS) {
        printf("Error: table is full\n");
        return;
    }
    if (id_exists(file, statement->row.id)) {
        printf("Error: id %d already exists\n", statement->row.id);
        return;
    }
    statement->row.is_deleted = 0;
    table->rows[table->num_rows] = statement->row;
    table->num_rows++;
    write_row(file, &statement->row);
}

void executeSelect(FILE* file)
{
    Row row;
    rewind(file);
    
    while (read_row(file, &row))
    {
        if (!row.is_deleted)
            printf("%d %s %d\n", row.id, row.username, row.age);
    }
}

void executeSelectOne(FILE* file, int id)
{
    Row row;
    rewind(file);
    while (read_row(file, &row))
    {
        if (row.id == id && row.is_deleted == 0)
        {
            printf("%d %s %d\n", row.id, row.username, row.age);
            return;
        }
    }
    printf("No row with id %d found\n", id);
}

void executeDelete(FILE* file, int id)
{
    if (delete_row(file, id))
    {
        printf("Row %d deleted\n", id);
    } else {
        printf("No row with id %d found\n", id);
    }
}

void executeUpdate(FILE* file, int id, char* new_name, int new_age)
{
    Row row;
    rewind(file);
    clearerr(file);
    int index = 0;
    
    while (read_row(file, &row))
    {
        if (row.id == id && row.is_deleted == 0)
        {
            strncpy(row.username, new_name, MAX_USERNAME - 1);
            row.username[MAX_USERNAME - 1] = '\0';
            row.age = new_age;
            fseek(file, (long)(index * sizeof(Row)), SEEK_SET);
            fwrite(&row, sizeof(Row), 1, file);
            fflush(file);
            printf("Row %d updated\n", id);
            return;
        }
        index++;
    }
    printf("No row with id %d found\n", id);
}