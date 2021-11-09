// Autor: Mariela Curiel
// Descripcion: ejemplo de un productor consumidor, el proceso central 
// va leyendo los registros del pipe y los coloca en un buffer. Un
// hilo va sacando los elementos del buffer y los imprime. 
// en el proyecto,  el hilo debería escribir en la copia de la BD que está 
// en la memoria las actualizaciones correspondientes a las operaciones de renovacion o devolucion. 

 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "empleados.h" 

// El tamaño del BUFFER debe ser una constante. La idea es poder modificar ese tamaño el dia de la 
// sustentacion

#define TAMBUF 5

//Variables globales para la implementación del buffer

emple BUFFER[TAMBUF]; // buffer donde se pondrá la informacion
int pcons=0, pprod=0;
sem_t s, espacios, elementos; // semaforos para la implementacion del buffer


void *take(emple *);
void *put(emple *);

// Funcion para tomar datos del buffer
void *take(emple *e) {

  emple temp,*pe;

  pe = e; //pe ahora apunta al buffer
  int i=0;

  for (;;) {
    
    sem_wait(&elementos);
    sem_wait(&s);
    memcpy(&temp, &pe[pcons], sizeof(emple));
    pe[pcons].edad = 0; // para indicar que la posición está vacia
    pcons= (pcons + 1) % TAMBUF;
    if (temp.edad == -1) { // el ultimo elemento
      sem_post(&s);
      sem_post(&espacios);
      break;
    } else {
      // Se imprime el elemento que se toma del buffer
      printf (" Thread leyo %d %f %s\n", temp.edad, temp.salario, temp.nombre);
      sem_post(&s);
      sem_post(&espacios);
    }  
    
  }
  printf("thread consumidor termina\n");
  pthread_exit(NULL);
  

}  

// Funcion para colocar elementos del buffer. 
void *put(emple *e) {
      sem_wait(&espacios);
      sem_wait(&s);
      if (BUFFER[pprod].edad == 0) 
	memcpy(&BUFFER[pprod], e, sizeof(emple));
      pprod = (pprod + 1) % TAMBUF;
      sem_post(&s);
      sem_post(&elementos);
}  


int main (int argc, char **argv)
{
  int fd, pid, n, bytes, cuantos, creado,i;
  emple em;
  pthread_t thread1;
 
   
  mode_t fifo_mode = S_IRUSR | S_IWUSR;

  // inicializacion de los semáforos siguiendo el algoritmo productor, consumidor
  sem_init(&s, 0, 1);
  sem_init(&espacios, 0, TAMBUF);
  sem_init(&elementos, 0, 0);
  
  // inicializacion del buffer (se coloca en 0 la edad para indicar que la posicion está vacia
  
  for (i=0; i < TAMBUF; i++) BUFFER[i].edad = 0;
  // Creación del hilo consumidor

  pthread_create(&thread1, NULL, (void*) take, (void*)BUFFER);
 
  // Creacion del pipe para recibir los elementos
  if (mkfifo (argv[1], fifo_mode) == -1) {
     perror("mkfifo");
     exit(1);
  }

     fd = open (argv[1], O_RDONLY);
     if (fd == -1) {
         perror("pipe");
         exit (0);
      }

     // Leer datos e irlos colocando en un buffer
  
  do {
    cuantos = read (fd, &em, sizeof(emple)); // recordar validar llamadas al sistema
    if (cuantos == 0) break;
    //printf (" El central leyó  %d %f %s, cuantos = %d \n", em.edad, em.salario, em.nombre, cuantos);
    put(&em);
  } while (cuantos > 0);
 
  // Se acabaron los datos, se coloca el ultimo elemento en el buffer

  em.edad = -1;
  put(&em);

  // Cerrar y eliminar el pipe
  
  close(fd);
  unlink(argv[1]);
}
