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
        printf("%d %s %d\n", row.id, row.username, row.age);
    }
}

void executeSelectOne(FILE* file, int id)
{
    Row row;
    rewind(file);
    while (read_row(file, &row))
    {
        if (row.id == id)
        {
            printf("%d %s %d\n", row.id, row.username, row.age);
            return;
        }
    }
    printf("No one has this id in the database");
}