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

        if (strncmp(input, ".exit", 5) == 0)
        {
            db_close(db_file);
            running = 0;
            break;
        }

        Statement* statement = malloc(sizeof(Statement));
        ParseResult parse_result = parse(input, statement);

        if (parse_result != PARSE_OK) {
            if (parse_result == PARSE_UNRECOGNIZED_STATEMENT) {
                printf("Unrecognized command\n");
            } else {
                printf("Syntax error\n");
            }
            free(statement);
            continue;
        }
        
        if (statement->type == INSERT)
        {
            printf("executed\n");
            executeInsert(table, statement, db_file);

        } else if (statement->type == SELECT)
        {
            executeSelect(db_file);
        } else if (statement->type == SELECTONE)
        { 
            executeSelectOne(db_file, statement->row.id);
        } else {
            printf("Invalid command\n");
        }
        
        free(statement);
    }
    return 0;
}
