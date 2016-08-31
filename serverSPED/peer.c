/*
 peer
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

#define PATHSIZE 101
#define PORT "7054" // the port client will be connecting to
#define MAXDATASIZE 256 // max number of bytes we can get at once
#define USERNAMESIZE 11
// get sockaddr, IPv4 or IPv6:

 void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}


int recv_file(int sock, char * file_name)
{
    char send_str [MAX_SEND_BUF];/* message to be sent to server*/
    int f; /* file handle for receiving file*/
    ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
    int recv_count;/* count of recv() calls*/
    char recv_str[MAX_RECV_BUF]; /* buffer to hold received data */
    size_t send_strlen; /* length of transmitted string */
    sprintf(send_str, "%s \n", file_name); /* add CR/LF (new line) */
    send_strlen = strlen(send_str); /* length of message to be transmitted */
    if( (sent_bytes = send(sock, file_name, send_strlen, 0)) <0)
    {
            perror("send error");
                        return -1;
    }/* attempt to create file to save received data. 0644 = rw-r--r-- */
    if( (f = open(file_name, O_WRONLY|O_CREAT,0644)) < 0)
    {
        perror("error creating file");
        return -1;
    }
    recv_count = 0; /* number of recv() calls required to receive the file */
    rcvd_file_size = 0; /* size of received file */
    
    /* continue receiving until ? (data or close) */
    while( (rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) > 0)
    {
        recv_count++;
        rcvd_file_size += rcvd_bytes;
        if(write(f, recv_str, rcvd_bytes) < 0)
        { 
            perror("error writing to file");
            return -1;
        }
    }
    close(f); /* close file*/
    printf("Client Received: %d bytes in %d recv(s) \n", rcvd_file_size, recv_count);
    return rcvd_file_size;
}



int main(int argc, char *argv[]) {

    int sockfd, numbytes;
    char buf[MAXDATASIZE]; // recv buffer
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char command[MAXDATASIZE]; //To send to server
    char cmd[MAXDATASIZE]; // Execute at local machine
    char username[USERNAMESIZE];
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
    memset(username, ' ', USERNAMESIZE);
    strncpy(username,argv[1],strlen(argv[1]));
    username[USERNAMESIZE-1]='\0';

    memset(command,'\0',MAXDATASIZE);
    strcat(command, "1 ");
    strncat(command, username, USERNAMESIZE-1); //append username
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
            printf("\n----MENU---- \n Enter a Number From following \n 1. Publish \n 2. Search & Fetch \n 3. Quit \n ");
            scanf("%d", &option);
            continue;
        }

        //Publish
        if (option == 1) {
            //send option 2 
            printf("Enter path of file to publish: ");


            char pathstring[PATHSIZE];
            memset(pathstring,' ',PATHSIZE);

            scanf("%s", pathstring);

            strncpy(pathstring,pathstring,strlen(pathstring));

            pathstring[PATHSIZE-1]='\0';

            memset(command, '\0', MAXDATASIZE);
            strcat(command, "2 ");
            
            strncat(command, username, USERNAMESIZE-1); //append username
            

            strncat(command, pathstring, PATHSIZE-1);
            printf("command sent: %s", command);

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


        }

        //Search & Fetch
        if (option == 2) {
            //send option 3

            printf("Enter key to search: ");


            char searchKey[PATHSIZE];
            memset(searchKey,' ',PATHSIZE);

            scanf("%s", searchKey);

            strncpy(searchKey,searchKey,strlen(searchKey));

            searchKey[PATHSIZE-1]='\0';

            memset(command, '\0', MAXDATASIZE);
            strcat(command, "3 ");
            
            strncat(command, username, USERNAMESIZE-1); //append username
            

            strncat(command, searchKey, PATHSIZE-1);
            printf("command sent: %s", command);

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
