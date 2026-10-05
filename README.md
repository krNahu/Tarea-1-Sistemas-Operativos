# Tarea 1: Sistemas Operativos

Para la realización de la Tarea 1 de Sistemas Operativos se desarrolló un programa que simula un planificador de tareas. Este lee un archivo de texto con un Grafo Acíclico Dirigido (DAG) y lanza procesos concurrentes usando `fork()`, respetando el límite de concurrencia establecido `K`. De esta manera, maneja la comunicación entre ellos mediante pipes de POSIX.

## Integrantes

* Jessica Aguilera
* Cristóbal Araya

## Compilación y Ejecución

El código se desarrolló en C++ y las normas de compilación fueron seguidas estrictamente. 

Para compilar:

```bash
g++ -Wall -Wextra -std=c++17 -o planificador planificador.cpp -lpthread
```

Para ejecutar, se debe proporcionar el archivo de texto y el límite de concurrencia "K", tal como se observa en la siguiente línea:

```bash
./planificador plan.txt 4
```


## Prueba de Estrés (10,000 tareas)

Para cumplir con el requerimiento de carga de trabajo pesada, desarrollamos un script en Python (`generar_estres.py`) que crea un plan de 10.000 actividades manteniendo la temática del enunciado original.

Este generador arma un DAG denso asegurando que:
* Las tareas tengan dependencias válidas hacia atrás (evitando ciclos lógicos).
* El 50% de las tareas se generen con el campo de tiempo en blanco, forzando al planificador C++ a asignarles una duración aleatoria.
* Un 1% de las tareas se generen intencionalmente con el prefijo `"falla_..."` (ej. `falla_asar_longaniza_402`). Esto nos permite gatillar la lógica de error y demostrar en tiempo real cómo funciona el efecto cascada que aborta tareas dependientes sin crashear el simulador.

Para crear el archivo de estrés y correr la prueba:
```bash
python3 generar_estres.py
./planificador plan_estres.txt 100


## Funciones implementadas

*   `leerPlan(string archivo)`: Esta función lee el archivo línea por línea, hace el split por ":" y maneja casos borde (como asignar un tiempo aleatorio si viene vacío). Además, arma la estructura del DAG guardando cuántas dependencias tiene cada tarea para saber cuándo están listas.

*   `lanzar(int i)`: Se encarga de instanciar los pipes de entrada y salida, y hacer el `fork()`. Maneja qué descriptores de archivo (FDs) se cierran en el padre y cuáles en el hijo.

*   `codigoHijo(int i, int fdEntrada, int fdSalida)`: Es el ciclo de vida del proceso hijo. Lee su pipe para contar cuántos insumos (`\n`) recibió, hace un `sleep()` para simular el trabajo, y luego escribe su resultado en el pipe de salida antes de morir.

*   `abortarRama(int origen)`: Recorre el grafo usando un recorrido en profundidad (DFS con una pila) para marcar como abortadas en cascada a todas las tareas que dependían de un proceso que acaba de fallar.

*   `abortarTodo()`: Cuando llega un `SIGINT` (Ctrl + C), el aviso del SEREMI, recorre la lista de procesos vivos, les manda un `SIGKILL`, hace un `waitpid()` para limpiar los zombies, y cierra el programa.

## Justificación de Decisiones de Diseño

Para pasar las pruebas de estrés (como la de 10.000 tareas) y cumplir con las especificaciones de la rúbrica, se tomaron las siguientes decisiones al momento de escribir el código:

1. **Cero Busy Waiting (`sigsuspend`)**
   Para no dejar al proceso padre consumiendo toda la CPU en un `while` infinito revisando si algún proceso terminó, se usó `sigprocmask` y `sigsuspend`. Básicamente, se bloquea `SIGCHLD` durante la ejecución normal y el padre se va a dormir con `sigsuspend` cuando topamos el límite "K" o no hay tareas listas. Cuando un hijo muere, despierta al padre al instante. Esto evita por completo las *race conditions*.

2. **Limpieza de File Descriptors (Evitar leaks)**
   Nos dimos cuenta de que al hacer `fork()`, los hijos heredan los descriptores de archivo abiertos del padre. Si lanzábamos muchos procesos, un hijo se quedaba con los pipes de lectura de los procesos lanzados anteriormente abiertos. Para evitar quedarnos sin FDs en la prueba de estrés, agregamos un iterador dentro de `codigoHijo` que cierra explícitamente los pipes de sus "hermanos" en ejecución antes de ponerse a trabajar.

3. **Centralización de IPC en el Padre**
   En vez de intentar que los hijos creen pipes directos entre ellos (lo cual era más complejo de enrutar), se centralizó la información. Cuando el padre lee que un hijo terminó, él mismo toma ese mensaje y lo concatena en un string `inbox` de todas las tareas dependientes. Así, cuando una tarea dependiente por fin arranca, el padre simplemente le inyecta ese gran string de texto por el pipe de una sola pasada.

4. **Uso de `_exit(1)` vs `exit(0)` en los hijos**
   En las simulaciones de fallo, los hijos llaman a `_exit()`. Decidimos usar esta syscall directa en lugar de la función estándar de C para evitar que el hijo vacíe accidentalmente los buffers de la consola (como los `cout` que el padre dejó pendientes), lo que habría ensuciado la salida por terminal.

5. **Manejo de `SIGPIPE`**
   Agregamos `signal(SIGPIPE, SIG_IGN)` en el `main`. Esto lo hicimos como medida de seguridad por si un hijo sufre un error y cierra su pipe de lectura antes de que el padre termine de escribir los insumos. En lugar de que el programa sufra un *crash* entero, el error de escritura simplemente retorna `-1` y el planificador sigue funcionando aislando el error.
