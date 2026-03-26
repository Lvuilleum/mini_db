#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "database.h"
#include "storage.h"

/* Print built-in command documentation for the CLI. */
static void print_help(void)
{
    printf("Available commands:\n");
    printf("  insert <id> <username> <age>\n");
    printf("  select\n");
    printf("  select <id>\n");
    printf("  delete <id>\n");
    printf("  update <id> <username> <age>\n");
    printf("  help or .help\n");
    printf("  .exit\n");
}

/* Program flow: user input -> parser -> database -> storage -> disk file. */
int main(void)
{
    char input[256];
    int running = 1;
    Table* table = calloc(1, sizeof(Table));
    FILE* db_file = db_open("database.db");

    if (table == NULL) {
        printf("Unable to allocate table\n");
        db_close(db_file);
        return EXIT_FAILURE;
    }

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

        if (strncmp(input, "help", 4) == 0 || strncmp(input, ".help", 5) == 0)
        {
            print_help();
            continue;
        }

        Statement statement;
        ParseResult parse_result = parse(input, &statement);

        if (parse_result != PARSE_OK) {
            if (parse_result == PARSE_UNRECOGNIZED_STATEMENT) {
                printf("Unrecognized command, type help for command list\n");
            } else {
                printf("Syntax error, type help for command list\n");
            }
            continue;
        }
        
        if (statement.type == INSERT)
        {
            printf("executed\n");
            executeInsert(table, &statement, db_file);

        } else if (statement.type == SELECT)
        {
            executeSelect(db_file);
        } else if (statement.type == SELECTONE)
        { 
            executeSelectOne(db_file, statement.row.id);
        } else if (statement.type == DELETE)
        {
            executeDelete(db_file, statement.row.id);
        } else if (statement.type == UPDATE)
        {
            executeUpdate(db_file, statement.row.id, statement.row.username, statement.row.age);
        } else {
            printf("Invalid command\n");
        }
    }

    free(table);
    return 0;
}
