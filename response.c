#include "response.h"
#include "content_type.h"
#include "code.h"
#include <stdio.h>
#include <string.h>

int response_measure_len(response *response)
{
    int len = 0;

    len += strlen(response->header->protocol_version);
    len++;    // space
    len += 3; // code
    len++;    // space
    len += strlen(response->header->response_message);
    len += 1; // CRLF
    len += strlen(CONTENT_TYPE_KEY);
    len++; // space
    len += strlen(response->header->content_type);
    len++;    // space
    len += 2; // CRLF
    len += strlen(CONTENT_LENGTH_KEY);

    // number in header
    int tmp_size = response->header->content_size;
    while (1)
    {
        if (((tmp_size % 10) == 0) && ((tmp_size / 10) == 0))
            break;

        tmp_size /= 10;
        len++;
    }
    len += 4; // CRLF * 2

    // actual size
    len += response->header->content_size;

    return len;
}

// TODO add remaining switch case
char *code_to_str(ssize_t code)
{
    switch (code)
    {
    case HTTP_STATUS_OK:
        return "200";
    case HTTP_STATUS_NOT_FOUND:
        return "404";
    default:
        return "500"; // is it good idea to use server error code?
    }

    return "500"; // is it good idea to use server error code?
}

int response_to_str(response *response, char *str)
{
    strcat(str, response->header->protocol_version);
    strcat(str, SPACE);
    strcat(str, code_to_str(response->header->code));
    strcat(str, SPACE);
    strcat(str, response->header->response_message);
    strcat(str, CRLF);
    strcat(str, CONTENT_TYPE_KEY);
    strcat(str, response->header->content_type);
    strcat(str, CRLF);
    strcat(str, CONTENT_LENGTH_KEY);
    strcat(str, SPACE);

    char len_str[15]; // 15digit len at max
    sprintf(len_str, "%d", response->header->content_size);

    strcat(str, len_str);
    strcat(str, CRLF);
    strcat(str, CRLF);
    strcat(str, response->body->content);

    return 0; // assuming all is good
}