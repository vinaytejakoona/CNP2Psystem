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


#define PORT "7056" // port peer is listening on
// get sockaddr, IPv4 or IPv6:

 void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);

    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "usage: p username servername \n");
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
    if ((rv = getaddrinfo(argv[2], SPORT, &hints, &servinfo)) != 0) {
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




        //get local address and port

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
	            //char ens33[6]=;
	            //char eth0[5]="eth0";
	            if(!strcmp(ifa->ifa_name,"ens33")||!strcmp(ifa->ifa_name,"eth0")){
	            	
	            	printf("%s IP Address %s \n", ifa->ifa_name, addressBuffer); 
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
        memset(username, ' ', USERNAMESIZE);
        strncpy(username,argv[1],strlen(argv[1]));
        username[USERNAMESIZE-1]='\0';

        memset(command,'\0',MAXDATASIZE);
        strcat(command, "1 ");
        strncat(command, username, USERNAMESIZE-1); //append username

        strncat(command, addressBuffer, INET_ADDRSTRLEN); //append IP address
        
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

        option = 0;


        // add the listener to the master set
        FD_SET(listener, &master);
        FD_SET(serverSocketFD, &master);
        FD_SET(STDIN_FILENO,&master);

        // keep track of the biggest file descriptor
        if(listener>serverSocketFD)
            fdmax = listener;
        else
        fdmax = serverSocketFD; // so far, it's this one


// main loop
    for (;;) {

        printf("active sockets: %d %d %d",serverSocketFD, listener, STDIN_FILENO);

        if (option == 0) {        
            printf("\n----MENU---- \n Enter a Number From following \n 1. Publish \n 2. Search  \n 3. Fetch \n 4. Quit \n ");            
        }

        read_fds = master; // copy it
        //write_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }




        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!

            	 printf("%d is set",i);
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


                        //act on sockets to write to 

                        if (send(newfd, "connected", 9, 0) == -1) {
                            perror("send");
                             close(serverSocketFD);
							close(listener);                                        
                            exit(0);
                        }

                        //recv file name and send file to the peer





                    }
                }

                else if (i == STDIN_FILENO) {
                    // read data from keyboard


                    memset(cmd,' ',MAXDATASIZE);
                    fgets(cmd, MAXDATASIZE, stdin);
                    sscanf(cmd,"%d",&option);
                    
                    

                        //recv file name and send file

			                    if (option == 1) {
			                                    //send option 2 
			                        printf("Enter path of file to publish: ");


			                        char pathstring[PATHSIZE];
			                        memset(pathstring,' ',PATHSIZE);


			                        scanf("%s",pathstring);


			                        strncpy(pathstring,pathstring,strlen(pathstring));

			                        pathstring[PATHSIZE-1]='\0';

			                        memset(command, '\0', MAXDATASIZE);
			                        strcat(command, "2 ");

                                    strncat(command, username, USERNAMESIZE-1); //append username
                                    

                                    strncat(command, pathstring, PATHSIZE-1);
                                    printf("command sent: %s", command);

                                    if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
                                        perror("send");
                                        close(serverSocketFD);
                                         close(listener);
                                        exit(0);
                                    }
                                  


                                }

                                //Search 
                                if (option == 2) {
                                    //send option 3

                                    printf("Enter key to search: ");


                                    char searchKey[PATHSIZE];
                                    memset(searchKey,'\0',PATHSIZE);

                                    scanf("%s",searchKey);
                                    
                                    memset(command, '\0', MAXDATASIZE);
                                    strcat(command, "3 ");
                                    
                                    strncat(command, username, USERNAMESIZE-1); //append username
                                    

                                    strcat(command, searchKey);

                                    printf("command sent: %s ", command);                             


                                    if (send(serverSocketFD, command, MAXDATASIZE - 1, 0) == -1) {
                                        perror("send");
                                        close(serverSocketFD);
                                        close(listener);
                                        exit(0);
                                    }


                                }
                                else if(option == 3){

 								 //connect to peer and fetch file



                                	char addressBuffer[INET_ADDRSTRLEN];
                                    char peerPort[10];

                                	printf("Enter IP Address of Peer: ");

                                    scanf("%s",addressBuffer);

                                    printf("Enter PORT of Peer: ");

                                    scanf("%s",peerPort);

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
												        if ((peerSocketFD = socket(p->ai_family, p->ai_socktype,
															            p->ai_protocol)) == -1) {
															            perror("client: socket");
															        continue;
															    }

															    int yes = 1;
															    if (setsockopt(peerSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
															        perror("setsockopt");
															        exit(1);
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
												inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),s, sizeof s);
												printf("client: connecting to %s\n", s);

												        freeaddrinfo(peerinfo); // all done with this structure

												        if ((numbytes = recv(peerSocketFD, buf, MAXDATASIZE - 1, 0)) == -1) {
												            perror("recv");
												            close(peerSocketFD);
												            exit(1);
												        }
												        buf[numbytes] = '\0';

												        printf("received: %s \n", buf);


                                }

                                //quit
                                if (option == 4) {

                                    //send option 4  
                                    memset(command, '\0', MAXDATASIZE);
                                    strcat(command, "4 ");
                                    strcat(command, argv[1]); //append username

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

                            	    if(option==1){

	                            	      if ((numbytes = recv(serverSocketFD, buf, MAXDATASIZE - 1, 0)) == -1) {
	                                         if (nbytes == 0) {
									                            // connection closed
									                            printf("peer: socket %d hung up\n", i);
									                        } else {
									                            perror("recv");
									                        }
									                        close(i); // bye!
									                        printf("%d is removed from master \n", i);
									                        FD_CLR(i, &master); // remove from master set
									                        close(listener);
									                        exit(0);
	                                      	  }
		                                    buf[numbytes] = '\0';
		                                    printf("received: %s ", buf);
		                                    option = 0;
                                	}

                            	 	if(option==2){



		                                        int f; /* file handle for receiving file*/
		                                        ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
		                                        int recv_count;/* count of recv() calls*/
		                                        char recv_str[MAXDATASIZE]; /* buffer to hold received data */   


                            	 		 		/* attempt to create file to save received data. 0644 = rw-r--r-- */
			                                    if( (f = open(SEARCHRESULTS, O_WRONLY|O_CREAT,0644)) < 0)
			                                    {
			                                        perror("error creating file");
			                                        return -1;
			                                    }
		                                        recv_count = 0; /* number of recv() calls required to receive the file */
		                                        rcvd_file_size = 0; /* size of received file */

												
		                                        /* continue receiving until ? (data or close) */

		                            	 		while((rcvd_bytes = recv(serverSocketFD, recv_str, MAXDATASIZE, MSG_DONTWAIT))>0)
		                                    	{                                  		 
                                    		
												  rcvd_file_size += rcvd_bytes;				                                        	

				                                        if(write(f, recv_str, rcvd_bytes) < 0)
				                                        { 
				                                            perror("error writing to file");
				                                            return -1;
				                                        }
				                                }
					                                                                                                                         

				                                        	                                     
		                                        close(f); /* close file*/
			                                    printf("Client Received: %d bytes in %d recv(s) \n", rcvd_file_size, recv_count);  


			                                    memset(cmd, '\0', MAXDATASIZE);
			                                    //write cat command to print searchResults file

			                                    printf("search results: \n");
			                                    strcat(cmd, "cat searchResults");


			                                    int ret;
			                                    if ((ret = system(cmd)) == -1) {
			                                        perror("print search results");
			                                    }

			                                    option=0;


                                        } // end act on data from server on option 2

                                       


                            	 	} // END handle data from server
                                        
                                  

                                } // END - isset
                    } // END looping through file descriptors

                } // END for(;;)--and you thought it would never end!

                return 0;
} 
        
