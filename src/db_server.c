#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "parser.h"
#include "database.h"
#include "storage.h"
#include "protocol.h"

#define INPUT_BUFFER_SIZE 16384
#define RESPONSE_END_MARKER "<END>\n"

static int setup_server_socket(void);
static int send_text(int conn_fd, const char* msg);
static int send_response_end(int conn_fd);
static void execute_statement(Table* table, const Statement* statement, int conn_fd);
static void handle_client(int conn_fd, Table* table);

int main(void)
{
    int conn_fd;
    struct sockaddr_in client_addr;

    int sockfd = setup_server_socket();

    Table* table = db_open("database.db");

    printf("Serveur available on port %d\n", PORT);

    // Multithreading, when accept, look for the next client 
    while (1) {
        socklen_t addr_len = sizeof(client_addr);
        conn_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        printf("New client connected, spawning thread...\n");

        thread_args_t* args = malloc(sizeof(thread_args_t));
        args->conn_fd = conn_fd;
        args->table = table;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread_handler, (void*)args) != 0) {
            perror("Thread create");
            close(conn_fd);
            free(args);
        }
        pthread_detach(thread_id);
    }

    db_close(table);
    return 0;
}



/**
 * ==================
 * HELPER FUNCTIONS
 * ==================
 */
static int setup_server_socket(void)
{
    int sockfd;
    int opt = 1;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static int send_text(int conn_fd, const char* msg)
{
    size_t len = strlen(msg);
    ssize_t sent = send(conn_fd, msg, len, 0);
    return sent >= 0;
}

static int send_response_end(int conn_fd)
{
    return send_text(conn_fd, RESPONSE_END_MARKER);
}

static void execute_statement(Table* table, const Statement* statement, int conn_fd)
{
    switch (statement->type) {
    case INSERT:
        executeInsert(table, statement);
        (void)send_text(conn_fd, "row inserted with sucess\n");
        (void)send_response_end(conn_fd);
        break;
    case SELECT:
        executeSelect(table, conn_fd);
        (void)send_response_end(conn_fd);
        break;
    case SELECTONE:
        executeSelectOne(table, statement->row.id, conn_fd);
        (void)send_response_end(conn_fd);
        break;
    case DELETE:
        executeDelete(table, statement->row.id);
        (void)send_text(conn_fd, "deleted with success\n");
        (void)send_response_end(conn_fd);
        break;
    case UPDATE:
        executeUpdate(table, statement->row.id, statement->row.vector);
        (void)send_text(conn_fd, "updated with success\n");
        (void)send_response_end(conn_fd);
        break;
    case SEARCH:
        executeSearch(table, statement->row.vector, conn_fd);
        (void)send_response_end(conn_fd);
        break;
    default:
        (void)send_text(conn_fd, "Invalid command\n");
        (void)send_response_end(conn_fd);
        break;
    }
}

static void handle_client(int conn_fd, Table* table)
{
    char input[INPUT_BUFFER_SIZE];
    ssize_t n;

    while ((n = recv(conn_fd, input, sizeof(input) - 1, 0)) > 0) {
        Statement statement;
        ParseResult parse_result;

        input[n] = '\0';
        /* Debug: print raw received input to diagnose parse failures */
        fprintf(stderr, "DEBUG_RECV(%zd): %s\n", n, input);
        fflush(stderr);
        parse_result = parse(input, &statement);

        if (parse_result != PARSE_OK) {
            (void)send_text(conn_fd, "Syntax error\n");
            (void)send_response_end(conn_fd);
            continue;
        }

        execute_statement(table, &statement, conn_fd);
    }
}

static void* client_thread_handler(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;

    int fd = args->conn_fd;
    Table* table = args->table;

    handle_client(fd, table);

    printf("Thread: client disconnected\n");
    close(fd);
    free(args);

    return NULL; 
}