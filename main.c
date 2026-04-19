/**
 * wish.c - Un shell simplificado (Wish Shell)
 *
 * Implementa un intérprete de comandos básico que:
 *   - Muestra un prompt "wish> " al usuario
 *   - Lee comandos desde stdin
 *   - Ejecuta programas ubicados en /usr/bin/
 *   - Soporta argumentos en los comandos
 *   - Soporta ejecución paralela con el operador &
 *   - Termina con el comando "exit"
 */

#include "stdio.h"
#include "stdlib.h"
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define MAX_PATHS 100

char *paths[MAX_PATHS] = {"/bin", NULL};

/* Prototipo de la función que procesa cada comando ingresado */
void manage_input(char *comando);
pid_t execute_single(char *sub);
void error();

/**
 * execute_single - Parsea y ejecuta un único sub-comando (sin &).
 *
 * Construye el array de args, gestiona redirección y hace fork+execv.
 * Si forkear, retorna el PID del hijo para que el llamador haga wait();
 * si no hay fork (built-in o error), retorna -1.
 *
 * @sub: Cadena con un único comando y sus argumentos (sin '\n' ni '&').
 * @return: PID del proceso hijo creado, o -1 si no se creó ninguno.
 */
pid_t execute_single(char *sub) {
  char buffer[200];
  int j = 0;

  for (int i = 0; sub[i] != '\0'; i++) {
    if (sub[i] == '>') {
      buffer[j++] = ' ';
      buffer[j++] = '>';
      buffer[j++] = ' ';
    } else {
      buffer[j++] = sub[i];
    }
  }
  buffer[j] = '\0';

  char *input = buffer;

  /* Recolectar argumentos */
  char *args[100];
  int i = 0;

  int redirect_index = -1;
  int redirect_counter = 0;

  while ((args[i] = strsep(&input, " \t")) != NULL) {
    /* Saltar tokens vacíos generados por espacios múltiples */
    if (args[i][0] == '\0') {
      continue;
    }
    else if (strcmp(args[i], ">>") == 0) {
        error();
        return -1;
    }
    else if (strcmp(args[i], ">") == 0) {
      redirect_index = i;
      redirect_counter++;
      if (redirect_counter > 1) {
        error();
        return -1;
      }
    }
    i++;
  }
  args[i] = NULL; /* execv requiere que el último sea NULL */

  /* Si no hubo ningún token real, ignorar */
  if (i == 0 || args[0] == NULL || args[0][0] == '\0') {
    return -1;
  }

  /* Si el usuario escribe "exit", terminar el shell */
  if (strncmp(args[0], "exit", 4) == 0) {
    if (args[1] != NULL && args[1][0] != '\0') {
      error();
      return -1;
    }
    exit(EXIT_SUCCESS);
  }

  /* Built-in: chd */
  if (strncmp(args[0], "chd", 3) == 0) {
    if (args[1] == NULL || args[2] != NULL) {

      error();
      return -1;
    }

    else if (chdir(args[1]) != 0) {
      error();
      return -1;
    }
    return -1;
  }

  /* Built-in: route */
  if (strncmp(args[0], "route", 5) == 0) {
    if (args[1] == NULL) {
      paths[0] = NULL;
      return -1;
    }
    int j;
    for (j = 1; args[j] != NULL; j++) {
      /*
       * strdup copia el string en memoria propia del heap,
       * evitando que el puntero quede apuntando al buffer
       * temporal de manage_input (que se sobreescribe en
       * cada nuevo comando).
       */
      paths[j - 1] = strdup(args[j]);
    }
    paths[j - 1] = NULL; /* terminador fuera del bucle */
    return -1;
  }

  // Built-in shell version
  if (strncmp(args[0], "wish", 4) == 0 &&
      (strncmp(args[1], "--version", 9) == 0 ||
       strncmp(args[1], "-v", 2) == 0)) {
    printf("v0.0.1\n");
    return -1;
  }

  char *output_file = NULL;

  if (redirect_counter == 1) {
    if (redirect_index < 0 || args[redirect_index + 1] == NULL ||
        args[redirect_index + 2] != NULL) {
      error();
      return -1;
    }
    output_file = args[redirect_index + 1];
    args[redirect_index] = NULL; /* cortar args para execv */
  }

  /* Buscar el ejecutable en paths */
  char fullpath[200];
  int found = 0;

  for (int k = 0; paths[k] != NULL; k++) {
    strcpy(fullpath, paths[k]);
    strcat(fullpath, "/");
    strcat(fullpath, args[0]);

    if (access(fullpath, X_OK) == 0) {
      found = 1;
      break;
    }
  }

  if (!found) {
    error();
    return -1;
  }

  /* Fork y exec */
  pid_t pid = fork();

  if (pid == 0) {
    /* Proceso hijo */
    if (output_file != NULL) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        error();
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }

    execv(fullpath, args);
    error();
  }

  /* Proceso padre: devuelve el PID para que el llamador espere */
  return pid;
}

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
int main(int argc, char *argv[]) {
  char comando[100]; /* Buffer para almacenar la línea ingresada */

  /*
   * Si se pasa un argumento, abrir el archivo como fuente de entrada
   * (batch mode). Si no, usar stdin (modo interactivo).
   * Más de un argumento es un error.
   */
  FILE *input;
  int interactive;

  if (argc == 1) {
    input = stdin;
    interactive = TRUE;
  } else if (argc == 2) {
    input = fopen(argv[1], "r");
    if (input == NULL) {
      error();
      exit(EXIT_FAILURE);
    }
    interactive = FALSE;
  } else {
    error();
    exit(EXIT_FAILURE);
  }

  while (TRUE) {
    /* El prompt solo se muestra en modo interactivo */
    if (interactive) {
      printf("wish> ");
    }

    /* Sale al llegar a EOF (Ctrl+D en interactivo, fin de archivo en batch) */
    if (fgets(comando, 100, input) == NULL) {
      if (!interactive) {
        fclose(input);
      }
      exit(EXIT_SUCCESS);
    }
    manage_input(comando);
  }
}

/**
 * manage_input - Procesa y ejecuta uno o varios comandos (separados por &).
 *
 * Pasos:
 *   1. Detecta el comando "exit" y termina el shell limpiamente.
 *   2. Elimina el salto de línea final del comando.
 *   3. Divide la línea por el operador '&' para obtener sub-comandos.
 *   4. Para cada sub-comando llama a execute_single(), recolectando PIDs.
 *   5. Espera a que todos los procesos hijos terminen con waitpid().
 *
 * @comando: Cadena con la línea completa ingresada por el usuario,
 *           incluyendo el '\n' al final tal como la devuelve fgets().
 */
void manage_input(char *comando) {

  /* Eliminar el '\n' del final */
  comando[strcspn(comando, "\n")] = '\0';

  /*
   * Dividir la línea por '&' para obtener sub-comandos paralelos.
   * Se copian los tokens en sub_cmds[] para poder luego ejecutarlos.
   */
  char *sub_cmds[100];
  int n_cmds = 0;
  char *rest = comando;
  char *token;

  while ((token = strsep(&rest, "&")) != NULL) {
    sub_cmds[n_cmds++] = token;
  }

  /*
   * Lanzar todos los sub-comandos y recolectar sus PIDs.
   * Los built-ins (chd, route) devuelven -1 y se ejecutan en el padre.
   */
  pid_t pids[100];
  int n_pids = 0;

  for (int i = 0; i < n_cmds; i++) {
    pid_t p = execute_single(sub_cmds[i]);
    if (p > 0) {
      pids[n_pids++] = p;
    }
  }

  /* Esperar a que todos los hijos terminen */
  for (int i = 0; i < n_pids; i++) {
    waitpid(pids[i], NULL, 0);
  }
}

void error() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}
