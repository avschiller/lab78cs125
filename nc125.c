#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

int main(int argc, char * argv[]){

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


  /**
   * FILL ME IN WITH CODE TO DO THE WORK
   **/

}
