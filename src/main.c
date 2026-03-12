#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "database.h"

/**
 * Voici l'implementation du programme 
 * utilisateur -> main.c -> parser.c -> database.c
 * -> storage.c -> fichier disque 
 */




int main(void)
{
    /**
     * example logique : 
     * while (true)
     *  lire input
     *  parser commande 
     *  exécuter commande 
     */
    char input[256];
    int running = 1;
    Table* table = calloc(1, sizeof(Table));
    while (running)
    {
        printf("db>");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }
        Statement* statement = malloc(sizeof(Statement));

        parse(input, statement);
        if (statement->type == INSERT)
        {
            printf("this person %s with id %d of age %d has been selected\n",
                 statement->row.username, statement->row.id, statement->row.age);
            executeInsert();
        } else if (statement->type == SELECT)
        {
            executeSelect(table);
        } else {
            printf("Invalid command\n");
        }
        
        if (strncmp(input, ".exit", 5) == 0)
        {
            running = 0;
        }
    }
    return 0;
}
