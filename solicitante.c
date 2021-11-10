//TEC 10
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
#include "booking.h" 

query createQuery(char type,  char book[MAXNAME],  int ISBN,int status, char* pipename );
//funcion de debbuging no usada

int readQueries(char* path, query *qv, char* PipeName);
int menu(query *qv,char* PipeName);
char* genPipeName();
int params(int argc, char** argv);


//Global varibles
char *thePipe; char * queriesFile;


int main (int argc, char **argv)
{
//CONTROL
    int iparam = params(argc,argv);
     if(iparam == -1){//./e queries pipe 
         printf ("Bad request: Check Documentation\n");
         printf ("Example: ./solicitante -i q1.txt -p pipeReceptor\n");
         exit(1);
     }

  int tp,ap, pid, n, bytes,nqueries;
  srand(time(NULL));
  mode_t fifo_mode = S_IRUSR | S_IWUSR;



//READ AND CREATE QUERY

    char *PipeName = genPipeName();
    //printf("Type: %c\tName: %s\tISBN: %d\tStatus: %d\tPipeName: %s\n",  q.type,q.book,q.ISBN,q.status,q.pipe);
    query queries[MAXQUERIES], response;
    nqueries= iparam? menu(queries, PipeName):readQueries(queriesFile, queries, PipeName);


//SENT QUERY
    tp = open(thePipe, O_WRONLY);
     if (tp == -1) {
        perror(thePipe);
        printf(" Se volvera a intentar despues\n");
	    sleep(5);        
    }
    printf ("Open ThePipe\n");

    unlink(PipeName);
    if (mkfifo (PipeName, fifo_mode) == -1) {
        perror("mkfifo");
        exit(1);
    }
    for (size_t i = 0; i < nqueries; i++){
        write (tp, &queries[i], sizeof(query));
        sleep(5);
    }
    ap = open (PipeName, O_RDONLY);
    if (ap == -1) {
        perror(PipeName);
        printf(" Se volvera a intentar despues\n");
	    sleep(5);        
    }
    printf ("Open a Pipe\n");

//WAIT RESPONSE

    for (size_t i = 0; i < nqueries; i++)
    {
        bytes = read (ap, &response, sizeof(query));
        if (bytes == -1) {
            perror("proceso lector:");
            exit(1);
        }
        printf("Type: %c\tName: %s\tISBN: %d\tStatus: %d\tPipeName: %s\n",  response.type,response.book,response.ISBN,response.status,response.pipe);
        if(response.status&200 == 200){
            printf("Success\n");
        }
    }
    unlink(PipeName);
    free(PipeName);
    free(thePipe);
    free(queriesFile);
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

int readQueries(char* path, query *qv, char* PipeName){
    FILE *f;
    int qc=0;
    size_t size=0;
    char *line = (char *) malloc (size);
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
            strcpy(qv[qc].pipe ,PipeName);
            // printf("Type: %c\tLibro: %s\tISBN: %d\t PIPEID:%s\n",qv[qc].type,qv[qc].book,qv[qc].ISBN,qv[qc].pipe);
            qc++;      
    }

    // Petición adicional para terminar 
    qv[qc] = createQuery('S',  PipeName, 0, 600,PipeName);
    qc++;
    //
    free(line);
    return qc;
}
// Error -1, 5 0, 3 1;
int params(int argc, char** argv){
  int pipe, queries;
  if(argc != 5 && argc != 3){
    return -1;
  }
  if(argc == 5){
    for (size_t i = 1; i < 5; i+=2)
    {
        if(strlen(argv[i]) > 2 ){
        return -1;
        }
        if(strcmp(argv[i], "-i") != 0 && strcmp(argv[i], "-p") != 0  ){
        return -1;
        }
        if(strcmp(argv[i], "-i") == 0){
        queries = i;
        }
        if(strcmp(argv[i], "-p") == 0){
        pipe = i;
        }    
    }
    queriesFile = malloc(strlen(argv[queries+1])+1); 
    strcpy(queriesFile,argv[queries+1]);
    thePipe = malloc(strlen(argv[pipe+1])+1); 
    strcpy(thePipe,argv[pipe+1]);
    return 0;
  }else{
    if(strcmp(argv[1], "-p") != 0){
        return -1;
    }else{
        queriesFile = malloc(1); 
        thePipe = malloc(strlen(argv[2])+1); 
        strcpy(thePipe,argv[2]);
        return 1;
    }
  }

}

int menu(query *qv,char* PipeName){
    int qc = 0, more = 0;
    int status = 100;
    char type, temp;//temp to clean buffer
    char book[MAXNAME];
    int ISBN;
    printf("Welcome, please complete this menu, in orther to create the queires \n");

    do
    {
        printf("Please Enter Type: ");
        scanf("%c",&type);
        printf("Please Enter Book: ");
        fflush( stdin );
        scanf("%c",&temp); // temp statement to clear buffer
        scanf ("%[^\n]", book);
        printf("Please Enter ISBN: ");
        scanf("%i",&ISBN);
        printf("%c/%s/%d/%d/%s\n",type,  book, ISBN, status,PipeName);
        qv[qc] = createQuery(type,  book, ISBN, status,PipeName);
        qc++;
        if(!(qc < (MAXQUERIES-1))){
            printf("Max.Number of queries created\n");
        }else{
            printf("Do you wanna create more?");
            scanf("%i",&more);
        }
    }while (more && qc < (MAXQUERIES-1));
    qv[qc] = createQuery('S',  PipeName, 0, 600,PipeName);
    qc++;
   return qc;
}