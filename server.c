#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "threadpool.h"
#include "request.h"

#define MAX_QUEUE 100

static const char *HOST_NAME = "127.0.0.1";
static const char *PORT = "8080";

struct addrinfo *setup_address(const char *hostname, const char *port);
int serve_client(int socket);
int init_threadpool(tpool *tp, int nthreads);
int free_threadpool(tpool *tp);

int main()
{
    // create tpool
    tpool tp;
    init_threadpool(&tp, 5);

    // get address
    struct addrinfo *server = setup_address(HOST_NAME, PORT);

    // create
    int server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (server_socket < 0)
    {
        perror("failed to creat socket, exiting...\n");
        exit(EXIT_FAILURE);
    }

    // enable reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt)))
    {
        perror("failed to set option, exiting...\n");
        free_threadpool(&tp);
        exit(EXIT_FAILURE);
    }

    // bind
    if (bind(server_socket, server->ai_addr, server->ai_addrlen) < 0)
    {
        perror("failed to bind, exiting...\n");
        free_threadpool(&tp);
        exit(EXIT_FAILURE);
    }

    // start listening
    if (listen(server_socket, MAX_QUEUE) < 0)
    {
        perror("failed to start listening, exiting...\n");
        free_threadpool(&tp);
        exit(EXIT_FAILURE);
    }

    // main loop
    while (1)
    {
        int conn_socket = accept(server_socket, NULL, NULL);
        printf("serving %d\n", conn_socket);
        tp_execute(&tp, conn_socket, serve_client);
        printf("closing connection for %d\n", conn_socket);
    }

    // close listening socket and free pool
    close(server_socket);
    free_threadpool(&tp);
}

struct addrinfo *setup_address(const char *hostname, const char *port)
{
    struct addrinfo *server;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(hostname, port, &hints, &server);

    return server;
}

int init_threadpool(tpool *tp, int nthreads)
{
    tpconfig tpc;
    tpc.max_pool_size = nthreads;

    printf("init_threadpool, nthreads == %d\n", nthreads);
    tp_init(tp, &tpc);

    return 0;
}

int free_threadpool(tpool *tp)
{
    tp_free(tp);

    return 0;
}

int serve_client(int socket)
{
    void *read_msg = malloc(4096);
    // read request
    read(socket, read_msg, 4096);
    printf("\n\nRaw request: %s\n\n", read_msg);

    // check that it is enough space

    // parse into struct
    request rqst;
    parse(read_msg, &rqst);

    // check if it is get request
    if (rqst.type != GET)
        return -1; // we don't handle anything except for get atm

    // try to find resource
    // tmp solution
    char *path = malloc(100);
    strcpy(path, "/Users/eugendryl/Projects/c/cs162/hw2/web");
    FILE *html = fopen(strcat(path, rqst.uri), "r");

    char *response = malloc(4096); // tmp solution
    if (html == NULL)
    {
        printf("Resource %s not found...\n", path);
        strcpy(response, "HTTP/1.0 404 Not Fount\r\n");
    }
    else
    {
        char *htmlbuf = malloc(4096);
        int cread = fread(htmlbuf, sizeof(char), 4096, html);
        strcat(response, "HTTP/1.0 200 OK\r\n");
        strcat(response, "Content-Type: text/html\r\nContent-Length:");
        strcat(response, "128"); // fix content length
        strcat(response, "\r\n\r\n");
        strcat(response, htmlbuf);
    }
    fclose(html);

    write(socket, response, strlen(response) + 1);
    printf("served meesage to client %d:\n %s\n", socket, response);
    close(socket);
    printf("closing connection socket %d\n", socket);

    free(read_msg);

    return 0;
}