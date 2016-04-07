#define _XOPEN_SOURCE 600
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
    // for ctime_r
#include <arpa/inet.h>
    // for htonl, htons, inet_ntop, etc.
#include <errno.h>
    // for errno
#include <netinet/in.h>
    // for sockaddr_in
#include <stdio.h>
    // for perror
#include <stdlib.h>
    // for exit, atoi
#include <string.h>
    // for memset
#include <sys/socket.h>
    // for socket, bind, listen, accept
#include <sys/stat.h>
    // for stat
#include <time.h>
    // for time, ctime, and time_t
#include <sys/types.h>
    // recommended for socket functions
#include <unistd.h>
    // for close
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>

/**
 * Structure to hold command-line arguments
 **/
typedef struct nc_args {
  char* port; // server port
  char* server;        // remote server for an active connection
  bool listen;         // listen flag
  bool verbose;        // verbose output info
  bool udp;
  bool udpProtected;
} nc_args_t;


/**
 *parse_args(nc_args_t * nc_args, int argc, char * argv[]) -> void
 *
 * Given a pointer to a nc_args struct and the command line argument
 * info, set all the arguments for nc_args to function use getopt()
 * procedure.
 *
 * Return:
 *     void, but nc_args will have return resutls
 **/

void parse_args(nc_args_t * nc_args, int argc, char * argv[]){

  //set defaults
  nc_args->listen = 0;
  nc_args->verbose = false;
  nc_args->server = NULL;
  nc_args->port = NULL;
  nc_args->udp = false;
  nc_args->udpProtected = false;

  int ch;
  while ((ch = getopt(argc, argv, "lvuU")) != -1) {
    switch (ch) {
    case 'l': //listen
      nc_args->listen = true;
      break;
    case 'v':
      nc_args->verbose = true;
      break;
    case 'u':
      nc_args->udp = true;
      break;
    case 'U':
      nc_args->udpProtected = true;
      break;
    default:
      fprintf(stderr,"ERROR: Unknown option '-%c'\n",ch);
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  if (nc_args->listen) {
    // Passive mode: remaining argument should be a port
    if (argc != 1) {
      fprintf(stderr, "ERROR: -l implies a single port argument\n");
      exit(1);
    } else {
      nc_args->port = (argv[0]);
    }

  } else {
    // Active mode: remaining arguments should be hostname and a port
    if (argc != 2) {
      fprintf(stderr, "ERROR: hostname and port expected\n");
      exit(1);
    } else {
      nc_args->server = argv[0];
      nc_args->port =argv[1];
    }
  }

  return;

}

int writen(int fd, const void *buffer, size_t n)
{
  const char *p = buffer;
  size_t nleft = n;

  while (nleft >0) {
    // write up to left bytes. Due to buffer limits
    //   and interruptions, we might not
    //   write everything. The return value tells us
    //   how many bytes did get written.
    int nwritten = write(fd, p, nleft);
    if (nwritten < 0 && errno == EINTR) {
      // some system event interrupted us;
      // try the write again.
      nwritten = 0;
    } else if (nwritten < 0) { 
      // Unrecoverable error
      return -1;
    }
    nleft -= nwritten;
    p += nwritten; 
  }
  // Success!  n bytes written.
  return n;
}

ssize_t readn(int fd, void *buffer, size_t n)
{
  char *p = buffer;
  size_t nleft = n;
  while (nleft > 0) {
     ssize_t nread = read(fd, p, nleft);
     if (nread < 0) {
       if (errno == EINTR) {
         // We were interrupted before reading and will retry
         nread = 0;
       } else {
         // Unrecoverable error
         return -1;
       }
     } 
     else if (nread == 0) {
       // A successful read of 0 bytes means end-of-file
      break; }
      nleft -= nread;
      p += nread; 
    }
  return (n - nleft);  // strictly positive if there weren’t n characters
}
//   PAINFULLY SLOW
ssize_t readline(int fd, char *buffer, size_t maxlen)
{
  for (int nsofar = 0; nsofar < maxlen-1; ++nsofar) {
    int nread = readn(fd, buffer+nsofar, 1);
    if (nread < 0) {
      // Unrecoverable error while reading.
      return -1;
    } else if (nread == 0) {
      // Hit end-of-file
      return nsofar;
    } else {
      if (buffer[nsofar] == '\n') {
        buffer[nsofar+1] = '\0';
        return nsofar+2;
      }
    } 
  }
  // Output buffer must have filled up.
  return maxlen;
}


// Sets up the server for UDP transfer protocol
void server_udp(nc_args_t nc_args) {





}




int main(int argc, char * argv[]) {

  nc_args_t nc_args;

  //initializes the arguments struct for your use
  parse_args(&nc_args, argc, argv);


  // FOR DEBUGGING PURPOSES ONLY - FEEL FREE TO DELETE
  fprintf(stderr, "Verbose flag is %s\n",
            nc_args.verbose ? "true" : "false");
  fprintf(stderr, "Listening flag is %s\n",
            nc_args.listen ? "true" : "false");
  fprintf(stderr, "Port is %s\n", nc_args.port);
  if (! nc_args.listen) {
    fprintf(stderr, "Server is %s\n", nc_args.server);
  }
  if (nc_args.udp) {
    fprintf(stderr, "Using UDP");
  } else if (nc_args.udpProtected) {
    fprintf(stderr, "Using Smart UDP");
  } else {
    fprintf(stderr, "Using TCP");
  }


  if (nc_args.listen){ // THE SERVER (listening)
    fprintf(stderr, "%s\n","Setting up the server");
    // Step 0: Choose the server port, based on the command-line arguments.
    //int serv_port = nc_args.port;

    // Setyp getaddrinfo section!
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;  // will point to the results
    int yes=1;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6

    // choose whether we are using TCP or UDP
    int protocol;
    if (nc_args.udp || nc_args.udpProtected) {
      protocol = SOCK_DGRAM;
    } else {
      protocol = SOCK_STREAM;
    }
    hints.ai_socktype = protocol; // protocol stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, (char*) nc_args.port, &hints, &servinfo)) != 0) {
      if (nc_args.verbose) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      }
      exit(1);
    }

    int sockfd;
    // loop through all the results and create the socket and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
          if (nc_args.verbose){
            perror("server: socket");
          }
          continue;
      }

      // what is this doing? - setting up socket permanent error?
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
              sizeof(int)) == -1) {
        if (nc_args.verbose) {
          perror("socket");
        }
        exit(2);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        if (nc_args.verbose) {
          perror("server: bind");
        }
          close(sockfd);
          continue;
      }

      break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    fprintf(stderr, "%s\n","freed serv");

    if (p == NULL)  {
      if (nc_args.verbose) {
        fprintf(stderr, "server: failed to bind\n");
      }
      exit(3);
    }


    // Do UDP
    if (nc_args.udp) {
      const int DATAGRAMSIZE = 1200;
      char buffer[DATAGRAMSIZE];
      //struct sockaddr *from;
      struct sockaddr_in addr;
      socklen_t fromlen;

      // receive datagrams from the client and print to stdout
      for (;;) {
        int recvBytes = recvfrom(sockfd, buffer, DATAGRAMSIZE, 0, (struct sockaddr *)&addr, &fromlen); 
        fprintf(stderr, "received bytes\n");
        if (recvBytes < 0) {
          if (nc_args.verbose) {
            fprintf(stderr, "receivefrom failed\n");
          }
          exit(4);
        }
        if (recvBytes > 0) {
          if (nc_args.verbose) {
            fprintf(stderr, "received %d bytes\n", recvBytes);
          }       

          fwrite(buffer+1, recvBytes-1, 1, stdout);
          fflush(stdout);
          //fwrite(buffer+1, recvBytes-1, 1, stdout);
          // Check if this is the last datagram to be received
          if (buffer[0] == '0') {
            fprintf(stderr, "closing the connection now");
            break;
          }
        }
      }
      close(sockfd);
    }

    // Do Better UDP
    if (nc_args.udpProtected) {
      const int DATAGRAMSIZE = 1200;
      char buffer[DATAGRAMSIZE];
      char ackbuffer[DATAGRAMSIZE];
      //struct sockaddr *from;
      struct sockaddr_in addr;
      socklen_t fromlen = (socklen_t) sizeof addr;

      // receive datagrams from the client and print to stdout
      for (;;) {
        int recvBytes = recvfrom(sockfd, buffer, DATAGRAMSIZE, 0, (struct sockaddr *)&addr, &fromlen); 
        if (recvBytes < 0) {
          if (nc_args.verbose) {
            perror("receive from failed");
            fprintf(stderr, "receivefrom failed\n");
          }
          exit(4);
        }
        if (recvBytes > 0) {
          if (nc_args.verbose) {
            fprintf(stderr, "received %d bytes\n", recvBytes);
          }
          fwrite(buffer+2, recvBytes-2, 1, stdout);
          fflush(stdout);

          // make what we are going to reply to the client to confirm message
          memcpy(&ackbuffer, &buffer, 2);
          //fromlen = sizeof(addr);
          // ACK the Client
          int reply = sendto(sockfd, ackbuffer, 2, 0, (struct sockaddr *) &addr, fromlen);
          if (reply < 0) {
            perror("error while acking");
            fprintf(stderr, "Error while acking");
            exit(5);
          }

          // Check if this is the last datagram to be received
          if (ackbuffer[0] == 0 && ackbuffer[1] == 0 ) {
            fprintf(stderr, "the client is done sending the data");
            fprintf(stderr, "closing the connection now");
            break;
          }
        }
      }
      close(sockfd);
    }

    // DO TCP
    else {
      // Step 3: Start listening on the specified port
      const int QUEUELEN = 5;
      int listenError = listen(sockfd, QUEUELEN);
      if (listenError) {
        // Couldn’t start listening, e.g., because
        //   someone else is using the same port
        if (nc_args.verbose) {
          perror("listen");
        }
        close(sockfd);
        exit(4); 
      }
      fprintf(stderr, "%s\n","listened");

      // Handle each connection sequentially
      for(;;) {        // C "loop forever" idiom
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Waiting for client to connect and send data");
        }
        // Accept a connection and find out who we’re talking to.
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientfd = accept(sockfd, (struct sockaddr*) &clientAddress, &clientAddressLength);
        
        // Get the client’s IP address as a string
        const char MAX_DOTTED_IP_LEN = 15;
        char ipAddressBuffer[MAX_DOTTED_IP_LEN + 1];  // Don’t forget \0
        inet_ntop(AF_INET, &clientAddress.sin_addr.s_addr,
                  ipAddressBuffer, sizeof(ipAddressBuffer));
        
        // Report the IP address for debugging purposes.
        if (nc_args.verbose){
          fprintf(stderr, "Connection from %s\n", ipAddressBuffer);
        }

        const char MAXLINE = 80;
        char linebuffer[MAXLINE];
        int nread = read(clientfd, linebuffer, MAXLINE);
        while (nread > 0) {
          //fprintf(stdout, "%s", nread, linebuffer);
          // use fwrite so we write the proper amount of the buffer
          fwrite(linebuffer, nread, 1, stdout);
          fflush(stdout); 
          //printf("%.*s", nread, linebuffer);
          nread = read(clientfd, linebuffer, MAXLINE);
          if (nread < 0) {
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","Error reading from client" );
            }
            exit(5);
          }
        }
        if (nc_args.verbose){
          fprintf(stderr, "%s\n", "Client has finished sending their data" );
        }

        // Close the connection to this client
        if (nc_args.verbose) {
          // TODO: print a bandwidth measurement
        }
        close(clientfd);
      }
      exit(6);
    }

  }

  else { //THE CLIENT
    // Step 0: Get the server ip and port, based on the command-line arguments.
    //int serv_port = nc_args.port;
    
    // Setup getaddrinfo section!

    fprintf(stderr, "%s\n","I'm the client");
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;  // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6

    // choose whether we are using TCP or UDP
    int protocol;
    if (nc_args.udp || nc_args.udpProtected) {
      protocol = SOCK_DGRAM;
    } else {
      protocol = SOCK_STREAM;
    }
    hints.ai_socktype = protocol; // protocol stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(nc_args.server, nc_args.port, &hints, &servinfo)) != 0) {
      if (nc_args.verbose) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      }
      exit(1);
    }

    /// TODO: Print the address of the server we connected to
    if (nc_args.verbose) {
      fprintf(stderr, "%s\n","Finished getaddrinfo");
    }

    int sockfd;

    // Communicating with UDP
    if (nc_args.udp) {
      for(p = servinfo; p != NULL; p = p->ai_next) {
        fprintf(stderr, "%s\n","1");
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
          if (nc_args.verbose) {
            perror("client: socket");
          }
            continue;
        }
      }
      fprintf(stderr, "%s\n","Created Socket");

      freeaddrinfo(servinfo); // all done with this structure


      // Write from stdin
      const char MAXLINE = 80;
      char linebuffer[MAXLINE];
      int nread = read(STDIN_FILENO, linebuffer+1, MAXLINE-1);
      linebuffer[0] = '1';
      clock_t start = clock();
      while (nread > 0) {//TODO should nread have +1
        int nwrite = sendto(sockfd, linebuffer, nread, 0, servinfo->ai_addr, servinfo->ai_addrlen);

        if (nwrite < 0){
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","Error with writing to the server");
          }
          exit(5);
        }
        fprintf(stderr, "%s\n","sending stuff");

        nread = read(STDIN_FILENO, linebuffer+1, MAXLINE-1);
        linebuffer[0] = '1';
      }
      // need to tell server we are done sending info
      fprintf(stderr, "%s\n","sending terminal stuff");
      int nwrite = sendto(sockfd, "0", strlen("0"), 0, servinfo->ai_addr, servinfo->ai_addrlen);
      if (nwrite < 0){
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Error with writing to the server");
        }
        exit(5);
      }

    }

    // Communicating with UDP with ACKs!
    else if (nc_args.udpProtected) {
      for(p = servinfo; p != NULL; p = p->ai_next) {
        fprintf(stderr, "%s\n","1");
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
          if (nc_args.verbose) {
            perror("client: socket");
          }
            continue;
        }
      }
      fprintf(stderr, "%s\n","Created Socket");

      freeaddrinfo(servinfo); // all done with this structure


      // Write from stdin
      const int MAXLINE = 1000;
      char linebuffer[MAXLINE];
      char ackbuffer[20];
      short sequenceNum = 1;
      struct sockaddr_in addr;
      socklen_t fromlen;
      
      clock_t start = clock();
      int nread = 1;
      while (true) {
        bool packetWasReceived = false;
        nread = read(STDIN_FILENO, linebuffer+2, MAXLINE-2);
        // copy the sequence number into the first 2 bytes of buffer
        memcpy(&linebuffer, &sequenceNum, sizeof(sequenceNum));
        for (int i = 0; i < nread+2; i++) {
          fprintf(stderr, "%d\n", linebuffer[i]);

        }
        if (nread < 0) {
          break;
        }

        while (!packetWasReceived) {
          int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);

          if (nwrite < 0){
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","Error with writing to the server");
            }
            exit(5);
          }
          fprintf(stderr, "%s\n","sending stuff");

          // Wait for ACK from Server
          // TODO what if the server doesn't respond...PROF STONEEEE...specify a time
          // use select()
          // clock_t startWait = clock();
          // clock_t currentWait = clock();
          // float maxWait = 0.1; // longest we want to wait for an ACK from the server
          // while ((currentWait - startWait) < maxWait) {
          //   nready = select()



          // }



          int recvBytes = recvfrom(sockfd, ackbuffer, MAXLINE, 0, (struct sockaddr *)&addr, &fromlen); 
          if (recvBytes < 0) {
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","Error with receiving to the server");
            }
            exit(6);

          }
          // check if ackbuffer matches the sequence number TODOOOOO
          short sequenceReceived;
          memcpy(&sequenceReceived, &ackbuffer, sizeof(sequenceNum));
          if (sequenceReceived == sequenceNum) {
            packetWasReceived = true;
          }
        }

        // The server verified receiving it!
        sequenceNum++;

        // // read the next line to be sent to the server
        // int nread = read(STDIN_FILENO, linebuffer+2, MAXLINE-2);
        // // copy the sequence number into the first 2 bytes of buffer
        // //memcpy(&linebuffer, &sequenceNum, sizeof(sequenceNum));
        // linebuffer[0] = sequenceNum;
      }

      // need to tell server we are done sending info
      fprintf(stderr, "%s\n","sending terminal stuff");
      sequenceNum = 0;
      int nwrite = sendto(sockfd, sequenceNum, strlen("0"), 0, servinfo->ai_addr, servinfo->ai_addrlen);
      if (nwrite < 0){
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Error with writing to the server");
        }
        exit(5);
      }

    }




    // Communicating with TCP
    else {

      // loop through all the results and connect to the first we can
      for(p = servinfo; p != NULL; p = p->ai_next) {
          fprintf(stderr, "%s\n","1");
          if ((sockfd = socket(p->ai_family, p->ai_socktype,
                  p->ai_protocol)) == -1) {
            if (nc_args.verbose) {
              perror("client: socket");
            }
              continue;
          }
          
          fprintf(stderr, "%s\n","2");
          if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
              close(sockfd);
              if (nc_args.verbose) {
                perror("client: connect");
              }
              continue;
          }
          break;
      }

      if (p == NULL) {
        if (nc_args.verbose) {
          perror("connect");
        }
        exit(2);
      }

      // TODO: print the server address
      fprintf(stderr, "%s\n","Connected to server");

      char ip4[INET_ADDRSTRLEN];  // A bit hacky, just assumed it is an IPv4.
      inet_ntop(AF_INET, &(((struct sockaddr_in*)(servinfo->ai_addr))->sin_addr), ip4, INET_ADDRSTRLEN);
      printf("hostaddr is : %s\n", ip4);
      // convert IP address to human-readable form
      // char s[INET6_ADDRSTRLEN];
      // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
      //         s, sizeof s);
      // if (nc_args.verbose) {
      //     fprintf(stderr, "client: connecting to %s\n", s);
      // }

      freeaddrinfo(servinfo); // all done with this structure

      const char MAXLINE = 80;
      char linebuffer[MAXLINE];
      int nread = read(STDIN_FILENO, linebuffer, MAXLINE);
      clock_t start = clock();
      while (nread > 0) {
        int nwrite = writen(sockfd, linebuffer, nread);
        //fprintf(stdout, "%s\n","writing stuff");
        if (nwrite < 0){
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","Error with writing to the server");
          }
          exit(5);
        }
        nread = read(STDIN_FILENO, linebuffer, MAXLINE);
      }
      int shut_status =  shutdown(sockfd, SHUT_WR);
      if(shut_status < 0) {
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Error in client shutdown");
        }
        exit(6);
      }
      if (nc_args.verbose){
        fprintf(stderr, "%s\n","Half Close the connection using shutdown");
      }
      clock_t end = clock();
      float elapsed = (float)(end - start) / CLOCKS_PER_SEC;
      
      for(;;) {        // C "loop forever" idiom
        int nbytes = readn(sockfd, linebuffer, MAXLINE);
        if (nbytes < 0) {
          if (nc_args.verbose) {
            perror("readn");
          }
          exit (3);
        } else if (nbytes == 0) {
          // end of server output
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","Nothing more to read from server");
          }
          break; 
        }
        // stdout log the data we are receiving
        printf("%s", linebuffer);
      }
    }

    if (nc_args.verbose) {
      // TODO: print a bandwidth measurement
    }
    close(sockfd);
    return 0;
  }

}
