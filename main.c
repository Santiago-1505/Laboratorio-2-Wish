#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

void manage_input(char *comando);

int main(int argc, char *argv[]){
  char comando[100];
  while (TRUE) {
    printf("wish> ");
    if(fgets(comando, 100,stdin) == NULL){
      exit(EXIT_FAILURE);
    }
    manage_input(comando);
  }
}

void manage_input(char *comando){
  char *delim = " ";

  if (strncmp("exit\n", comando, 4) == EXIT_SUCCESS){
    exit(EXIT_SUCCESS);
  }

  char *token = strsep(&comando, " ");
  char path[100] ="/usr/bin/";
  strcat(path, token);
  if (access(path, X_OK) == EXIT_SUCCESS){
    printf("Si es ejecutable");
  }

  while(token != NULL){
  printf("%s",token);
  token = strsep(&comando, " ");
  }
}

