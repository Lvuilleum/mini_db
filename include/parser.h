#pragma once

# define MAX_USERNAME 32 

/**
 * =============
 * Structure 
 * =============
 */
typedef struct {
    int id;
    char username[MAX_USERNAME];
    int age;
} Row;

typedef enum {
    SELECT,
    INSERT
} parserChoice;

typedef struct {
    parserChoice type;
    Row row;
} Statement;


typedef enum {
    PARSE_OK = 0,
    PARSE_UNRECOGNIZED_STATEMENT,
    PARSE_SYNTAX_ERROR
} ParseResult;


/**
 * =============
 * Functions
 * =============
 */
int parse(char* entry, Statement* statement);
int insertParse(Statement* statement);