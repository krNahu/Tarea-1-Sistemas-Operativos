# Tarea-1-Sistemas-Operativos

Para la realización de la Tarea 1 de sistemas operativos se construirá un programa que simula un planificador de tareas, este lee un archvo de texto con un Grafo Acíclico (DAG) qye kabza oricesis concurrentes usando "fork()" respetando el límite establecido "K", de esa manera maneja la comunicación entre ellos mediante pipes de POSIX.

## Integrantes

* Jessica Aguilera
* Cristóbal Araya

## Complicación y Ejecución

El código se realizó en C++ y las normas de compliación fueron seguidas estrictamente

Para compilar:

```bash
g++ -Wall -Wextra -std=c++17 -o planificador planificador.cpp -lpthread
```

Y para ejecutar se debe proporcionar el archivo de texto y el límite de concurrencia "K", tal como se observa en la siguiente línea:

```bash
./planificador plan.txt 4
```

## Funciones implementadas

*   `leerPlan(string archivo)`: Esta función lee el archivo línea por línea, hace el split por ":" y maneja casos borde (como asignar un tiempo aleatorio si viene vacío). Además, arma la estructura del DAG guardado cuántas dependencias tiene cada tarea para saber cuándo están listas.

*   `lanzar(int i)`: Se encartga de instanciar los pipes, entrada y salida, y hacer el "fork()". Maneja qué descriptores de archivo (FDs) se cierran en el padre y cuáles en el hijo.

*   `codigoHijo(int i, int fdEntrada, int fdSalida)`: Es el ciclo de vida del proceso hijo. Lee su pipe para contar cuántos insumos (`\n`) recibió, hace un sleep para simular el trabajo, y luego escribe su resultado en el pipe de salida antes de morir.

*   `abortarRama(int origen)`: Recorre el grafo usando  un recorrido en profundidad (DFS con una pila) para marcar como abortadas en cascada a todas las tareas que dependían de un proceso que acaba de fallar.

*   `abortarTodo()`: Cuando llega un SIGINT (Ctrl + C), el aviso de seremi, recorre la lista de procesos vivos, les manda un SIGKILL, hace un waitpid para limpiar los zombies, y cierra el programa.
