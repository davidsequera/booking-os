//Autor Mariela Curiel
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "empleados.h"

void EnviarPipe(emple *miemp, int cuantos,char pipe[]) {

  int i, creado=0, fd;
  
 do {
     fd = open(pipe, O_WRONLY|O_NONBLOCK);
     if (fd == -1) {
         perror("pipe");
         printf(" Se volvera a intentar despues\n");
         sleep(5);
     } else creado = 1;
  } while (creado == 0);

  
 for (i= 0; i < cuantos; i++) {
   write(fd, &miemp[i], sizeof(emple));
   sleep(1); // este sleep es unicamente para que se entremezclen las peticiones de varios buscadores
   printf (" se manda  %d %f %s \n", miemp[i].edad, miemp[i].salario, miemp[i].nombre);
 }
 close(fd);
}

/* asignar
   inserta un nuevo empleado en la lista en la posicion pos
   e inicializa su edad, tiempo de trabajo y nombre
*/
void asignar(emple *emp, char *nombre, int edad, float salary, int pos)
{
     
  char *p;
   
  emp[pos].edad = edad;
  emp[pos].salario = salary;
  strcpy(emp[pos].nombre, nombre);
 
}

/* imprimir
   imprime los primeros cuantos empleados del arreglo
*/
void  imprimir(emple *emp, int cuantos)
{
  int i;

    printf("Empleados:\n");

  for(i=0; i < cuantos; i++)
    printf("%s %d %f\n",  emp[i].nombre, emp[i].edad, emp[i].salario);
}


int main(int argc, char *argv[]) 
{
  emple miemp[MAXEMP];
  char line[MAXLIN], nom[MAXNOMBRE];
  FILE *fp;
  int i, edad=0;
  float salario;
  
  fp = fopen(argv[1], "r");
  i = 0;

  while (!feof(fp))  {
    fscanf(fp, "%s %d %f\n", nom,&edad, &salario);
    asignar(miemp, nom, edad, salario, i++);
  }
  
  imprimir(miemp, i); // i tiene el total de empleados leidos
  // Enviar por el pipe

  EnviarPipe(miemp,i,argv[2]);

     
  fclose(fp); 
}
