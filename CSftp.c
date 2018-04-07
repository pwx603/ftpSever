#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <memory.h>
#include "dir.h"
#include "usage.h"
#include<stdarg.h>

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

#define BACKLOG 1
void *connectionHandler(void*);




int portNumber;


int sendingPath( int  descriptor, char * file , uint32_t offset);
int sendingFiles(int descriptor, FILE *f);

/**
 * sendingPath: handles file transfer, using fopen to read the file (non-text files)
 *  calls sendingFiles to actually pass the file bit by bit
 * @param descriptor
 * @param file
 * @param offset
 * @return integer, if 0 meaning successfully read file, -1 if stream not closed or file not read properly
 */
int sendingPath( int  descriptor, char * file , uint32_t offset){
  FILE * f = fopen (file, "rb");
  if(f){
    fseek(f, offset, SEEK_SET);
    int st = sendingFiles(descriptor, f);
    if(st< 0 ){
      return -1;
    }


    int ret = fclose(f);
    return ret == 0 ? 0: -1;        //returns -1 if not 0, meaning stream has not been successfully closed

  }

}

/**
 *
 * @param descriptor
 * @param f
 * @return
 */
int sendingFiles(int descriptor, FILE *f){
  char filebuf[1025];
  int n, ret = 0;

  while((n = fread(filebuf, 1, 1024, f))>0){
    int st = send (descriptor, filebuf, n , 0);

    if(st<0){
      ret = -1;
      break;
    }else{
      filebuf[n] = 0;
    }

  }

  return ret;

}

int main(int argc, char **argv) {

  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc

  int i;

  //added
  int serverSock, clientStock, c, *newSock;
  struct sockaddr_in server,client;

  // Check the command line arguments
  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }

  portNumber = atoi(argv[1]);


  // start create sock

  serverSock = socket(AF_INET,SOCK_STREAM,0);

  if(serverSock ==-1){
    perror(" Error: Failed to create socket. \n ");
    return -1;
  }

  puts("Socket is created");

  // prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(portNumber);


  //bind
  if(bind(serverSock,(struct sockaddr *)&server, sizeof(server) )){
    perror("Error: Bind failed");
    return -1;
  }

  puts("Bind done.");


  //Listen
  listen(serverSock, BACKLOG);

  puts("Waiting for incoming connections...");

  c = sizeof(struct sockaddr_in);


  while ((clientStock = accept(serverSock, (struct sockaddr *) &client, (socklen_t *) &c))>=0) {

    puts("Connection accepted");


    // handle connection
    newSock = malloc(1);
    *newSock = clientStock;

    void (*connectionHandler_ptr)(int) = &connectionHandler;

    (*connectionHandler_ptr)((void *) newSock);

    puts("Closing connection.");


    puts("Waiting for incoming connections...");

  }



  // This is how to call the function in dir.c to get a listing of a directory.
  // It requires a file descriptor, so in your code you would pass in the file descriptor
  // returned for the ftp server's data connection
/*
    printf("Printed %d directory entries\n", listFiles(1, "."));
*/

  return 0;

}

void *connectionHandler( void * serverSock){

  int sock = *(int*)serverSock;
  int readSize;

  struct sockaddr_in serverAddr;
  int serverAddrLen = sizeof(serverAddr);

  /*
   *  getsockname () return the current address to which the sock is bound
   */
  getsockname(sock, (struct sockaddr*)&serverAddr,&serverAddrLen );


  int dataClient = -1;
  struct sockaddr_in dataClientAdrr;
  int dataClientLen = sizeof(dataClientAdrr);

  char cwd[1024], parent[1024];

  char* message, clientMessage[2000],command[4], parameter[1000];

  int loggedIn = 0, asciiType = 0, streamMode = 0, fsType = 0, passiveMode = 0;

  int pasvPort, pasvSock;
  struct sockaddr_in pasvServer;

  getcwd(parent,sizeof(parent));


  // send the first message
  message = "220 - Welcome to FTP server. This server only supports the username: cs317\n";
  write(sock, message, strlen(message));

  while((readSize = recv(sock, clientMessage, 2000, 0))>0){

    sscanf(clientMessage,"%s",command );

    //message = strcat(command, "\n");

    puts(command);
    puts("\n");

    if(strcasecmp(command,"USER")==0){
      // If not logged in
      if(loggedIn ==0){
        sscanf(clientMessage, "%s%s", parameter,parameter);

        if(!strcasecmp(parameter,"cs317")){
          loggedIn = 1;

          message = "230 - Login successful\n";
          write(sock, message, strlen(message));
          //    sendStr(sock,"230 - Login successful\n" );

        }else{
          message = "530 - This server only supports the username: cs317\n";
          write(sock, message, strlen(message));
        }
      }else{
        message = "530 - Can't change from cs317\n";
        write(sock, message, strlen(message));
      }
    }else if(strcasecmp(command,"TYPE")== 0){

      if(loggedIn ==1){

        sscanf(clientMessage,"%s",parameter);

        if(strcasecmp(parameter,"A")==0){

          if (asciiType == 0){
            asciiType =1;
            message = "200 - Setting TYPE to ASCII\n";
            write(sock, message, strlen(message));

          }else{
            message = "200 -Type is already ASCII\n";
            write(sock, message, strlen(message));

          }
        }else if(strcasecmp(parameter,"I")){

          if (asciiType == 1){
            asciiType = 0;
            message = "200 - Setting TYPE to Image\n";
            write(sock, message, strlen(message));

          }else{
            message = "200 -Type is already Image\n";
            write(sock, message, strlen(message));

          }

        }else{
          message = "504 -This server onlys support Type A and Type I. \n";
          write(sock, message, strlen(message));

        }

      }else{
        message = "530 -Must login first. \n";
        write(sock, message, strlen(message));
      }


    }else if (strcasecmp(command, "MODE")==0){


      if( loggedIn ==1) {
        sscanf(clientMessage, "%s", parameter);

        if (strcasecmp(parameter, "S")) {

          if (streamMode == 0) {
            streamMode = 1;
            message = "200 - Entering Stream mode. \n";
            write(sock, message, strlen(message));

          } else {
            message = "200 - Already in Stream mode. \n";
            write(sock, message, strlen(message));
          }

        } else {
          message = "504 - This server only supports MODE S. \n";
          write(sock, message, strlen(message));
        }
      } else{
        message = "530 -Must login first. \n";
        write(sock, message, strlen(message));

      }

    }else if(strcasecmp(command, "STRU")==0){

      if(loggedIn==1){

        sscanf(clientMessage, "%s", parameter);

        if (strcasecmp(parameter, "F")) {

          if (fsType == 0) {
            fsType = 1;
            message = "200 - Data Structure set to File Structure. \n";
            write(sock, message, strlen(message));

          } else {
            message = "200 - Data Structure is alreadyset to  File structure. \n";
            write(sock, message, strlen(message));
          }

        } else {
          message = "504 - This server only supports STRU F. \n";
          write(sock, message, strlen(message));
        }


      }else{
        message = "530 -Must login first. \n";
        write(sock, message, strlen(message));

      }


    }else if (strcasecmp(command,"PASV")==0){


      if(passiveMode ==0){

        // Loop until a passive socket is succesfully created

        do{
          //create a random port
          pasvPort = (rand()% 64512 +1024);

          //Create a new socket
          pasvSock = socket(AF_INET, SOCK_STREAM, 0);
          pasvServer.sin_family = AF_INET;
          pasvServer.sin_addr.s_addr = INADDR_ANY;
          pasvServer.sin_port = htons(pasvPort);

        }while( bind(pasvSock, (struct sockaddr *) &pasvServer, sizeof(pasvServer))<0);


        if(pasvPort<0){

          message = "500 - Error: entering Passive Mode. \n";
          write(sock, message, strlen(message));


        }else{

          listen(pasvSock,1);
          passiveMode =1;

          uint32_t t = serverAddr.sin_addr.s_addr;

          int a = t&0xff;
          int b = (t>>8 )&0xff;
          int c = (t>>16 )& 0xff;
          int d = (t>> 24)&0xff;
          int e =  pasvPort >>8;
          int f = pasvPort&0xff;
          char buf [256];
          // snprintf(buf, sizeof buf,"%s%d%s%d%s%d%s%d%s%d%s%d%s", "227 Entering passive mode(", a,",",b, ",",c,",",d,",",e,",",f,")\n" );
          snprintf(buf, sizeof buf,"%d%s%d%s%d%s%d%s%d%s%d%s%d%s",pasvPort, " 227 Entering passive mode(", a,",",b, ",",c,",",d,",",e,",",f,")\n" );

          write(sock, buf,strlen(buf)+1);

        }


      }else{

        char buf [256];
        snprintf(buf, sizeof buf,"%s%d\n", "227 Already in passive mode. Port number: ",pasvPort);
        write(sock, buf,strlen(buf));
      }


    }//else if (strcasecmp(command, "PORT") == 0){

      //}
    else if (strcasecmp(command,"NLST")==0){

      if(loggedIn ==1 ){


        if(passiveMode ==1){

          if(pasvPort > 1024 && pasvPort <= 65535 && pasvSock>= 0){
            asciiType = 1;
            message = "150 - here comes the directionry listing. \n";
            write(sock, message, strlen(message));


            listen(pasvSock,BACKLOG);
            dataClient = accept(pasvSock, (struct sockaddr *)&dataClientAdrr, &dataClientLen);

            getcwd(cwd, sizeof(cwd));
            listFiles(dataClient, cwd);

            message = "260 - Transfer complete. \n";
            write(sock, message, strlen(message));


            close(dataClient);
            dataClient = -1;

            close(pasvSock);
            passiveMode = 0;


          }else{
            message = "No passive server created. \n";
            write(sock, message, strlen(message));


          }



        }else{
          message = " Use PASV first. \n";
          write(sock, message, strlen(message));

        }




      }else{
        message = "Must login first. \n";
        write(sock, message, strlen(message));

      }



    }else if (strcasecmp(command,"RETR")==0){


      if(loggedIn ==1 ){


        if(passiveMode ==1){

          if(pasvPort > 1024 && pasvPort <= 65535 && pasvSock>= 0){
            asciiType = 0;
            message = " Opening binary mode data connection. \n";
            write(sock, message, strlen(message));


            listen(pasvSock,BACKLOG);
            dataClient = accept(pasvSock, (struct sockaddr *)&dataClientAdrr, &dataClientLen);


            sscanf(clientMessage,"%s%s", parameter, parameter);


            int st = sendingPath(dataClient, parameter, 0);

            if (st>= 0){
              message = "200 - Transfer complete. \n";
              write(sock, message, strlen(message));
            }else {
              message = " File not found. \n";
              write(sock, message, strlen(message));
            }


            close(dataClient);
            dataClient = -1;

            close(pasvSock);
            passiveMode = 0;


          }else{
            message = " No passive server created. \n";
            write(sock, message, strlen(message));


          }



        }else{
          message = " Use PASV first. \n";
          write(sock, message, strlen(message));

        }




      }else{
        message = "Must login first. \n";
        write(sock, message, strlen(message));

      }



    }else if( strcasecmp(command, "CWD") ==0 ){

      sscanf(clientMessage, "%s%s", parameter,parameter);


      if(strstr(parameter,"../")!= NULL){
        message = "  Failed to change path \n";
        write(sock, message, strlen(message));
      }else if( strncmp(parameter,"./",2)==0){
        message = "  Failed to change path \n";
        write(sock, message, strlen(message));
      }else if (chdir((char *) parameter) == 0) {
        message = " Changed path \n";
        write(sock, message, strlen(message));
      } else {
        message = "  Failed to change path \n";
        write(sock, message, strlen(message));
      }

    } else if (strcasecmp(command, "CDUP") ==0){


      chdir((char*)parent);

    } else if ( strcasecmp(command, "QUIT") ==0){
      message = " User has  quit Terminating connection. \n";
      write(sock, message, strlen(message));
      fflush(stdout);
      close(sock);
      close(pasvSock);
      break;

    }else{
      message = "500 - no this command\n";
      write(sock, message, strlen(message));
    }

    if(readSize == -1){
      perror("recv failed");
    }

    memset(clientMessage,0,sizeof clientMessage);

  }



}


