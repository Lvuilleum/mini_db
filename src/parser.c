/**
 * Pasers transforme le texte 
 * en commande executable 
 * texte -> Structure C
 * 
 */

# define MAX_USERNAME 32 

 /**
  * Exemple de structure utilisable 
  */

typedef enum {
    SELECT,
    INSERT
} parserChoice;

typedef struct {
    parserChoice type;
    Row rowtoInsert;
} Statement;


 typedef struct {
    int id;
    char username[MAX_USERNAME];
    int age;
} Row;

/**
 * ================
 * Functions 
 * ================
 */

 int parse(char* entry, Statement* statement)
 {
    return 0;
 }