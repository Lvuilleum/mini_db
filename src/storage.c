#include <stdio.h>
#include "parser.h"
#include <stdlib.h>

/**
 * Gère le fichier de la base de donnée 
 * ouvre les fichier/lis les données/écris les données 
 * mémoire <-> fihcier 
 */

//a → append (écriture à la fin) / + → lecture + écriture -> a+
// PASSAGE à r+b pour que is_delete marche
 FILE* db_open(const char* filename)
 {
    FILE* file = fopen(filename, "r+b");
    if (file == NULL)
    {
        file = fopen(filename, "w+b");
        if (file == NULL)
        {
            printf("Unable to open file\n");
            exit(EXIT_FAILURE);
        }
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

int delete_row(FILE* file, int id)
{
    Row row;
    rewind(file);
    int index = 0;
    
    while (read_row(file ,&row))
    {
        if (row.id == id && row.is_deleted == 0)
        {
            row.is_deleted = 1;
            fseek(file, (long)(index * sizeof(Row)), SEEK_SET);
            fwrite(&row, sizeof(Row), 1, file);
            fflush(file);
            return 1; // succès
        }
        index++;
    }
    return 0; // id not found
}

int id_exists(FILE* file, int id)
{
    Row row;
    rewind(file);
    while (read_row(file, &row))
    {
        if (row.is_deleted == 0 && row.id == id)
            return 1; // found
    }
    return 0; // not found
}