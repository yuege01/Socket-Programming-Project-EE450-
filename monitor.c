#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include "client.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "25703" // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:
//----------------code reused from Beej's Guide to Network Programming--------------------//
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char buf[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
            continue; 
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue; 
        }
        break; 
    }
    // check if connection failed
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
//-------------------------code reused from--------------------------//
    // bootup message of monitor
    printf("The monitor is up and running.\n");

    //set monitor in an infinite loop to keep receiving messages from AWS server.
    while(1) {
        char buf[1024];
        memset(buf, 0, sizeof buf);
        if((numbytes = recv(sockfd, buf, sizeof buf, 0)) > 0) {
            buf[numbytes] = '\0';
            printf("%s",buf);
        }
    }

    //tear down socket connectionSS
    close(sockfd);
    return 0;
}

