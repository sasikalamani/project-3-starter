/********************************************************
*  Author: Sannan Tariq                                *
*  Email: stariq@cs.cmu.edu                            *
*  Description: This code creates a simple             *
*                proxy with some HTTP parsing          *
*                and pipelining capabilities           *
*  Bug Reports: Please send email with                 *
*              subject line "15441-proxy-bug"          *
*  Complaints/Opinions: Please send email with         *
*              subject line "15441-proxy-complaint".   *
*              This makes it easier to set a rule      *
*              to send such mail to trash :)           *
********************************************************/



#include "proxy.h"
#include <time.h>

int bitrates[100];
int arrayP = 0; 


char* logFile;
double alpha;
unsigned short;
char* fake_ip;
char* dns_ip;
int dns_port;
char* www_ip;

struct request_struct *parse_line(client **clients, size_t i){
    char* met;
    char* u;
    char* ver;
    char* CRLF; char* intermediate;
    struct request_struct *req_line;
    int n;

    CRLF = memmem(clients[i]->recv_buf, clients[i]->recv_buf_len, "\r\n\r\n", strlen("\r\n\r\n"));

    CRLF = memmem(clients[i]->recv_buf, clients[i]->recv_buf_len, "\r\n", strlen("\r\n\r\n"));
    n = (size_t)(CRLF - clients[i]->recv_buf);

    intermediate = strndup(clients[i]->recv_buf, n);

    met = strtok(intermediate, " ");
    u = strtok(NULL, " ");
    ver = strtok(NULL, "\r\n");

    req_line->method = met;
    req_line->URI = u;
    req_line->version = ver;
    // fprintf(stdout, "%s\n", met);
    // fprintf(stdout, "%s\n", u);
    // fprintf(stdout, "%s\n", ver);

    return req_line;

}

void parse_uri(struct request_struct *req_line){
    char* uri = req_line->URI;
    char* find_end;
    char* intermediate;
    int n;

    find_end = strstr(uri, "/");
    while(find_end){
        memset(intermediate, 0, 1024);
        memcpy(intermediate, find_end, strlen(find_end));
        if(strstr(find_end+1,  "/") == NULL){
            n = (find_end - uri) + 1;
            break;
        }
        find_end = strstr(find_end +1 , "/");
        strncpy(intermediate, uri, n);
        strncat(intermediate, "\0", 1);
        memcpy(req_line->path, intermediate, strlen(intermediate));
    }

}
void parse_seg_frag(struct request_struct *req){
    char bit;
    char seq;
    char frag;

    int first; int last;

    while(isdigit((req->URI)[last])){
        last++;
    }

    strncpy(bit, req->URI + first, last - first);
    strncat(bit, "\0", 1);

    while(!isdigit((req->URI)[last])){
        last++;
    }
    first = last;

    while(isdigit((req->URI)[last])){
        last++;
    }

    strncpy(seq, req->URI + first, last - first);
    strncat(seq, "\0", 1);

    while(!isdigit((req->URI)[last])){
        last++;
    }
    first = last;

    strncpy(frag, req->URI + first, last - first);
    strncat(frag, "\0", 1);

    req->seg = atoi(seq);
    req->frag = atoi(frag);
}

long long timenow(){
    struct timeval cur;
    long long tv;
    gettimeofday(&cur, NULL);
    tv = cur.tv_sec;
    tv = tv * 1000000 * cur.tv_usec;
    return tv;
}

/*
 *  @REQUIRES:
 *  client_fd: The fd of the client you want to add
 *  is_server: A flag to tell us whether this client is a server or a requester
 *  sibling_idx: For a server it will be its client, for a client it will be its server
 *  
 *  @ENSURES: returns a pointer to a new client struct
 *
*/
client *new_client(int client_fd, int is_server, size_t sibling_idx) {
    client *new = calloc(1, sizeof(client));
    new->fd = client_fd;
    new->recv_buf = calloc(INIT_BUF_SIZE, 1);
    new->send_buf = calloc(INIT_BUF_SIZE, 1);
    new->recv_buf_len = 0;
    new->send_buf_len = 0;
    new->recv_buf_size = INIT_BUF_SIZE;
    new->send_buf_size = INIT_BUF_SIZE;
    new->sibling_idx = sibling_idx;
    return new;
}

void free_client(client* c) {
    free(c->recv_buf);
    free(c->send_buf);
    free(c);
    return;
}

/*
 *  @REQUIRES:
 *  client_fd: The fd of the client you want to add
 *  clients: A pointer to the array of client structures
 *  read_set: The set we monitor for incoming data
 *  is_server: A flag to tell us whether this client is a server or a requester
 *  sibling_idx: For a server it will be its client's index, for a client it will be its server index
 *  
 *  @ENSURES: Returns the index of the added client if possible, otherwise -1
 *
*/
int add_client(int client_fd, client **clients, fd_set *read_set, int is_server, size_t sibling_idx) {
    int i;

    // //bind to the server
    // int status, sock;
    // struct addrinfo hints; struct sockaddr_in fake;
    // struct addrinfo *servinfo; 
    // memset(&hints, 0, sizeof (hints));
    // hints.ai_family = AF_INET;     
    // hints.ai_socktype = SOCK_STREAM; 
    // hints.ai_flags = AI_PASSIVE;     

    // /* Bind the fake IP to the socket */

    // memset(&fake, 0, sizeof(fake));
    // fake.sin_family      = AF_INET;
    // fake.sin_addr.s_addr = inet_addr(fake_ip);
    // fake.sin_port        = 0;

    // /* Connect to the www_ip. */
    // if ((status = getaddrinfo(www_ip, "8080", &hints, &servinfo)) != 0)
    // {
    //     fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
    //     return EXIT_FAILURE;
    // }

    // if((sock = socket(servinfo->ai_family, servinfo->ai_socktype,
    //                   servinfo->ai_protocol)) == -1)
    // {
    //     fprintf(stderr, "Socket failed");
    //     return EXIT_FAILURE;
    // }

    // if (connect (sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    // {
    //     fprintf(stderr, "Connect");
    //         return EXIT_FAILURE;
    // }

    // /* Bind this socket to the fake-ip with an ephemeral port */
    // bind(sock, (struct sockaddr *) &fake, sizeof(fake));

    /* We now have a unique connection established for this client */
    // state->servfd = sock;
    // state->servst = calloc(sizeof(struct serv_rep), 1);

    // strncpy(state->serv_ip, webip, INET_ADDRSTRLEN);
    // freeaddrinfo(servinfo);

    for (i = 0; i < MAX_CLIENTS - 1; i ++) {
        if (clients[i] == NULL) {
            clients[i] = new_client(client_fd, is_server, sibling_idx);
            FD_SET(client_fd, read_set);
            return i;
        }
    }
    return -1;
}

/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  read_set: The set we monitor for incoming data
 *  write_set: The set we monitor for outgoing data
 *  
 *  @ENSURES: Removes the client and its sibling from all our data structures
 *
*/
int remove_client(client **clients, size_t i, fd_set *read_set, fd_set *write_set) {
    
    if (clients[i] == NULL) {
        return -1;
    }
    printf("Removing client on fd: %d\n", clients[i]->fd);
    int sib_idx;
    close(clients[i]->fd);
    FD_CLR(clients[i]->fd, read_set);
    FD_CLR(clients[i]->fd, write_set);
    sib_idx = clients[i]->sibling_idx;
    if (clients[sib_idx] != NULL) {
        printf("Removing client on fd: %d\n", clients[sib_idx]->fd);
        close(clients[sib_idx]->fd);
        FD_CLR(clients[sib_idx]->fd, read_set);
        FD_CLR(clients[sib_idx]->fd, write_set);
        free_client(clients[sib_idx]);
        clients[sib_idx] = NULL;
    }
    free(clients[i]);
    clients[i] = NULL;
    return 0;
}

int find_maxfd(int listen_fd, client **clients) {
    int max_fd = listen_fd;
    int i;
    for (i = 0; i < MAX_CLIENTS - 1; i ++) {
        if (clients[i] != NULL) {
            if (max_fd < clients[i]->fd) {
                max_fd = clients[i]->fd;
            }
        }
    }
    return max_fd;
}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  
 *  @ENSURES: 
 *  - tries to send the data present in a clients send buffer to that client
 *  - If data is sent, returns the remaining bytes to send, otherwise -1
 *
*/
int process_client_send(client **clients, size_t i) {
    int n;
    char *new_send_buffer;
    
    n = send(clients[i]->fd, clients[i]->send_buf, clients[i]->send_buf_len, 0);

    if (n <= 0) {
        return -1;
    }

    size_t new_size = max(INIT_BUF_SIZE, clients[i]->send_buf_len - n);
    new_send_buffer = calloc(new_size, sizeof(char));
    memcpy(new_send_buffer, clients[i]->send_buf + n, clients[i]->send_buf_len - n);
    free(clients[i]->send_buf);
    clients[i]->send_buf = new_send_buffer;
    clients[i]->send_buf_len = clients[i]->send_buf_len - n;
    clients[i]->send_buf_size = new_size;

    return clients[i]->send_buf_len;
}

/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  
 *  @ENSURES: 
 *  - tries to recv data from the client and updates its internal state as appropriate
 *  - If data is received, return the number of bytes received, otherwise return 0 or -1
 *
*/
int recv_from_client(client** clients, size_t i) {
    int n;
    char buf[INIT_BUF_SIZE];
    size_t new_size;

    n = recv(clients[i]->fd, buf, INIT_BUF_SIZE, 0);

    
    if (n <= 0) {
        return n;
    }

    new_size = clients[i]->recv_buf_size;

    while (n > new_size - clients[i]->recv_buf_len) {
        new_size *= 2;
        
    }
    clients[i]->recv_buf = resize(clients[i]->recv_buf, 
        new_size, clients[i]->recv_buf_size);
    clients[i]->recv_buf_size = new_size;

    memcpy(&(clients[i]->recv_buf[clients[i]->recv_buf_len]), buf, n);
    clients[i]->recv_buf_len += n;

    return n;
}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  buf: The message to add to the send buffer
 *  
 *  @ENSURES: 
 *  - appends data to the client's send buffer and returns the number of bytes appended
 *
*/
int queue_message_send(client **clients, size_t i, char *buf) {
    size_t n = strlen(buf);
    size_t new_size;

    new_size = clients[i]->send_buf_size;

    while (n > new_size - clients[i]->send_buf_len) {
        new_size *= 2;
        
    }
    clients[i]->send_buf = resize(clients[i]->send_buf, 
        new_size, clients[i]->send_buf_size);
    clients[i]->send_buf_size = new_size;

    memcpy(&(clients[i]->send_buf[clients[i]->send_buf_len]), buf, n);
    clients[i]->send_buf_len += n;
    return n;
}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  data_available: flag whether you can call recv on this client without blocking
 *  write_set: the set containing the fds to observe for being ready to write to
 *  
 *  @ENSURES: 
 *  - tries to read data from the client, then tries to reap a complete http message
 *      and finally tries to queue the message to be forwarded to its sibling
 *  - returns number of bytes queued if no errors, -1 otherwise
 *
*/
int process_client_read(client **clients, size_t i, int data_available, fd_set *write_set) {
    char *msg_rcvd;
    int nread;
    if (data_available == 1) {
        if ((nread = recv_from_client(clients, i)) < 0) {
            fprintf(stderr, "start_proxying: Error while receiving from client\n");
            return -1;
        }
        else if (nread == 0) {
            return -1;
        }
    }
    
    msg_rcvd = pop_message(&(clients[i]->recv_buf), &(clients[i]->recv_buf_len), &clients[i]->recv_buf_size);
    clients[i]->recv_buf = msg_rcvd;
    clients[i]->recv_buf_len = strlen(msg_rcvd);
    if (msg_rcvd == NULL) {
        return 0;
    }

    else {
        int sibling_idx = clients[i]->sibling_idx;
        int bytes_queued = queue_message_send(clients, sibling_idx, msg_rcvd);
        FD_SET(clients[sibling_idx]->fd, write_set);
        return bytes_queued;
    }

}

// void parse_msg(client c){
//     char* end = strstr(c)
// }


long long findGreatestBi(int* bitrates){
    long long maxBitrate = 0;
    int i;
    for(i=0; i<sizeof(bitrates); i++){
        if(bitrates[i]>maxBitrate){
            maxBitrate = bitrates[i];
        }
    }
    return maxBitrate;
}

void calc_throughput(client **clients, size_t i, int* bitrates, double alpha){
    long long elapsed_time;
    struct timespec *ts = &clients[i]->ts;
    struct timespec *tf = &clients[i]->tf;
    long long maxBitrate = 0;
    long long minBitrate = 10000000000;
    long long T;
    int j; 

    elapsed_time = (1000000000*(ts->tv_sec) + (ts->tv_nsec))  - (1000000000*(tf->tv_sec) + (tf->tv_nsec));

    T = ((clients[i]->recv_buf_size) * 8000000) / elapsed_time;

    clients[i]->throughput = alpha*T + (1-alpha)*clients[i]->throughput;

    for(j=0; j<sizeof(bitrates); j++){
        if(clients[i]->throughput > bitrates[j]*1.5){
            if(maxBitrate < bitrates[j]){
                maxBitrate = bitrates[j];
            }
        }
        if(minBitrate > bitrates[j]){
            minBitrate = bitrates[j];
        }
        //no bitrate > 1.5
        if(maxBitrate == 0){
            clients[i]->bestBitrate = findGreatestBi(bitrates);
        }
        else{
            clients[i]->bestBitrate = maxBitrate;
        }
    }

}

void parseVideo(client **clients, size_t i){
    char* haystack = clients[i]->recv_buf;
    size_t haystack_len = clients[i]->recv_buf_len;
    char* needle = NULL;
    char* str2 = NULL;
    char* string = "bitrate=";
    size_t needle_len = strlen("bitrate=");
    char val[100] = {0};
    int curBitrate;
    size_t bitrate_len;

    while((needle = memmem(haystack, haystack_len, string, needle_len)) != NULL){
        str2 = strstr(needle + needle_len + 1, "\"");
        bitrate_len = str2 - (needle + needle_len + 1); 
        bzero(val, 200);
        strncpy(val, needle + needle_len + 1, bitrate_len);
        curBitrate = atoi(val);

        //add bitrate into the array
        bitrates[arrayP] = curBitrate;
        arrayP += 1;

        haystack = str2;
    }

}

int main(int argc, char* argv[]) {
    int max_fd, nready, listen_fd;
    fd_set read_set, read_ready_set, write_set, write_ready_set;
    struct sockaddr_in cli_addr;
    socklen_t cli_size;
    client **clients;
    size_t i;
    int n;
    char buf[INIT_BUF_SIZE];
    struct request_struct *req_line;

    //command line arguments
    char* logFile = argv[1];
    double alpha = atof(argv[2]);
    unsigned short listen_port = atoi(argv[3]);
    char* fake_ip = argv[4];
    char* dns_ip = argv[5];
    int dns_port = atoi(argv[6]);
    char* www_ip = argv[7];

    //unsigned short listen_port = 8888;
    char *server_ip = www_ip;
    unsigned short server_port = 8080;
    char *my_ip = "0.0.0.0";


    int* bitrates;
    int client_fd;
    int test;

    //char* tester = 'GET /v1/vod/big_buck_bunny.f4m HTTP/1.1\r\n\r\n';
    
    printf("Starting the proxy...\n");

    if ((listen_fd = open_listen_socket(listen_port)) < 0) {
        fprintf(stderr, "start_proxy: Failed to start listening\n");
        return -1;
    }


    // init_multiplexer(listen_fd, clients, read_set, write_set);

    clients = calloc(MAX_CLIENTS - 1, sizeof(client*));
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(listen_fd, &read_set);
    
    max_fd = listen_fd;
    printf("Initiating select loop\n");
    while (1) {
        read_ready_set = read_set;
        write_ready_set = write_set;
        // printf("Watining to select...\n");
        nready = select(max_fd+1, &read_ready_set, &write_ready_set, NULL, NULL);

        if (nready > 0) {
            if (FD_ISSET(listen_fd, &read_ready_set)) {
                int client_idx;
                nready --;

                cli_size = sizeof(cli_addr);

                if ((client_fd = accept(listen_fd, (struct sockaddr *) &cli_addr,
                                    &cli_size)) == -1) {
                    fprintf(stderr, "start_proxying: Failed to accept new connection");
                }

                // add the client to the client_fd list of filed descriptors
                else if ((client_idx = add_client(client_fd, clients, &read_set, 0, -1))!= -1) {
                    
                    int sibling_fd = open_socket_to_server(my_ip, server_ip, server_port);
                    int server_idx = add_client(sibling_fd, clients, &read_set, 1, client_idx);
                    clients[client_idx]->sibling_idx = server_idx;
                    // clients[client_idx]->recv_buf = tester;
                    // clients[client_idx]->recv_buf_len = strlen(tester);
                    // test = parse_line(clients, client_idx);

                    printf("start_proxying: Connected to %s on FD %d\n"
                    "And its sibling %s on FD %d\n", inet_ntoa(cli_addr.sin_addr),
                        client_fd, server_ip, sibling_fd);

                }
                else
                    close(client_fd);         
            }





            for (i = 0; i < MAX_CLIENTS - 1 && nready > 0; i++) {
                if (clients[i] != NULL) {
                    int data_available = 0;
                    if (FD_ISSET(clients[i]->fd, &read_ready_set)) {
                        nready --;
                        data_available = 1;
                    }

                    int nread = process_client_read(clients, i, data_available, &write_set);

                    if (nread < 0) {
                        if (remove_client(clients, i, &read_set, &write_set) < 0) {
                            fprintf(stderr, "start_proxying: Error removing client\n");
                        }
                    }
                    // if(nread > 0){
                    //     fprintf(stdout, "%s %d\n", "Trying to parse", nread);
                    //     fprintf(stdout, "%s\n", clients[i]->recv_buf);
                    //     req_line = parse_line(clients, i);
                    //     fprintf(stdout, "%s %s %s\n", req_line->method, req_line->URI,
                    //                             req_line->version);
                    //     parse_uri(req_line);
                    //     parse_seg_frag(req_line);

                    // }

                    if (nread >= 0 && nready > 0) {
                        if (FD_ISSET(clients[i]->fd, &write_ready_set)) {
                            nready --;
                            int nsend = process_client_send(clients, i);
                            clock_gettime(CLOCK_MONOTONIC, &clients[i]->ts);
                            if (nsend < 0) {
                                if (remove_client(clients, i, &read_set, &write_set) < 0) {
                                    fprintf(stderr, "start_proxying: Error removing client\n");
                                }
                            }                    
                            else if (nsend == 0) {
                                FD_CLR(clients[i]->fd, &write_set);
                            }
                        }
                        else{
                            fprintf(stdout, "%s\n", "Going to the video portion");
                        }
                    }
                    //for media
                    // if(clients[i]->is_server){
                    //     fprintf(stdout, "%s\n", "Going to the video portion");

                    //     if(FD_ISSET(clients[i]->fd, &read_ready_set)){
                    //         nready --;
                    //         data_available = 1;
                    //         //n = recv_from_client(clients, i);
                    //         n = recv(clients[i]->fd, buf, INIT_BUF_SIZE, 0);
                    //         clock_gettime(CLOCK_MONOTONIC, &clients[i]->tf);
                    //         if(n > 0){
                    //             send(client_fd, buf, n, 0);
                    //             clients[i]->recv_buf_size = n;
                    //             //bitrate throughput
                    //             calc_throughput(clients, i, bitrates, alpha);
                    //             clients[i]->recv_buf_size = 0;
                    //         }
                    //     }
                    // }

                }
 
            }
            max_fd = find_maxfd(listen_fd, clients);
        }

    }
    
    return 0;
}
