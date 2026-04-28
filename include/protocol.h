#pragma once

#define PORT 8080

void discard_remainder_of_line(void);
int is_exact_command(const char* input, const char* command);