//David Sequera
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "conexion.h" 

query createQuery(char type,  char book[MAXNAME],  int ISBN,int status, char* pipename );


int main (int argc, char **argv)
{
//CONTROL

  int tp,ap, pid, n, bytes;
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
    // if(argc != 3){//./e pipe db
    //     printf ("Bad request: Check Documentation\n");
    //     exit(1);
    // }

//MENU
// informacion quemada
//CREATE QUERY
    query q = createQuery('S',"Love Child",6829,100,"PipePrueba"), response;
    printf("Type: %c\tName: %s\tISBN: %d\tStatus: %d\tPipeName: %s\n",  q.type,q.book,q.ISBN,q.status,q.pipe);

//SENT QUERY
    tp = open("ThePipe", O_WRONLY);
     if (tp == -1) {
        perror("ThePipe");
        printf(" Se volvera a intentar despues\n");
	      sleep(5);        
      }
    printf ("Open ThePipe\n");
    write (tp, &q, sizeof(query));

//WAIT RESPONSE

    unlink(q.pipe);
    if (mkfifo (q.pipe, fifo_mode) == -1) {
        perror("mkfifo");
        exit(1);
    }
    ap = open (q.pipe, O_RDONLY);
    printf ("Open a Pipe\n");

    bytes = read (ap, &response, sizeof(query));
    if (bytes == -1) {
      perror("proceso lector:");
      exit(1);
    }
    if(response.status == 200){
        printf("Success\n");
        exit(0);
    }else{
      exit(1);
    }
    unlink("PipePrueba");
    exit(0);
}


query createQuery(char type,  char book[MAXNAME],  int ISBN,int status, char* pipename ){
  query q;
  q.type = type;
  strcpy(q.book,book);
  q.ISBN = ISBN;
  strcpy(q.pipe,pipename);
  q.status = status;
  return q;
}



int genPipeName(char *a){
    //rangos ASCII 0-1 (48-57)10 a-z(97-122)26 A-Z(65-90)26
    const int w = 11, z = 20;
    int n;
    char s[w];//random first
    char e[z];//finale random
    char* t = itoa(time(NULL));//time
    for (size_t i = 0; i < w; i++)
    {
        int a = rand() % 3;
        if(a==0){
            n = rand() % 10 + 48;
        }else if(a==1){
            n = rand() % 26 + 97;
        }else{
            n = rand() % 26 + 65;
        }
        *(s+i)= i==(w-1)? (char)0 : (char)n;
    }
    for (size_t i = 0; i < z; i++)
    {
        if(i%2 == 0){
            *(e+i)= *(s+(i/2)) ;
        }else{
            *(e+i)= i==(z-1)? (char)0 :t[((i-1)/2)] ;
        }
    }
    strcat(a,s);
    strcat(a,z);
    strcat(a,t);
    return 0;
}
