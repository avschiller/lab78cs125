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

  int ch;
  while ((ch = getopt(argc, argv, "lv")) != -1) {
    switch (ch) {
    case 'l': //listen
      nc_args->listen = true;
      break;
    case 'v':
      nc_args->verbose = true;
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

  if (nc_args.listen){ // THE SERVER (listening)
    fprintf(stderr, "%s\n","I'm the server");
    // Step 0: Choose the server port, based on the command-line arguments.
    //int serv_port = nc_args.port;

    // Setyp getaddrinfo section!
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;  // will point to the results
    int yes=1;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
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
      fprintf(stderr, "%s\n","for loop lol");
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
      fprintf(stderr, "Connection from %s\n", ipAddressBuffer);
      

      const char MAXLINE = 80;
      char linebuffer[MAXLINE];
      int nread = readline(sockfd, linebuffer, MAXLINE);
      while (nread > 0) {
        printf("%.*s", nread, linebuffer);
        nread = readline(sockfd, linebuffer, MAXLINE);
        if (nread < 0) {
          if (nc_args.verbose){
            fprintf(stderr, "%s\n","Error reading from client" );
          }
          exit(5);
        }
      }

      // Close the connection to this client
      if (nc_args.verbose) {
        // TODO: print a bandwidth measurement
      }
      close(clientfd);
    }

  }

  else { //THE CLIENT
    // Step 0: Get the server ip and port, based on the command-line arguments.
    //int serv_port = nc_args.port;
    
    // Setyp getaddrinfo section!

    fprintf(stderr, "%s\n","I'm the client");
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;  // will point to the results
    // char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(nc_args.server, nc_args.port, &hints, &servinfo)) != 0) {
      if (nc_args.verbose) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      }
      exit(1);
    }
    fprintf(stderr, "%s\n","Finished getaddrinfo");
    int sockfd;
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        fprintf(stderr, "%s\n","1");
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            //perror("client: socket");
            continue;
        }
        
        fprintf(stderr, "%s\n","2");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            //perror("client: connect");
            continue;
        }
        fprintf(stderr, "%s\n","in loop");

        break;
    }
    fprintf(stderr, "%s\n","Connected to server");
    if (p == NULL) {
      if (nc_args.verbose) {
        perror("connect");
      }
      exit(2);
    }

    // convert IP address to human-readable form
    // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    //         s, sizeof s);
    //printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    fprintf(stderr, "%s\n","Freed servinfo");
    const char MAXLINE = 80;
    char linebuffer[MAXLINE];
    int nread = readline(STDIN_FILENO, linebuffer, MAXLINE);
    clock_t start = clock()
    while (nread > 0) {
      int nwrite = writen(sockfd, linebuffer, MAXLINE);
      fprintf(stdout, "%s\n","writing stuff");
      if (nwrite < 0){
        if (nc_args.verbose){
          fprintf(stderr, "%s\n","Error with writing to the server");
        }
        exit(5);
      }
      nread = readline(STDIN_FILENO, linebuffer, MAXLINE);
    }
    int shut_status =  shutdown(sockfd,1);
    if(shut_status < 0) {
      if (nc_args.verbose){
        fprintf(stderr, "%s\n","Error in client shutdown");
      }
      exit(6);
    }
    clock_t end = clock()
    float elapsed = (float)(end - start) / CLOCKS_PER_SEC;
    
    for(;;) {        // C "loop forever" idiom
      int nbytes = readline(sockfd, linebuffer, MAXLINE);
      if (nbytes < 0) {
        if (nc_args.verbose) {
          perror("readLine");
        }
        exit (3);
      } else if (nbytes == 0) {
        // end of server output
        break; 
      }
      // stdout log the data we are receiving
      if (nc_args.verbose){
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
