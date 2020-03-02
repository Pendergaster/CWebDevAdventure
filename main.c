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
server_run();

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
    HTTP_BAD_REQUEST,
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


static const char badRequest[] =
"HTTP/1.1 400 Bad Request\r\n"
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
        data = (char*)malloc(sizeof(sizeof(okMessage) + 253));
        *headerLen = sprintf(data, okMessage , contentType, contentLenght);
    } else if(status == HTTP_NOT_FOUND){
        data = (char*)malloc(sizeof(sizeof(notFoundMessage) + 253));
        *headerLen = sprintf(data, notFoundMessage , contentType, contentLenght);
    } else if(status == HTTP_BAD_REQUEST){
        data = (char*)malloc(sizeof(sizeof(badRequest) + 253));
        *headerLen = sprintf(data, badRequest , contentType, contentLenght);
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
        fprintf(stderr, "Did not find content lenght for post\n");
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

    char data[482];
    int contentLen = sprintf(data,
             "<link rel=\"stylesheet\" href=\"style.css\">\n"
            "You are using %s\n"
             "<img src=\"hacker.jpeg\" alt=\"Italian Trulli\">\n", userAgent.value);
    if(0 >= contentLen)
        return;

    size_t size = 0;
    void* image = load_binary_file("hacker.jpeg", &size);
    if(!image) return;

    u32 headerLen = 0;
    char* respHeaders =
        header_construct(HTTP_OK, "text/html; charset=utf-8l", contentLen, &headerLen);
    //header_construct(HTTP_OK, "image/jpeg", size, &headerLen);

    if(!respHeaders)
        return;

    //printf("Headers (len %d): \n%s", headerLen, respHeaders);
    printf("Headers (len %d): \n%s", headerLen, respHeaders);

    write(clientfd, respHeaders, headerLen);

    printf("%s", data);
    write(clientfd, data, contentLen);
    //write(clientfd, image, size);

    free(respHeaders);
    free(image);
}


static void
client_get_image(i32 clientfd, Header* header) {

    //Check file extension to prevent user accessing random files
    char* uri = header->uri + 1;
    const char* fileExt = filename_get_ext(uri);
    if(!fileExt) {
        fprintf(stderr, "not file extension found %s\n", uri);
        return;
    }

    if(strcmp(fileExt, "png") == 0 ||
            strcmp(fileExt, "jpeg") == 0 ||
            strcmp(fileExt, "jpg") == 0)
    {
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

        u32 headerLen = 0;
        char* respHeaders = header_construct(HTTP_OK, contentType, size, &headerLen);

        if(!respHeaders)
            return;

        printf("Headers (len %d): \n%s", headerLen, respHeaders);

        write(clientfd, respHeaders, headerLen);

        printf("sending image %s\n", uri);
        write(clientfd, image, size);

        free(respHeaders);
        free(image);
        free(contentType);
    }
}


static void
client_get_css(i32 clientfd, Header* header) {

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

        u32 headerLen = 0;
        char* respHeaders = header_construct(HTTP_OK, "text/css; charset=utf-8l", lenght - 1, &headerLen);

        if(!respHeaders)
            return;

        printf("Headers (len %d): \n%s", headerLen, respHeaders);
        write(clientfd, respHeaders, headerLen);
        printf("%s", cssData);
        write(clientfd, cssData, lenght - 1);

        free(respHeaders);
        free(cssData);
    }
}

static void
client_unknown_page(i32 clientfd) {
            static const char unknownPage[] =
                "<link rel=\"stylesheet\" href=\"style.css\">\n"
                "<b>Unknown page, please go to </b>"
                "<a href=\"http://127.0.0.1:12913\">Index</a>\n";

            u32 headerLen = 0;
            char* respHeaders = header_construct(HTTP_NOT_FOUND, "text/html; charset=utf-8l",
                    sizeof(unknownPage) - 1, &headerLen);

            if(!respHeaders)
                return;

            printf("Headers (len %d): \n%s", headerLen, respHeaders);

            write(clientfd, respHeaders, headerLen);

            printf("%s", unknownPage);
            write(clientfd, unknownPage, sizeof(unknownPage) - 1);
}

static void
client_bad_request(i32 clientfd) {

            static const char badRequestMsg[] =
                "<link rel=\"stylesheet\" href=\"style.css\">\n"
                "<b>Bad Request 400 </b>"
                "<a href=\"http://127.0.0.1:12913\">Index</a>\n";

            u32 headerLen = 0;
            char* respHeaders = header_construct(HTTP_BAD_REQUEST, "text/html; charset=utf-8l",
                    sizeof(badRequestMsg) - 1, &headerLen);

            if(!respHeaders)
                return;

            printf("Headers (len %d): \n%s", headerLen, respHeaders);

            write(clientfd, respHeaders, headerLen);

            printf("%s", badRequestMsg);
            write(clientfd, badRequestMsg, sizeof(badRequestMsg) - 1);
}

static void
client_get(i32 clientfd, Header* header) {

    HeaderField acceptField;

    if(header_get_correct_field(&acceptField, header, "Accept") != 0) {
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
            client_get_index();
        } else if (strcmp(header->uri, "/dev") == 0) { // Get dev
            client_get_dev(clientfd, header);
        } else { // unknown page
            client_unknown_page(clientfd);
        }
    } else if(string_list_contains(acceptList, "image/webp") != NULL) { // Get image
        client_get_image(clientfd, header);
    } else if (string_list_contains(acceptList, "text/css") != NULL) { // Get css
        client_get_css(clientfd, header);
    } else {
        client_bad_request(clientfd);
    }
    string_list_dispose(acceptList);
}

static void
client_default_method(i32 clientfd) {

    char buf[] = "HTTP/1.1 500 Not Handled\r\n\r\nThe server has no handler to the request.\r\n";
    write(clientfd, buf, sizeof(buf));
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
    free(buf);
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

