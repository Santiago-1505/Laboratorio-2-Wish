/**
 * wish.c - Un shell simplificado (Wish Shell)
 *
 * Implementa un intérprete de comandos básico que:
 *   - Muestra un prompt "wish> " al usuario
 *   - Lee comandos desde stdin
 *   - Ejecuta programas ubicados en /usr/bin/
 *   - Soporta argumentos en los comandos
 *   - Termina con el comando "exit"
 */

#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define MAX_PATHS 100

char *paths[MAX_PATHS] = {
"/usr/bin/",
"/bin/",
NULL
};

/* Prototipo de la función que procesa cada comando ingresado */
void manage_input(char *comando);
void check_command(char *comando);
/**
 * main - Punto de entrada del shell.
 *
 * Ejecuta un bucle infinito que:
 *   1. Muestra el prompt "wish> "
 *   2. Lee una línea de entrada del usuario
 *   3. Pasa la línea a manage_input() para procesarla
 *
 * Si fgets() retorna NULL (EOF o error), el shell termina con fallo.
 *
 * @argc: Número de argumentos de línea de comandos (no utilizados)
 * @argv: Vector de argumentos de línea de comandos (no utilizados)
 * @return: No retorna en condiciones normales (bucle infinito)
 */

int main(int argc, char *argv[]){
  char comando[100]; /* Buffer para almacenar la línea ingresada */
  while (TRUE) {
    printf("wish> ");

    /* Lee hasta 99 caracteres + '\0'; sale si hay EOF o error */
    if(fgets(comando, 100,stdin) == NULL){
      exit(EXIT_FAILURE);
    }
    manage_input(comando);
  }
}

/**
 * manage_input - Procesa y ejecuta un comando ingresado por el usuario.
 *
 * Pasos:
 *   1. Detecta el comando "exit" y termina el shell limpiamente.
 *   2. Elimina el salto de línea final del comando.
 *   3. Tokeniza la entrada por espacios para separar comando y argumentos.
 *   4. Construye la ruta absoluta del ejecutable en /usr/bin/.
 *   5. Verifica que el ejecutable tenga permisos de ejecución.
 *   6. Crea un proceso hijo con fork() y ejecuta el comando con execv().
 *   7. El proceso padre espera a que el hijo termine con wait().
 *
 * @comando: Cadena con la línea completa ingresada por el usuario,
 *           incluyendo el '\n' al final tal como la devuelve fgets().
 */

void manage_input(char *comando){

  /* Si el usuario escribe "exit", terminar el shell */
  if (strncmp(comando, "exit\n", 5) == 0){
    exit(EXIT_SUCCESS);
  }

  /* Eliminar el '\n' del final para evitar que forme parte del comando */
  comando[strcspn(comando, "\n")] = '\0';
  char *input = comando;

  // Recolectar argumentos
  char *args[100]; /* Array de punteros: args[0]=comando, args[1..]=argumentos */
  int i = 0;

  /*
   * strsep() divide 'input' por espacios en cada iteración,
   * retornando cada token. Termina cuando input llega a NULL.
   */
  
  int redirect_index;
  int redirect_counter = 0;
  while ((args[i] = strsep(&input, " ")) != NULL){
    if (strncmp(args[i], ">", 1) == 0)
    {
      redirect_index = i;
      redirect_counter++;
      if (redirect_counter > 1)
      {
        printf("Error: Redirección múltiple no permitida\n");
        return;
      }
    }
    
    i++;
  }
  char *output_file = NULL;

  if (redirect_counter == 1)
  {
    if (args[redirect_index + 1] == NULL)
    {
      printf("Error: Falta archivo de redirección\n");
      return;
    }

    output_file = args[redirect_index + 1];

    args[redirect_index] = NULL; // cortar args para execv
  }
  args[i] = NULL; // execv requiere que el último sea NULL


  if(strncmp(args[0], "chd", 3) == 0){
    if(chdir(args[1]) != 0){
      printf("Error: No se pudo cambiar el directorio a %s\n", args[1]);
    }
    return;
  }

  if(strncmp(args[0], "route", 5) == 0){
    if(args[1] == NULL){
      paths[0] = NULL;
      return;
    }
    for (int j = 1; args[j] != NULL; j++){
      paths[j - 1] = args[j];
      paths[j] = NULL; // Asegurar que el siguiente sea NULL
      return;
    }
  }

  char fullpath[200];
  int found = 0;

  for (int i = 0; paths[i] != NULL; i++)
  {
    strcpy(fullpath, paths[i]);
    /*strcat(fullpath, "/");*/
    strcat(fullpath, args[0]);

    if(access(fullpath, X_OK) == 0){
      found = 1;
      break;
    }
  }
  if (!found){
    printf("the command does not exist\n");
    return;
  }

  pid_t pid = fork();

  if (pid == 0)
  {

    if (output_file != NULL)
    {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0)
      {
        printf("Error al abrir archivo\n");
        exit(EXIT_FAILURE);
      }

      dup2(fd, STDOUT_FILENO);
      close(fd);
    }

    execv(fullpath, args);
    exit(EXIT_FAILURE);
  }
  else
  {
    wait(NULL);
  }
}
