#include "parser.h"

#define MAX_ROWS 1000

/* In-memory table used for basic runtime constraints. */
typedef struct {
    Row rows[MAX_ROWS];
    int num_rows;
} Table;

/* Database command executors. */
void executeInsert(Table* table, Statement* statement, FILE* file);


void executeSelect(FILE* file);
void executeSelectOne(FILE* file, int id);
void executeDelete(FILE* file, int id);
void executeUpdate(FILE* file, int id, char* new_name, int new_age);
