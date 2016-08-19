/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 ** selectserver.c -- a cheezy multiperson chat server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#define USERNAMESIZE 11
#define MAXDATASIZE 256


#define PORT "7054" // port we're listening on
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);

    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(void) {
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    fd_set write_fds;
    int fdmax; // maximum file descriptor number
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[MAXDATASIZE]; // buffer for client data
    char cmd[MAXDATASIZE];
    char username[USERNAMESIZE];
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    struct addrinfo hints, *ai, *p;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    // main loop
    for (;;) {
        read_fds = master; // copy it
        write_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                            (struct sockaddr *) &remoteaddr,
                            &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("server: new connection from %s on "
                                "socket %d\n",
                                inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*) &remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                                newfd);



                        //store socket and username in file

                        char newsockfd[5];
                        snprintf(newsockfd,5,"%d", newfd);
                        memset(cmd, '\0', MAXDATASIZE);
                        strcat(cmd, "echo ");
                        strcat(cmd, newsockfd);
                        strcat(cmd, " ");
                        strcat(cmd, username);
                        strcat(cmd, " >> clientlist");

                        int ret;
                        if ((ret = system(cmd)) == -1) {
                            perror("write to clientlist");
                        }

                        if (WIFSIGNALED(ret) &&
                                (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
                            perror("write to clientlist");
                        }

                        //act on sockets to write to 

                        if (send(newfd, "connected", 9, 0) == -1) {
                            perror("send");
                            exit(0);
                        }
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        printf("%d is removed from master \n", i);
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client

                        //data from socket i is in buf

                        

                        //receive options
                        memset(cmd, '\0', MAXDATASIZE);
                        int option = (int) buf[0];

                        
                        memset(username, '\0', USERNAMESIZE);
                        strncpy(username, buf + 2, USERNAMESIZE - 1);


                        printf("server: option: %d , user: %s ", option, username);
                        memset(buf, '\0', MAXDATASIZE);
                        strcat(buf, "hello ");
                        strcat(buf, username);
                        strcat(buf, " \n you joined successfully \n");
                        if (send(i, buf, MAXDATASIZE - 1, 0) == -1) {
                            perror("send");
                            exit(0);
                        }




                        /*for (j = 0; j <= fdmax; j++) {
                            // send to everyone!                          
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves                              
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }*/
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    return 0;
}
