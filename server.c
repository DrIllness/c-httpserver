#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "threadpool.h"
#include "request.h"
#include "logger.h"

#define MAX_QUEUE 100
#define ENABLED 1
#define DISABLED 0

static const char *DEFAULT_HOST_NAME = "127.0.0.1";
static const char *DEFAULT_PORT = "8080";
static const int DEFAULT_THREAD_POOL_SIZE = 5;
static const int MAX_EXPIRATION_DATE = 259200;

static int logs_expiration_date = 0; // default behaviour, no loggin
static int logs_enabled = 0;

struct addrinfo *setup_address(const char *hostname, const char *port);
int serve_client(int socket);
int init_threadpool(tpool *tp, int nthreads);
int free_threadpool(tpool *tp);
void try_to_log(char *msg);

// function to display the help menu
void show_help(const char *prog_name)
{
	printf("Usage: %s -t threadpool_size -i ip_address -p port\n", prog_name);
	printf("Options:\n");
	printf("  -t    Size of the threadpool (integer)\n");
	printf("  -i    IP address (string)\n");
	printf("  -p    Port (string)\n");
	printf("  -h    Show this help message\n");
	printf("  -l    Set expiration date for logs in seconds. Max value 259200s (3 days). By default is set to 0, which disables logging.\n");
}

int is_ip_valid(char *ip) { return ip != NULL; }

int is_thread_pool_size_valid(int s) { return s > 0 && s < 10; }

int is_logs_expiration_valid(int s) { return s <= MAX_EXPIRATION_DATE; }

int is_logging_enabled(int s) { return s > 0; }

int is_port_valid(char *p) { return p != NULL; }

// function to parse command-line arguments
void parse_arguments(int argc, char *argv[], int *threadpool_size, char **ip_address, char **port, int *le)
{
	int opt;
	while ((opt = getopt(argc, argv, "t:i:p:h:l:")) != -1)
	{
		switch (opt)
		{
		case 'l':
			*le = atoi(optarg);
			break;
		case 't':
			*threadpool_size = atoi(optarg);
			break;
		case 'i':
			*ip_address = strdup(optarg);
			break;
		case 'p':
			*port = strdup(optarg);
			break;
		case 'h':
			show_help(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, "Usage: %s -t threadpool_size -i ip_address -p port\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv)
{
	int threadpool_size = 0;
	int logs_expiration = 0;
	char *ip_address = NULL;
	char *port = NULL;

	// parse command-line arguments
	parse_arguments(argc, argv, &threadpool_size, &ip_address, &port, &logs_expiration);

	// print parsed arguments for verification

	if (!is_ip_valid(ip_address))
	{
		ip_address = strdup(DEFAULT_HOST_NAME);
		printf("Missing valid ip address, defaulting to %s\n", DEFAULT_HOST_NAME);
	}
	if (!is_thread_pool_size_valid(threadpool_size))
	{
		threadpool_size = DEFAULT_THREAD_POOL_SIZE;
		printf("Missing valid thread pool size, defaulting to %d\n", DEFAULT_THREAD_POOL_SIZE);
	}
	if (is_logging_enabled(logs_expiration))
	{
		logs_enabled = ENABLED;
		if (is_logs_expiration_valid(logs_expiration))
		{
			logs_expiration_date = logs_expiration;
		}
		else
		{
			printf("Missing valid expiration date, defaulting to %ds\n", MAX_EXPIRATION_DATE);
		}
	}
	else
	{
		logs_enabled = DISABLED;
	}
	if (!is_port_valid(port))
	{
		port = strdup(DEFAULT_PORT);
		printf("Missing valid port, defaulting to %s\n", DEFAULT_PORT);
	}
	printf("======================================\n");
	printf("Starting server with following config:\n");
	printf("  Threadpool Size: %d\n", threadpool_size);
	printf("  IP Address: %s\n", ip_address);
	printf("  Port: %s\n", port);
	if (logs_enabled)
	{
		printf("  Logging: ENABLED\n  Logs expiration date: %d(s)\n", logs_expiration_date);
	}
	else
	{
		printf("  Logging: DISABLED\n");
	}

	if (logs_enabled)
	{
		if (init_log("logs") == 0)
		{
			printf("Inited logs\n");
		}
		else
		{
			perror("Failed initing log\n");
		}
	}

	// create tpool
	tpool tp;

	init_threadpool(&tp, threadpool_size);

	// get address
	struct addrinfo *server = setup_address(ip_address, port);

	// create socket
	int server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (server_socket < 0)
	{
		perror("Failed to creat socket, exiting...\n");
		exit(EXIT_FAILURE);
	}

	// enable reuse
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET,
				   SO_REUSEADDR, &opt,
				   sizeof(opt)))
	{
		perror("Failed to set option, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}

	// bind
	if (bind(server_socket, server->ai_addr, server->ai_addrlen) < 0)
	{
		perror("Failed to bind, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}
	// start listening
	if (listen(server_socket, MAX_QUEUE) < 0)
	{
		perror("Failed to start listening, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}
	
	// main loop
	while (1)
	{
		int conn_socket = accept(server_socket, NULL, NULL);
		tp_execute(conn_socket, serve_client);
	}

	// close listening socket and free pool
	close(server_socket);
	close_log_file();
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
	tp->config = &tpc;
	tp->curr_size = 0;
	tpc.max_pool_size = nthreads;

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
	//char log_message[256];
	//snprintf(log_message, sizeof(log_message), "Serving client %d", socket);
	//try_to_log(log_message);

	void *read_msg = malloc(4096);
	// read request
	read(socket, read_msg, 4096);

	//snprintf(log_message, sizeof(log_message), "\n\nRaw request: %s\n\n", (char *)read_msg);
	//try_to_log(log_message);

	// check that it is enough space

	// parse into struct
	request rqst;
	parse(read_msg, &rqst);

	// check if it is get request
	if (rqst.type != GET)
		return -1; // we don't handle anything except for get atm

	// try to find resource
	char *path = malloc(100);
	strcpy(path, "/Users/eugendryl/Projects/c-learning/cs162/c-server/web"); // fix const path
	
	FILE *html = fopen(strcat(path, "/index.html"), "r");

	char *response = malloc(4096);
	if (html == NULL)
	{
		//snprintf(log_message, sizeof(log_message), "Resource %s not found...\n", path);
		//try_to_log(log_message);
		strcpy(response, "HTTP/1.0 404 Not Found\r\n"); // ... and this also
	}
	else
	{
		char *htmlbuf = malloc(4096);
		int cread =  fread(htmlbuf, sizeof(char), 4096, html);
		strcat(response, "HTTP/1.0 200 OK\r\n");
		strcat(response, "Content-Type: text/html\r\nContent-Length:");
		
		char *buffer = malloc(8);
		snprintf(buffer, sizeof(buffer), "%d", cread);
		
		strcat(response, buffer); 
		strcat(response, "\r\n\r\n");
		strcat(response, htmlbuf);
	}
	fclose(html);

	ssize_t swrite = write(socket, response, strlen(response) + 1);
	//snprintf(log_message, sizeof(log_message), "served meesage to client %d:\n %s\n", socket, response);
	//try_to_log(log_message);
	close(socket);
	//snprintf(log_message, sizeof(log_message), "closing connection socket %d\n", socket);
	//try_to_log(log_message);

	free(read_msg);

	return 0;
}

void try_to_log(char *msg)
{
	if (logs_enabled)
	{
		write_log(msg);
	}
}