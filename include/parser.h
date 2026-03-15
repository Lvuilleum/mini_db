#pragma once

# define MAX_USERNAME 32 

/* Database row persisted to disk. */
typedef struct {
    int id;
    char username[MAX_USERNAME];
    int age;
    int is_deleted; /* 0 = active, 1 = deleted */
} Row;

/* Supported statement types. */
typedef enum {
    SELECT,
    SELECTONE,
    INSERT,
    DELETE
} StatementType;

typedef struct {
    StatementType type;
    Row row;
} Statement;


typedef enum {
    PARSE_OK = 0,
    PARSE_UNRECOGNIZED_STATEMENT,
    PARSE_SYNTAX_ERROR
} ParseResult;


/* Parse one command line into a statement. */
int parse(char* entry, Statement* statement);
int insertParse(Statement* statement);
int deleteParse(Statement* statement);