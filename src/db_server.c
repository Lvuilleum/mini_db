#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "parser.h"
#include "database.h"
#include "storage.h"
#include "protocol.h"

int main(void)
{
    int conn_fd;
    int sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int opt = 1;

    Table* table = db_open("database.db");

    // 1. Create socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    // 3. Option of reuse of port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // 3. Addres of server configuration 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 4. Bind
    if(bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Serveur available on port %d\n", PORT);

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);
        conn_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);
        if (conn_fd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Client connected\n");

        char input[1024];
        int n;

        // Boucle of dialogue with client
        while ((n = recv(conn_fd, input, sizeof(input)-1, 0)) > 0) {
            input[n] = '\0';

            Statement statement;
            ParseResult parse_result = parse(input, &statement);


            if (parse_result != PARSE_OK) {
                char* err_msg = "Syntaxe error\n";
                send(conn_fd, err_msg, strlen(err_msg), 0);
                continue;
            }

            if (statement.type == INSERT) {
                executeInsert(table, &statement);
                char *msg = "row inserted with sucess\n";
                send(conn_fd, msg, strlen(msg), 0);
            } else if (statement.type == SELECT) {
                executeSelect(table, conn_fd);
                char *msg = "selected with sucess\n";
                send(conn_fd, msg, strlen(msg), 0);
            } else if (statement.type == SELECTONE) {
                executeSelectOne(table, statement.row.id);
                char *msg = "select one with success\n";
                send(conn_fd, msg, strlen(msg), 0);
            } else if (statement.type == DELETE) {
                executeDelete(table, statement.row.id);
                char *msg = "deleted with success\n";
                send(conn_fd, msg, strlen(msg), 0);
            } else if (statement.type == UPDATE) {
                executeUpdate(table, statement.row.id, statement.row.username, statement.row.age);
                char *msg = "updated with success\n";
                send(conn_fd, msg, strlen(msg), 0);
            } else {
                char *msg = "Invalid command\n";
                send(conn_fd, msg, strlen(msg), 0);
            }
                }

       printf("client disconnected\n");
       close(conn_fd);
    }

    db_close(table);
    return 0;
}