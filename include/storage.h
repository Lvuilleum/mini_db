#pragma once

#include <stdint.h>
#include "parser.h"

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
#define ROWS_PER_PAGE (PAGE_SIZE / sizeof(Row))
#define MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES]; // The Cache
} Pager;

typedef struct {
    Pager* pager;
    uint32_t num_rows;
} Table;


/* Open or create the binary database file. */
Table* db_open(const char* filename);

/* Close the database and flush pages to disk. */
void db_close(Table* table);

/* Append one row at the end of the file. */
int write_row(Table* table, const Row* row);

/* Read one row by index. */
int read_row(Table* table, uint32_t row_num, Row* row);

/* Mark a row as deleted using id. */
int delete_row(Table* table, int id);

/* Return non-zero if an active row with id exists. */
int id_exists(Table* table, int id);

/* Return the number of active (non-deleted) rows in the file. */
int count_active_rows(Table* table);

/* Find one active row by id and optionally return its file index. */
int find_active_row_by_id(Table* table, int id, Row* out_row, uint32_t* out_index);

/* Overwrite one row at a known file index. */
int write_row_at(Table* table, uint32_t row_num, const Row* row);