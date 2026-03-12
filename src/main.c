#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "database.h"
#include "storage.h"

/**
 * Voici l'implementation du programme 
 * utilisateur -> main.c -> parser.c -> database.c
 * -> storage.c -> fichier disque 
 */
int main(void)
{
    char input[256];
    int running = 1;
    Table* table = calloc(1, sizeof(Table));
    FILE* db_file = db_open("database.db");

    while (running)
    {
        printf("db> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        Statement* statement = malloc(sizeof(Statement));
        parse(input, statement);
        
        if (statement->type == INSERT)
        {
            printf("executed\n");
            executeInsert(table, statement, db_file);

        } else if (statement->type == SELECT)
        {
            executeSelect(db_file);

        } else {
            printf("Invalid command\n");
        }
        
        if (strncmp(input, ".exit", 5) == 0)
        {
            db_close(db_file);
            running = 0;
        }
    }
    return 0;
}
