//David Sequera
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "conexion.h" 

query createQuery(char type,  char book[MAXNAME],  int ISBN,int status, char* pipename );
int readQueries(char* path, query *qv);



int main (int argc, char **argv)
{
//CONTROL

  int tp,ap, pid, n, bytes,nqueries;
  srand(time(NULL));
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
    // if(argc != 3){//./e pipe db
    //     printf ("Bad request: Check Documentation\n");
    //     exit(1);
    // }

//MENU
// informacion quemada
//CREATE QUERY

    //query q = createQuery('S',"Love Child",6829,100,"PipePrueba"), 
    query response;
    //printf("Type: %c\tName: %s\tISBN: %d\tStatus: %d\tPipeName: %s\n",  q.type,q.book,q.ISBN,q.status,q.pipe);
    query queries[MAXQUERIES];
    nqueries= readQueries("queries.txt", queries);
    query q = *queries;


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


char* genPipeName(){
    //rangos ASCII 0-1 (48-57)10 a-z(97-122)26 A-Z(65-90)26
    const int w = 11, z = 20;
    int n;
    char s[w];//random first
    char e[z];//finale random
    char t[w];//time
    sprintf(t, "%d", (int)time(NULL));
    // strcpy(t,itoa((int)time(NULL)));
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
    char *b = malloc(z);
    for (size_t i = 0; i < z; i++)
        b[i] = e[i];
    return (char *) b;
}

int readQueries(char* path, query *qv){
    FILE *f;
    int qc=0;
    size_t size=100;
    char *line;

     if((f = fopen(path, "r")) == NULL){
        printf("[fileEdit] Error opening file\n");
        return -1;
    }
    
    while (!feof(f))  {//!feof(tp)
            getline(&line,&size,f);    // address of the first character position,addres of the varible that holds the size,input file 
            sscanf(line,"%c", &(qv[qc].type)); // 
            sscanf(&line[2],"%[^,]s", qv[qc].book);
            sscanf(&line[strlen(qv[qc].book)+2],",%d",&(qv[qc].ISBN));
            qv[qc].status = 100; // Continuar
            strcpy(qv[qc].pipe ,genPipeName());
            printf("Type:%c\tLIRBO:%s\tISBN:%d\t PIPEID:%s\n",qv[qc].type,qv[qc].book,qv[qc].ISBN,qv[qc].pipe);
            qc++;      
    }
    return qc;
}

