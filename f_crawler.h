#define RESOURCE_AVAILABLE 1
#define RESOURCE_NOT_AVAILABLE 0

typedef int is_available;

is_available crawler_resource_is_available(char *path, char* home_dir);
