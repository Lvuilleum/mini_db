#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "parser.h"

static int set_username(Row* row, const char* name);
static int parse_id_name_age(Statement* statement, char** name_out);
static int parse_int_token(const char* token, int* out);
static ParseResult parse_non_negative_int_token(const char* token, int* out);

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

    return PARSE_UNRECOGNIZED_STATEMENT;
}


int insertParse(Statement* statement)
{
    char* name_str = NULL;

    statement->type = INSERT;
    if (parse_id_name_age(statement, &name_str) != PARSE_OK)
        return PARSE_SYNTAX_ERROR;

    if (statement->row.id < 0 || statement->row.age < 0) {
        return PARSE_CONSTRAINT_ERROR;
    }

    if (!set_username(&statement->row, name_str)) {
        return PARSE_CONSTRAINT_ERROR;
    }

    return PARSE_OK;
}

int deleteParse(Statement* statement)
{
    ParseResult parse_result;

    statement->type = DELETE;
    char* id_str = strtok(NULL, " \t\r\n");

    parse_result = parse_non_negative_int_token(id_str, &statement->row.id);
    if (parse_result != PARSE_OK)
        return parse_result;

    return PARSE_OK;
}

int updateParse(Statement* statement)
{
    char* new_name = NULL;

    statement->type = UPDATE;
    if (parse_id_name_age(statement, &new_name) != PARSE_OK)
        return PARSE_SYNTAX_ERROR;

    if (statement->row.id < 0 || statement->row.age < 0) {
        return PARSE_CONSTRAINT_ERROR;
    }

    if (!set_username(&statement->row, new_name)) {
        return PARSE_CONSTRAINT_ERROR;
    }

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
        ParseResult parse_result = parse_non_negative_int_token(id_str, &statement->row.id);
        if (parse_result != PARSE_OK)
            return parse_result;
    }

    statement->type = SELECTONE;
    return PARSE_OK;
}


/**
 * =====================
 * Helper Functions
 * =====================
 */
static int set_username(Row* row, const char* name)
{
    size_t name_len;

    if (row == NULL || name == NULL) {
        return 0;
    }

    name_len = strlen(name);
    if (name_len == 0 || name_len >= MAX_USERNAME) {
        return 0;
    }

    strncpy(row->username, name, MAX_USERNAME - 1);
    row->username[MAX_USERNAME - 1] = '\0';
    return 1;
}

static int parse_id_name_age(Statement* statement, char** name_out)
{
    char* id_str = strtok(NULL, " \t\r\n");
    char* name_str = strtok(NULL, " \t\r\n");
    char* age_str = strtok(NULL, " \t\r\n");
    char* extra_str = strtok(NULL, " \t\r\n");
    
    if (id_str == NULL || name_str == NULL || age_str == NULL || extra_str != NULL || name_out == NULL) {
        return PARSE_SYNTAX_ERROR;
    }
    
    if (!parse_int_token(id_str, &statement->row.id)) {
        return PARSE_SYNTAX_ERROR;
    }
    
    if (!parse_int_token(age_str, &statement->row.age)) {
        return PARSE_SYNTAX_ERROR;
    }

    *name_out = name_str;
    
    return PARSE_OK;
}

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

/* Parse one integer token and ensure value is >= 0. */
static ParseResult parse_non_negative_int_token(const char* token, int* out)
{
    if (!parse_int_token(token, out)) {
        return PARSE_SYNTAX_ERROR;
    }

    if (*out < 0) {
        return PARSE_CONSTRAINT_ERROR;
    }

    return PARSE_OK;
}