#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    char* type = strtok(entry, " ");

    if (type == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    if (strcmp(type, "insert") == 0) {
        statement->type = INSERT;
        return insertParse(statement);
    }

    if (strcmp(type, "select") == 0) {
        statement->type = SELECT;
        return PARSE_OK;
    }

    return PARSE_UNRECOGNIZED_STATEMENT;
}

int insertParse(Statement* statement)
{
    char* id_str = strtok(NULL, " ");
    char* name_str = strtok(NULL, " ");
    char* age_str = strtok(NULL, "\n");
    
    if (id_str == NULL || name_str == NULL || age_str == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    statement->row.id = atoi(id_str);
    strncpy(statement->row.username, name_str, MAX_USERNAME - 1);
    statement->row.username[MAX_USERNAME - 1] = '\0';
    statement->row.age = atoi(age_str);
    return PARSE_OK;
}