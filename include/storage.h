#include <stdio.h>
#include "parser.h"

/* Open or create the binary database file. */
FILE* db_open(const char* filename);

/* Close the database file handle. */
void db_close(FILE* file);

/* Append one row at the end of the file. */
int write_row(FILE* file, const Row* row);

/* Read one row from the current file position. */
int read_row(FILE* file, Row* row);

/* Mark a row as deleted using id. */
int delete_row(FILE* file, int id);

/* Return non-zero if an active row with id exists. */
int id_exists(FILE* file, int id);

/* Return the number of active (non-deleted) rows in the file. */
int count_active_rows(FILE* file);

/* Find one active row by id and optionally return its file index. */
int find_active_row_by_id(FILE* file, int id, Row* out_row, int* out_index);

/* Overwrite one row at a known file index. */
int write_row_at(FILE* file, int index, const Row* row);