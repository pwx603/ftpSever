#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dir.h"
#include "usage.h"

#define BACKLOG 10

char *portNumber;

void *get_in_addr(struct sockaddr *);

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

int main(int argc, char **argv)
{

  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc

  // Check the command line arguments
  if (argc != 2)
  {
    usage(argv[0]);
    return -1;
  }

  portNumber = argv[1];

  int sockfd, new_fd;
  socklen_t sin_size;

  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;

  int yes = 1;
  int status;
  char s[INET6_ADDRSTRLEN];

  // Setting addrinfo hints to 0

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, portNumber, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  for (;;)
  {
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
      perror("send");
    close(new_fd);
    exit(0);
  }

  // This is how to call the function in dir.c to get a listing of a directory.
  // It requires a file descriptor, so in your code you would pass in the file descriptor
  // returned for the ftp server's data connection

  printf("Printed %d directory entries\n", listFiles(1, "."));
  return 0;
}

// get IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
