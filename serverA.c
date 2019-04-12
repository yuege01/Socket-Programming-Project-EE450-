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
#define MYPORT "21703"
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
// serverA: waiting for commands from AWS server to decide whether to write to database or search for data in database
int main(void)
{
    // create UDP socket to communicate with AWS server, code reused from Beej's Guide to Network Programming
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    struct commandInfo myCommand;
    memset(&myCommand, 0, sizeof (struct commandInfo));
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
    printf("The Server A is up and running using UDP on port 21703.\n");
    while(1) {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, &myCommand, sizeof (struct commandInfo) , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        // when command is write, write data to database
        if(strcmp(myCommand.command, "write") == 0) {
            printf("The Server A received input for writing\n");
            FILE *fp = fopen("database.txt", "ab+");
            char * line = NULL;
            size_t len = 0;
            ssize_t read;
            int id = 1;
            while ((read = getline(&line, &len, fp)) != -1) {
               id++;
            }
            if (line)
                free(line);
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "%d\t%f\t%f\t%f\t%f\n", id, myCommand.bandwidth, myCommand.length, myCommand.velocity, myCommand.noisePower);
            fprintf(fp, "%s", buffer);
            printf("The Server A wrote link <%d> to database\n", id);
            sendto(sockfd, "The AWS received response from Backend-Server A for writing using UDP over port <23703>\n", 1024, 0, (struct sockaddr *) &their_addr, addr_len);
            fclose(fp);
        }
        // when command is compute, search for demanded LinkID in database, send found record back to AWS or notify it with "Link not Found"
        else if(strcmp(myCommand.command, "compute") == 0){
            printf("The Server A received input <%d> for computing\n", myCommand.linkID);
            int id = myCommand.linkID;
            int lookupId = 1;
            FILE *fp = fopen("database.txt", "ab+");
            char * line = NULL;
            size_t len = 0;
            ssize_t read;
            if (fp == NULL)
            exit(EXIT_FAILURE);
            int isFound = 0;
            while ((read = getline(&line, &len, fp)) != -1) {
               if(id == lookupId) {
                isFound = 1;
                    break;
               }
               lookupId++;
            }
            fclose(fp);
            if(!isFound) {
                struct commandInfo nullValue;
                memset(&nullValue, 0, sizeof (struct commandInfo));
                sendto(sockfd, &nullValue, sizeof (struct commandInfo), 0, (struct sockaddr *) &their_addr, addr_len);
                printf("Link ID not found\n");
            }
            else {
                struct commandInfo nextCommand;
                memset(&nextCommand, 0, sizeof (struct commandInfo));
                nextCommand.linkID = id;
                nextCommand.size = myCommand.size;
                nextCommand.signalPower = myCommand.signalPower;
                char * pch;
                pch = strtok (line,"\t");
                pch = strtok(NULL, "\t");
                nextCommand.bandwidth = atof(pch);
                printf("%s\n", pch);
                pch = strtok (NULL,"\t");
                nextCommand.length = atof(pch);
                printf("%s\n", pch);
                pch = strtok (NULL,"\t");
                nextCommand.velocity = atof(pch);
                printf("%s\n", pch);
                pch = strtok (NULL,"\t");
                nextCommand.noisePower = atof(pch);
                printf("%s\n", pch);
                sendto(sockfd, &nextCommand, sizeof (struct commandInfo), 0, (struct sockaddr *) &their_addr, addr_len);
                printf("The Server A finished sending the search result to AWS\n");
            }
        }
    }
    close(sockfd);
    return 0; 
}