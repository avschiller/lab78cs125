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
    fprintf(stderr, "Using UDP \n");
  } else if (nc_args.udpProtected) {
    fprintf(stderr, "Using Smart UDP \n");
  } else {
    fprintf(stderr, "Using TCP \n");
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
      const int DATAGRAMSIZE = 1100;
      const int MAXLINE = 1100;
      char buffer[DATAGRAMSIZE];
      short ackbuffer[2]; //char ackbuffer[4];
      //struct sockaddr *from;
      struct sockaddr_storage addr;
      socklen_t fromlen;

      const int MAX_UNACK = 50;
      int buffer_size = MAXLINE;
      short sequenceArray[MAX_UNACK];
      memset(sequenceArray, 0, sizeof(sequenceArray));

      // add 4 to acount for the int to be stored at the beginning
      char messArray[(buffer_size+4) * MAX_UNACK];
      int currSequenceNum = 1;

      // receive datagrams from the client and print to stdout
      for (;;) {
        fromlen = sizeof(struct sockaddr_storage);          
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
          
          short receivedSeqNum;
          memcpy(&receivedSeqNum, &ackbuffer, 2);
          // print it if it's next to be printed
          if (nc_args.verbose) {
            fprintf(stderr, "received sequence number %d, next expected was %d \n", receivedSeqNum, currSequenceNum);
          }
          if (receivedSeqNum == currSequenceNum) {
            if (nc_args.verbose) {
              fprintf(stderr, "confirmed next to print\n");
            }
            fwrite(buffer+2, recvBytes-2, 1, stdout);
            fflush(stdout);
            currSequenceNum++;
          } else if (receivedSeqNum > currSequenceNum) { // this message shouldn't be displayed yet so we store it
            if (nc_args.verbose) {
              fprintf(stderr, "storing the packet to be pringted later. it is this long: %d and has seqnum: %d \n", recvBytes-2, receivedSeqNum);
            }
            memcpy(&sequenceArray[(receivedSeqNum) % MAX_UNACK], &ackbuffer, sizeof(ackbuffer));
            if (nc_args.verbose) {
              fprintf(stderr, "stored the sequence num %d \n", receivedSeqNum);
            }
            int lengthOfMessage = recvBytes - 2;
            fprintf(stderr, "length of message is %d \n", lengthOfMessage);

            memcpy(&messArray[(receivedSeqNum)*(buffer_size+4) % (MAX_UNACK* (buffer_size+4))], &lengthOfMessage, 4);
            int messageLengthtest;
            memcpy(&messageLengthtest, &messArray[(receivedSeqNum)*(buffer_size+4) % (MAX_UNACK* (buffer_size+4))], 4);
            if (nc_args.verbose) {
              fprintf(stderr, "storing the packet to be printed later. it is this long: %d and has seqnum: %d \n", recvBytes-2, receivedSeqNum);
              fprintf(stderr, "repring the messagelengthtest: %d \n", messageLengthtest);
            }
            memcpy(&messArray[ ((receivedSeqNum)*(buffer_size+4) + 4) % (MAX_UNACK* (buffer_size+4))], (buffer + 2), recvBytes - 2);
          }
          
          short arraySeqNum;
          memcpy(&arraySeqNum, &sequenceArray[currSequenceNum % MAX_UNACK], 2);
          // print from the array as long as the array holds that next message to be printed
          while(arraySeqNum == currSequenceNum) {
            if (nc_args.verbose) {
              fprintf(stderr, "the next array value also matches. received sequence number %d, next expected was %d \n", arraySeqNum, currSequenceNum);
            }
            // copy the message length to an int
            int messageLength;
            memcpy(&messageLength, &messArray[(currSequenceNum)*(buffer_size+4) % (MAX_UNACK* (buffer_size+4))], 4);
            if (nc_args.verbose) {
              fprintf(stderr, "copied the message length %d \n", messageLength);
            }
            // copy the message to be printed to the buffer
            memcpy(&buffer, &messArray[(((currSequenceNum)*(buffer_size+4)) + 4) % (MAX_UNACK* (buffer_size+4))], messageLength); 
            if (nc_args.verbose) {
              fprintf(stderr, "copied the message %d \n", messageLength);
            }
            fwrite(buffer, messageLength, 1, stdout);
            fflush(stdout);
            currSequenceNum++;
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
    if (nc_args.verbose){
      fprintf(stderr, "%s\n","I'm the client");
    }
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
      //clock_t start = clock();
      while (nread > 0) {
        int nwrite = sendto(sockfd, linebuffer, nread+1, 0, servinfo->ai_addr, servinfo->ai_addrlen);

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
      const int MAXLINE = 1100;
      const int MAX_UNACK = 4;
      const int MAX_ACKSTORAGE = MAX_UNACK * 10;
      char linebuffer[MAXLINE];
      char ackbuffer[20];
      short sequenceNum = 1;
      struct sockaddr_storage addr;
      socklen_t fromlen;
      
      struct timeval startTime, currTime, packetTime;
      gettimeofday(&startTime, NULL);

      int nread = 1;
      int buffer_size = MAXLINE;
      int length = 0;
      int head = 0;
      struct timeval timeQueue[MAX_UNACK];
      short sequenceQueue[MAX_UNACK];
      // add 4 to acount for the int to be stored at the beginning
      char messQueue[(buffer_size+4) * MAX_UNACK];
      short sequenceArray[MAX_ACKSTORAGE];
      
      double timeOut = 2;

      struct timeval timeSelectCheck; // how long we want to wait while checking for ACK's with select
      timeSelectCheck.tv_usec = 1000; // 1 milliseconds
      timeSelectCheck.tv_sec = 0;

      while (nread > 0) {
        // send data if we are under the max unacked packets
        if (length < MAX_UNACK) {
          // if there is nothing to read does it get stuck? use select? TODO
          // use select to see if there is anything to read!
          fd_set allset;
          FD_ZERO(&allset);
          FD_SET(sockfd, &allset);   // Start out with a singleton set

          int nready = select(STDIN_FILENO+1, &allset, NULL, NULL, &timeSelectCheck);
          //fprintf(stderr, "stdin nready: %d.\n", nready);
          nready = 1;
          if (nready < 0) {
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","Nothing to currently read from stdin");
            }
          }
          else if (nready > 0) {
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","waiting to read from stdin");
            }
            nread = read(STDIN_FILENO, linebuffer+2, MAXLINE-2);
            // copy the sequence number into the first 2 bytes of buffer
            memcpy(&linebuffer, &sequenceNum, sizeof(sequenceNum));

            if (nread <= 0) {
              if (nc_args.verbose){
                fprintf(stderr, "%s\n","nread was less than zero so break");
              }
              break;
            }

            gettimeofday(&currTime, NULL);
            memcpy(&timeQueue[(head+length) % MAX_UNACK], &currTime, sizeof(currTime)); 
            if (sequenceNum == 0) {
              fprintf(stderr, "%s\n","after storing timeing");
              for (int i = head; i < length+head+1; i++) {
                fprintf(stderr, "logged sequence number for index queue index %d is %d.\n", i%MAX_UNACK, sequenceQueue[i % MAX_UNACK]);
                fprintf(stderr, "logged timequeue number for index queue index %d is %ld , %d.\n", i%MAX_UNACK, timeQueue[i % MAX_UNACK].tv_sec, timeQueue[i % MAX_UNACK].tv_usec );
              }
              return 0;
            }
            memcpy(&sequenceQueue[(head+length) % MAX_UNACK], &sequenceNum, sizeof(sequenceNum));
            if (nc_args.verbose) {
              fprintf(stderr, "%s\n","after storing seq num");
              for (int i = head; i < length+head+1; i++) {
                fprintf(stderr, "logged sequence number for index queue index %d is %d.\n", i%MAX_UNACK, sequenceQueue[i % MAX_UNACK]);
              }
            }
            memcpy(&messQueue[(head+length)*(buffer_size+4) % (MAX_UNACK* (buffer_size+4))], &nread, 4);
            memcpy(&messQueue[ ((head+length)*(buffer_size+4) + 4) % (MAX_UNACK* (buffer_size+4))], (linebuffer + 2), nread);

            int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);
            if (nwrite < 0){
              if (nc_args.verbose){
                fprintf(stderr, "%s\n","Error with writing to the server");
              }
              exit(5);
            }
            if (nc_args.verbose){
              fprintf(stderr, "sent a message to the server of sequence number %d, queue head is %d and length is: %d .\n", sequenceNum, head, length);
              fprintf(stderr, "logged sequence number is %d.\n", sequenceQueue[(head+length) % MAX_UNACK]);
            }

            length++;
            sequenceNum++;
          }
        }

        if (nc_args.verbose){
          fprintf(stderr, "%s\n","right after reading everything from std in");
          for (int i = head; i < length+head; i++) {
            fprintf(stderr, "logged sequence number for index queue index %d is %d.\n", i%MAX_UNACK, sequenceQueue[i % MAX_UNACK]);
          }
        }
        
        // check if the oldest packet sent has timed out or if the queue is full
        gettimeofday(&currTime, NULL);
        packetTime = timeQueue[head % MAX_UNACK];
        int totalCurrTime = currTime.tv_sec + (currTime.tv_usec/1000000);
        int totalPacketTime = packetTime.tv_sec + (packetTime.tv_usec/1000000);

        while ( ( ((totalCurrTime - totalPacketTime) > timeOut ) || length == MAX_UNACK) && (length > 0) ) {
          // look at the ack's that have been received and store in array
          fd_set allset;
          FD_ZERO(&allset);
          FD_SET(sockfd, &allset);   // Start out with a singleton set
          fromlen = sizeof(struct sockaddr_storage);          
          
          int nready = select(sockfd+1, &allset, NULL, NULL, &timeSelectCheck);
          if (nc_args.verbose){
              fprintf(stderr,"checking for ack, nready from server is: %d \n", nready);
          }
          while(nready == 1) {
            int recvBytes = recvfrom(sockfd, ackbuffer, MAXLINE, 0, (struct sockaddr *)&addr, &fromlen); 
            if (recvBytes < 0) {
              if (nc_args.verbose){
                fprintf(stderr, "%s\n","Error with receiving to the server");
              }
              exit(6);
            }
            // check if ackbuffer matches the sequence number
            short sequenceReceived;
            memcpy(&sequenceReceived, &ackbuffer, sizeof(sequenceNum));
            
            if (nc_args.verbose){
              fprintf(stderr, "Received an ACK number: %d  from the server.\n", sequenceReceived);
            }
            // store ack in array under index (sequenceReceived % MAX_ACKSTORAGE)
            memcpy(&sequenceArray[(sequenceReceived) % MAX_UNACK], &sequenceReceived, sizeof(sequenceReceived));

            // recheck if there are more ack's waiting
            fd_set allset;
            FD_ZERO(&allset);
            FD_SET(sockfd, &allset);   // Start out with a singleton set
            fromlen = sizeof(struct sockaddr_storage);          
            
            nready = select(sockfd+1, &allset, NULL, NULL, &timeSelectCheck);
            if (nc_args.verbose){
                fprintf(stderr,"checking for ack, nready from server is: %d \n", nready);
            }
            if (nc_args.verbose) {
              fprintf(stderr, "%s\n","after storing an ack from server");
              for (int i = head; i < length+head; i++) {
                fprintf(stderr, "logged sequence number for index queue index %d is %d.\n", i%MAX_UNACK, sequenceQueue[i % MAX_UNACK]);
              }
            }
          }
          if (nc_args.verbose){
            fprintf(stderr,"done waiting for acks \n");
          }

          bool ackMatch = false;
          short ackValForHead;
          short headSequenceVal;
          memcpy(&headSequenceVal, &sequenceQueue[head % MAX_UNACK], 2);
          memcpy(&ackValForHead, &sequenceArray[headSequenceVal % MAX_UNACK], 2);
          if (nc_args.verbose){
            fprintf(stderr,"the ack we expect is %d, the one we see at that index is %d \n", headSequenceVal, ackValForHead);
          }
          if (ackValForHead == headSequenceVal) {
            ackMatch = true;
          }
          if (headSequenceVal == 0) {
            if (nc_args.verbose) {
              fprintf(stderr, "%s\n","after done reading ack's from server");
              for (int i = head; i < length+head; i++) {
                fprintf(stderr, "logged sequence number for index queue index %d is %d.\n", i%MAX_UNACK, sequenceQueue[i % MAX_UNACK]);
              }
            }
            return 0;
          }

          // if we have no match and this one has expired, then we must resend it
          if (!ackMatch && ((totalCurrTime - totalPacketTime) > timeOut ) ) {
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","select resulted in a timeout, so nothing received from server");
            }
            if ( (totalCurrTime - totalPacketTime) > timeOut ) {
              // this packet was not acknowledged in the timeOut given!
              // Resend the packet and adjust the timeOut value?
              //use a different sequence number for the packet being resent
              int oldSequenceNum = sequenceQueue[head % MAX_UNACK];
              if (nc_args.verbose){
                  fprintf(stderr, "Resending packet number: %d  since an ACK has not been received within the timeOut.\n", oldSequenceNum);
              }
              memcpy(linebuffer, &oldSequenceNum, sizeof(sequenceNum));
              memcpy(&sequenceQueue[(head+length) % MAX_UNACK], &oldSequenceNum, sizeof(oldSequenceNum));
              gettimeofday(&currTime, NULL);
              memcpy(&timeQueue[(head+length) % MAX_UNACK], &currTime, sizeof(currTime)); 
              int mess_len;
              // copy the length of the message into a variable
              memcpy(&mess_len, &messQueue[(head*(buffer_size+4)) % (MAX_UNACK*(buffer_size+4))], 4);
              // copy the old message into the line buffer
              memcpy((linebuffer + 2), &messQueue[ (head * (buffer_size + 4) + 4) % (MAX_UNACK*(buffer_size+4))], mess_len);
              int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);
              if (nwrite < 0){
                if (nc_args.verbose){
                  fprintf(stderr, "%s\n","Error with writing to the server");
                }
                exit(5);
              }
              if (nc_args.verbose){
                fprintf(stderr, "%s\n","sending stuff");
              }
              head++;
            }
          }
          // otherwise there is a match for that sequence number
          else if (ackMatch) {
            // check if the received ack matches the un-acked packet at the head
            if (nc_args.verbose){
                fprintf(stderr, "confirmed packet number: %d  since a ACK had been received of value %d \n", headSequenceVal, ackValForHead);
            } 

            head++;
            length--;
            // otherwise we received an ACK for a message either already acked in which case we don't care
            // or we received one for a later message in which case we must resend the earlier messages
            // and then mark that one as ACKed
            // else if (sequenceReceived != sequenceQueue[head % MAX_UNACK]) {
            //   while (sequenceReceived != sequenceQueue[head % MAX_UNACK]){
            //     //use a different sequence number for the packet being resent
            //     int oldSequenceNum;// = sequenceQueue[head % MAX_UNACK];
            //     memcpy(&oldSequenceNum, &sequenceQueue[head % MAX_UNACK], 2);
            //     if (nc_args.verbose){
            //       fprintf(stderr, "Resending packet number: %d  since a latter ACK was received of value %d \n", oldSequenceNum, sequenceReceived);
            //     }
            //     memcpy(&linebuffer, &oldSequenceNum, sizeof(sequenceNum));
                
            //     int mess_len;
            //     // copy the length of the message into a variable
            //     memcpy(&mess_len, &messQueue[(head*(buffer_size+4)) % (MAX_UNACK*(buffer_size+4))], 4);
            //     // copy the old message into the line buffer
            //     memcpy((linebuffer + 2), &messQueue[ (head * (buffer_size + 4) + 4) % (MAX_UNACK*(buffer_size+4))], mess_len);
            //     //copy the old sequence number to the sequence queue
            //     memcpy(&sequenceQueue[(head+length) % MAX_UNACK], &oldSequenceNum, sizeof(sequenceNum));
            //     //copy the time sent to the timequeue
            //     gettimeofday(&currTime, NULL);
            //     memcpy(&timeQueue[(head+length) % MAX_UNACK], &currTime, sizeof(currTime)); 

            //     int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);
            //     if (nwrite < 0){
            //       if (nc_args.verbose){
            //         fprintf(stderr, "%s\n","Error with writing to the server");
            //       }
            //       exit(5);
            //     }
            //     if (nc_args.verbose){
            //       fprintf(stderr, "%s\n","sending stuff");
            //     }
            //     head++;
            //     sleep(1);
            //   }
            //   // now we can mark that that this message was ACKed
            //   head++;
            //   length--;
            // } 
          }
        gettimeofday(&currTime, NULL);
        packetTime = timeQueue[head % MAX_UNACK];
        totalCurrTime = currTime.tv_sec + (currTime.tv_usec/1000000);
        totalPacketTime = packetTime.tv_sec + (packetTime.tv_usec/1000000);
        }
      }

      //making sure the rest of messages were delivered
      if (nc_args.verbose){
        fprintf(stderr, "%s\n","done sending, but making sure the rest of the unacked messages get delivered");
      }
      
      //TODO 
      

      // need to tell server we are done sending info
      if (nc_args.verbose){
        fprintf(stderr, "%s\n","sending terminal message");
      }
      sequenceNum = 0; //TODO
      int nwrite = sendto(sockfd, sequenceNum, strlen("0"), 0, servinfo->ai_addr, servinfo->ai_addrlen);

      if (nwrite < 0){
        if (nc_args.verbose){
          perror("errror with writing to the server");
          //fprintf(stderr, "%s\n","Error with writing to the server");
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




/*
void checkAck(timeQueue, sequenceQueue, messQueue, length, head, sockfd) {
// check if the oldest packet sent has timed out or if the queue is full
  struct timeval currTime, packetTime;
  gettimeofday(&currTime, NULL);
  packetTime = timeQueue[head % MAX_UNACK];
  int totalCurrTime = currTime.tv_sec + (currTime.tv_usec/1000000);
  int totalPacketTime = packetTime.tv_sec + (packetTime.tv_usec/1000000);
  while ( ( ((totalCurrTime - totalPacketTime) > timeOut ) || length == MAX_UNACK) && (length > 0) ) {
    // check if there is an ACK for this packet
    // check if we have received acknowledgements!
    fd_set allset;
    FD_ZERO(&allset);
    FD_SET(sockfd, &allset);   // Start out with a singleton set
    fromlen = sizeof(struct sockaddr_storage);          
    
    int nready = select(sockfd+1, &allset, NULL, NULL, &timeSelectCheck);
    if (nc_args.verbose){
        fprintf(stderr,"checking for ack, nready from server is: %d \n", nready);
    }

    if (nready <= 0) {
      if (nc_args.verbose){
        fprintf(stderr, "%s\n","select resulted in a timeout, so nothing received from server");
      }
      if ( (totalCurrTime - totalPacketTime) > timeOut ) {
        // this packet was not acknowledged in the timeOut given!
        // Resend the packet and adjust the timeOut value?
        //use a different sequence number for the packet being resent
        int oldSequenceNum = sequenceQueue[head % MAX_UNACK];
        if (nc_args.verbose){
            fprintf(stderr, "Resending packet number: %d  since an ACK has not been received within the timeOut.\n", oldSequenceNum);
        }
        memcpy(&linebuffer, &sequenceNum, sizeof(sequenceNum));
        sequenceNum++;
        int mess_len;
        // copy the length of the message into a variable
        memcpy(&mess_len, &messQueue[(head*(buffer_size+4)) % (MAX_UNACK*(buffer_size+4))], 4);
        // copy the old message into the line buffer
        memcpy((linebuffer + 2), &messQueue[ (head * (buffer_size + 4) + 4) % (MAX_UNACK*(buffer_size+4))], mess_len);
        int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);
        if (nwrite < 0){
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","Error with writing to the server");
          }
          exit(5);
        }
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","sending stuff");
        }
        head++;
      }

    }
    else if (nready > 0) {
      int recvBytes = recvfrom(sockfd, ackbuffer, MAXLINE, 0, (struct sockaddr *)&addr, &fromlen); 
      if (recvBytes < 0) {
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Error with receiving to the server");
        }
        exit(6);
      }
      // check if ackbuffer matches the sequence number
      short sequenceReceived;
      memcpy(&sequenceReceived, &ackbuffer, sizeof(sequenceNum));
      
      if (nc_args.verbose){
        fprintf(stderr, "Received an ACK number: %d  from the server.\n", sequenceReceived);
      }
      // check if the received ack matches the un-acked packet at the head
      if (sequenceReceived == sequenceQueue[head % MAX_UNACK]) {
        int oldSequenceNum = sequenceQueue[head % MAX_UNACK];
        if (nc_args.verbose){
            fprintf(stderr, "confirmed packet number: %d  since a ACK was received of value %d \n", oldSequenceNum, sequenceReceived);
        }
        head++;
        length--;
      }
      // otherwise we received an ACK for a message either already acked in which case we don't care
      // or we received one for a later message in which case we must resend the earlier messages
      // and then mark that one as ACKed
      else if (sequenceReceived >= sequenceQueue[head % MAX_UNACK]) {
        while (sequenceReceived > sequenceQueue[head % MAX_UNACK]){
          //use a different sequence number for the packet being resent
          int oldSequenceNum;// = sequenceQueue[head % MAX_UNACK];
          memcpy(&oldSequenceNum, &sequenceQueue[head % MAX_UNACK], 2);
          if (nc_args.verbose){
            fprintf(stderr, "Resending packet number: %d  since a latter ACK was received of value %d \n", oldSequenceNum, sequenceReceived);
          }
          memcpy(&linebuffer, &sequenceNum, sizeof(sequenceNum));
          sequenceNum++;
          int mess_len;
          // copy the length of the message into a variable
          memcpy(&mess_len, &messQueue[(head*(buffer_size+4)) % (MAX_UNACK*(buffer_size+4))], 4);
          // copy the old message into the line buffer
          memcpy((linebuffer + 2), &messQueue[ (head * (buffer_size + 4) + 4) % (MAX_UNACK*(buffer_size+4))], mess_len);
          int nwrite = sendto(sockfd, linebuffer, nread+2, 0, servinfo->ai_addr, servinfo->ai_addrlen);
          if (nwrite < 0){
            if (nc_args.verbose){
              fprintf(stderr, "%s\n","Error with writing to the server");
            }
            exit(5);
          }
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","sending stuff");
          }
          head++;
        }
        // now we can mark that that this message was ACKed
        head++;
        length--;
      } 
    }
  gettimeofday(&currTime, NULL);
  packetTime = timeQueue[head % MAX_UNACK];
  totalCurrTime = currTime.tv_sec + (currTime.tv_usec/1000000);
  totalPacketTime = packetTime.tv_sec + (packetTime.tv_usec/1000000);
  }

  return NULL;
}

*/



