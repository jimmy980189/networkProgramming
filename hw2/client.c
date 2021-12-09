#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define MAX_LINE 2048

typedef struct args {
    int src;
    int dest;
} args;

void *thread(void *arg);
void client(char *address, char *port);

int main() {
    client("127.0.0.1", "8000");
    return 0;
}

void *thread(void *arg) {
    args a = *(args *)arg;
    int n;
    char buf[MAX_LINE];

    while ((n = read(a.src, buf, MAX_LINE - 1)) > 0) {
        write(a.dest, buf, n);
    }

    if (n == -1) {
        perror("thread read()");
    }

    return NULL;
}

void client(char *address, char *port) {
    
    int sockfd, n;
    char buf[MAX_LINE];
    pthread_t printer;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;       // AF_INET is IPv4
    hints.ai_socktype = SOCK_STREAM; // use TCP

    struct addrinfo *host_addr;
    getaddrinfo(address, port, &hints, &host_addr); // if host is not NULL, AI_PASSIVE flag is ignored

    printf("Creating socket...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, host_addr->ai_addr, host_addr->ai_addrlen) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(host_addr);

    args a;
    a.src = STDIN_FILENO;
    a.dest = sockfd;

    if (pthread_create(&printer, NULL, thread, (void *)&a) != 0) {
        perror("pthread_create()");
    }

    while ((n = read(sockfd, buf, MAX_LINE)) > 0) {
        if (write(STDOUT_FILENO, buf, n) == -1) {
            perror("write()");
        }
    }

    if (n == -1) {
        perror("read()");
    }

    pthread_cancel(printer);
    pthread_join(printer, NULL);

    close(sockfd);
}
