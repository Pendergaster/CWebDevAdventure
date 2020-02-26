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


typedef struct Header {
    char* name, *value;
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
header_get_correct(Header* res, Header* headers, u32 numHeaders,const char* name) {

    for(u32 i = 0; i < numHeaders; i++) {
        printf("ale ale ale \n");
        if(headers[i].name == NULL)
            return -1;
        if (strcmp(headers[i].name, name) == 0) {
            *res = headers[i];
            return 0;
        }
    }
    return -1;
}

static const char okMessage[] = "HTTP/1.1 200 OK\r\n\r\n";

static void
client_post(i32 clientfd, i32 payloadSize) {

    write(clientfd, okMessage, ARRAY_SIZE(okMessage));

    char data[256];
    int written = sprintf (data, "You POSTed %d bytes. \r\n", payloadSize);
    if(written > 0) {
        write(clientfd, data, written);
    }

}

static void
client_get(i32 clientfd, Header* headers, u32 numHeaders) {

    printf("Getting! \n");
    write(clientfd, okMessage, ARRAY_SIZE(okMessage) - 1);

    Header userAgent = {};
    if(header_get_correct(&userAgent, headers, numHeaders, "User-Agent") == -1)
        return;

    char data[256];
    int written = sprintf (data, "You are using %s", userAgent.value);
    printf("sending %s", data);
    if(written > 0) {
        write(clientfd, data, written);
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

        // extraxt tokens from string

        // "GET" or "POST"
        char* method = strtok(buf, " \t\r\n");
        // "/index.html" things before '?'
        char* uri    = strtok(NULL, " \t");
        // "HTTP/1.1"
        char* prot   = strtok(NULL, " \t\r\n");
        (void) prot;

        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

        Header reqhdr[17] = {0};
        char *payload = NULL;
        for(u32 i = 0; i < ARRAY_SIZE(reqhdr); i++) {
            reqhdr[i].name = strtok(NULL, "\r\n: \t");

            if (!reqhdr[i].name)
                break;

            char* val =  strtok(NULL, "\r\n");

            while(*val && *val==' ')
                val++;

            reqhdr[i].value = val;

            fprintf(stderr, "[H] %s: %s\n", reqhdr[i].name, reqhdr[i].value);

            // end of value string
            payload = reqhdr[i].value + 1 + strlen(reqhdr[i].value);
            if (payload[1] == '\r' && payload[2] == '\n')
                break;
        }

        printf("rererere\n"); fflush(stdout);

        payload = strtok(NULL, "\r\n");
        printf("rererere\n"); fflush(stdout);

        Header contentLenght;
        i32 payloadSize = 0;

        if(header_get_correct(&contentLenght, reqhdr, ARRAY_SIZE(reqhdr), "Content-Length") == 0) {
            payloadSize = atol(contentLenght.value);
        }

        printf("rererere\n");

        if(strcmp("GET", method)==0) {
            client_get(clientfd, reqhdr, ARRAY_SIZE(reqhdr));
        } else if(strcmp("POST", method)==0) {
            client_post(clientfd, payloadSize);
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

