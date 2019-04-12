/*
** listener.c -- a datagram sockets "server" demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "client.h"
#include <math.h>
#define MYPORT "22703"
#define MAXBUFLEN 100
// the port users will be connecting to
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// serverB: waiting for compute commands from AWS and calculate end-to-end delay
int main(void)
{
    // create UDP socket to communicate with AWS server, code reused from Beej's Guide to Network Programming
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo("127.0.0.1", MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; 
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
        continue; 
        }
        break; 
    }
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    printf("The Server B is up and running using UDP on port 22703.\n");
    while (1) {
        addr_len = sizeof their_addr;
        struct commandInfo myCommand;
        memset(&myCommand, 0, sizeof (struct commandInfo));
        if ((numbytes = recvfrom(sockfd, &myCommand, sizeof (struct commandInfo), 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("The Server B received link information: link <%d>, file size <%g>, and signal power <%g>\n", myCommand.linkID, myCommand.size, myCommand.signalPower);
        // calculate end-to-end delay
        double ratio = log2(myCommand.signalPower / myCommand.noisePower + 1);
        double trans = myCommand.bandwidth * 1000000 * ratio;
        myCommand.transDelay = myCommand.size / trans;
        myCommand.propDelay = myCommand.length / myCommand.velocity;
        printf("The Server B finished the calculation for link <%d>\n", myCommand.linkID);
        sendto(sockfd, &myCommand, sizeof (struct commandInfo), 0, (struct sockaddr *) &their_addr,addr_len);
        printf("The Server B finished sending the output to AWS\n");
    }
    close(sockfd);
    return 0; 
}