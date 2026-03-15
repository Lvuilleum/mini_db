#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "parser.h"

/* Parse one integer token and validate full conversion and bounds. */
static int parse_int_token(const char* token, int* out)
{
    char* endptr = NULL;
    long value = 0;

    if (token == NULL || out == NULL) {
        return 0;
    }

    value = strtol(token, &endptr, 10);
    if (*token == '\0' || *endptr != '\0' || value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

int parse(char* entry, Statement* statement)
{
    char* type = strtok(entry, " \t\r\n");

    if (type == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    if (strcmp(type, "insert") == 0) {
        statement->type = INSERT;
        return insertParse(statement);
    }

    if (strcmp(type, "select") == 0) {
        char* id_str = strtok(NULL, " \t\r\n");
        char* extra_str = strtok(NULL, " \t\r\n");

        if (id_str == NULL) {
            statement->type = SELECT;
            return PARSE_OK;
        }

        if (extra_str != NULL) {
            return PARSE_SYNTAX_ERROR;
        }

        if (!parse_int_token(id_str, &statement->row.id)) {
            return PARSE_SYNTAX_ERROR;
        }

        statement->type = SELECTONE;
        return PARSE_OK;
    }

    if (strcmp(type, "delete") == 0) {
        statement->type = DELETE;
        return deleteParse(statement);
    }

    return PARSE_UNRECOGNIZED_STATEMENT;
}

int insertParse(Statement* statement)
{
    char* id_str = strtok(NULL, " \t\r\n");
    char* name_str = strtok(NULL, " \t\r\n");
    char* age_str = strtok(NULL, " \t\r\n");
    
    if (id_str == NULL || name_str == NULL || age_str == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    if (!parse_int_token(id_str, &statement->row.id)) {
        return PARSE_SYNTAX_ERROR;
    }

    if (!parse_int_token(age_str, &statement->row.age)) {
        return PARSE_SYNTAX_ERROR;
    }

    strncpy(statement->row.username, name_str, MAX_USERNAME - 1);
    statement->row.username[MAX_USERNAME - 1] = '\0';
    return PARSE_OK;
}

int deleteParse(Statement* statement)
{
    char* id_str = strtok(NULL, " \t\r\n");

    if (!parse_int_token(id_str, &statement->row.id)) {
        return PARSE_SYNTAX_ERROR;
    }

    return PARSE_OK;
}