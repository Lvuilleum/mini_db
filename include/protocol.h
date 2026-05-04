#pragma once

#define PORT 8080

typedef struct {
    int conn_fd;
    Table* table;
} thread_args_t;

void discard_remainder_of_line(void);
int is_exact_command(const char* input, const char* command);