#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "request.h"

#define PARSING_FAILED -1

int parse(char *raw, request *request)
{
    // using 4kB for substrings, maybe revisit it?
    char *loc_buf = malloc(4096);
    if (loc_buf == NULL)
        return PARSING_FAILED;
        
    char *tmp_loc_buf = loc_buf;
    while ((*tmp_loc_buf++ = *raw++) != ' ')
        ;

    *--tmp_loc_buf = '\0'; // returning after 1 extra inc

	// default value
	request->uri = NULL;
    // set type
    if (strcmp(loc_buf, "GET") == 0)
        request->type = GET;
    else
        request->type = UNSUPPORTED;

    // parse path
    tmp_loc_buf = loc_buf; // reset tmp buf
    while ((*tmp_loc_buf++ = *raw++) != ' ')
        ;

    *--tmp_loc_buf = '\0'; // returning after 1 extra inc

    // copy path
    request->uri = strdup(loc_buf);

	return 0;
}