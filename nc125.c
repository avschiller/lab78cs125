#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#define _XOPEN_SOURCE 600
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


/**
 * Structure to hold command-line arguments
 **/
typedef struct nc_args {
  unsigned short port; // server port
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
  nc_args->port = 0;

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
      nc_args->port = atoi(argv[0]);
    }

  } else {
    // Active mode: remaining arguments should be hostname and a port
    if (argc != 2) {
      fprintf(stderr, "ERROR: hostname and port expected\n");
      exit(1);
    } else {
      nc_args->server = argv[0];
      nc_args->port = atoi(argv[1]);
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
  fprintf(stderr, "Port is %d\n", nc_args.port);
  if (! nc_args.listen) {
    fprintf(stderr, "Server is %s\n", nc_args.server);
  }

  if (nc_args.listen){ // THE SERVER (listening)
    // Step 0: Choose the server port, based on the command-line arguments.
    int serv_port = nc_args.port;

    // Step 1: Create a socket.
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
      // Couldn’t create the socket.
      perror("socket");
      exit(1);
    }
    
    // Step 2: Configure the socket to a local port address.
    struct sockaddr_in serverAddress;
    // First zero out the address struct, and then
    //   fill in the fields we care about.
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_port        = htons(serv_port);
    // Using IPv4 ...
    // listen for a connection on the
    //   specified port ...
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  // on any network interface
    int bindError = bind(listenfd,
                         (struct sockaddr*) &serverAddress,
                         sizeof(serverAddress));
    if (bindError) {
      // Couldn’t bind the socket
      perror("bind");
      close(listenfd);
      exit(2);
    }
    fprintf(stderr, "Listening on port %d\n", serv_port);
    
    // Step 3: Start listening on the specified port
    const int QUEUELEN = 5;
    int listenError = listen(listenfd, QUEUELEN);
    if (listenError) {
      // Couldn’t start listening, e.g., because
      //   someone else is using the same port
      perror("listen");
      close(listenfd);
      exit(3); 
    }
    
    // Handle each connection sequentially
    for(;;) {        // C "loop forever" idiom
      
      // Accept a connection and find out who we’re talking to.
      struct sockaddr_in clientAddress;
      socklen_t clientAddressLength = sizeof(clientAddress);
      int clientfd = accept(listenfd, (struct sockaddr*) &clientAddress, &clientAddressLength);
      
      // Get the client’s IP address as a string
      const char MAX_DOTTED_IP_LEN = 15;
      char ipAddressBuffer[MAX_DOTTED_IP_LEN + 1];  // Don’t forget \0
      inet_ntop(AF_INET, &clientAddress.sin_addr.s_addr,
                ipAddressBuffer, sizeof(ipAddressBuffer));
      
      // Report the IP address for debugging purposes.
      fprintf(stderr, "Connection from %s\n", ipAddressBuffer);
      
      // Get the current time as a string (with newline)
      const char MAX_TIME_LEN = 25;
      char timeBuffer[MAX_TIME_LEN + 1];
      time_t secondsSinceEpoch = time(NULL);
      ctime_r(&secondsSinceEpoch, timeBuffer);
      
      // Send the string to the client 10 times
      for (int i = 0; i < 10; ++i) {
        writen(clientfd, timeBuffer, strlen(timeBuffer));
      }
      
      // Close the connection to this client
      close(clientfd);
    }
  }

  else { //THE CLIENT
    // Step 0: Get the server ip and port, based on the command-line arguments.
    struct sockaddr_in serverAddress;
    
    // First zero out the address struct, and then
    //   fill in the fields we care about.
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;            // Using IPv4 ...
    serverAddress.sin_port = htons(nc_args.port);
    //serverAddress.sin_addr.s_addr = nc_args.server;

    // Backconvert the server address for debugging purposes
    const char MAX_DOTTED_IP_LEN = 15;
    char ipAddressBuffer[MAX_DOTTED_IP_LEN + 1];  // Don’t forget \0
    inet_ntop(AF_INET, &serverAddress.sin_addr.s_addr,
              ipAddressBuffer, sizeof(ipAddressBuffer));

    fprintf(stderr, "Server at %s:%d\n", ipAddressBuffer, ntohs(serverAddress.sin_port));
    
    // Step 1: Create a socket for the connection.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      // Couldn’t create the socket.
      perror("socket");
      exit(1);
    }

    // Step 2: Connect the socket to the remote server
    int connectError = connect (sockfd,
                                (struct sockaddr*) &serverAddress,
                                sizeof(serverAddress));
    if (connectError) {
      perror("connect");
      exit(2);
    }
    fprintf(stderr, "Connected to server.\n");

    for(;;) {        // C "loop forever" idiom

      const char MAXLINE = 80;
      char linebuffer[MAXLINE];

      int nbytes = readline(sockfd, linebuffer, MAXLINE);
      if (nbytes < 0) {
        perror("readLine");
        exit (3);
      } else if (nbytes == 0) {
        // end of server output
        break; 
      }
      printf("LINE: %s", linebuffer);
    }

    close(sockfd);
    return 0;
  }



}
