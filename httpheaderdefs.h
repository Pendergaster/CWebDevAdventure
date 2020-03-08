/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */

#ifndef HTTPHEADERDEFS_H
#define HTTPHEADERDEFS_H

typedef enum HTTPStatus {
    HTTP_OK,
    HTTP_NOT_FOUND,
    HTTP_BAD_REQUEST,
    HTTP_MOVED_PERMANENTLY,
    HTTP_PERMANENTLY_REDIRECTED,
} HTTPStatus;


static const char okMessage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"\r\n";

static const char notFoundMessage[] =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"\r\n";

static const char badRequest[] =
"HTTP/1.1 400 Bad Request\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"\r\n";


static const char movedPermanently[] =
"HTTP/1.1 301 Moved Permanently\r\n"
"Location: " SERVER_LOCATION "\r\n"
"\r\n";

static const char permanentRedirect[] =
"HTTP/1.1 308 Permanent Redirect\r\n"
"Location: " SERVER_LOCATION "\r\n"
"\r\n";

#endif /* HTTPHEADERDEFS_H */
