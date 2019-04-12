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

#define PORT "24703" // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:
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
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
 //    if (argc != 2) {
 //        fprintf(stderr,"The client is up and running\n");
 //        exit(1);
	// }
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
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
	}
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);

	printf("The client is up and running\n");
    struct commandInfo myCommand;
    memset(&myCommand, 0, sizeof (struct commandInfo));
    if(argc == 6) {
        strncpy(myCommand.command, "write", sizeof myCommand.command);
        myCommand.command[5] = '\0';
        myCommand.bandwidth = atof(argv[2]);
        myCommand.length = atof(argv[3]);
        myCommand.velocity = atof(argv[4]);
        myCommand.noisePower = atof(argv[5]);
    }
    else {
        strncpy(myCommand.command, "compute", sizeof myCommand.command);
        myCommand.command[7] = '\0';
        myCommand.linkID = atoi(argv[2]);
        myCommand.size = atoi(argv[3]);
        myCommand.signalPower = atof(argv[4]);        
    }
    if (send(sockfd, &myCommand, sizeof (struct commandInfo), 0) == -1) {
        perror("send");
    }
	freeaddrinfo(servinfo); // all done with this structure
    if(strcmp(myCommand.command, "write") == 0) {
        printf("The client sent write operation to AWS\n");
    }
    else {
        printf("The client sent ID=<%d>, size=<%.2f>, and power=<%.2f> to AWS\n", myCommand.linkID, myCommand.size, myCommand.signalPower);
    }
    char resMessage[1024];
    memset(resMessage, 0, sizeof resMessage);
    if ((numbytes = recv(sockfd, resMessage, 1024, 0)) == -1) {
        perror("recv");
        exit(1); 
    }
    resMessage[numbytes] = '\0';
    printf("%s",resMessage);
    close(sockfd);
	return 0; 
}

