#include <stdio.h>

#include "database.h"
#include "parser.h"
#include "storage.h"


/**
 *  ce fichier : insère une ligne/affiche les lignes/
 *  vérifie si la table est pleine 
 *  parser -> database -> storage 
 */

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
    printf("No one has this id in the database\n");
}

void executeDelete(FILE* file, int id)
{
    if (delete_row(file, id))
    {
        printf("row %d deleted\n", id);
    } else {
        printf("No row with id %d found\n", id);
    }
}