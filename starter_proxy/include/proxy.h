#include "httpparser.h"
#include "customsocket.h"

#define INIT_BUF_SIZE 8192
#define MAX_CLIENTS FD_SETSIZE

struct client_struct
{
	int fd;
    int servfd;
	char *recv_buf;
    char *send_buf;
    size_t recv_buf_len;
    size_t recv_buf_size;
    size_t send_buf_len;
    size_t send_buf_size;
    size_t is_server;
    size_t sibling_idx;
    struct timespec ts;
    struct timespec tf;
    long long throughput;
    long long bestBitrate;
    char* method;
    char* URI;
    char* version;
    char* header;
};

typedef struct client_struct client;

struct request_struct{
    int seg;
    int frag;
    char *method;
    char *URI;
    char *version;
    char *path;
};


void calc_throughput(client **clients, size_t i, int* bitrates, double alpha);
int main(int argc, char* argv[]);