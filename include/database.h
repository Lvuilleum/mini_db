#include "parser.h"
#include "storage.h"



typedef struct {
    uint32_t id;
    float distance;
} Match;

typedef struct {
    Table* table;
    const float* query_vector;
    uint32_t start_row;
    uint32_t end_row;
    // Résultats locaux du thread
    int local_top_ids[3];
    float local_top_distances[3];
} search_worker_args_t;



/* Database command executors. */
void executeInsert(Table* table, const Statement* statement);


void executeSelect(Table* table, int output_fd);
void executeSelectOne(Table* table, uint32_t id, int output_fd);
void executeDelete(Table* table, uint32_t id);
void executeUpdate(Table* table, uint32_t id, const float* new_vector);
void executeSearch(Table* table, const float* query_vector, int conn_fd);
float calculateDistance(const float* v1, const float* v2);