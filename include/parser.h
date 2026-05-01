#pragma once

#include <stdint.h>

# define DIMENSION 2

/* Database row persisted to disk. */
typedef struct {
    uint32_t id;
    float vector[DIMENSION];
    int is_deleted; /* 0 = active, 1 = deleted */
} Row;

/* Supported statement types. */
typedef enum {
    SELECT,
    SELECTONE,
    INSERT,
    DELETE,
    UPDATE,
    SEARCH
} StatementType;

typedef struct {
    StatementType type;
    Row row;
} Statement;


typedef enum {
    PARSE_OK = 0,
    PARSE_UNRECOGNIZED_STATEMENT,
    PARSE_SYNTAX_ERROR,
    PARSE_CONSTRAINT_ERROR
} ParseResult;


/* Parse one command line into a statement. */
int parse(char* entry, Statement* statement);
int insertParse(Statement* statement);
int deleteParse(Statement* statement);
int updateParse(Statement* statement);
int parse_select(Statement* statement);
int searchParse(Statement* statement);