#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>

#define _GNU_SOURCE

#define INIT_BUF_SIZE 8192

int max(int a, int b);

int get_header_val(char *head, size_t head_len, char *key, size_t key_len, char *val);

char *pop_message(char **recv_buffer, size_t *recv_buffer_len, size_t *recv_buffer_size);

char *resize(char *buf, int new_len, int old_len);