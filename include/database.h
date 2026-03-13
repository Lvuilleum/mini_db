#include "parser.h"

#define MAX_ROWS 1000

/**
 * ================
 * STRUCTURES
 * ================
 */
typedef struct {
    Row rows[MAX_ROWS];
    int num_rows;
} Table;



/**
 * ===============
 * FUNCTIONS
 * ===============
 */
void executeInsert(Table* table, Statement* statement, FILE* file);


void executeSelect(FILE* file);
void executeSelectOne(FILE* file, int id);