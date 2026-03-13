#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "parser.h"
/**
 * Pasers transforme le texte 
 * en commande executable 
 * texte -> Structure C
 * 
 */

/**
 * ================
 * Functions 
 * ================
 */

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

        char* endptr = NULL;
        long id = strtol(id_str, &endptr, 10);
        if (*id_str == '\0' || *endptr != '\0' || id < INT_MIN || id > INT_MAX) {
            return PARSE_SYNTAX_ERROR;
        }

        statement->row.id = (int)id;
        statement->type = SELECTONE;
        return PARSE_OK;
    }

    return PARSE_UNRECOGNIZED_STATEMENT;
}

int insertParse(Statement* statement)
{
    char* endptr = NULL;
    long id = 0;
    char* id_str = strtok(NULL, " \t\r\n");
    char* name_str = strtok(NULL, " \t\r\n");
    char* age_str = strtok(NULL, " \t\r\n");
    
    if (id_str == NULL || name_str == NULL || age_str == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    id = strtol(id_str, &endptr, 10);
    if (*id_str == '\0' || *endptr != '\0' || id < INT_MIN || id > INT_MAX) {
        return PARSE_SYNTAX_ERROR;
    }

    statement->row.id = (int)id;
    strncpy(statement->row.username, name_str, MAX_USERNAME - 1);
    statement->row.username[MAX_USERNAME - 1] = '\0';
    statement->row.age = atoi(age_str);
    return PARSE_OK;
}