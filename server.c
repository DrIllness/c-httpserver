#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define MAX_QUEUE 100

static const char *HOST_NAME = "127.0.0.1";
static const char *PORT = "8080";

struct addrinfo *setup_address(char *hostname, char *port);
int serve_client(int socket);

int main()
{
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
        exit(EXIT_FAILURE);
    }

    // bind
    if (bind(server_socket, server->ai_addr, server->ai_addrlen) < 0)
    {
        perror("failed to bind, exiting...\n");
        exit(EXIT_FAILURE);
    }

    // start listening
    if (listen(server_socket, MAX_QUEUE) < 0)
    {
        perror("failed to start listening, exiting...\n");
        exit(EXIT_FAILURE);
    }

    // main loop
    while (1)
    {
        int conn_socket = accept(server_socket, NULL, NULL);
        printf("serving %lu\n", conn_socket);
        serve_client(conn_socket);

        printf("closing connection for %lu\n", conn_socket);
        close(conn_socket);
    }

    // close listening socket
    close(server_socket);
}

struct addrinfo *setup_address(char *hostname, char *port)
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

int serve_client(int socket)
{
    char *message = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: 128\r\n\r\n<html>\n<body>\n<h1>Hello World</h1>\n<p>\nLet's see if this works\n</p>\n</body>\n</html>\n";
    write(socket, message, strlen(message) + 1);
    printf("served meesage to client %lu:\n %s\n", socket, message);
}