# define MAX_USERNAME 32 

/**
 * =============
 * Structure 
 * =============
 */
typedef struct {
    int id;
    char username[MAX_USERNAME];
    int age;
} Row;

typedef enum {
    SELECT,
    INSERT
} parserChoice;

typedef struct {
    parserChoice type;
    Row rowtoInsert;
} Statement;



/**
 * =============
 * Functions
 * =============
 */
int parse(char* entry, Statement* statement);