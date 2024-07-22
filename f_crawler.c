#include "f_crawler.h"

static char **abs_paths;
static int crawled;

static void crawl_fs(char *home_dir)
{
    // traverse home dir in filesystem
    // init abs_paths
}

static is_available traverse(char *path, char *home_dir);

is_available crawler_resource_is_available(char *path, char *home_dir)
{
    if (crawled)
    {
        crawl_fs(home_dir);
    }

    return (path, home_dir);
}

static is_available traverse(char *path, char *home_dir)
{
    // traverse through abs_path
    // return 1 if found, 0 if not found

    return 0;
}
