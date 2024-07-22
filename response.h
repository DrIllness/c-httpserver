#include <stdlib.h>
#include "code.h"

#define CRLF "\r\n"
#define SPACE " "
#define CONTENT_TYPE_KEY "Content-Type:"
#define CONTENT_LENGTH_KEY "Content-Length:"
#define PROTOCOL_VERSION "HTTP/1.0"

typedef char *content_type;
typedef char *content;
typedef char *protocol_version;
typedef int response_code;
typedef char *response_message;
typedef size_t content_size;

typedef struct header
{
    response_code code;
    protocol_version protocol_version;
    response_message response_message;
    content_size content_size;
    content_type content_type;
} header;

typedef struct body
{
    content content;
} body;

typedef struct response
{
    header *header;
    body *body;
} response;

int response_measure_len(response *response);
int response_to_str(response *response, char *str);
