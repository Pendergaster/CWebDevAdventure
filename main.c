/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <libtcc.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "defs.h"
#include "fileload.h"
#include "stringutil.h"


typedef struct HeaderField {
    char* name, *value;
} HeaderField ;

typedef struct Header {
    char*       method;
    char*       protocol;
    char*       uri;
    HeaderField fields[17];
    char*       payload;
} Header;

static void
server_run(SSL_CTX *ctx);

static void
client_run(SSL* clientCon);

static void
client_start(SSL* clientCon, i32 clientfd);

static SSL_CTX*
ssl_create_context() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    const SSL_METHOD *method;
    SSL_CTX *ctx;

    // deprecated??
    // method = SSLv23_server_method();

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void ssl_configure_context(SSL_CTX *ctx) {

    if (SSL_CTX_set_ecdh_auto(ctx, 1) != 1) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, SSL_CERT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, SSL_KEY, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

static void
ssl_context_dispose(SSL_CTX* context) {
    SSL_CTX_free(context);
    EVP_cleanup();
}

int
main(int argc, char** argv) {

    SSL_library_init();
    SSL_CTX *ctx = ssl_create_context();


    ssl_configure_context(ctx);

    if (argc == 2) {
        SERVER_PORT = argv[2];
        // not valid port
        if(atol(SERVER_PORT) == 0) {
            exit(EXIT_FAILURE);
        }
    }

    printf("Hello web\n");
    server_run(ctx);


    ssl_context_dispose(ctx);

}



// -1 not found
static i32
header_get_field(HeaderField* res, Header* header, const char* name) {

    for(u32 i = 0; i < ARRAY_SIZE(header->fields); i++) {
        if(header->fields[i].name == NULL)
            return -1;
        if (strcmp(header->fields[i].name, name) == 0) {
            *res = header->fields[i];
            return 0;
        }
    }
    return -1;
}

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

static void
header_parse(Header* header, char* buf) {

    // "GET" or "POST"
    header->method = strtok(buf, " \t\r\n");
    // "/index.html" things before '?'
    header->uri    = strtok(NULL, " \t");
    // "HTTP/1.1"
    header->protocol   = strtok(NULL, " \t\r\n");

    fprintf(stderr, "\x1b[32m + [%s] [%s] %s\x1b[0m\n", header->method, header->protocol, header->uri);

    // Parse header
    for(u32 i = 0; i < ARRAY_SIZE(header->fields); i++) {
        header->fields[i].name = strtok(NULL, "\r\n: \t");

        if (! header->fields[i].name)
            break;

        char* val =  strtok(NULL, "\r\n");

        while(*val && *val==' ')
            val++;

        header->fields[i].value = val;

        fprintf(stderr, "[H] %s: %s\n", header->fields[i].name, header->fields[i].value);

        // end of value string
        header->payload = header->fields[i].value + 1 + strlen( header->fields[i].value);
        if (header->payload[1] == '\r' && header->payload[2] == '\n')
            break;
    }

    header->payload = strtok(NULL, "\r\n");
}

typedef struct ResponseHeader {
    char*       data;
    size_t      size;
    HTTPStatus  type;
} ResponseHeader;

static void
responseheader_dispose(ResponseHeader* header) {
    if(header->data &&
            header->type != HTTP_MOVED_PERMANENTLY &&
            header->type != HTTP_PERMANENTLY_REDIRECTED) {
        free(header->data);
    }
    *header = (ResponseHeader){};
}

static ResponseHeader
responseheader_construct(HTTPStatus status,char* contentType, u32 contentLenght) {

    ResponseHeader ret = {.type = status};
    // 999999 max contentLenght
    if(contentLenght > 9999999) {
        fprintf(stderr, "Too long content lenght, exiting..;\n");
        fflush(stderr);
        return (ResponseHeader){};
    }
    if(status == HTTP_OK) {
        ret.data = (char*)malloc(sizeof(okMessage) + 253);
        ret.size = sprintf(ret.data, okMessage , contentType, contentLenght);
    } else if(status == HTTP_NOT_FOUND){
        ret.data = (char*)malloc(sizeof(notFoundMessage) + 253);
        ret.size = sprintf(ret.data, notFoundMessage , contentType, contentLenght);
    } else if(status == HTTP_BAD_REQUEST){
        ret.data = (char*)malloc(sizeof(badRequest) + 253);
        ret.size = sprintf(ret.data, badRequest , contentType, contentLenght);
    } else if(status == HTTP_MOVED_PERMANENTLY) {
        ret.data = (char*)movedPermanently;
        ret.size = sizeof(movedPermanently) - 1;
    } else if(status == HTTP_PERMANENTLY_REDIRECTED){
        ret.data = (char*)permanentRedirect;
        ret.size = sizeof(permanentRedirect) - 1;
    } else {
        return (ResponseHeader){};
    }

    if(ret.size > 0)
        return ret;
    else {
        fprintf(stderr, "Too long content lenght, exiting..;\n");
        fflush(stderr);
        return (ResponseHeader){};
    }
}

u8 char_to_digit[256] = {
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['A']  = 10,
    ['B']  = 11,
    ['C']  = 12,
    ['D']  = 13,
    ['E']  = 14,
    ['F']  = 15,
};

// 0 - 31 & 127 edge case
static char*
parse_payload(const char* payload) {

    const char* start  = payload;
    int  len = strlen(payload);
    char* result = malloc(len + 1);


    char* data = result;
    char c;
    int val = 0;

    start += sizeof("message=") - 1; // TODO:



    while((c = *start++)) {
        if (c == '+') {
            *data++ = ' ';
        } else if (c == '%') {
            for(int i = 0; i < 2; i++) {
                int digit = char_to_digit[(u8)*start++];
                val = 16 * val + digit;
            }
            *data++ = (char)val;
        } else {
            *data++ = c;
        }
    }
    *data++ = 0;

    FILE* dbg = fopen("debug.txt", "w");
    if (dbg) {
        fwrite(result, 1, len, dbg);
        fclose(dbg);
    }

    return result;
}

static void
compile_and_run_string(const char* code) {

    TCCState* state = tcc_new();
    if (!state) {
        printf("could not create state\n");
        return;
    }
    // tcc_set_lib_path(state, "./");
    // tcc_add_include_path(state, "./");
    tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    tcc_set_options(state, "-m64 -std=c99 -bench");

    tcc_compile_string(state, code);

    tcc_add_library(state, "tcc");
    tcc_add_library(state, "dl");
    tcc_add_library(state, "pthread");
    tcc_add_library(state, "ssl");
    tcc_add_library(state, "crypto");

    int size = tcc_relocate(state, 0);
    printf("code size: %i kb, %i b\n", size / 1024, size);
    void* mem = malloc(size);
    tcc_relocate(state, mem);

    void (*main)(int argc, char** argv) = tcc_get_symbol(state, "main");
    if (!main) {
        printf("no main found\n");
    } else {
        main(0, 0);
    }

    tcc_delete(state);
    free(mem);
}

int pipe_fd[2];
int saved_stdout_fd;
#define STDOUT_PIPE_FD pipe_fd[0]
static void stdout_redirect_to_pipe() {
    saved_stdout_fd = dup(STDOUT_FILENO);
    pipe(pipe_fd);
    dup2(pipe_fd[1], STDOUT_FILENO);
}
static void stdout_restore() {
    dup2(saved_stdout_fd, STDOUT_FILENO);
    close(saved_stdout_fd);
    for (int i = 0; i < 2; i++)
        close(pipe_fd[i]);
}

static void
client_post(SSL* clientCon, Header* header) {

    if(strcmp(header->uri, "/compile") == 0) {
        char* rst = parse_payload(header->payload);

        stdout_redirect_to_pipe();
        compile_and_run_string(rst);
        fflush(stdout);
        char buffer[65536];
        int page_len = read(STDOUT_PIPE_FD, buffer, ARRAY_SIZE(buffer));

        stdout_restore();


        // char todo[] = "<h1>TODO<h1>";
        // i32 contentLen = sizeof(todo);


        ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", page_len);
        if(res.data) {
            SSL_write(clientCon, res.data, res.size);
            SSL_write(clientCon, buffer, page_len);
        }
        free(rst);
        responseheader_dispose(&res);
        return;
    }

    HeaderField contentLenght;
    i32 payloadSize = 0;

    if(header_get_field(&contentLenght, header, "Content-Length") != 0) {
        fprintf(stderr, "Did not find content lenght for post\n");
        return;
    }

    payloadSize = atol(contentLenght.value);

    char data[256];
    i32 contentLen = sprintf (data, "You POSTed %d bytes. \r\n", payloadSize);
    if(contentLen >= 0)
        return;

    ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", contentLen);
    if(res.data) {
        SSL_write(clientCon, res.data, res.size);
        SSL_write(clientCon, data, contentLen);
    }

    responseheader_dispose(&res);
}


static void
client_get_index(SSL* clientCon, Header* header) {
    (void)header;

#if 0
    char page[] =
        "<!DOCTYPE html>"
        "<html>"
        "<body>"
        "<form action=\"/compile\" method=\"post\" target=\"_blank\"> "
        // "<label for=\"fname\">First name: </label>"
        // "<input type=\"text\" id=\"fname\" name=\"fname\"><br><br>"
        "<textarea name=\"message\" rows=\"50\" cols=\"50\"> </textarea> "
        "<br><br>"
        "<input type=\"submit\" value=\"compile\">"
        "</form>"
        "</body>"
        "</html>";
    (void)page;
#endif


    stdout_redirect_to_pipe();

    system("./index.sh");

    char buffer[65536];
    int page_len = read(STDOUT_PIPE_FD, buffer, ARRAY_SIZE(buffer));

    stdout_restore();

    ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", page_len);
    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        SSL_write(clientCon, res.data, res.size);
        printf("%s", buffer);
        SSL_write(clientCon, buffer, page_len);
    }
    responseheader_dispose(&res);
}



static void
client_get_dev(SSL* clientCon, Header* header) {

    HeaderField userAgent = {};
    if(header_get_field(&userAgent, header, "User-Agent") == -1)
        return;

    char data[482];
    int contentLen = sprintf(data,
            "<link rel=\"stylesheet\" href=\"style.css\">\n"
            "You are using %s\n"
            "<img src=\"hacker.jpeg\" alt=\"Italian Trulli\">\n", userAgent.value);
    if(0 >= contentLen)
        return;

    ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", contentLen);

    if(res.data) {

        printf("Headers (len %lu): \n%s", res.size, res.data);
        SSL_write(clientCon, res.data, res.size);
        printf("%s", data);
        SSL_write(clientCon, data, contentLen);
    }

    responseheader_dispose(&res);
}

static char* imageTypes[] = {
    "png",
    "jpeg",
    "jpg",
    "ico",
    NULL
};


static void
client_get_image(SSL* clientCon, Header* header) {

    //Check file extension to prevent user accessing random files
    char* uri = header->uri + 1;
    char* fileExt = filename_get_ext(uri);
    if(!fileExt) {
        fprintf(stderr, "not file extension found %s\n", uri);
        return;
    }

    if(string_list_contains(imageTypes, fileExt)) {
        size_t size = 0;
        void* image = load_binary_file(uri, &size);
        if(!image) {
            fprintf(stderr, "Failed to load file %s\n", uri);
            return;
        }

        char* contentType;
        if(strcmp(header->uri, "jpg") == 0) {
            // concat strings to free them later...
            contentType = concat("image/", "jpeg");
        } else {
            contentType = concat("image/", uri);
        }

        ResponseHeader res = responseheader_construct(HTTP_OK, contentType, size);

        if(res.data) {
            printf("Headers (len %lu): \n%s", res.size, res.data);
            SSL_write(clientCon, res.data, res.size);

            printf("sending image %s\n", uri);
            SSL_write(clientCon, image, size);
        }

        responseheader_dispose(&res);
        free(image);
        free(contentType);
    }
}


static void
client_get_css(SSL* clientCon, Header* header) {

    //Check file extension to prevent user accessing random files
    char* uri = header->uri + 1;
    const char* fileExt = filename_get_ext(uri);
    if(!fileExt) {
        fprintf(stderr, "not file extension found %s\n", uri);
        return;
    }

    if(strcmp(fileExt, "css") == 0) {
        size_t lenght = 0;
        char* cssData = load_file(uri, &lenght);
        if(!cssData) {
            fprintf(stderr, "Failed to load file %s\n", uri);
            return;
        }

        ResponseHeader res = responseheader_construct(HTTP_OK, "text/css; charset=utf-8l", lenght - 1);

        if(res.data) {
            printf("Headers (len %lu): \n%s", res.size, res.data);
            SSL_write(clientCon, res.data, res.size);
            printf("%s", cssData);
            SSL_write(clientCon, cssData, lenght - 1);
        }

        responseheader_dispose(&res);
        free(cssData);
    }
}

static void
client_unknown_page(SSL* clientCon) {
    static const char unknownPage[] =
        "<link rel=\"stylesheet\" href=\"style.css\">\n"
        "<b>Unknown page, please go to </b>"
        "<a href=\"http://127.0.0.1:12913\">Index</a>\n";

    ResponseHeader res = responseheader_construct(HTTP_NOT_FOUND, "text/html; charset=utf-8l",
            sizeof(unknownPage) - 1);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        SSL_write(clientCon, res.data, res.size);

        printf("%s", unknownPage);
        SSL_write(clientCon, unknownPage, sizeof(unknownPage) - 1);

    }
    responseheader_dispose(&res);
}

static void
client_bad_request(SSL* clientCon) {

    static const char badRequestMsg[] =
        "<link rel=\"stylesheet\" href=\"style.css\">\n"
        "<b>Bad Request 400 </b>"
        "<a href=\"http://127.0.0.1:12913\">Index</a>\n";

    ResponseHeader res = responseheader_construct(HTTP_BAD_REQUEST, "text/html; charset=utf-8l",
            sizeof(badRequestMsg) - 1);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        SSL_write(clientCon, res.data, res.size);

        printf("%s", badRequestMsg);
        SSL_write(clientCon, badRequestMsg, sizeof(badRequestMsg) - 1);

    }
    responseheader_dispose(&res);
}

static void
client_get(SSL* clientCon, Header* header) {

    HeaderField acceptField;

    if(header_get_field(&acceptField, header, "Accept") != 0) {
        fprintf(stderr, "Did not find accept field for post\n");
        return;
    }

    // Parse accept fields values
    char** acceptList = string_split(acceptField.value, ',');
    if(!acceptList) {
        fprintf(stderr, "failed to parse acceptList\n");
        return;
    }

    if(string_list_contains(acceptList, "text/html") != NULL) { // Get html page
        if(strcmp(header->uri, "/") == 0) { // Get index
            client_get_index(clientCon, header);
        } else if (strcmp(header->uri, "/dev") == 0) { // Get dev
            client_get_dev(clientCon, header);
        } else { // unknown page
            client_unknown_page(clientCon);
        }
    } else if(string_list_contains(acceptList, "image/webp") != NULL) { // Get image
        client_get_image(clientCon, header);
    } else if (string_list_contains(acceptList, "text/css") != NULL) { // Get css
        client_get_css(clientCon, header);
    } else {
        client_bad_request(clientCon);
    }
    string_list_dispose(acceptList);
}

static void
client_default_method(SSL* clientCon) {

    char buf[] = "HTTP/1.1 500 Not Handled\r\n\r\nThe server has no handler to the request.\r\n";
    SSL_write(clientCon, buf, sizeof(buf));
}

static void
client_run(SSL* clientCon) {

    // handle client request
    //TODO poista
    char* buf = malloc(65535);

    int numCharacters = SSL_read(clientCon, buf, 65535);
    if(numCharacters <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }

    if(numCharacters == -1) {

        perror("recv()");
        exit(EXIT_FAILURE);
    } else if (numCharacters == 0) {
        printf("Client disconnect\n");
    } else {
        // handle request
        buf[numCharacters] = '\0';

        Header header = {0};

        header_parse(&header, buf);

        HeaderField contentLen;
        if(header_get_field(&contentLen, &header, "Content-Length") == 0) {
            int len = atoi(contentLen.value);


            while (numCharacters < len) {
                u32 temp = SSL_read(clientCon, buf + numCharacters, 65535 - numCharacters);

                if(temp <= 0) {
                    ERR_print_errors_fp(stderr);
                    return;
                }

                numCharacters +=temp;
            }
        }

        if(strcmp("GET", header.method) == 0) {
            client_get(clientCon, &header);
        } else if(strcmp("POST", header.method) == 0) {
            client_post(clientCon, &header);
        } else {
            client_default_method(clientCon);
        }

        // ssl sync?
        //fsync(clientfd);
    }


    free(buf);
}

static void
client_run_redirect(i32 clientfd) {

    char* buf = malloc(1204);
    int numCharacters = recv(clientfd, buf, 1204, 0);

    if(numCharacters == -1) {
        perror("recv()");
        exit(1);
    } else if (numCharacters == 0) {
        printf("Client disconnect\n");
    } else {
        // handle request
        buf[numCharacters] = '\0';

        Header header = {0};
        header_parse(&header, buf);
        ResponseHeader res = {};
        if(strcmp("GET", header.method) == 0) {
            res = responseheader_construct(HTTP_MOVED_PERMANENTLY, NULL, 0);
            printf("Headers (len %lu): \n%s", res.size, res.data);
            write(clientfd, res.data, res.size);
        } else {
            res = responseheader_construct(HTTP_PERMANENTLY_REDIRECTED, NULL, 0);
            printf("Headers (len %lu): \n%s", res.size, res.data);
            write(clientfd, res.data, res.size);
        }

        responseheader_dispose(&res);
    }
    free(buf);
}

static void
client_start(SSL* clientCon, i32 clientfd) {

#if 0 // for debugging
    if (clientCon == NULL) { // http
        client_run_redirect(clientfd);
    } else { // https
        client_run(clientCon);
        SSL_shutdown(clientCon);
        SSL_free(clientCon);
    }
    // cleanup
    shutdown(clientfd, SHUT_RDWR);//All further send and recieve operations are DISABLED...
    close(clientfd);
#else

    i32 pid = fork();
    if(pid == -1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    } else if(pid == 0) {

        if (clientCon == NULL) { // http
            client_run_redirect(clientfd);
        } else { // https
            client_run(clientCon);
            SSL_shutdown(clientCon);
            SSL_free(clientCon);
        }
        // cleanup
        shutdown(clientfd, SHUT_RDWR);//All further send and recieve operations are DISABLED...
        close(clientfd);

        printf("Child cleaned up succesfully \n");
        exit(EXIT_SUCCESS);
    }
#endif
}

static i32
socket_open(const char* port) {

    struct addrinfo hints, *res, *p;

    memset (&hints, 0, sizeof(hints));
    // ipv 4
    hints.ai_family = AF_INET;
    // tcp
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo( NULL, port, &hints, &res) != 0) {
        perror ("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    i32 mainfd, option = 1;
    // socket and bind
    for (p = res; p!=NULL; p=p->ai_next) {
        mainfd = socket (p->ai_family, p->ai_socktype, 0);
        setsockopt(mainfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if (mainfd == -1) continue;
        if (bind(mainfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if (p == NULL) {
        perror("socket()");
    }

    freeaddrinfo(res);

    // mark connection mode socket
    if (listen (mainfd, 1000000) != 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    return mainfd;
}

static void
server_run(SSL_CTX *ctx) {

    printf( "Server started %s" SERVER_LOCATION ":%s%s\n", "\033[92m",SERVER_PORT,"\033[0m");

    i32 mainfd = socket_open(SERVER_PORT);
    i32 redirectfd = socket_open(REDIRECT_PORT);

    // Ignore killed childs
    signal(SIGCHLD,SIG_IGN);

    // server loop
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    while(1) {

        // wait for connection to either http or https port
        fd_set readfds;
        FD_ZERO(&readfds);
        int fds[] = { mainfd, redirectfd};

        FD_SET(fds[0], &readfds);
        FD_SET(fds[1], &readfds);

        i32 maxfd = -1;
        for (u32 i = 0; i < ARRAY_SIZE(fds); i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i] > maxfd)
                maxfd = fds[i];
        }

        i32 status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (status == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fds[0], &readfds)) { // https
            int clientfd = accept(fds[0],(struct sockaddr *)&clientaddr, &addrlen);

            SSL* ssl = SSL_new(ctx);
            SSL_set_fd(ssl, clientfd);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(clientfd);
                continue;
            }
            else {
                // show certificates
                X509 *cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
                if ( cert != NULL ) {
                    printf("Server certificates:\n");
                    char* line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
                    printf("Subject: %s\n", line);
                    free(line);
                    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
                    printf("Issuer: %s\n", line);
                    free(line);
                    X509_free(cert);
                }
                else {
                    printf("No certificates.\n");
                }// TODO reject connection?

                //SSL_write(ssl, reply, strlen(reply));
                client_start(ssl, clientfd);
            }
        } else if (FD_ISSET(fds[1], &readfds)) { // http
            int clientfd = accept(fds[1], (struct sockaddr *)&clientaddr, &addrlen);
            client_start(NULL, clientfd);
        }

    }
}
