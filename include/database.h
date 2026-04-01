#include "parser.h"
#include "storage.h"

/* Database command executors. */
void executeInsert(Table* table, const Statement* statement);


void executeSelect(Table* table);
void executeSelectOne(Table* table, int id);
void executeDelete(Table* table, int id);
void executeUpdate(Table* table, int id, const char* new_name, int new_age);
