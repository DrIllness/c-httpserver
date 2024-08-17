#include "response.h"
#include "content_type.h"
#include <stdlib.h>
#include <stdio.h>

int main()
{
    response r;
    body b;
    header h;
    h.code = 200;
    h.content_size = 6;
    h.protocol_version = PROTOCOL_VERSION;
    h.response_message = "OK";
    h.content_type = CONTENT_TYPE_TEXT_HTML;
    b.content = "Kekus";

    r.body = &b;
    r.header = &h;

    printf("measuring response len...\n");
    int len = response_measure_len(&r);
    printf("response len is %d\n", len);

    printf("malloc string\n");
    char* str = malloc(len);
    printf("converting response to raw string...\n");
    response_to_str(&r, str);
    printf("raw string: %s\n", str);
}