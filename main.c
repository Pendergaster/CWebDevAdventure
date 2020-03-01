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

typedef uint64_t    u64;
typedef int64_t     i64;
typedef uint32_t    u32;
typedef int32_t     i32;
typedef uint16_t    u16;
typedef int16_t     i16;
typedef uint8_t     u8;
typedef int8_t      i8;

#define ARRAY_SIZE(ARR) (sizeof((ARR)) / sizeof(*(ARR)))

char* SERVER_PORT = "12913";

static void server_run();

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
client_run(i32 clientfd);

static void
client_start(i32 clientfd);

int
main(int argc, char** argv) {
    if (argc == 2) {
        SERVER_PORT = argv[2];
        // not valid port
        if(atol(SERVER_PORT ) == 0) {
            exit(1);
        }
    }

    printf("Hello web\n");

    server_run();
}


// -1 not found
static i32
header_get_correct_field(HeaderField* res, Header* header, const char* name) {

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

typedef enum HTTPStatus{
    HTTP_OK,
    HTTP_NOT_FOUND,
} HTTPStatus;



static const char okMessage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\n"
"\r\n";


static const char notFoundMessage[] =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\n"
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

static char*
header_construct(HTTPStatus status,char* contentType, u32 contentLenght, u32* headerLen) {
    // 999999 max contentLenght
    if(contentLenght > 9999999) {
        fprintf(stderr, "Too long content lenght, exiting..;\n");
        fflush(stderr);
        return NULL;
    }
    char* data = NULL;
    if(status == HTTP_OK) {
        data = (char*)malloc(sizeof(ARRAY_SIZE(okMessage) + 253));
        *headerLen = sprintf(data, okMessage , contentType, contentLenght);
    } else if(status == HTTP_NOT_FOUND){
        data = (char*)malloc(sizeof(ARRAY_SIZE(notFoundMessage) + 253));
        *headerLen = sprintf(data, notFoundMessage , contentType, contentLenght);
    } else {
        return NULL;
    }

    if(*headerLen > 0)
        return data;
    else {
        fprintf(stderr, "Too long content lenght, exiting..;\n");
        fflush(stderr);
        return NULL;
    }
}



static void
client_post(i32 clientfd, Header* header) {



    HeaderField contentLenght;
    i32 payloadSize = 0;

    if(header_get_correct_field(&contentLenght, header, "Content-Length") != 0) {
        fprintf(stderr, "Did not foind content lengt for post\n");
        return;
    }

    payloadSize = atol(contentLenght.value);

    char data[256];
    i32 contentLen = sprintf (data, "You POSTed %d bytes. \r\n", payloadSize);
    if(contentLen >= 0)
        return;

    u32 headerLen = 0;
    char* respHeader = header_construct(HTTP_OK, "text/html; charset=utf-8l", contentLen, &headerLen);
    if(!respHeader)
        return;

    write(clientfd, respHeader, headerLen);
    write(clientfd, data, contentLen);

    free(respHeader);
}

static void
client_get_index() {

}

static void
client_get_dev(i32 clientfd, Header* header) {

    HeaderField userAgent = {};
    if(header_get_correct_field(&userAgent, header, "User-Agent") == -1)
        return;

    char data[256];
    int contentLen = sprintf(data, "You are using %s\n", userAgent.value);
    if(0 >= contentLen)
        return;

    u32 headerLen = 0;
    char* respHeaders = header_construct(HTTP_OK, "text/html; charset=utf-8l", contentLen, &headerLen);
    if(!respHeaders)
        return;

    printf("Headers (len %d): \n%s", headerLen, respHeaders);

    write(clientfd, respHeaders, headerLen);

    printf("%s", data);
    write(clientfd, data, contentLen);

    free(respHeaders);
}

static void
client_get(i32 clientfd, Header* header) {


    if(strcmp(header->uri, "/") == 0) {
        client_get_index();
    } else if (strcmp(header->uri, "/dev") == 0){
        client_get_dev(clientfd, header);
    }
    else { // unknown

        static const char unknownPage[] =
            "<b>Unknown page, please go to </b>"
            "<a href=\"http://127.0.0.1:12913\">Index</a>\n";

        u32 headerLen = 0;
        char* respHeaders = header_construct(HTTP_NOT_FOUND, "text/html; charset=utf-8l",
                ARRAY_SIZE(unknownPage) - 1, &headerLen);

        if(!respHeaders)
            return;

        printf("Headers (len %d): \n%s", headerLen, respHeaders);

        write(clientfd, respHeaders, headerLen);

        printf("%s", unknownPage);
        write(clientfd, unknownPage, ARRAY_SIZE(unknownPage) - 1);
    }

}

static void
client_default_method(i32 clientfd) {

    char buf[] = "HTTP/1.1 500 Not Handled\r\n\r\nThe server has no handler to the request.\r\n";
    write(clientfd, buf, ARRAY_SIZE(buf));
}

static void
client_run(i32 clientfd) {

    // handle client request

    //TODO poista
    char* buf = malloc(65535);
    int numCharacters = recv(clientfd, buf, 65535, 0);

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


        if(strcmp("GET", header.method) == 0) {
            client_get(clientfd, &header);
        } else if(strcmp("POST", header.method) == 0) {
            client_post(clientfd, &header);
        } else {
            client_default_method(clientfd);
        }

        fsync(clientfd);
    }

    //Closing SOCKET
    shutdown(clientfd, SHUT_RDWR);//All further send and recieve operations are DISABLED...
    close(clientfd);
    exit(0);
}

static void
client_start(i32 clientfd) {

    i32 pid = fork();
    if(pid == -1) {
        perror("fork()");
        exit(1);
    } else if(pid == 0) {
        client_run(clientfd);
    }

}

static void
server_run() {

    printf( "Server started %shttp://127.0.0.1:%s%s\n", "\033[92m",SERVER_PORT,"\033[0m");

    struct addrinfo hints, *res, *p;

    memset (&hints, 0, sizeof(hints));
    // ipv 4
    hints.ai_family = AF_INET;
    // tcp
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo( NULL, SERVER_PORT, &hints, &res) != 0) {
        perror ("getaddrinfo()");
        exit(1);
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
        exit(1);
    }

    // Ignore killed childs
    signal(SIGCHLD,SIG_IGN);

    // server loop
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    while(1) {

        // wait for connection
        i32 clientfd = accept (mainfd, (struct sockaddr *) &clientaddr, &addrlen);

        if (clientfd < 0) {
            perror("accept()");
            exit(1);
        }
        // start client
        client_start(clientfd);
    }
}

