# Laboratorio de Sistemas Operativos donde debemos de hacer un shell simple en C

## Integrantes del equipo:
- Ricardo Medina Herrera 
    - C.C: 1036251162
    - Email: ricardo.medinah@udea.edu.co
- Santiago Villegas Naranjo
    - C.C: 1001479227
    - Email: santiago.villegasn@udea.edu.co

## [Video sustentación]()

## Documentación de las funciones
### Programa `wish`

#### Función `main`
La función principal es el punto de entrada del shell y se encarga de controlar cómo se reciben los comandos del usuario.

Primero se define un buffer `comando` de tamaño 100 para almacenar cada línea que el usuario ingresa.

Luego se determina el modo de ejecución del programa:

- Si `argc == 1`, el shell funciona en modo interactivo, es decir, lee directamente desde la entrada estándar (`stdin`) y muestra el prompt `wish> ` en cada iteración.
- Si `argc == 2`, se interpreta que se pasó un archivo como argumento, por lo que el programa entra en modo batch. En este caso, intenta abrir el archivo con `fopen` y leer los comandos desde ahí. Si no logra abrirlo, imprime un error y termina la ejecución.
- Si se pasan más de dos argumentos, se considera un uso incorrecto y se muestra el mensaje:
Usage: wish [batch_file]

y el programa finaliza.

Después de esto, se entra en un bucle infinito (`while (TRUE)`) donde:

1. Si está en modo interactivo, imprime el prompt `wish> `.
2. Lee una línea usando `fgets`.
3. Si `fgets` retorna `NULL` (por ejemplo, al terminar el archivo en modo batch), el programa cierra el archivo si es necesario y termina.
4. Si la lectura fue exitosa, se envía el comando a la función `manage_input` para que lo procese y ejecute.

---

#### Función `manage_input`
Esta función se encarga de procesar la línea completa ingresada por el usuario y ejecutar uno o varios comandos.

Primero verifica si el usuario escribió el comando `exit`. Si es así, el programa termina inmediatamente con `exit(EXIT_SUCCESS)`.

Luego elimina el salto de línea (`\n`) que deja `fgets` al final del string, usando `strcspn`.

Después, la función divide la línea en sub-comandos usando el operador `&`, que permite ejecutar comandos en paralelo. Esto se hace con `strsep`, guardando cada sub-comando en un arreglo `sub_cmds`.

Una vez separados los comandos, se recorren uno por uno:

- Para cada sub-comando se llama a la función `execute_single`.
- Si esta función retorna un PID mayor a 0, significa que se creó un proceso hijo, por lo que se guarda ese PID en el arreglo `pids`.

Es importante tener en cuenta que algunos comandos internos (como `chd` o `route`) no crean procesos hijos, por lo que `execute_single` retorna `-1` en esos casos y simplemente se ejecutan en el proceso padre.

Finalmente, la función espera a que todos los procesos hijos terminen usando `waitpid`, recorriendo el arreglo de PIDs almacenados previamente. Esto asegura que el shell no continúe hasta que todos los comandos en paralelo hayan finalizado.

### Función `execute_single`

Esta función se encarga de procesar y ejecutar un subcomando junto con sus argumentos. Generalmente es llamada desde la función `manage_input` y recibe como parámetro un puntero a `char` llamado `sub`, cuyo nombre proviene de “subcomando”. Esto se debe a que la función puede recibir comandos individuales que han sido previamente separados de una cadena mayor mediante el uso de ampersands (`&`).

La función inicia separando el subcomando en tokens utilizando el espacio como delimitador mediante `strsep`. Durante este proceso se ignoran los tokens vacíos generados por múltiples espacios consecutivos. Además, se verifica si alguno de los tokens corresponde al operador de redirección (`>`), lo cual indicaría que el comando requiere redirigir su salida. También se comprueba que exista como máximo un operador de redirección; en caso contrario, se reporta un error.

Posteriormente, se realizan validaciones iniciales sobre el subcomando. Si no se encuentra ningún token válido, la función retorna `-1`. También se contempla un caso especial para el comando `wish` con las opciones `--version` o `-v`, en cuyo caso se imprime la versión y se retorna `-1`.

En caso de que exista redirección, se valida que se haya especificado un archivo de salida inmediatamente después del operador `>`. Si esta condición no se cumple, se reporta un error. Cuando la redirección es válida, se guarda el nombre del archivo y se ajusta el arreglo de argumentos colocando `NULL` en la posición del operador para que `execv` reciba correctamente la lista de argumentos.

La función implementa dos comandos built-in. El primero es `chd`, que permite cambiar el directorio de trabajo utilizando `chdir`; si ocurre un error, se imprime un mensaje indicando el fallo. El segundo es `route`, que permite modificar la variable global `paths`, la cual almacena las rutas donde se buscarán los ejecutables. Si `route` no recibe argumentos, se limpia la lista estableciendo su primera posición en `NULL`. En caso contrario, se copian los argumentos en `paths` utilizando `strdup`, sobrescribiendo los valores existentes y finalizando con un `NULL`.

Para la ejecución de comandos externos, la función recorre el arreglo global `paths` y construye rutas completas concatenando cada path con el nombre del comando (ubicado en `args[0]`). Luego, utiliza la función `access` para verificar si el ejecutable existe y tiene permisos de ejecución. Si no se encuentra el comando en ninguna ruta, se imprime un mensaje de error y se retorna `-1`.

Si el ejecutable es encontrado, la función realiza un `fork`. En el proceso hijo, si se ha especificado redirección, se abre el archivo correspondiente y se redirige la salida estándar utilizando `dup2`. Posteriormente, se ejecuta el programa con `execv`. En caso de que falle la ejecución, el proceso hijo finaliza con error.

Finalmente, el proceso padre retorna el PID del proceso hijo, lo que permite que la función que llamó a `execute_single` pueda esperar su finalización utilizando `waitpid`. En caso de errores o cuando se ejecuta un comando built-in, la función retorna `-1`.

## Problemas presentados durante el desarrollo

## Pruebas realizadas
### Prueba 1: Ejecución básica de comandos
`ls`

![alt text](image.png)

`pwd`

![alt text](image-1.png)

`echo hola`

![alt text](image-2.png)

### Prueba 2: Comandos con argumentos
`ls -l`

![alt text](image-3.png)

`echo hola mundo`

![alt text](image-4.png)

### Prueba 3: Ejecución en paralelo (`&`)
`ls & pwd`

![alt text](image-5.png)

`echo hola & echo mundo`

![alt text](image-6.png)

### Prueba 4: Redirección de salida
`echo hola > salida.txt`

![alt text](image-7.png)
![alt text](image-8.png)

`cat salida.txt`

![alt text](image-9.png)

### Prueba 5: Error en redirección
`echo hola >`

![alt text](image-10.png)

`echo hola > archivo1 > archivo2`

![alt text](image-11.png)

### Prueba 6: Comando inexistente

`ll`

![alt text](image-12.png)

### Prueba 7: Comando `chd`

`chd ..`

![alt text](image-13.png)

`pwd`

![alt text](image-14.png)

### Prueba 8: Comando `route`
`route /usr/bin/`

![alt text](image-15.png)

`ls`

![alt text](image-16.png)

### Prueba 9: Salida del shell
`exit`

![alt text](image-17.png)

## Transparencia
