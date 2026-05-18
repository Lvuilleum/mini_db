#include "parser.h"
#include "storage.h"


#define MSG_NOT_FOUND "No row with id %u found\n"
#define MSG_UPDATED "Row %u updated\n"
#define MSG_IO_ERROR "I/O error while accessing database file\n"
#define MSG_TOP_SEARCH "--- Top Search Results ---\n"
#define RESPONSE_END_MARKER "<END>\n"
#define MATCH_SIZE 3
#define THREAD_NUMBER 5

typedef struct {
    uint32_t id;
    float distance;
} Match;

typedef struct {
    Table* table;
    const float* query_vector;
    uint32_t start_row;
    uint32_t end_row;
    Match local_result[MATCH_SIZE];
} search_worker_args_t;



/* Database command executors */
void executeInsert(Table* table, const Statement* statement);
void executeSelect(Table* table, int output_fd);
void executeSelectOne(Table* table, uint32_t id, int output_fd);
void executeDelete(Table* table, uint32_t id);
void executeUpdate(Table* table, uint32_t id, const float* new_vector);
void executeSearch(Table *table, const float *query_vector, int conn_fd);

/* Helper functions */
float calculateDistance(const float* v1, const float* v2);
void print_top_search(int conn_fd, const Match final_results[MATCH_SIZE]);