/*
 ** client.c -- a stream socket client demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define PATHSIZE 100
#define PORT "7054" // the port client will be connecting to
#define MAXDATASIZE 256 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char cmd[MAXDATASIZE];
    char* serverIp;
    int option = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: p username servername \n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[2], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            exit(1);
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
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        close(sockfd);
        exit(1);
    }
    buf[numbytes] = '\0';

    printf("received: %s \n", buf);


    //join - send option 1
    char command[MAXDATASIZE];
    memset(command, '\0', MAXDATASIZE);
    strcat(command, "1 ");
    strcat(command, argv[1]); //append username
    if (send(sockfd, command, MAXDATASIZE - 1, 0) == -1) {
        perror("send");
        close(sockfd);
        exit(0);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        close(sockfd);
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("received: %s ", buf);

    option = 0;
    while (1) {

        if (option == 0) {
            // menu
            printf("----MENU---- \n Enter a Number From following \n 1. Publish \n 2. Search & Fetch \n 3. Quit \n ");
            scanf("%d", &option);
            continue;
        }

        //Publish
        if (option == 1) {
            //send option 2 
            printf("Enter path of folder or file to publish: ");
            char path[PATHSIZE];
            scanf("%s", path);


            memset(cmd, '\0', MAXDATASIZE);
            strcat(cmd, "./publish.sh ");
            strcat(cmd, path);
            strcat(cmd, " ");
            strcat(cmd, s); //append server ip address
            strcat(cmd, " ");
            strcat(cmd, PORT); //append server port number

            int ret;
            if ((ret = system(cmd)) == -1) {
                perror("write to clientlist");
            }

            if (WIFSIGNALED(ret) &&
                    (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
                perror("write to clientlist");
            }

            option = 0;


        }

        //Search & Fetch
        if (option == 2) {
            //send option 3

        }

        //quit
        if (option == 3) {

            //send option 4   
            memset(command, '\0', MAXDATASIZE);
            strcat(command, "4 ");
            strcat(command, argv[1]); //append username

            if (send(sockfd, command, MAXDATASIZE - 1, 0) == -1) {
                perror("send");
                close(sockfd);
                exit(0);
            }

            close(sockfd);
            exit(1);
        }

    }
}
