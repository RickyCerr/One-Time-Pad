#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
                        int portNumber){

  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address));

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){

  int is_enc;

  #ifdef DEC
    //puts("I am dec server!");
    is_enc = 0;
  #else
    //puts("I am enc server!");
    is_enc = 1;
  #endif


  alarm(1800);
  //int bool_continue = 1;  //******
  int connectionSocket, plaint_charsRead;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) {
    fprintf(stderr,"USAGE: %s port\n", argv[0]);
    exit(1);
  }

  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket,
          (struct sockaddr *)&serverAddress,
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5);

  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket,
                (struct sockaddr *)&clientAddress,
                &sizeOfClientInfo);
    if (connectionSocket < 0){
      error("ERROR on accept");
    }


    //checking if its allowed to connect (enc vs dec)
    int is_client_enc, is_allowed;
    int id_charsRead = recv(connectionSocket, &is_client_enc, sizeof(is_client_enc), 0);
    //printf("\nClient: %d, Server: %d\n", is_client_enc, is_enc);
    if (id_charsRead <= 0){
      error("CLIENT: ERROR reading from socket");
      break;
    }
    if (id_charsRead > 0){
      if (is_enc && !is_client_enc){

        //printf("\nNOT ALLOWED\n");
        is_allowed = 0;
        int id_charsWritten = send(connectionSocket, &is_allowed, sizeof(is_allowed), 0);
        if (id_charsWritten < 0){
          error("CLIENT: ERROR writing to socket");
          break;
        }
        continue;
      }
      else if (!is_enc && is_client_enc){

        //printf("\nNOT ALLOWED\n");
        is_allowed = 0;
        int id_charsWritten = send(connectionSocket, &is_allowed, sizeof(is_allowed), 0);
        if (id_charsWritten < 0){
          error("CLIENT: ERROR writing to socket");
          break;
        }
        continue;
      }
      else{

        //printf("\nIS ALLOWED\n");
        is_allowed = 1;
        int id_charsWritten = send(connectionSocket, &is_allowed, sizeof(is_allowed), 0);
        if (id_charsWritten < 0){
          error("CLIENT: ERROR writing to socket");
          break;
        }

      }
    }




    for(;;){//while(bool_continue){ //******
      char char_plaint, char_key;
      char combined[2];

      plaint_charsRead = recv(connectionSocket, combined, 2, 0);
      if (plaint_charsRead < 0){
        error("ERROR reading from socket");
      }
      if (plaint_charsRead == 2){
        char_plaint = combined[0];
        char_key = combined[1];

        if(char_plaint == '\4' ){
          //printf("<<received EOT>>");
          //bool_continue = 0; //*****
          break;
        }
        //----------------------encryption
        char cipher_char;
        if(is_enc){
          int plaint_value = (char_plaint == ' ') ? 26 : (char_plaint - 'A');
          int key_value = (char_key == ' ') ? 26 : (char_key - 'A');
          int cipher_int = (plaint_value + key_value) % 27;
          if(cipher_int <= 25){
            cipher_char = 'A' + cipher_int;
          }
          else if(cipher_int == 26){
            cipher_char = ' ';
          }
        }
        //----------------------end of encryption

        //----------------------decryption
        else if(!is_enc){
          int cipher_int = (char_plaint == ' ') ? 26 : (char_plaint - 'A');
          int key_value = (char_key == ' ') ? 26 : (char_key - 'A');
          int plain_int = (cipher_int - key_value + 27) % 27;
          if (plain_int <= 25) {
              cipher_char = 'A' + plain_int;
          } else if (plain_int == 26) {
              cipher_char = ' ';
          }
        }
        //----------------------end of decryption

        //printf("\nCipher     : %c", cipher_char);
        //fwrite(&cipher_char, sizeof(char), 1, stdout);
        plaint_charsRead = send(connectionSocket, &cipher_char, 1, 0);
        if(plaint_charsRead == 0){

          printf("\nEnd of file for both SERVER\n");
          //bool_continue = 0; //*****
          break;
        }
      }

      if(char_plaint == '\4' ){
        //printf("<<received EOT>>");
        //bool_continue = 0; //*****
        break;
      }
    }

    close(connectionSocket);
  }
  // Close the listening socket
  close(listenSocket);
  return 0;
}
