/* archivo empleados.h */
/* Mariela Curiel 
 */

#define MAXEMP 100000
#define MAXLIN 80
#define MAXNOMBRE 20
#define SELMINTT 5
#define SELMINEDAD 34
typedef struct reg {
  char nombre[MAXNOMBRE];
  int edad;
  float salario;

} emple;
