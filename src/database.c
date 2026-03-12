#include <stdio.h>

#include "database.h"
#include "parser.h"

/**
 *  ce fichier : insère une ligne/affiche les lignes/
 *  vérifie si la table est pleine 
 *  parser -> database -> storage 
 */

void executeInsert(Table* table, Statement* statement)
{
    if (table->num_rows >= MAX_ROWS) {
        printf("Error: table is full\n");
        return;
    }
    table->rows[table->num_rows] = statement->row;
    table->num_rows++;
}

void executeSelect(Table* table)
{
    for (int i = 0; i < table->num_rows; i++)
    {
        Row row = table->rows[i];
        printf("%d %s %d\n", row.id, row.username, row.age);
    }
}