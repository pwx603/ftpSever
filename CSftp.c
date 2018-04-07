#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dir.h"
#include "usage.h"
#include "ConnectionManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold
#define PORT "1200"


#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#define BACKLOG 10
#define BUFFSIZE 1000
#define sendMsg(socket, msg) send(socket, msg, strlen(msg), 0)
#define recvMsg(socket, msg) recv(socket, msg, BUFFSIZE, 0)

char *portNumber;
typedef enum command { INVALID = -1,
                       USER,
                       TYPE,
                       MODE,
                       STRU,
                       PASV,
                       NLST,
                       RETR,
                       CWD,
                       CDUP,
                       QUIT } command_t;
struct
{
  unsigned int is_logged_in : 1;
  unsigned int is_ascii : 1;
  unsigned int is_fs : 1;
  unsigned int is_passive : 1;
  unsigned int is_stream : 1;
} flags;
struct addrinfo *p;

void *get_in_addr(struct sockaddr *);
void handleConnection(int *sock);
command_t handeleCommand(char *);
void splitStr(char *, char, char *[]);

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

void debug(void)
{
  printf("ai_family %d\n", p->ai_family);
  printf("ai_socktype %d\n", p->ai_socktype);
  printf("ai_protocol %d\n", p->ai_protocol);
  char ipstring[INET6_ADDRSTRLEN];
  if (p->ai_family == AF_INET)
  {

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    void *addr = &(ipv4->sin_addr);
    int port = htons(ipv4->sin_port);
    printf("port is: %d\n", port);
    inet_ntop(p->ai_family, addr, ipstring, sizeof ipstring);
    printf(" addr: %s\n", ipstring);
  }
}

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

  struct addrinfo hints, *servinfo;
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
    debug();

    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;

    }
    //printf("%s", port);

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    //printf("%s", port);
    if ((rv = getaddrinfo(NULL, portNumber, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
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

  //freeaddrinfo(servinfo); // all done with this structure

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

    if (sendMsg(new_fd, "Hello, world!\n") == -1)
      perror("send");

    handleConnection(new_fd);
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

void handleConnection(int *sock)
{
  char *msg, clientMsg[BUFFSIZE], arg[BUFFSIZE], hostName[MAXHOSTNAMELEN];
  int recvStatus, sendStatus;
  int pasvPort, pasvSock;
  struct sockaddr_in pasvServer, controlServer;
  socklen_t sin_size;
  char ipstr[INET6_ADDRSTRLEN];

  msg = "220 - Welcome to FTP server. This server only supports the username: cs317\n";

  // Initialize flags
  flags.is_logged_in = flags.is_ascii = flags.is_stream = flags.is_fs = flags.is_passive = 0;

  if (sendMsg(sock, msg) == -1)
  {
    perror("send");
    return;
  }

  while ((recvStatus = recvMsg(sock, clientMsg)) > 0)
  {
    sscanf(clientMsg, "%s", arg);

    switch (handeleCommand(arg))
    {
    case USER:
      if (flags.is_logged_in == 0)
      {
        sscanf(clientMsg, "%s %s", arg, arg);

        if (!strcasecmp(arg, "cs317"))
        {
          flags.is_logged_in = 1;

          msg = "230 - Login successful\n";
          sendMsg(sock, msg);
        }
        else
        {
          msg = "430 - This server only supports the username: cs317\n";
          sendMsg(sock, msg);
        }
      }
      else
      {
        msg = "430 - Can't change from cs317\n";
        sendMsg(sock, msg);
      }

      printf("%d\n", USER);
      break;
    case TYPE:
      if (flags.is_logged_in == 1)
      {
        sscanf(clientMsg, "%s %s", arg, arg);
        if (strcasecmp(arg, "A") == 0)
        {
          flags.is_ascii = 1;
          msg = "200 - Switching to ASCII mode\n";
        }
        else if (strcasecmp(arg, "I") == 0)
        {
          flags.is_ascii = 0;
          msg = "200 - Switching to Image mode\n";
        }
        else
        {
          msg = "504 - This server onlys support Type A and Type I. \n";
        }
      }
      else
      {
        msg = "504 - Login first\n";
      }
      sendMsg(sock, msg);
      printf("%d\n", TYPE);
      break;
    case MODE:
      if (flags.is_logged_in == 1)
      {
        sscanf(clientMsg, "%s %s", arg, arg);

        if (strcasecmp(arg, "S"))
        {
          flags.is_stream = 1;
          msg = "200 Mode set to S.\n";
        }
        else
        {
          msg = "504 Bad MODE command.\n";
        }
      }
      else
      {
        msg = "530 Login first.\n";
      }
      sendMsg(sock, msg);
      printf("%d\n", MODE);
      break;
    case STRU:
      if (flags.is_logged_in == 1)
      {
        sscanf(clientMsg, "%s %s", arg, arg);

        if (strcasecmp(arg, "F"))
        {
          flags.is_fs = 1;
          msg = "200 Data Structure set to File Structure.\n";
        }
        else
        {
          msg = "504 Bad STRU command.\n";
        }
      }
      else
      {
        msg = "530 Login first.\n";
      }
      sendMsg(sock, msg);

      printf("%d\n", STRU);
      break;
    case PASV:
      if (flags.is_logged_in == 1)
      {
        // Random Port
        do
        {
          pasvPort = (rand() % 64512 + 1024);
          // New Socket
          if ((pasvSock = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1)
          {
            perror("server: socket");
            continue;
          }

          pasvServer.sin_family = p->ai_family;
          pasvServer.sin_addr.s_addr = INADDR_ANY;
          pasvServer.sin_port = htons(pasvPort);

        } while (bind(pasvSock, (struct sockaddr *)&pasvServer, sizeof(pasvServer)) < 0);

        if(!(pasvPort > 1024 && pasvPort <= 65535)){
          perror("server: pasvPort error");
          continue;
        }

        listen(pasvSock, 1);
        flags.is_passive = 1;

        sin_size = sizeof controlServer;
        getsockname(sock, (struct sockaddr *)&controlServer, &sin_size);

        inet_ntop(p->ai_family, &controlServer.sin_addr, ipstr, sizeof ipstr);
        printf(" addr: %s\n", ipstr);
        char *splitIp[4];
        splitStr(ipstr, '.', splitIp);

        char temp[BUFFSIZE];
        sprintf(temp, "227 Entering Passive Mode (%s,%s,%s,%s,%d,%d).", splitIp[0], splitIp[1], splitIp[2], splitIp[3], pasvPort >> 8, pasvPort & 0xff);
        msg = temp;
      }else{
        msg = "530 Please login with USER.";
      }
      sendMsg(sock, msg);
      printf("%d\n", PASV);
      break;
    case NLST:
      if(flags.is_logged_in == 1){
        if(flags.is_passive == 1){

          
        }else{
          msg = "425 Use PASV first.";
        }
      }else{
        msg = "530 Please login with USER.";
      }
      sendMsg(sock, msg);
      printf("%d\n", NLST);
      break;
    case RETR:
      printf("%d\n", RETR);
      break;
    case CWD:
      printf("%d\n", CWD);
      break;
    case CDUP:
      printf("%d\n", CDUP);
      break;
    case QUIT:
      exit(0);
    case INVALID:
      msg = "500 - no this command\n";
      sendMsg(sock, msg);
    }

    // if(strcasecmp(command,"EXIT")==0){
    //   exit(0);
    // }
  }
}

command_t handeleCommand(char *s)
{
  if (strcasecmp(s, "USER") == 0)
    return USER;

  if (strcasecmp(s, "TYPE") == 0)
    return TYPE;

  if (strcasecmp(s, "MODE") == 0)
    return MODE;

  if (strcasecmp(s, "STRU") == 0)
    return STRU;

  if (strcasecmp(s, "PASV") == 0)
    return PASV;

  if (strcasecmp(s, "NLST") == 0)

    return NLST;

  if (strcasecmp(s, "RETR") == 0)

    return RETR;

  if (strcasecmp(s, "CWD") == 0)

    return CWD;

  if (strcasecmp(s, "CDUP") == 0)

    return CDUP;

  if (strcasecmp(s, "QUIT") == 0)

    return QUIT;

  return INVALID;
}

void splitStr(char *s, char delim, char *arr[])
{
  printf("str: %s\n", s);
  printf("c: %c\n", delim);
  char *p = strtok(s, &delim);
  int i = 0;

  while (p != NULL)
  {
    arr[i++] = p;
    p = strtok(NULL, &delim);
  }

  for (i = 0; i < 4; ++i)
    printf("%s\n", arr[i]);

  return;
}

// void toUpper(char * s) {
//   char * name;
//   name = strtok(s,":");

//   // Convert to upper case
//   char *s = name;
//   while (*s) {
//     *s = toupper((unsigned char) *s);
//     s++;
//   }

// }
