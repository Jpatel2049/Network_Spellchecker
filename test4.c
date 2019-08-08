
/* Test program to check and make sure that the dictionary checking works. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_DICTIONARY "words.txt"
#define DICTIONARY_LENGTH 99171
#define BUF_SIZE 256

char **open_dictionary(char *);

int main(int argc, char **argv) {
  char* file = DEFAULT_DICTIONARY;

  char ** list = open_dictionary(file);

  while(1) {
    char temp [BUF_SIZE] = "";
    printf("Enter a word. ");
    scanf("%s", temp);

    if(strcmp(temp,"quit") == 0 || strcmp(temp,"exit") == 0) {
      break;
    }

    char *valid = " INCORRECT\n";
    for (int i = 0; i < DICTIONARY_LENGTH; i++) {
      if (strcmp(temp, list[i]) == 0) {
        valid = " CORRECT\n";
        break;
      }
    }
    strcat(temp, valid);
    printf("%s", temp);
  }
  return 0;
}

char ** open_dictionary(char *input) {
  FILE *fd;
  char ** output = malloc(DICTIONARY_LENGTH * sizeof(char *) + 1);
  if(output == NULL) {
    printf("Unable to allocate memory for the dictionary.\n");
    exit(1);
  }
  char line [BUF_SIZE];
  int index = 0;

  fd = fopen(input, "r");
  if(fd == NULL) {
    printf("Unable to open dictionary file. \n");
    exit(1);
  }

  while((fgets(line, BUF_SIZE, fd)) != NULL) {
    output[index] = (char *) malloc(strlen(line) * sizeof(char *) + 1);
    if(output[index] == NULL) {
      printf("Unable to allocate memory for the dictionary.\n");
      exit(1);
    }
    int temp = strlen(line) - 2;
    line[temp] = '\0';
    strcpy(output[index], line);
    index++;
  }
  fclose(fd);
  return output;
}
