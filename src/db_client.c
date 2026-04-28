#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "parser.h"
#include "database.h"
#include "storage.h"
#include "protocol.h"

#define RESPONSE_END_MARKER "<END>\n"


/* Print built-in command documentation for the CLI. */
static void print_help(void)
{
    printf("Availables commands:\n");
    printf("  insert <id> <username> <age>\n");
    printf("  select\n");
    printf("  select <id>\n");
    printf("  delete <id>\n");
    printf("  update <id> <username> <age>\n");
    printf("  help or .help\n");
    printf("  .exit\n");
}

static void read_response(int sockfd)
{
    char response[2048];
    size_t used = 0;

    while (1) {
        ssize_t n = recv(sockfd, response + used, sizeof(response) - 1 - used, 0);
        if (n > 0) {
            used += (size_t)n;
            response[used] = '\0';

            if (strstr(response, RESPONSE_END_MARKER) != NULL) {
                char* marker = strstr(response, RESPONSE_END_MARKER);
                *marker = '\0';
                printf("%s", response);
                return;
            }

            if (used == sizeof(response) - 1) {
                printf("%s", response);
                used = 0;
            }

        } else if (n == 0) {
            printf("Server closed connections\n");
            return;
        } else {
            perror("recv failed");
            return;
        }
    }
}

/* Program flow: user input -> parser -> database -> storage -> disk file. */
int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;

    char input[256];

    // 1. Creation of a socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Initialization of the server 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. connect to the server 
    if(connect(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server\n");


    // 4. Loop to read user input
    while (1)
    {
        printf("db> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (strchr(input, '\n') == NULL) {
            discard_remainder_of_line();
            printf("Input too long\n");
            continue;
        }

        if (is_exact_command(input, ".exit")) {
            break;
        }

        if (is_exact_command(input, "help") || is_exact_command(input, ".help")) {
            print_help();
            continue;
        }

        // network part 
        if ((send(sockfd, input, strlen(input), 0)) < 0) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }

        read_response(sockfd);
    }

    close(sockfd);
    return 0;
}

int is_exact_command(const char* input, const char* command)
{
    size_t command_len;
    size_t token_len;
    const char* start = input;

    while (*start == ' ' || *start == '\t') {
        start++;
    }

    command_len = strlen(command);
    token_len = strcspn(start, " \t\r\n");

    return token_len == command_len && strncmp(start, command, command_len) == 0;
}

void discard_remainder_of_line(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        ;
    }
}
