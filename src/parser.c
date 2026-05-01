#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <limits.h>
#include "parser.h"

static int parse_uint32_token(const char* token, uint32_t* out);
static ParseResult parse_non_negative_uint32_token(const char* token, uint32_t* out);
static int parse_float_token(const char* token, float* out);
static int parse_id_vector(Statement* statement);

/**
 * ============
 * Functions
 * ============
 */

int parse(char* entry, Statement* statement)
{
    char* type = strtok(entry, " \t\r\n");

    if (type == NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    if (strcmp(type, "insert") == 0) {
        return insertParse(statement);
    }

    if (strcmp(type, "select") == 0) {
        return parse_select(statement);
    }

    if (strcmp(type, "delete") == 0) {
        return deleteParse(statement);
    }

    if (strcmp(type, "update") == 0) {
        return updateParse(statement);
    }

    if (strcmp(type, "search") == 0) {
        return searchParse(statement);
    }

    return PARSE_UNRECOGNIZED_STATEMENT;
}

int insertParse(Statement* statement)
{
    statement->type = INSERT;

    if (parse_id_vector(statement) != PARSE_OK)
        return PARSE_SYNTAX_ERROR;

    return PARSE_OK;
}


int deleteParse(Statement* statement)
{
    ParseResult parse_result;

    statement->type = DELETE;
    char* id_str = strtok(NULL, " \t\r\n");

    parse_result = parse_non_negative_uint32_token(id_str, &statement->row.id);
    if (parse_result != PARSE_OK)
        return parse_result;

    return PARSE_OK;
}


int updateParse(Statement* statement)
{
    statement->type = UPDATE;

    if (parse_id_vector(statement) != PARSE_OK)
        return PARSE_SYNTAX_ERROR;

    return PARSE_OK;
}

int parse_select(Statement* statement)
{
    char* id_str = strtok(NULL, " \t\r\n");
    char* extra_str = strtok(NULL, " \t\r\n");

    if (id_str == NULL) {
        statement->type = SELECT;
        return PARSE_OK;
    }

    if (extra_str != NULL) {
        return PARSE_SYNTAX_ERROR;
    }

    {
        ParseResult parse_result = parse_non_negative_uint32_token(id_str, &statement->row.id);
        if (parse_result != PARSE_OK)
            return parse_result;
    }

    statement->type = SELECTONE;
    return PARSE_OK;
}

int searchParse(Statement* statement) {
    statement->type = SEARCH;

    for (int i = 0; i < DIMENSION; i++) {
        char* val_token = strtok(NULL, " \t\r\n");
        if (val_token == NULL || !parse_float_token(val_token, &statement->row.vector[i])) {
            return PARSE_SYNTAX_ERROR;
        }
    }
    
    return PARSE_OK;
}


/**
 * =====================
 * Helper Functions
 * =====================
 */

static int parse_id_vector(Statement* statement)
{
    char* id_str = strtok(NULL, " \t\r\n");
    
    if (id_str == NULL) {
        return PARSE_SYNTAX_ERROR;
    }
    
    if (!parse_uint32_token(id_str, &statement->row.id)) {
        return PARSE_SYNTAX_ERROR;
    }

    for (int i = 0; i < DIMENSION; i++) {
        char* val_token = strtok(NULL, " \t\r\n");
        if (val_token == NULL || !parse_float_token(val_token, &statement->row.vector[i])) {
            return PARSE_SYNTAX_ERROR;
        }
    }
    
    return PARSE_OK;
}

/* Parse one integer token and validate full conversion and bounds. */
static int parse_uint32_token(const char* token, uint32_t* out)
{
    char* endptr = NULL;
    unsigned long value = 0;

    if (token == NULL || out == NULL) {
        return 0;
    }

    if (*token == '-') {
        return 0;
    }

    errno = 0;
    value = strtoul(token, &endptr, 10);
    if (*token == '\0' || *endptr != '\0' || errno == ERANGE || value > UINT32_MAX) {
        return 0;
    }

    *out = (uint32_t)value;
    return 1;
}

/* Parse one integer token and ensure it fits in uint32_t. */
static ParseResult parse_non_negative_uint32_token(const char* token, uint32_t* out)
{
    if (!parse_uint32_token(token, out)) {
        return PARSE_SYNTAX_ERROR;
    }

    return PARSE_OK;
}

static int parse_float_token(const char* token, float* out)
{
    char* endptr = NULL;
    if (token == NULL || out == NULL) return 0;

    *out = strtof(token, &endptr);
    
    // Vérifie si la conversion a échoué ou s'il y a des caractères invalides à la fin
    if (*token == '\0' || *endptr != '\0') {
        return 0;
    }
    return 1;
}