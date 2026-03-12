#include <stdio.h>
#include "parser.h"

FILE* db_open(const char* filename);

void db_close(FILE* file);

void write_row(FILE* file, Row* row);

int read_row(FILE* file, Row* row);