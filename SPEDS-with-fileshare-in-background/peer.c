/*
 peer
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
#include <fcntl.h>
#include <ifaddrs.h>

#define PATHSIZE 101

#define MAXDATASIZE 256 // max number of bytes we can get at once
#define USERNAMESIZE 11

#define SEARCHRESULTS "searchResults"

#define SPORT "7054" // port server is listening on


#define PORT "7055" // port peer is listening on
// get sockaddr, IPv4 or IPv6:

 void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);

    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}


int  recv_file(int sock, char * file_name,int options)
{

    int f; /* file handle for receiving file*/
    ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
    int recv_count; /* count of recv() calls*/
    char recv_str[MAXDATASIZE]; /* buffer to hold received data */
    
    if( (f = open(file_name, O_WRONLY|O_CREAT,  0644)) < 0) {
        perror( "error creating file");
        return   -1;      }
            recv_count = 0;     /* number of recv() calls required to receive the file */     
            rcvd_file_size = 0;     /* size of received file */
            /* continue receiving until ? (data or close) */
        while( (rcvd_bytes = recv(sock, recv_str, MAXDATASIZE, options)) > 0)
        {
            recv_count++;
            rcvd_file_size += rcvd_bytes;
            if(write(f, recv_str, rcvd_bytes) < rcvd_bytes)
            {
                perror("error writing to file");
                return   -1;
            }
        }
      close(f);/* close file*/
        printf("Client Received:  %d bytes in %d recv(s) \n", rcvd_file_size, recv_count);
        return  rcvd_file_size;
    }




    int send_file(int  sock, char  *file_name)
    {
    int    sent_count; /* how many sending chunks, for debugging */
    ssize_t read_bytes, /* bytes read from local file */
    sent_bytes, /* bytes sent to connected socket */
        sent_file_size; 
    char  send_buf[MAXDATASIZE]; /* max chunk size for sending file */
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

            while( (read_bytes = read(f, send_buf, MAXDATASIZE)) >  0 )
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



 int main(int argc, char *argv[]) {

        
            if (argc != 4) {
                fprintf(stderr, "usage: p username serverip serverport \n");
                exit(1);
            }

    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    fd_set write_fds;
    int fdmax; // maximum file descriptor number
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char cmd[MAXDATASIZE]; // for local execution 
    char username[USERNAMESIZE];
    char socketstring[5];

    int serverSocketFD, numbytes;
    char buf[MAXDATASIZE]; // recv buffer
    struct addrinfo hints,*ai, *servinfo, *p;

    char s[INET6_ADDRSTRLEN];
    char command[MAXDATASIZE]; //To send to server
    char recvFilePath[PATHSIZE];
    

    char* serverIp;
    int option = 0;
    
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    //get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    sprintf(username,"%s",argv[1]);


    //setup listening socket on PORT to let other peers connect
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "peer: %s\n", gai_strerror(rv));
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
        fprintf(stderr, "peer: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen

    printf("peer: waiting for other peers \n");

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }               


       // setup connection on SPORT with server 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


        // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((serverSocketFD = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
        continue;
    }

    int yes = 1;
    if (setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (connect(serverSocketFD, p->ai_addr, p->ai_addrlen) == -1) {
        close(serverSocketFD);
        perror("client: connect");
        continue;
    }
    break;
}


if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
}
inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),s, sizeof s);
printf("client: connecting to %s\n", s);

        freeaddrinfo(servinfo); // all done with this structure

        if ((numbytes = recv(serverSocketFD, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            close(serverSocketFD);
            exit(1);
        }
        buf[numbytes] = '\0';

        printf("received: %s \n", buf);




        //get local address 

        struct ifaddrs * ifAddrStruct=NULL;
        struct ifaddrs * ifa=NULL;
        void * tmpAddrPtr=NULL;
        char addressBuffer[INET_ADDRSTRLEN];

        getifaddrs(&ifAddrStruct);

        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) {
                continue;
            }
            if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

                if(!strcmp(ifa->ifa_name,"ens33")||!strcmp(ifa->ifa_name,"eth0")){

                    printf("%s IP Address %s \n", ifa->ifa_name, addressBuffer); 
                    break;
                }
            } 
            // else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            //     // is a valid IP6 Address
            //     tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            //     char addressBuffer[INET6_ADDRSTRLEN];
            //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //     printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            // }
        }
        if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);


         //join - send option 1
       
        memset(command,'\0',MAXDATASIZE);

        sprintf(command,"%d %s %s %s",1,username,addressBuffer,PORT);
        
        if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
            perror("send");
            close(serverSocketFD);
            exit(0);
        }

        if ((numbytes = recv(serverSocketFD, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            close(serverSocketFD);
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("received: %s ", buf);

        FD_SET(listener, &master);

        FD_SET(STDIN_FILENO,&master);
        FD_SET(serverSocketFD, &master);

          // keep track of the biggest file descriptor
        fdmax=listener;
        if(serverSocketFD>fdmax)
            fdmax = serverSocketFD;
        
        option=0;


// main loop
    for (;;) {

            //printf("before select: active sockets: %d %d %d , option: %d \n",serverSocketFD, listener, STDIN_FILENO, option);

            if (option == 0) {        
                printf("\n----MENU---- \n Enter a Command \n 1. Publish [1 filename] \n 2. Search [2 searchkey]  \n 3. Fetch [3 peerip peerport filename] \n 4. Quit \n ");            
            }

            read_fds = master; // copy it
            //write_fds = master;
            if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(4);
            }

        //printf("after select: active sockets: %d %d %d , option: %d \n",serverSocketFD, listener, STDIN_FILENO, option);




        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!

                //printf("%d is set",i);
                if (i == listener) {
                    // handle new connections from peers
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
                        printf("peer: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*) &remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);


                        if (send(newfd, "Hi \n Enter File Name :", 22, 0) == -1) {
                            perror("send");
                            close(serverSocketFD);
                            close(listener);                                        
                            exit(0);
                        }                       

                    }
                }

                else if (i == STDIN_FILENO) {
                    // read data from keyboard


                    memset(cmd,'\0',MAXDATASIZE);
                    fgets(cmd, MAXDATASIZE, stdin);
                    
                    char pathstring[PATHSIZE];
                    memset(pathstring,'\0',PATHSIZE);

                    sscanf(cmd,"%d %s",&option,pathstring);        

                    if(option!=1&&option!=2&&option!=3&&option!=4){
                        printf("incorrect usage, try again  %d\n",option);
                         option =0;
                    }             

                    

                    if (option == 1) {

                                 // check if file exists
                                if( access( pathstring, F_OK ) != -1 ) {                                       

                                    if(strlen(pathstring)==0){
                                        printf("incorrect usage, try again \n");
                                        option =0;

                                    }
                                    else{                         

                                       memset(command, '\0', MAXDATASIZE);

                                       sprintf(command,"%d %s %s",2,username,pathstring);                      
                                       printf("command sent: %s \n", command);


                                       if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
                                        perror("send");                                   

                                        }
                                    }

                                }
                            else {// file doesn't exist
                                    printf("File doesn't exist \n");
                                    option =0;
                            }
                            


                    }                   
                    else if (option == 2) { //Search                                                                 

                                    if(strlen(pathstring)==0){
                                        printf("incorrect usage, try again \n");
                                        option =0;

                                    }
                                    else{

                                          

                                            memset(command, '\0', MAXDATASIZE);                                        
                                            sprintf(command,"%d %s",3,pathstring);
                                            printf("command sent: %s \n", command);                          


                                            if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
                                                perror("send");                                                
                                                
                                            }
                                        

                                    }

                    }
                    else if(option == 3){

                                 //connect to peer and fetch file

                                    char addressBuffer[INET_ADDRSTRLEN];
                                    char peerPort[10];


                                     memset(recvFilePath,'\0',PATHSIZE);
                                     memset(addressBuffer,'\0',INET_ADDRSTRLEN);
                                     memset(peerPort,'\0',10);

                                    sscanf(cmd,"%d %s %s %s",&option,addressBuffer,peerPort,recvFilePath);   

                                    if(strlen(pathstring)==0||strlen(addressBuffer)==0||strlen(peerPort)==0||strlen(recvFilePath)==0){
                                        printf("incorrect usage, try again \n");
                                        option =0;

                                    }
                                    else{                                    
                                                       

                                                struct addrinfo hints,*ai, *peerinfo, *p;
                                                int peerSocketFD;


                                                memset(&hints, 0, sizeof hints);
                                                hints.ai_family = AF_UNSPEC;
                                                hints.ai_socktype = SOCK_STREAM;
                                                if ((rv = getaddrinfo(addressBuffer, peerPort, &hints, &peerinfo)) != 0) {
                                                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                                                    return 1;
                                                }


                                                // loop through all the results and connect to the first we can
                                                for (p = peerinfo; p != NULL; p = p->ai_next) {
                                                    if ((peerSocketFD = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
                                                        perror("client: socket");
                                                        continue;
                                                    }

                                                    int yes = 1;
                                                    if (setsockopt(peerSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
                                                        perror("setsockopt");
                                                        
                                                    }

                                                    if (connect(peerSocketFD, p->ai_addr, p->ai_addrlen) == -1) {
                                                        close(peerSocketFD);
                                                        perror("client: connect");
                                                        continue;
                                                    }
                                                    break;
                                                }


                                                if (p == NULL) {
                                                    fprintf(stderr, "client: failed to connect to peer\n");
                                                    return 2;
                                                }


                                                FD_SET(peerSocketFD, &master); // add to master set
                                                if (peerSocketFD > fdmax) { // keep track of the max
                                                 fdmax = peerSocketFD;
                                             }

                                             inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),s, sizeof s);
                                             printf("client: connecting to %s\n", s);

                                                freeaddrinfo(peerinfo); // all done with this structure

                                                                                                    
                                                memset(command, '\0', MAXDATASIZE);
                                                sprintf(command,"%d %s",7,recvFilePath);    //option 7 for requesting file   

                                                //printf("command sent: %s \n", command);                                 



                                                if (send(peerSocketFD, command, MAXDATASIZE, 0) == -1) {
                                                    perror("send filename");
                                                    close(peerSocketFD);                                                     


                                                }

                                                option=8;


                                            }                                         

                                        }

                                //quit
                                    else if (option == 4) {

                                
                                                    memset(command, '\0', MAXDATASIZE);
                                                
                                                    sprintf(command,"%d %s",5,argv[1]);

                                                    if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
                                                        perror("send");
                                                        close(serverSocketFD);
                                                        close(listener);
                                                        exit(0);
                                                     }

                                                    close(listener);
                                                    close(serverSocketFD);
                                                    exit(0);
                                    
                                }      

                            }                  

                            else if(i==serverSocketFD) {
                                     // handle data from server socket


                                        if(option==1){ // receive message after publish success

                                                       if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                                                                // got error or connection closed by client
                                                               if (nbytes == 0) {
                                                                                                // connection closed
                                                                   printf("peer: socket %d hung up\n", i);
                                                               } else {
                                                                   perror("recv");
                                                               }
                                                                close(i); // bye!
                                                                printf("%d is removed from master \n", i);
                                                                FD_CLR(i, &master); // remove from master set
                                                        }else{

                                                            printf("received: %s \n", buf);

                                                        }


                                                    } 

                                        else if(option==2){ // recv search results

                                                        memset(cmd, '\0', MAXDATASIZE);

                                                        strcat(cmd, "rm searchResults");

                                                        int ret;
                                                        if ((ret = system(cmd)) == -1) {
                                                            perror("print search results");
                                                        }


                                                        int recvFileSize= recv_file(i,SEARCHRESULTS,MSG_DONTWAIT);

                                                        memset(cmd, '\0', MAXDATASIZE);
                                                //write cat command to print searchResults file

                                                        if(recvFileSize==0){
                                                            printf("No results\n");
                                                        }
                                                        else{
                                                            printf("search results: \n");
                                                            strcat(cmd, "cat searchResults");



                                                            if ((ret = system(cmd)) == -1) {
                                                                perror("print search results");
                                                            }

                                                        }




                                                    } // end act on data from server on option 2

                                        else{ // handle server exit

                                              if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                                                                    // got error or connection closed by client
                                               if (nbytes == 0) {
                                                                                    // connection closed
                                                   printf("peer: socket %d hung up\n", i);
                                               } else {
                                                   perror("recv");
                                               }
                                                                close(i); // bye!
                                                                printf("connection with server is lost.. exiting\n");
                                                                for(int i=0;i<fdmax;i++){
                                                                    close(i); // close all sockets
                                                                }                                                                
                                                                exit(0);
                                                            }else{

                                                                printf("received: %s \n", buf);

                                                            }


                                            }

                                            option=0;


                                 } // END handle data from server

                                 else // Handle Data From Peer
                                 {                           

                                            if(option==8){ // peer is sending file
                                                fflush(0);
                                                if(!fork()){ // recv file with child process
                                                     for(int j=0;j<fdmax;j++){
                                                      if(j!=i) close(j);
                                                     }
                                                  int recvFileSize= recv_file(i,recvFilePath,0);
                                                  printf("%s received, size : %d\n", recvFilePath, recvFileSize);
                                                  close(i);
                                                  exit(0);
                                                }
                                          close(i);

                                          FD_CLR(i,&master);

                                      }
                                            else{ // peer is requesting a file

                                                int peerOption=0,nbytes=0;

                                                char pathstring[PATHSIZE];

                                                if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                                                                // got error or connection closed by client
                                                   if (nbytes == 0) {
                                                                                // connection closed
                                                       printf("peer: socket %d hung up\n", i);
                                                   } else {
                                                       perror("recv");
                                                   }
                                                            close(i); // bye!
                                                            printf("%d is removed from master \n", i);
                                                            FD_CLR(i, &master); // remove from master set
                                                        }else{

                                                            sscanf(buf,"%d %s",&peerOption,pathstring);

                                                            if(peerOption==7){

                                                                if( access(pathstring, F_OK ) != -1 ) {
                                                                        // file exists
                                                                    fflush(0);
                                                                    if(!fork()){ // send file with a child process
                                                                         for(int j=0;j<fdmax;j++){
                                                                            if(j!=i) close(j);
                                                                         }
                                                                        int sentFileSize=send_file(i,pathstring);
                                                                        printf("%s sent, size: %d \n", pathstring,sentFileSize);
                                                                        close(i);
                                                                        exit(0);
                                                                    }
                                                                close(i);
                                                                FD_CLR(i,&master);

                                                            } else {
                                                                        // file doesn't exist
                                                                if (send(i, "File Not Found\n", 15, 0) == -1) {
                                                                    perror("send");                                                     
                                                                    close(i);
                                                                    exit(0);
                                                                }

                                                            }
                                                        }
                                                    }
                                                }

                                                option=0;       
                                        //}


                                 }// end Handle Data From Peer


                            } // END - isset

                    } // END looping through file descriptors

                } // END for(;;)--and you thought it would never end!

                return 0;
            } 
