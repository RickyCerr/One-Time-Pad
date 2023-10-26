#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <err.h>
#include <errno.h>
#include <ctype.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(0);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber){

  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address));

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname("localhost");
  if (hostInfo == NULL) {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(0);
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {

  int is_enc;

  #ifdef DEC
    //puts("I am dec client!");
    is_enc = 0;
  #else
    //puts("I am enc client!");
    is_enc = 1;
  #endif


  int socketFD, portNumber, plaint_charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  // Check usage & args
  if (argc < 4) {
    fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }

  FILE* plaintext_file = fopen(argv[1], "r");
  if (plaintext_file == NULL) {                        // FILE NAME DOES NOT EXIST ERROR
      err(1, "Failed to open the plaintext file"); // Read error
      return 1;
  }
  fseek(plaintext_file, 0, SEEK_END);
  long plaint_size = ftell(plaintext_file);
  fseek(plaintext_file, 0, SEEK_SET);

  //--------------------------------------bad char check
  char checked_char;
  for(int i = 0; i < plaint_size; i++){
    fread(&checked_char, sizeof(char), 1, plaintext_file);
    if(!isupper(checked_char) && checked_char != ' ' && checked_char != '\n'){
      fprintf(stderr, "Error: Bad character(s) detected: %c\n", checked_char);
      exit(1);
    }
  }
  fseek(plaintext_file, 0, SEEK_SET);
  //--------------------------------------------bad char check end

  FILE* key_file = fopen(argv[2], "r");
  if (key_file == NULL) {                        // FILE NAME DOES NOT EXIST ERROR
      err(1, "Failed to open the key file"); // Read error
      return 1;
  }
  fseek(key_file, 0, SEEK_END);
  long key_size = ftell(key_file);
  fseek(key_file, 0, SEEK_SET);

  if(key_size < plaint_size){
    fprintf(stderr, "Error: Key size was smaller than plaintext size\n");
    exit(1);
  }

  portNumber = atoi(argv[3]);
  if(portNumber <= 0){
    error("CLIENT: Invalid port number");
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, portNumber);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  //printf("\nIs enc (client): %d\n", is_enc);
  //checking if its allowed to connect (enc vs dec)
  int id_charsWritten = send(socketFD, &is_enc, sizeof(is_enc), 0);
  if (id_charsWritten < 0){
    error("CLIENT: ERROR writing to socket");
    exit(1);
  }
  int is_allowed;
  int id_charsRead = recv(socketFD, &is_allowed, sizeof(is_allowed), 0);
  if (id_charsRead <= 0){
    error("CLIENT: ERROR reading from socket");
    exit(1);
  }
  //printf("\nIs allowed received (client): %d\n", is_allowed);
  if (id_charsRead > 0){
    if (!is_allowed){
      fprintf(stderr,"This client doesn't have permission to access port: %d\n", portNumber);
      exit(2);
    }
  }


  char combined[2];
  char EOT_msg[2] = {'\4', '\4'};
  char newline = '\n';
  for(;;){
    //printf("\n<<<<HERE>>>>\n");
    char plaint_char;
    char key_char;

    size_t plaint_read_size = fread(&plaint_char, sizeof(char), 1, plaintext_file);
    fread(&key_char, sizeof(char), 1, key_file);
    //printf("\n<<<<HERE2>>>>\n");
    if(plaint_char == '\n'){
      //printf("\n<<<<END>>>>\n");
      plaint_charsWritten = send(socketFD, EOT_msg, 2, 0);
      fwrite(&newline, sizeof(char), 1, stdout);
      fflush(stdout);
      fclose(plaintext_file);
      //printf("\nclosed\n");
      break;
    }

    combined[0] = plaint_char;
    combined[1] = key_char;
    //printf("\n<<<<HERE3>>>>\n");
    //printf("\nRead size: %ld", plaint_read_size);
    if(plaint_read_size == 1){
    //  printf("%c", plaint_char);
      //printf("%c", combined[1]);
      plaint_charsWritten = send(socketFD, combined, 2, 0);
      if (plaint_charsWritten < 0){
        error("CLIENT: ERROR writing to socket");
        break;
      }
    }

    char cipher_char;
    charsRead = recv(socketFD, &cipher_char, 1, 0);
    if (charsRead == 0){
      //printf("\nEnd of stuff\n");
      break;
    }
    if (charsRead < 0){
      error("CLIENT: ERROR reading from socket");
      //break;
    }
    if (charsRead == 1){
      //printf("\nReceived back: %c", cipher_char);
      fwrite(&cipher_char, sizeof(char), 1, stdout);
      fflush(stdout);
    }
    if (ferror(plaintext_file) || ferror(key_file)) {
        err(1, "Read error"); // Read error
        break;
    }

  }

  close(socketFD);
  return 0;
}
