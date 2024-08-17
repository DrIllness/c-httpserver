#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "logger.h"

static FILE *log_file;

int is_dir_name_valid(const char *dir)
{
	if (dir == NULL || strlen(dir) == 0)
	{
		return 0; // invalid if directory name is NULL or empty
	}

	const char *invalid_chars = ":*?\"<>|";
	while (*dir)
	{
		if (strchr(invalid_chars, *dir) != NULL)
		{
			return 0; // invalid if any invalid character is found
		}
		dir++;
	}

	return 1; // valid directory name
}

int directory_exists(const char *path)
{
	struct stat info;
	if (stat(path, &info) != 0)
	{
		return 0; // path does not exist
	}
	else if (info.st_mode & S_IFDIR)
	{
		return 1; // path is a directory
	}
	else
	{
		return 0; // path exists but is not a directory
	}
}

int init_log(char *dir)
{
	if (!is_dir_name_valid(dir))
	{
		fprintf(stderr, "Invalid directory name: %s\n", dir);
		return -1;
	}

	if (!directory_exists(dir))
	{
		// try to create the directory
		if (mkdir(dir, 0777) != 0)
		{
			perror("Failed to create directory");
			return -1;
		}
	}

	// open log file in the specified directory
	char log_file_path[256];
	snprintf(log_file_path, sizeof(log_file_path), "%s/log.txt", dir);
	log_file = fopen(log_file_path, "a");
	if (log_file == NULL)
	{
		perror("Failed to open log file");
		return -1;
	}

	return 0;
}

void close_log_file()
{
	if (log_file != NULL)
	{
		fclose(log_file);
	}
}

int write_log(const char *message)
{
	if (log_file == NULL)
	{
		fprintf(stderr, "Log file is not open\n");
		return -1;
	}

	// get the current time
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char timestamp[64];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

	// write the timestamp and message to the log file
	fprintf(log_file, "[%s] %s\n", timestamp, message);
	fflush(log_file); // ensure the message is written immediately

	return 0; // success
}