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

int main(int argc, char *argv[]) {

  if (argc != 2 && argc != 4) {
    errx(1, "Incorrect number of arguments.\n");
  }

  int key_length = atoi(argv[1]);
  //printf("KEY Length: %d\n", key_length);
  if (key_length <= 0) {
    errx(1, "Invalid key length\n");
  }

  FILE *output_file = stdout;

  if (argc == 4 && strcmp(argv[2], ">") != 0){
    errx(1, "Invalid redirect opperator\n");
  }
  else if (argc == 4 && !strcmp(argv[2], ">")) {
    output_file = fopen(argv[3], "w");
    if (output_file == NULL) {
            err(EXIT_FAILURE, "Error opening file for writing");
        }
  }

  srand(time(NULL));

  char *new_key = (char *)malloc((key_length + 1) * sizeof(char));
    if (new_key == NULL) {
        err(EXIT_FAILURE, "Memory allocation error");
    }

  for (int i = 0; i < key_length; i++) {
    int random_int = (rand() % 27);
    if (random_int <= 25){
      new_key[i] = 'A' + random_int;
    }
    else if(random_int == 26){
      new_key[i] = ' ';
    }
  }

  new_key[key_length] = '\n';


  //char *output = "hello world";
  int len = strlen(new_key);
  //printf("Length: %d\n", len);
  size_t elements_written = fwrite(new_key, sizeof(char), len, output_file);
  if (elements_written == -1) {
        perror("fwrite");
        fclose(output_file);
        return 1;
    }

  fclose(output_file);
  //char *key = generateRandomKey(key_length);
  //printf("Generated key: %s\n", key);

  //free(key);

  return EXIT_SUCCESS;

}
