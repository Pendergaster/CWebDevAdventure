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
#include <termios.h>
#include "defs.h"
#include "fileload.h"
#include "stringutil.h"
#include "httpheaderdefs.h"

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
#ifdef HTTP
client_run(i32 clientCon);
#else
client_run(SSL* clientCon);
#endif

static void
client_start(SSL* clientCon, i32 clientfd);

static int
password_cb(char *buf, i32 num, i32 rwflag,void *userdata) {
    (void)rwflag; (void)userdata;

    char* pass = getpass("Password: ");

    if(num < (i32)strlen(pass) + 1)
        return 0;

    strcpy(buf, pass);
    return(strlen(pass));
}

static SSL_CTX*
ssl_create_context() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

static void
ssl_configure_context(SSL_CTX *ctx) {

    if (SSL_CTX_set_ecdh_auto(ctx, 1) != 1) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, SSL_CERT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_default_passwd_cb(ctx, password_cb);

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

    SSL_CTX *ctx = NULL;
#ifndef HTTP
    SSL_library_init();
    ctx = ssl_create_context();

    ssl_configure_context(ctx);
#endif
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
    // 999 999 max contentLenght
    if(contentLenght > 999999999) {
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
compile_and_run_string(const char* code, int* size_out) {

    TCCState* state = tcc_new();
    if (!state) {
        printf("could not create state\n");
        return;
    }
    // tcc_set_lib_path(state, "./");
    // tcc_add_include_path(state, "./");
    tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    tcc_set_options(state, "-w -m64 -std=c99 -bench");

    if (tcc_compile_string(state, code) == -1) {
        return;
    }

    tcc_add_library(state, "tcc");
    tcc_add_library(state, "dl");
    tcc_add_library(state, "pthread");
    tcc_add_library(state, "ssl");
    tcc_add_library(state, "crypto");

    int size = tcc_relocate(state, 0);
    void* mem = malloc(size);
    tcc_relocate(state, mem);

    void (*main)(int argc, char** argv) = tcc_get_symbol(state, "main");
    if (!main) {
        printf("no main found\n");
    } else {
        main(0, 0);
    }

    if (size_out) {
        *size_out = size;
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

static const char navigation_menu_str[] = {
    "<body>"
        "<pre>"
        "<a href=\"/dev\">Dev</a> "
        "<a href=\"/\">Compile</a> "
        "\n"
        "\n"
        "</pre>"
};

static void
client_post(ClientHandle clientCon, Header* header) {
    if(strcmp(header->uri, "/compile") == 0) {
        char* rst = parse_payload(header->payload);

        stdout_redirect_to_pipe();
        printf("</head>");
        printf("<link rel=\"stylesheet\" href=\"style.css\">\n");
        printf("</head>");


        printf(navigation_menu_str);
        printf("<canvas class=\"game\" id=\"canvas\" oncontextmenu=\"event.preventDefault()\" "
               "style=\"width: 100%%; height:100%%;\"></canvas>");

        printf("<script type=\"text/javascript\" src=\"init_canvas.js\"> </script>");

        int size_out = 0;
        printf("<pre>");

        {   // also redirect stderr for compile errors
            int saved_stderr;
            saved_stderr = dup(STDERR_FILENO);
            dup2(pipe_fd[1], STDERR_FILENO);
            compile_and_run_string(rst, &size_out);
            dup2(saved_stderr, STDERR_FILENO);
            close(saved_stderr);
        }

        printf("</pre>");
        //
        printf("<br>");
        printf("<br>");
        printf("code size: %i kb, %i b\n", size_out / 1024, size_out);

        printf("</body>");

        fflush(stdout);
        char buffer[65536];
        int page_len = read(STDOUT_PIPE_FD, buffer, ARRAY_SIZE(buffer));

        stdout_restore();


        // char todo[] = "<h1>TODO<h1>";
        // i32 contentLen = sizeof(todo);


        ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", page_len);
        if(res.data) {
            client_write(clientCon, res.data, res.size);
            client_write(clientCon, buffer, page_len);
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
        client_write(clientCon, res.data, res.size);
        client_write(clientCon, data, contentLen);
    }

    responseheader_dispose(&res);
}


static void
client_get_index(ClientHandle clientCon, Header* header) {
    (void)header;

    stdout_redirect_to_pipe();

    printf(navigation_menu_str);
    system("./index.sh");

    char buffer[65536];
    int page_len = read(STDOUT_PIPE_FD, buffer, ARRAY_SIZE(buffer));

    stdout_restore();

    ResponseHeader res = responseheader_construct(HTTP_OK, "text/html; charset=utf-8l", page_len);
    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);
        printf("%s", buffer);
        client_write(clientCon, buffer, page_len);
    }
    responseheader_dispose(&res);
}



static void
client_get_dev(ClientHandle clientCon, Header* header) {
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
        client_write(clientCon, res.data, res.size);
        printf("%s", data);
        client_write(clientCon, data, contentLen);
    }
    responseheader_dispose(&res);
}

static int // -1 error
client_get_audio(ClientHandle clientCon, Header* header) {
    char* uri = header->uri + 1;
    char* fileExt = filename_get_ext(uri);

    size_t size = 0;
    void* audio = load_binary_file(uri, &size);
    if(!audio) {
        fprintf(stderr, "Failed to load file %s\n", uri);
        return -1;
    }

    printf("Audio size is %ld /n", size);

    char* contentType;
    if(strcmp(fileExt, "mp3") == 0) {
        // concat strings to free them later...
        contentType = concat("audio/", "mpeg");
    } else {
        contentType = concat("audio/", fileExt);
    }

    ResponseHeader res = responseheader_construct(HTTP_OK, contentType, size);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);

        printf("sending audio %s\n", uri);
        client_write(clientCon, audio, size);
    }

    responseheader_dispose(&res);
    free(audio);
    free(contentType);
    return 0;
}

static int // -1 error
client_get_image(ClientHandle clientCon, Header* header) {
    char* uri = header->uri + 1;
    char* fileExt = filename_get_ext(uri);

    size_t size = 0;
    void* image = load_binary_file(uri, &size);
    if(!image) {
        fprintf(stderr, "Failed to load file %s\n", uri);
        return -1;
    }

    char* contentType;
    if(strcmp(fileExt, "jpg") == 0) {
        // concat strings to free them later...
        contentType = concat("image/", "jpeg");
    } else {
        contentType = concat("image/", fileExt);
    }

    ResponseHeader res = responseheader_construct(HTTP_OK, contentType, size);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);

        printf("sending image %s\n", uri);
        client_write(clientCon, image, size);
    }

    responseheader_dispose(&res);
    free(image);
    free(contentType);
    return 0;
}

static int //-1 error
client_send_text_file(ClientHandle clientCon, Header* header, char* contentType) {

    //Check file extension to prevent user accessing random files
    char* uri = header->uri + 1;

    size_t lenght = 0;
    char* file = load_file(uri, &lenght);
    if(!file) {
        fprintf(stderr, "Failed to load file %s\n", uri);
        return -1;
    }

    ResponseHeader res = responseheader_construct(HTTP_OK, contentType, lenght - 1);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);
        //printf("%s", file); lets not print large files
        client_write(clientCon, file, lenght - 1);
    }

    responseheader_dispose(&res);
    free(file);
    return 0;
}

static int // -1 error
client_get_css(ClientHandle clientCon, Header* header) {

    return client_send_text_file(clientCon, header,  "text/css; charset=utf-8l");
}

static int // -1 error
client_get_html(ClientHandle clientCon, Header* header) {

    printf("Getting html!");
    return client_send_text_file(clientCon, header,  "text/html; charset=utf-8l");
}

static int // -1 error
client_get_js(ClientHandle clientCon, Header* header) {

    printf("Getting javascript!");
    return client_send_text_file(clientCon, header,  "text/javascript; charset=utf-8l");
}

static int // -1 error
client_get_mem(ClientHandle clientCon, Header* header) {

    char* uri = header->uri + 1;
    char* fileExt = filename_get_ext(uri);

    size_t size = 0;
    void* mem = load_binary_file(uri, &size);
    if(!mem) {
        fprintf(stderr, "Failed to load file %s\n", uri);
        return -1;
    }

    ResponseHeader res = responseheader_construct(HTTP_OK, "application/octet-stream", size);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);

        printf("sending mem %s\n", uri);
        client_write(clientCon, mem, size);
    }

    responseheader_dispose(&res);
    free(mem);
    return 0;
}

static void
client_unknown_page(ClientHandle clientCon) {
    static const char unknownPage[] =
        "<link rel=\"stylesheet\" href=\"style.css\">\n"
        "<b>Unknown page, please go to </b>"
        "<a href=\""
        SERVER_LOCATION
        "\">Index</a>\n";

    ResponseHeader res = responseheader_construct(HTTP_NOT_FOUND, "text/html; charset=utf-8l",
            sizeof(unknownPage) - 1);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);

        printf("%s", unknownPage);
        client_write(clientCon, unknownPage, sizeof(unknownPage) - 1);

    }
    responseheader_dispose(&res);
}

static void
client_not_found(ClientHandle clientCon) {
    static const char contentNotFound[] =
        "<link rel=\"stylesheet\" href=\"style.css\">\n"
        "<b>Bad Request 400 </b>\n";

    ResponseHeader res = responseheader_construct(HTTP_NOT_FOUND, "text/html; charset=utf-8l",
            sizeof(contentNotFound) - 1);

    if(res.data) {
        printf("Headers (len %lu): \n%s", res.size, res.data);
        client_write(clientCon, res.data, res.size);

        printf("%s", contentNotFound);
        client_write(clientCon, contentNotFound, sizeof(contentNotFound) - 1);

    }
    responseheader_dispose(&res);
}

typedef struct AcceptCallback {
    char* name;
    int (*fun)(ClientHandle, Header*);
} AcceptCallback;

static AcceptCallback Callbacks[] = {
    {"png", client_get_image},
    {"jpeg", client_get_image},
    {"jpg", client_get_image},
    {"ico", client_get_image},
    {"wav", client_get_audio},
    {"css", client_get_css},
    {"html", client_get_html},
    {"js", client_get_js},
    {"mem", client_get_mem},
    {NULL, NULL}
};

static void
client_get(ClientHandle clientCon, Header* header) {
    //Check file extension to prevent user accessing random files
    char* uri = header->uri + 1;
    char* fileExt = filename_get_ext(uri);

    if(strcmp(header->uri, "/") == 0) { // Get index
        client_get_index(clientCon, header);
    } else if (strcmp(header->uri, "/dev") == 0) { // Get dev
        client_get_dev(clientCon, header);
    } else if( fileExt != NULL) { // send some file
        AcceptCallback* callback = NULL;
        for (int i = 0; (Callbacks + i)->name; i++) {
            if(strcmp((Callbacks + i)->name, fileExt) == 0) {
                callback = (Callbacks + i);
                break;
            }
        }
        if(callback) {
            if(callback->fun(clientCon, header) == -1) {
                client_not_found(clientCon);
            }
        } else {
            client_not_found(clientCon);
        }
    } else { // unknown page or html
        // Check if it is html page

        header->uri = concat(header->uri, ".html");
        if(client_get_html(clientCon, header) == -1) {
            client_unknown_page(clientCon);
        }
        free(header->uri);
    }
}

static void
client_default_method(ClientHandle clientCon) {
    char buf[] = "HTTP/1.1 500 Not Handled\r\n\r\nThe server has no handler to the request.\r\n";
    client_write(clientCon, buf, sizeof(buf));
}

static void
client_run(ClientHandle clientCon) {
    // handle client request
    //TODO poista
    char* buf = malloc(65535);

    int numCharacters = client_read(clientCon, buf, 65535);
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
                u32 temp = client_read(clientCon, buf + numCharacters, 65535 - numCharacters);

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
    (void) clientCon;

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
#ifndef HTTP
        if (clientCon == NULL) { // http
            client_run_redirect(clientfd);
        } else { // https
            client_run(clientCon);
            SSL_shutdown(clientCon);
            SSL_free(clientCon);
        }
#else
        client_run(clientfd);
#endif
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

    (void) ctx;
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
            SSL* ssl = NULL;
#ifndef HTTP
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, clientfd);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(clientfd);
                continue;
            }
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

            //client_write(ssl, reply, strlen(reply));
#endif
            client_start(ssl, clientfd);
        } else if (FD_ISSET(fds[1], &readfds)) { // http
            int clientfd = accept(fds[1], (struct sockaddr *)&clientaddr, &addrlen);
            client_start(NULL, clientfd);
        }
    }
}
