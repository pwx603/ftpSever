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
 * sendingFiles, reads the file byte by byte using fread, and then sends info to the descriptor socket
 * @param descriptor
 * @param f - file name
 * @return
 */
int sendingFiles(int descriptor, FILE *f){
  char filebuf[1025];
  int n, ret = 0;

  while((n = fread(filebuf, 1, 1024, f))>0){
    int st = send (descriptor, filebuf, n , 0); // send buffer holding the info

    if(st<0){
      ret = -1;   //if cannot send, return -1
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
  // 1st argument is port number
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

  // accepts connection from client
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
/**
 * handles the connection (control connection at first), handles all the RFC commands
 * @param serverSock - socket that is being listened
 * @return
 */
void *connectionHandler( void * serverSock){

  int mainSocket = *(int*)serverSock;   //dereference
  int readSize;

  struct sockaddr_in serverAddr;
  int serverAddrLen = sizeof(serverAddr);

  /*
   *  getsockname () return the current address to which the mainSocket is bound
   */
  getsockname(mainSocket, (struct sockaddr*)&serverAddr,&serverAddrLen );



  // initialize some string to hold inputs and parameters
  char* message, clientMessage[2000],command[4], parameter[1000];
  // keeps track of states
  int loggedIn = 0, streamMode = 0, fsType = 0, asciiType = 0, passiveMode = 0;

  int dataClient = -1;
  struct sockaddr_in dataClientAdrr;
  int dataClientLength = sizeof(dataClientAdrr);
  struct sockaddr_in pasvServer;
  int pasvPort, pasvSocket;


  char parentDir[1024];

  char cwdDir[1024];



  //gets path and name of current (meaning parentDir) working directory
  getcwd(parentDir,sizeof(parentDir));


  // send the first message
  message = "220 - Welcome to FTP server. This server only supports the username: cs317\n";
  write(mainSocket, message, strlen(message));

  while((readSize = recv(mainSocket, clientMessage, 2000, 0))>0){

    // Gets client command
    sscanf(clientMessage,"%s",command );

    //message = strcat(command, "\n");
    // Debug: Prints command received from Client side
    puts(command);
    puts("\n");

    if(strcasecmp(command,"USER")==0){
      // If not logged in
      if(loggedIn ==0){
        sscanf(clientMessage, "%s%s", parameter,parameter);

        if(!strcasecmp(parameter,"cs317")){
          loggedIn = 1;

          message = "230 - Login successful\n";
          write(mainSocket, message, strlen(message));  //sends message/output to client through the mainSocket

        }else{
          message = "530 - This server only supports the username: cs317\n";
          write(mainSocket, message, strlen(message));
        }
      }else{
        message = "530 - Can't change from cs317\n";
        write(mainSocket, message, strlen(message));
      }
    }else if(strcasecmp(command,"TYPE")== 0){

      if(loggedIn ==1){

        sscanf(clientMessage,"%s",parameter);

        if(strcasecmp(parameter,"A")==0){
          // allow setting to ASCII mode, not fully implemented
          if (asciiType == 0){
            asciiType =1;
            message = "200 - Setting TYPE to ASCII\n";
            write(mainSocket, message, strlen(message));

          }else{
            message = "200 -Type is already ASCII\n";
            write(mainSocket, message, strlen(message));

          }
        }else if(strcasecmp(parameter,"I")){
          // file type set to Image
          if (asciiType == 1){
            asciiType = 0;
            message = "200 - Setting TYPE to Image\n";
            write(mainSocket, message, strlen(message));

          }else{
            message = "200 -Type is already Image\n";
            write(mainSocket, message, strlen(message));

          }

        }else{
          message = "504 - This server only allows Type Image and ASCII. \n";
          write(mainSocket, message, strlen(message));

        }

      }else{
        message = "530 - Not logged in. \n";
        write(mainSocket, message, strlen(message));
      }


    }else if (strcasecmp(command, "MODE")==0){


      if( loggedIn ==1) {
        sscanf(clientMessage, "%s", parameter);

        if (strcasecmp(parameter, "S")) {

          if (streamMode == 0) {
            streamMode = 1;
            message = "200 - Switching to Stream mode. \n";
            write(mainSocket, message, strlen(message));

          } else {
            message = "200 - Already in Stream mode. \n";
            write(mainSocket, message, strlen(message));
          }

        } else {
          message = "504 - This server only supports Stream mode. \n";
          write(mainSocket, message, strlen(message));
        }
      } else{
        message = "530 - Not logged in. \n";
        write(mainSocket, message, strlen(message));

      }

    }else if(strcasecmp(command, "STRU")==0){

      if(loggedIn==1){

        sscanf(clientMessage, "%s", parameter);

        if (strcasecmp(parameter, "F")) {

          if (fsType == 0) {
            fsType = 1;
            message = "200 - Structure set to File Structure. \n";
            write(mainSocket, message, strlen(message));

          } else {
            message = "200 - Structure already set to File structure. \n";
            write(mainSocket, message, strlen(message));
          }

        } else {
          message = "504 - This server only allows File Structure. \n";
          write(mainSocket, message, strlen(message));
        }


      }else{
        message = "530 - Not logged in. \n";
        write(mainSocket, message, strlen(message));

      }


    }else if (strcasecmp(command,"PASV")==0){
      // Passive mode

      if(passiveMode ==0){

        // Loop until a passive mainSocket is created (should try tries all available ports)

        do{
          //create a random port
          pasvPort = (rand()% 64512 +1024); //find difference between limit

          //Create a new mainSocket
          pasvSocket = socket(AF_INET, SOCK_STREAM, 0);
          pasvServer.sin_family = AF_INET;
          pasvServer.sin_addr.s_addr = INADDR_ANY;
          pasvServer.sin_port = htons(pasvPort); //translate int to port, making sure bytes stored correctly

        }while( bind(pasvSocket, (struct sockaddr *) &pasvServer, sizeof(pasvServer))<0); //should be able to bind correctly


        if(pasvPort<0){

          message = "550 - Error: cannot enter Passive Mode. \n";
          write(mainSocket, message, strlen(message));


        }else{

          listen(pasvSocket,1);
          passiveMode =1;

          uint32_t t = serverAddr.sin_addr.s_addr;

          int a = t&0xff;
          int b = (t>>8 )&0xff;
          int c = (t>>16 )& 0xff;
          int d = (t>> 24)&0xff;
          int e =  pasvPort >>8;
          int f = pasvPort&0xff;
          char buf [256];



          snprintf(buf, sizeof buf,"%s%d%s%d%s%d%s%d%s%d%s%d%s","227 Entering passive mode(", a,",",b, ",",c,",",d,",",e,",",f,")\n" );

          write(mainSocket, buf,strlen(buf)+1); //size issue
          //outputs the required passive port info

        }


      }else{

        char buf [256];
        snprintf(buf, sizeof buf,"%s%d\n", "227 Already in passive mode. Port number: ",pasvPort);
        write(mainSocket, buf,strlen(buf));
      }


    }//else if (strcasecmp(command, "PORT") == 0){
          //Not handling this yet
      //}
    else if (strcasecmp(command,"NLST")==0){

      if(loggedIn ==1 ){


        if(passiveMode ==1){
          // check correct passive connection port
          if(pasvPort > 1024 && pasvPort <= 65535 && pasvSocket>= 0){
            asciiType = 1;
            message = "150 - Outputs Directory listing. \n";
            write(mainSocket, message, strlen(message));


            listen(pasvSocket,BACKLOG);
            dataClient = accept(pasvSocket, (struct sockaddr *)&dataClientAdrr, &dataClientLength);

            getcwd(cwdDir, sizeof(cwdDir)); //gets current directory
            listFiles(dataClient, cwdDir);

            message = "260 - Transfer complete. \n";
            write(mainSocket, message, strlen(message));


            close(dataClient);
            dataClient = -1;

            close(pasvSocket);
            passiveMode = 0;


          }else{
            message = "550 - No passive server created. \n";
            write(mainSocket, message, strlen(message));


          }



        }else{
          message = "503 - Use PASV first. \n";
          write(mainSocket, message, strlen(message));

        }




      }else{
        message = "530 - Not logged in. \n";
        write(mainSocket, message, strlen(message));

      }



    }else if (strcasecmp(command,"RETR")==0){


      if(loggedIn ==1 ){


        if(passiveMode ==1){

          if(pasvPort > 1024 && pasvPort <= 65535 && pasvSocket>= 0){
            asciiType = 0;
            message = " Enabling binary mode transfer. \n";
            write(mainSocket, message, strlen(message));


            listen(pasvSocket,BACKLOG);
            dataClient = accept(pasvSocket, (struct sockaddr *)&dataClientAdrr, &dataClientLength);


            sscanf(clientMessage,"%s%s", parameter, parameter);

            // begin trying to read the file and send it over to client
            int st = sendingPath(dataClient, parameter, 0);

            if (st>= 0){
              message = "200 - Transfer complete. \n";
              write(mainSocket, message, strlen(message));
            }else {
              message = "550 - File not found. \n";
              write(mainSocket, message, strlen(message));
            }


            close(dataClient);
            dataClient = -1;

            close(pasvSocket);
            passiveMode = 0;


          }else{
            message = "550 - No passive server created. \n";
            write(mainSocket, message, strlen(message));


          }



        }else{
          message = "503 - Use PASV first. \n";
          write(mainSocket, message, strlen(message));

        }




      }else{
        message = "530 - Not logged in. \n";
        write(mainSocket, message, strlen(message));

      }



    }else if( strcasecmp(command, "CWD") ==0 ){

      sscanf(clientMessage, "%s%s", parameter,parameter);


      if(strstr(parameter,"../")!= NULL){
        message = "550 - Illegal path ../ \n";
        write(mainSocket, message, strlen(message));
      }else if( strncmp(parameter,"./",2)==0){
        message = "550 - Illegal path \n";
        write(mainSocket, message, strlen(message));
      }else if (chdir((char *) parameter) == 0) {
        message = "250 - Changed path \n";
        write(mainSocket, message, strlen(message));
      } else {
        message = "550 - Failed to change path, recheck input \n";
        write(mainSocket, message, strlen(message));
      }

    } else if (strcasecmp(command, "CDUP") ==0){

      // return to previously stored parent directory
      chdir((char*)parentDir);

    } else if ( strcasecmp(command, "QUIT") ==0){
      message = "221 - User has  quit Terminating connection. \n";
      write(mainSocket, message, strlen(message));
      fflush(stdout);
      close(mainSocket);
      close(pasvSocket);
      break;

    }else{
      message = "500 - Unrecognized command, if PORT, EPSV, not implemented.\n";
      write(mainSocket, message, strlen(message));
    }

    if(readSize == -1){
      perror("recv failed");
    }

    memset(clientMessage,0,sizeof clientMessage);

  }



}


