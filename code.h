#ifndef HTTP_STATUS_CODES_H
#define HTTP_STATUS_CODES_H

// Informational responses
#define HTTP_STATUS_CONTINUE 100
#define HTTP_STATUS_SWITCHING_PROTOCOLS 101
#define HTTP_STATUS_PROCESSING 102

// Success
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_CREATED 201
#define HTTP_STATUS_ACCEPTED 202
#define HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION 203
#define HTTP_STATUS_NO_CONTENT 204
#define HTTP_STATUS_RESET_CONTENT 205
#define HTTP_STATUS_PARTIAL_CONTENT 206
#define HTTP_STATUS_MULTI_STATUS 207
#define HTTP_STATUS_ALREADY_REPORTED 208
#define HTTP_STATUS_IM_USED 226

// Redirection
#define HTTP_STATUS_MULTIPLE_CHOICES 300
#define HTTP_STATUS_MOVED_PERMANENTLY 301
#define HTTP_STATUS_FOUND 302
#define HTTP_STATUS_SEE_OTHER 303
#define HTTP_STATUS_NOT_MODIFIED 304
#define HTTP_STATUS_USE_PROXY 305
#define HTTP_STATUS_SWITCH_PROXY 306 // No longer used
#define HTTP_STATUS_TEMPORARY_REDIRECT 307
#define HTTP_STATUS_PERMANENT_REDIRECT 308

// Client errors
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_PAYMENT_REQUIRED 402
#define HTTP_STATUS_FORBIDDEN 403
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405
#define HTTP_STATUS_NOT_ACCEPTABLE 406
#define HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED 407
#define HTTP_STATUS_REQUEST_TIMEOUT 408
#define HTTP_STATUS_CONFLICT 409
#define HTTP_STATUS_GONE 410
#define HTTP_STATUS_LENGTH_REQUIRED 411
#define HTTP_STATUS_PRECONDITION_FAILED 412
#define HTTP_STATUS_PAYLOAD_TOO_LARGE 413
#define HTTP_STATUS_URI_TOO_LONG 414
#define HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE 416
#define HTTP_STATUS_EXPECTATION_FAILED 417
#define HTTP_STATUS_IM_A_TEAPOT 418 // April Fools' joke in RFC 2324
#define HTTP_STATUS_MISDIRECTED_REQUEST 421
#define HTTP_STATUS_UNPROCESSABLE_ENTITY 422
#define HTTP_STATUS_LOCKED 423
#define HTTP_STATUS_FAILED_DEPENDENCY 424
#define HTTP_STATUS_TOO_EARLY 425
#define HTTP_STATUS_UPGRADE_REQUIRED 426
#define HTTP_STATUS_PRECONDITION_REQUIRED 428
#define HTTP_STATUS_TOO_MANY_REQUESTS 429
#define HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE 431
#define HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS 451

// Server errors
#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED 501
#define HTTP_STATUS_BAD_GATEWAY 502
#define HTTP_STATUS_SERVICE_UNAVAILABLE 503
#define HTTP_STATUS_GATEWAY_TIMEOUT 504
#define HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED 505
#define HTTP_STATUS_VARIANT_ALSO_NEGOTIATES 506
#define HTTP_STATUS_INSUFFICIENT_STORAGE 507
#define HTTP_STATUS_LOOP_DETECTED 508
#define HTTP_STATUS_NOT_EXTENDED 510
#define HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED 511

#endif // HTTP_STATUS_CODES_H