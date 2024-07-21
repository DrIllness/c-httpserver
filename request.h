#define GET_STR "GET"

enum request_type {
    GET,
    UNSUPPORTED 
};

typedef struct {
    enum request_type type;
    char* uri;
} request;

int parse(char* raw, request* request);