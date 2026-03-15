#include <stdio.h>
#include "parser.h"

/* Open or create the binary database file. */
FILE* db_open(const char* filename);

/* Close the database file handle. */
void db_close(FILE* file);

/* Append one row at the end of the file. */
void write_row(FILE* file, Row* row);

/* Read one row from the current file position. */
int read_row(FILE* file, Row* row);

/* Mark a row as deleted using id. */
int delete_row(FILE* file, int id);

/* Return non-zero if an active row with id exists. */
int id_exists(FILE* file, int id);