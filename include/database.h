#include "parser.h"

#define MAX_ROWS 1000

/* Database command executors. */
void executeInsert(FILE* file, const Statement* statement);


void executeSelect(FILE* file);
void executeSelectOne(FILE* file, int id);
void executeDelete(FILE* file, int id);
void executeUpdate(FILE* file, int id, const char* new_name, int new_age);
