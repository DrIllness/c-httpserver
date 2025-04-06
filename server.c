#include <sys/socket.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
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
static const char* DEFAULT_WEBSITE_DIR_NAME = "web";

static int logs_expiration_date = 0; // default behaviour, no logging
static int logs_enabled = 0;
static char* web_dir_name = NULL;

static int pipe_fds[2];
static int server_socket = -1;
static tpool tp;
volatile sig_atomic_t stop_requested = 0; // global flag set on shutdown

struct addrinfo *setup_address(const char *hostname, const char *port);
int serve_client(int socket);
int init_threadpool(tpool *tp, int nthreads);
int free_threadpool(tpool *tp);
void try_to_log(char *msg);

void show_help(const char *prog_name)
{
	printf("Usage: %s -t threadpool_size -i ip_address -p port\n", prog_name);
	printf("Options:\n");
	printf("  -t    Size of the threadpool (integer).\n");
	printf("  -i    IP address (string).\n");
	printf("  -p    Port (string).\n");
	printf("  -h    Show this help message.\n");
	printf("  -l    Set expiration date for logs in seconds. Max value 259200s (3 days). By default is set to 0, which disables logging.\n");
	printf("  -w    Sets name for the web directory to look up in root dir.\n");
}

int is_ip_valid(char *ip) { return ip != NULL; }
int is_thread_pool_size_valid(int s) { return s > 0 && s < 10; }
int is_logs_expiration_valid(int s) { return s <= MAX_EXPIRATION_DATE; }
int is_logging_enabled(int s) { return s > 0; }
int is_port_valid(char *p) { return p != NULL; }

void parse_arguments(int argc, char *argv[], int *threadpool_size, char **ip_address, char **port, int *le)
{
	int opt;
	while ((opt = getopt(argc, argv, "t:i:p:hl:w:")) != -1)
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
			break;
		case 'w':
			web_dir_name = strdup(optarg);		
			break;
		default:
			fprintf(stderr, "Usage: %s -t threadpool_size -i ip_address -p port\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
}

void signal_handler(int signo)
{
	printf("Caught signal %d\n", signo);
	stop_requested = 1;
	write(pipe_fds[1], "x", 1);
}

void setup_signal_handler()
{
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char **argv)
{
	int threadpool_size = 0;
	int logs_expiration = 0;
	char *ip_address = NULL;
	char *port = NULL;

	parse_arguments(argc, argv, &threadpool_size, &ip_address, &port, &logs_expiration);

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
	if (web_dir_name == NULL)
	{
		web_dir_name = strdup(DEFAULT_WEBSITE_DIR_NAME);
	}

	printf("======================================\n");
	printf("Starting server with following config:\n");
	printf("  Threadpool Size: %d\n", threadpool_size);
	printf("  IP Address: %s\n", ip_address);
	printf("  Port: %s\n", port);
	printf("  Web site dir: %s\n", web_dir_name);
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

	// create the pipe used to signal shutdown
	if (pipe(pipe_fds) < 0)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	// make the write end non-blocking
	fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK); 
	setup_signal_handler();

	init_threadpool(&tp, threadpool_size);

	// get the server address and create the socket
	struct addrinfo *server = setup_address(ip_address, port);
	server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (server_socket < 0)
	{
		perror("Failed to create socket, exiting...\n");
		try_to_log("Failed to create socket, exiting...\n");
		close_log_file();
		exit(EXIT_FAILURE);
	}

	// enable socket reuse
	int optval = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		perror("Failed to set socket option, exiting...\n");
		try_to_log("Failed to set socket option, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}

	// bind the socket
	if (bind(server_socket, server->ai_addr, server->ai_addrlen) < 0)
	{
		perror("Failed to bind, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(server); // no longer needed

	// start listening
	if (listen(server_socket, MAX_QUEUE) < 0)
	{
		perror("Failed to start listening, exiting...\n");
		close_log_file();
		free_threadpool(&tp);
		exit(EXIT_FAILURE);
	}

	fd_set readfds;
	int maxfd = (pipe_fds[0] > server_socket ? pipe_fds[0] : server_socket);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(pipe_fds[0], &readfds);
		FD_SET(server_socket, &readfds);

		int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		if (ready < 0)
		{
			perror("Select error\n");
			try_to_log("Select error\n");
			break;
		}

		// if a shutdown signal has been received, break out of the loop
		if (FD_ISSET(pipe_fds[0], &readfds))
		{
			char buf;
			read(pipe_fds[0], &buf, 1); // consume the signal byte
			printf("\nReceived shutdown signal. Closing...\n");
			try_to_log("Received shutdown signal. Closing...");
			break;
		}

		// if there is a new connection, accept it and hand it off to the thread pool
		if (FD_ISSET(server_socket, &readfds))
		{
			int conn_socket = accept(server_socket, NULL, NULL);
			if (conn_socket >= 0)
				tp_execute(conn_socket, serve_client);
		}
	}

	// cleanup
	close(server_socket);
	close(pipe_fds[0]);
	close(pipe_fds[1]);
	free_threadpool(&tp);
	close_log_file();
	free(web_dir_name);

	return 0;
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
	void *read_msg = malloc(4096);
	if (!read_msg)
		return -1;

	read(socket, read_msg, 4096);

	request rqst;
	parse(read_msg, &rqst);

	if (rqst.type != GET)
		return -1; // we only handle GET requests for now

	char *file_to_open = malloc(64);
	if (rqst.uri == NULL || !strcmp(rqst.uri, "/")) rqst.uri = strdup("/index.html"); // fallback
	strcat(file_to_open, web_dir_name);
	strcat(file_to_open, rqst.uri);
	FILE *html = fopen(file_to_open, "r");

	char *response = malloc(4096);
	if (html == NULL)
	{
		strcpy(response, "HTTP/1.0 404 Not Found\r\n");
		try_to_log("Requested resources is not found");
	}
	else
	{
		char *htmlbuf = malloc(4096);
		int cread = fread(htmlbuf, sizeof(char), 4096, html);
		strcat(response, "HTTP/1.0 200 OK\r\n");
		strcat(response, "Content-Type: text/html\r\nContent-Length:");
		char *buffer = malloc(8);
		snprintf(buffer, 8, "%d", cread);
		strcat(response, buffer);
		strcat(response, "\r\n\r\n");
		strcat(response, htmlbuf);
		free(htmlbuf);
		free(buffer);
	}
	if (html)
		fclose(html);

	write(socket, response, strlen(response) + 1);
	try_to_log("Writing to socket");
	close(socket);
	free(read_msg);
	free(response);
	free(rqst.uri);

	return 0;
}

void try_to_log(char *msg)
{
	if (logs_enabled)
	{
		write_log(msg);
	}
}
