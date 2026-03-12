#include <stdio.h>
#include "parser.h"
#include <stdlib.h>

/**
 * Gère le fichier de la base de donnée 
 * ouvre les fichier/lis les données/écris les données 
 * mémoire <-> fihcier 
 */

//a → append (écriture à la fin) / + → lecture + écriture -> a+
 FILE* db_open(const char* filename)
 {
    FILE* file = fopen(filename, "a+");
    if (file == NULL)
    {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }
    
    return file;
 }

void db_close(FILE* file)
{
    fclose(file);
}

void write_row(FILE* file, Row* row)
{
    fwrite(row, sizeof(Row), 1, file);
}

int read_row(FILE* file, Row* row)
{
    return fread(row, sizeof(Row), 1, file);
}