# Lab 7: Implementing `netcat`

The goal of this week's lab is to implement a complete useful program `nc125` using C and TCP sockets, a (simplified) form of the `nc` command you've already seen.

Your implementation of `nc125` will support the following usage:

* Given a hostname and a port, create an active TCP connection; write all bytes of standard input to the network connection until you reach the end of input, and then copy all received bytes to standard output. (You may assume that all writes precede all reads.)

    Thus, a command like 
    
        echo -n "GET / HTTP/1.0\r\n\r\n" | nc125 www.cs.hmc.edu 80 
        
    will print an HTTP response from the CS web server. (It just redirects you to use `https`. Using `www.google.com` instead gives more interesting [or at least longer] results.)

* Given the `-l` flag and a port, create a passive (listening) connection. Wait for one incoming connection, and then copy all incoming data to standard output. On end of input (when the connection is closed by the other side), the program should exit.

    This allows us to use `nc125` to transfer single files, i.e., `nc125 -l 4567 > output.txt` on knuth and then `nc125 knuth.cs.hmc.edu 4567 < input.txt` on another machine.

* The `-v` flag can be added both for active or passive connections, and signals a request for verbose (debugging) output to be sent to standard *error*, e.g., `nc125 -l -v 4567` or `nc125 -v -l 4567` or `nc125 -v www.cs.hmc.edu 80`


You are given a Makefile and some starter code in `nc125.c` for option-handling.  I strongly recommend you use some time reading and planning **before** you jump into coding.

## Requirements:

1. The client and server modes should work as described above.

2. Your code should do full error checking, giving useful messages when things go wrong instead of crashing or silently exiting.

3. The `-v` output should be written to standard output, not standard error. The contents of this output can be anything you think useful. However, it must end by printing a bandwidth measure (bytes per second transmitted or received, between when the network connection was established and when it closed).

4. The final version of your code should permit domain names or IPv6 adressess for active connections, not just IPv4 dotted addresses. You must handle this by using `getaddrinfo` (see below).

5. You may refer to examples shown in class or on-line, but you should be writing your own code. **Do not just use other people's code** (either by copy-and-paste or by blindly retyping code you've found into your own program.) In fact, you and your partner should probably not be looking at sample code at the same time that you're typing. You may refer to man pages and other API documentation while typing, of course.

6. Your submitted code should contain minimal (ideally, no) IPv4 or IPv6-specific code. Again, `getaddrinfo` helps a lot with this.

## Useful tools

* You will, of course, need to use C socket functions (`socket`, `connect`, `bind`, etc.). 

* Remember that Unix (and variants such as OS X) provide POSIX `read` and `write` system calls that work directly with file descriptors (small integers). System calls are described in section 2 of the man pages (e.g., "man 2 read").

* There are separate C functions (`fopen`, `fread`, `fwrite`, `fprintf`, `fseek`, `fclose`, etc.) that use `FILE*` stream identifiers such as `stderr` instead of file descriptors. Library functions are described in section 3 of the man pages (e.g., "man 3 fopen"). [There are also POSIX library functions `fdopen` and `fileno` that convert back and forth between streams and file descriptors.]

* The "traditional" way of converting domain names to IPv4 addresses is `gethostbyname`, but that is considered old-fashioned and inflexible. The modern way is to use `getaddrinfo`. Among other reasons, `getaddrinfo` returns a linked list of addresses (e.g., you might get both IPv4 and IPv6 addresses for a given host name), and automatically fills out address structs for you. It handles both numeric IPv4 and IPv6 addresses and domain names. You can find information about `getaddrinfo` online; a good place to start is [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/output/html/multipage/syscalls.html).

* The correct way (required for this assignment) to use `getaddrinfo` is to loop through **all** the addresses in the linked list, until you successfully create a socket with the specified protocol *and* successfully `bind` or `connect` as appropriate. (If you create the socket but `bind` or `connect` fails, don't forget to close the socket before you retry with the next address.)

## Acknowledgments

This assignment was inspired by a lab used by Prof. Adam Aviv at Swarthmore College.
