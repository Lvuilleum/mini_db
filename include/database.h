#include "parser.h"
#include "storage.h"



typedef struct {
    uint32_t id;
    float distance;
} Match;

/* Database command executors. */
void executeInsert(Table* table, const Statement* statement);


void executeSelect(Table* table, int output_fd);
void executeSelectOne(Table* table, uint32_t id, int output_fd);
void executeDelete(Table* table, uint32_t id);
void executeUpdate(Table* table, uint32_t id, const float* new_vector);
void executeSearch(Table* table, const float* query_vector, int conn_fd);
float calculateDistance(const float* v1, const float* v2);