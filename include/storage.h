#pragma once

#include <stdint.h>
#include <pthread.h>
#include "parser.h"

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 10000
#define ROWS_PER_PAGE (PAGE_SIZE / sizeof(Row))
#define MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)
#define HEADER_PAGE 0
#define DATA_PAGES_RESERVED 100
#define INDEX_START_PAGE (DATA_PAGES_RESERVED + 1)

/**
 * We will have 3 zones : 
 *  Page 0 (header): contains metadata
 *  Page 1 to N : page of data (Row)
 *  Page N+1 to M : page of index (hash map)
 */

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES]; // The Cache
} Pager;

typedef struct {
    uint32_t key;
    uint32_t row_num;
    uint8_t state; /* 0 = empty, 1 = occupied, 2 = deleted */
} IndexEntry;

typedef struct {
    Pager* pager;
    uint32_t num_rows;
    IndexEntry* id_index;
    uint32_t id_index_capacity;
    pthread_mutex_t lock;
} Table;

typedef struct {
    uint32_t num_rows;
    uint32_t index_capacity;
    uint32_t root_page; // Utile pour le futur (B-Tree)
    // On peut ajouter un "Magic Number" pour vérifier que c'est bien notre fichier
    uint32_t magic_number; 
} DbHeader;


/* Open or create the binary database file. */
Table* db_open(const char* filename);

/* Close the database and flush pages to disk. */
void db_close(Table* table);

/* Append one row at the end of the file. */
int write_row(Table* table, const Row* row);

/* Read one row by index. */
int read_row(Table* table, uint32_t row_num, Row* row);

/* Mark a row as deleted using id. */
int delete_row(Table* table, uint32_t id);

/* Return non-zero if an active row with id exists. */
int id_exists(Table* table, uint32_t id);

/* Return the number of active (non-deleted) rows in the file. */
int count_active_rows(Table* table);

/* Find one active row by id and optionally return its file index. */
int find_active_row_by_id(Table* table, uint32_t id, Row* out_row, uint32_t* out_index);

/* Overwrite one row at a known file index. */
int write_row_at(Table* table, uint32_t row_num, const Row* row);

IndexEntry* get_index_entry_ptr(Table* table, uint32_t slot);