#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "client.h"
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#define CLIENTPORT "24703"  // the port users will be connecting to
#define MONITORPORT "25703"
#define SERVERA "21703"
#define SERVERB "22703"
#define UDPPORT "23703"
#define BACKLOG 10   // how many pending connections queue will hold
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}// code reused from Beej's Guide to Network Programming
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// send related data and command to serverB and get end-to-end delay results from serverB
int compute(double *res, struct commandInfo link, struct commandInfo myCommand) {
    //create UDP socket with destination port of serverB, code reused from Beej's Guide to Network Programming
    link.linkID = myCommand.linkID;
    link.size = myCommand.size;
    link.signalPower = myCommand.signalPower;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct commandInfo result;
    memset(&result, 0, sizeof (struct commandInfo));
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", SERVERB, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue; 
        }
        break; 
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    // AWS starts to send compute command to serverB
    if ((numbytes = sendto(sockfd, &link, sizeof(struct commandInfo), 0,
        p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1); 
    }  
    freeaddrinfo(servinfo);
    // AWS has sent compute command to serverB
    printf("The AWS sent link ID=<%d>, size=<%g>, power=<%g>, and link information to Backend-Server B using UDP over port <23703>\n", myCommand.linkID, link.size, link.signalPower);
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, &result, sizeof (struct commandInfo) , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    res[0] = result.transDelay;
    res[1] = result.propDelay;
    // AWS gets results from serverB
    printf("The AWS received outputs from Backend-Server B using UDP over port <23703>\n");
    // tear down socket
    close(sockfd);
    return 0;  
}
// search for records in serverA for later delay computation
int retrieve(struct commandInfo *result, struct commandInfo myCommand) {
// create UDP socket to send and retrieve data from serverA, code reused from Beej's Guide to Network Programming
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct commandInfo res;
    memset(&res, 0, sizeof (struct commandInfo));
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", SERVERA, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue; 
        }
        break; 
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    if ((numbytes = sendto(sockfd, &myCommand, sizeof(struct commandInfo), 0,
        p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1); 
    }  
    freeaddrinfo(servinfo);
    //AWS has sent commpute command to serverA
    printf("The AWS sent operation <commpute> to Backend-Server A using UDP over port <23703>\n");
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, &res, sizeof (struct commandInfo) , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    if(res.linkID == 0) {
        close(sockfd);
        return 0;
    }
    *result = res;
    close(sockfd);
    return 1; 
}
// AWS sends write command and data to serverA to be written into database
int writeToDataBase(struct commandInfo myCommand) {
// create UDP socket to establish communication with serverA, code reused from Beej's Guide to Network Programming
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char buf[1024];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", SERVERA, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue; 
        }
        break; 
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    if ((numbytes = sendto(sockfd, &myCommand, sizeof(struct commandInfo), 0,
        p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1); 
    }  
    freeaddrinfo(servinfo);
    // AWS has sent write command to serverA
    printf("The AWS sent operation <write> to Backend-Server A using UDP over port <23703>\n");
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, sizeof buf , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);
    // tear down UDP socket
    close(sockfd);
    return 0; 
}
int main(void)
{
    // create 2 TCP sockets to communicate with client and monitor, code reused from Beej's Guide to Network Programming
    int sockfd, sockfd1, new_fd, new_fd1;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, hints1, *servinfo, *servinfo1, *p, *p1;
    struct sockaddr_storage their_addr, their_addr1; // connector's address information
    socklen_t sin_size, sin_size1;
    struct sigaction sa;
    int yes=1, yes1 = 1;
    char s[INET6_ADDRSTRLEN], s1[INET6_ADDRSTRLEN];
    int rv, rv1;
//---------------------------Create TCP socket for Client------------------------
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo("127.0.0.1", CLIENTPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
	}
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            	perror("server: socket");
				continue; 
			}
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
		    exit(1); 
		}
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
			continue; 
		}
		break; 
	}
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
//------------------------------Create TCP socket for Monitor------------------//
    memset(&hints1, 0, sizeof hints1);
    hints1.ai_family = AF_UNSPEC;
    hints1.ai_socktype = SOCK_STREAM;
    hints1.ai_flags = AI_PASSIVE; // use my IP
    if ((rv1 = getaddrinfo("127.0.0.1", MONITORPORT, &hints1, &servinfo1)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next) {
        if ((sockfd1 = socket(p1->ai_family, p1->ai_socktype,
                p1->ai_protocol)) == -1) {
                perror("server: socket");
                continue; 
            }
        if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes1,
            sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1); 
        }
        if (bind(sockfd1, p1->ai_addr, p1->ai_addrlen) == -1) {
            close(sockfd1);
            perror("server: bind");
            continue; 
        }
        break; 
    }
    freeaddrinfo(servinfo1); // all done with this structure
    if (p1 == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
	}
    if (listen(sockfd1, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
		exit(1); 
	}
    printf("The AWS is up and running.\n");
    new_fd1 = accept(sockfd1, (struct sockaddr *)&their_addr1, &sin_size1);
    while(1) {  // main accept() loop
        if (listen(sockfd, BACKLOG) == -1) {
            perror("listen");
            exit(1);
        }
        sin_size = sizeof their_addr;
        sin_size1 = sizeof their_addr1;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1 || new_fd1 == -1) {
            perror("accept");
			continue; 
		}
        struct sockaddr_in add, add1;
        memset(&add, 0, sizeof(add));
        memset(&add1, 0, sizeof(add));
        int len = sizeof(add);
        int len1 = sizeof(add1);
        getpeername(new_fd, (struct sockaddr *) &add, (socklen_t *) &len);
        getpeername(new_fd1, (struct sockaddr *) &add1, (socklen_t *) &len1);
        int clientPort = add.sin_port;
        int monitorPort = add1.sin_port;
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            close(sockfd1);
            int numbytes = 0;
            struct commandInfo myCommand;
            memset(&myCommand, 0, sizeof (struct commandInfo));
            char buffer[1024];
            if ((numbytes = recv(new_fd, &myCommand, sizeof (struct commandInfo), 0)) == -1) {
                perror("recv");
                exit(1); 
            }
            // operation taken when command from client is write
            if(strcmp(myCommand.command, "write") == 0) {
                printf("The AWS received operation <write> from the client using TCP over port <24703>\n");
                printf("The AWS sent operation <write> and arguments to the monitor using TCP over port <25703>\n");
                char buffer[1024];
                snprintf(buffer, sizeof(buffer), "The monitor received BW = <%f>, L = <%f>, V = <%f> and P = <%f> from the AWS\n", myCommand.bandwidth, myCommand.length, myCommand.velocity, myCommand.noisePower);
                if (send(new_fd1, buffer, 1024, 0) == -1)
                    perror("send");
                writeToDataBase(myCommand);
                printf("The AWS sent write response to the monitor using TCP over port <25703>\n");
                printf("The AWS sent result to client for operation <write> using TCP over port <24703>\n");
                if (send(new_fd, "The write operation has been completed successfully\n", 1024, 0) == -1)
                    perror("send");
                if (send(new_fd1, "The write operation has been completed successfully\n", 1024, 0) == -1)
                    perror("send");
                close(new_fd);
                exit(0);
            }
            // operation taken when command from client is commpute
            else if(strcmp(myCommand.command, "compute") == 0){
                printf("The AWS received operation <compute> from the client using TCP over port <24703>\n");
                char buffer1[1024];
                snprintf(buffer1, sizeof(buffer), "The monitor received link ID=<%d>, size=<%g>, and power=<%g> from the AWS\n", myCommand.linkID, myCommand.size, myCommand.signalPower);
                if (send(new_fd1, buffer1, 1024, 0) == -1)
                    perror("send");
                printf("The AWS sent operation <compute> and arguments to the monitor using TCP over port <25703>\n");
                struct commandInfo result;
                memset(&result, 0, sizeof (struct commandInfo));
                int message = retrieve(&result, myCommand);
                // operation taken when LinkID is not found
                if(message == 0) {
                    printf("Link ID not found\n");
                    if (send(new_fd, "Link ID not found\n", 1024, 0) == -1)
                        perror("send");
                    printf("The AWS sent compute result to the client using TCP over port <24703>\n");
                    if (send(new_fd1, "Link ID not found\n", 1024, 0) == -1)
                        perror("send");
                    printf("The AWS sent compute results to the monitor using TCP over port <25703>\n");
                }
                // LinkID found, computation can continue
                else {
                    printf("The AWS received link information from Backend-Server A using UDP over port <23703>\n");
                    double res[2];
                    compute(res, result, myCommand);
                    char buffer[1024];
                    snprintf(buffer, sizeof(buffer), "The delay for link <%d> is <%.2f>ms\n", myCommand.linkID, (res[0] + res[1]) * 1000);
                    if (send(new_fd, buffer, 1024, 0) == -1)
                        perror("send");
                    printf("The AWS sent compute result to the client using TCP over port <24703>\n");
                    char buffer1[1024];
                    snprintf(buffer1, sizeof(buffer1), "The result for link <%d>: \nTt = <%.2f>ms,\nTp = <%.2f>ms,\nDelay = <%.2f>ms\n", myCommand.linkID, res[0] * 1000, res[1] * 1000, (res[0] + res[1]) * 1000);
                    if (send(new_fd1, buffer1, 1024, 0) == -1)
                        perror("send");
                    printf("The AWS sent compute results to the monitor using TCP over port <25703>\n");
                }
                close(new_fd);
                exit(0);
            } 
		}
        close(new_fd);  // parent doesn't need this
    }
	return 0; 
}



