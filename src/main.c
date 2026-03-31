#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "database.h"
#include "storage.h"

static int is_exact_command(const char* input, const char* command);
static void discard_remainder_of_line(void);

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

static void print_parse_error(ParseResult parse_result)
{
    if (parse_result == PARSE_UNRECOGNIZED_STATEMENT) {
        printf("Unrecognized command, type help for command list\n");
    } else if (parse_result == PARSE_CONSTRAINT_ERROR) {
        printf("Constraint error: id/age must be non-negative and username length must be 1-%d\n", MAX_USERNAME - 1);
    } else {
        printf("Syntax error, type help for command list\n");
    }
}

static void execute_statement(FILE* db_file, const Statement* statement)
{
    if (statement->type == INSERT) {
        executeInsert(db_file, statement);
    } else if (statement->type == SELECT) {
        executeSelect(db_file);
    } else if (statement->type == SELECTONE) {
        executeSelectOne(db_file, statement->row.id);
    } else if (statement->type == DELETE) {
        executeDelete(db_file, statement->row.id);
    } else if (statement->type == UPDATE) {
        executeUpdate(db_file, statement->row.id, statement->row.username, statement->row.age);
    } else {
        printf("Invalid command\n");
    }
}

/* Program flow: user input -> parser -> database -> storage -> disk file. */
int main(void)
{
    char input[256];
    FILE* db_file = db_open("database.db");

    while (1)
    {
        printf("db> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (strchr(input, '\n') == NULL) {
            discard_remainder_of_line();
            printf("Input too long\n");
            continue;
        }

        if (is_exact_command(input, ".exit")) {
            break;
        }

        if (is_exact_command(input, "help") || is_exact_command(input, ".help")) {
            print_help();
            continue;
        }

        Statement statement;
        ParseResult parse_result = parse(input, &statement);

        if (parse_result != PARSE_OK) {
            print_parse_error(parse_result);
            continue;
        }

        execute_statement(db_file, &statement);
    }

    db_close(db_file);
    return 0;
}

static int is_exact_command(const char* input, const char* command)
{
    size_t command_len;
    size_t token_len;
    const char* start = input;

    while (*start == ' ' || *start == '\t') {
        start++;
    }

    command_len = strlen(command);
    token_len = strcspn(start, " \t\r\n");

    return token_len == command_len && strncmp(start, command, command_len) == 0;
}

static void discard_remainder_of_line(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        ;
    }
}
