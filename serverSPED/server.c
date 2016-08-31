/*
 server
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
#define PATHSIZE 101


#define PORT "7054" // port we're listening on
// get sockaddr, IPv4 or IPv6:

 void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);

    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int send_file(int  sock, char  *file_name)
{
    int    sent_count; /* how many sending chunks, for debugging */
    ssize_t read_bytes, /* bytes read from local file */
    sent_bytes, /* bytes sent to connected socket */
    sent_file_size; 
    char  send_buf[MAX_SEND_BUF]; /* max chunk size for sending file */
    char   * errmsg_notfound = "File not found\n";
    int  f; /* file handle for reading local file*/
    sent_count =  0 ;
    sent_file_size =  0 ; /* attempt to open requested file for reading */
    if( (f = open(file_name, O_RDONLY)) < 0 )  /* can't open requested file */
    {
        perror(file_name);
        if( (sent_bytes = send(sock, errmsg_notfound , strlen(errmsg_notfound), 0)) < 0 )
        {
          perror( "send error");
          return  -1;
      }
  }
  else/* open file successful */
  {
    printf("Sending file:  %s \n",file_name);
    while( (read_bytes = read(f, send_buf, MAX_RECV_BUF)) >  0 )
    {
        if ( (sent_bytes = send(sock, send_buf, read_bytes,0)) < read_bytes )
        {
            perror("send error");
            return  -1;
        }
        sent_count++;
        sent_file_size += sent_bytes;
    }
    close(f);
}/* end else */
printf("Done with this client. Sent %d  bytes in %d  send(s) \n\n", sent_file_size, sent_count);
return sent_count;
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
    char buf[MAXDATASIZE];  
    char cmd[MAXDATASIZE]; // for local execution 
    char username[USERNAMESIZE];
    char socketstring[5];
    
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

    printf("server: waiting for connections \n");

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one



    // create required directories
    memset(cmd, '\0', MAXDATASIZE);
    strcat(cmd, "mkdir -p  publishedFiles");


    int ret;
    if ((ret = system(cmd)) == -1) {
        perror("create publishedFiles dierectory");
    }
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
                        int option = buf[0] - '0';


                        memset(username, ' ', USERNAMESIZE);
                        strncpy(username, buf + 2,USERNAMESIZE-1);
                        username[USERNAMESIZE-1]='\0';


                        printf("server: option: %d , user: %s \n", option, username);

                        //join 
                        if (option == 1) {

                            //store socket and username in file


                            snprintf(socketstring, 5, "%d", i);
                            memset(cmd, '\0', MAXDATASIZE);
                            strcat(cmd, "echo ");
                            strcat(cmd, username);
                            strcat(cmd, " ");
                            strcat(cmd, socketstring);
                            strcat(cmd, " >> clientlist");

                            int ret;
                            if ((ret = system(cmd)) == -1) {
                                perror("write to clientlist");
                            }

                            if (WIFSIGNALED(ret) &&
                                (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
                                perror("write to clientlist");
                        }


                        memset(buf, '\0', MAXDATASIZE);
                        strcat(buf, "hello ");
                        strcat(buf, username);
                        strcat(buf, " \n you joined successfully \n");
                        if (send(i, buf, MAXDATASIZE - 1, 0) == -1) {
                            perror("send");
                            exit(0);
                        }
                    }

                        //publish
                    if (option == 2) {

                        char pathstring[PATHSIZE];

                        memset(pathstring,' ', PATHSIZE);
                        strncpy(pathstring, buf+2+(USERNAMESIZE-2)+1, PATHSIZE-1);
                        pathstring[PATHSIZE-1]='\0';

                            // put published path in a file 
                        strcat(cmd, "echo ");
                        strcat(cmd, pathstring);                            
                        strcat(cmd, " >> ./publishedFiles/");
                        strncat(cmd, username, USERNAMESIZE-1);                                        

                        int ret;
                        if ((ret = system(cmd)) == -1) {
                            perror("remove from clientlist");
                        }

                        memset(buf, '\0', MAXDATASIZE);
                        strcat(buf, pathstring);
                        strcat(buf, " is published successfully");

                        if (send(i, buf, MAXDATASIZE - 1, 0) == -1) {
                            perror("send");
                            exit(0);
                        }


                    }

                        //search & fetch
                    if (option == 3) {

                        char searchKey[PATHSIZE];

                        memset(searchKey,'\0', PATHSIZE);
                        strncpy(searchKey, buf+2+(USERNAMESIZE-2)+1, PATHSIZE-1);


                        //write awk command to search all files and store results in searchResults file
                        strcat(cmd, "cat * | awk /");
                        strcat(cmd, searchKey);   
                        strcat(cmd, "/");                            
                        strcat(cmd, " >> searchResults");                                      

                        int ret;
                        if ((ret = system(cmd)) == -1) {
                            perror("remove from clientlist");
                        }

                        //send contents of searchResults

                        send_file(i,"./publishedFiles/searchResults");



                    }

                        //quit
                    if (option == 4) {

                            //remove user from clientlist
                        snprintf(socketstring, 5, "%d", i);
                        memset(cmd, '\0', MAXDATASIZE);                            
                        strcat(cmd, "sed -i \"/$1 $2/d\" clientlist ");
                        strcat(cmd, username);                            
                        strcat(cmd, " ");
                        strcat(cmd, socketstring);            

                        int ret;
                        if ((ret = system(cmd)) == -1) {
                            perror("remove from clientlist");
                        }

                    }





                }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    return 0;
}
