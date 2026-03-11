#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <parser.h>
#include <database.h>

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
    while (running)
    {
        printf("db>");
        fget(input, sizeof(char), stdin);
        Statement* statement = malloc(sizeof(Statement));

        parse(input, &statement);
        if (statement->type == INSERT)
        {
            executeInsert();
        } else if (statement->type == SELECT)
        {
            executeSelect();
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
