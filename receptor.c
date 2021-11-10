//TEC 10
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include <time.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "booking.h" 

#define BUFFSIZE 1

//main functions
int saveDB();
edit queryEdit(query *q, int bs);
int readQuery(int ap, query *q);

//subfunctions
void* EditDB(void *arg);
int readBook(char *str, book *);
int readCopy(char *str, copy *c);
int queryLogic(query *q, copy *c);
void setDate(copy *c,int w); 
int params(int argc, char** argv);
int pipeInt(char *PipeName,Pipe *array );
int pipeOut(char *PipeName,Pipe *array );
void *put(edit *e);

//Global varibles
sem_t s, spaces, elements; // semaforos para la implementacion del buffer
pthread_t hilo;

char *thePipe; char * dbin;
edit BUFFER[BUFFSIZE];
book DB[MAXQUERIES];// Estrcutura de la base de datos
int pcons=0, pprod=0, nbook = 0, nPipes = 0;

int main (int argc, char** argv)
{
//CONTROL
  if(params(argc, argv) == -1){// ./e pipe db
    printf ("Bad request: Check out Documentation\n");
    printf ("./receptor -p pipeReceptor -f dbin\n");
    exit(1);
  } 
  sem_init(&s, 0, 1);
  sem_init(&spaces, 0, BUFFSIZE);
  sem_init(&elements, 0, 0);
  int tp,ap, pid,bytes; //THEPIPE = tp A PIPE = ap
  query queries[MAXQUERIES];//BUFFER
  Pipe pipes[MAXPIPES];//Buffer
  for(int i= 0; i<MAXPIPES; i++)pipes[i].number = -1;
  for(int i=0; i < BUFFSIZE; i++) BUFFER[i].bool = 0;
  nbook = saveDB();

//OPEN PIPE AND THREAD
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  unlink(thePipe);
  if (mkfifo (thePipe, fifo_mode) == -1) {
     perror("[mkfifo]");
     exit(1);
  }
  
  tp = open (thePipe, O_RDONLY);
  printf ("Pipe Opening\n");

  if(pthread_create(&hilo, NULL, &EditDB, NULL))
  {
      printf("\n ERROR creating thread");
  }

//READ AND RESPOND QUERIES
  // query counter and general status
  int qc= 0, status;
  edit Edit;
  do
  {
    readQuery(tp,&queries[qc]); 
    status = queries[qc].status;
    Edit = queryEdit(&queries[qc], nbook);
    if(Edit.bool != -1) put(&Edit);
    ap = pipeInt(queries[qc].pipe, pipes);
    if (ap == -1) {
      printf("Error: Se volvera a intentar despues\n");
      sleep(5);
    }
    if(status == 600){
      pipeOut(queries[qc].pipe, pipes);
    }
    // printf("PIPE INT: %d \n", ap);
    //Abrir pipe segun arreglo de pipes
    // printf ("Open pipe\n");
    write (ap, &queries[qc], sizeof(query));
    if(nPipes > MAXPIPES){
      printf("Number of pipes exceed\n");
      break;
    }
    qc++;
  } while (nPipes != 0);//end everything

  Edit.bool = -2; put(&Edit);

  if(pthread_join(hilo, NULL))	/* wait for the thread 1 to finish */
  {
      printf("ERROR joining thread\n");
  }

//CLOSE PIPE, LINKS THREATH , FREE MEMORY AND EXIT
    unlink(thePipe);
    free(thePipe);
    free(dbin);
    exit(0);
}

//returns the number of books or -1 if error
int saveDB(){
  // printf("Hello\n");
  size_t size = 0;
  int b = 0;
  char* sample;
  sample = (char *) malloc (size);
  FILE *fi;
  if((fi = fopen(dbin, "r"))== NULL){
    return -1;
  }
  while (!feof(fi) )  {
    getline(&sample,&size,fi);
    readBook(sample,&(DB[b]));
    for (int j = 0; j < DB[b].copies && !feof(fi) ; j++)
    {
      getline(&sample,&size,fi);
      readCopy(sample,&(DB[b].Copies[j]));
      DB[b].Copies[j].end = ftell(fi);
      DB[b].Copies[j].start = (DB[b].Copies[j].end)-strlen(sample);
    }
    b++;
  }
  fclose(fi);
  free(sample);
  return b;
}

//Search in DB if query could be made. Returs edit.bool = 1 if is found else -1
edit queryEdit(query *q, int bs){
  int correctBook = 0,correctCopy = 0 , founded = 0;//booleans
  size_t size = 0;
  char *token;
  token = (char *) malloc (size);
  copy c;
  edit Edit; Edit.bool = -1;
  sem_wait(&s);
  for (int i = 0; i < bs && !founded ; i++) {
    if(strcmp(DB[i].name,(q)->book) == 0   && DB[i].ISBN == (q)->ISBN){//comparar 2 strings
        correctBook = 1;
        printf("%s\t%d\t%d\n", DB[i].name,DB[i].ISBN,DB[i].copies);
    }else if (!correctBook){
      q->status = 404;
    }
    for (size_t j = 0; j < DB[i].copies && !founded ; j++)
    {
      memcpy(&c, &(DB[i].Copies[j]), sizeof(copy));
      if(correctBook){
        correctCopy = queryLogic(q,&c);
        if(correctCopy){
          founded = 1;
        }
      }
    }    
  }
  if(founded){
    sprintf(token, "%d,%c,%d-%d-%d\n",c.index,c.state,(c.date.day),(c.date.month),(c.date.year));
    // printf("%d\t%d\t%s\n",start,end,token);
    Edit.start = c.start;
    Edit.end = c.end;
    strcpy(Edit.token,token);
    // printf("[original]path: %s start: %d end: %d token: %s\n", dbin,start,end,Edit.token);
    Edit.bool = 1;
  }
  free(token);
  sem_post(&s);
  return Edit;
}

// converts a string into a book
int readBook(char *str, book *b){
  sscanf(str,"%[^,]s", b->name);
  sscanf(&str[strlen(b->name)],",%d,%d",&b->ISBN,&b->copies);
  return 0;
}

// converts a string into a copy
int readCopy(char *str, copy *c){
  sscanf(str, "%d,%c,%d-%d-%d",&c->index,&c->state,&(c->date.day),&(c->date.month),&(c->date.year)); 
  c->start = -1; c->end = -1;
  return 0; 
}

int readQuery(int tp, query *qv){
    int bytes;
    bytes = read (tp, qv, sizeof(query));
    printf("Type:%c\tName:%s\tISBN:%d\tStatus:%d\tPipeName:%s\n",  qv->type,qv->book,qv->ISBN,qv->status,qv->pipe);
    if (bytes == -1) {
      perror("[ReadQuery]");
      return bytes;
    }
    return 0;
}



//logic of the project and sets copy
int queryLogic(query *q, copy *c){
  if(q->type == 'P' && c->state == 'D'){  // P = Prestamo 
    c->state = 'P';
    setDate(c,0);
    // c->date
    q->status = 201;//Created
  }else
  if(q->type == 'R' && c->state == 'P'){ 
    q->status = 202;//Renewed 
    long nowQL = time(NULL);
    struct tm tmm = *localtime(&nowQL);
    if(
          c->date.year == tmm.tm_year + 1900 && 
          c->date.month == tmm.tm_mon + 1 &&
          c->date.day ==  tmm.tm_mday
      )
    {
      setDate(c,1);
    }
    else if(
          c->date.year >= (tmm.tm_year + 1900) && 
          c->date.month >= (tmm.tm_mon + 1) &&
          c->date.day >=  tmm.tm_mday
          )
    {
      struct tm rt = *localtime(&nowQL);
      rt.tm_year = c->date.year-1900 ; 
      rt.tm_mon = c->date.month - 1;
      rt.tm_mday = c->date.day;
      long renewQL = mktime(&rt)+7*24*3600;
      rt = *localtime(&renewQL);
      c->date.year = rt.tm_year + 1900;
      c->date.month = rt.tm_mon + 1;
      c->date.day =  rt.tm_mday;  
    }else{
      setDate(c,0);
    }
  }else
  if(q->type == 'D' && c->state == 'P'){
    c->state = 'D';
    q->status = 200;//Returned
    setDate(c,0);
  }else{
    q->status = 401;//No Match means fail
    return 0;
  }
  return 1;
}

//Puts structure in buffer
void *put(edit *e) {
      sem_wait(&spaces);
      sem_wait(&s);
      if (BUFFER[pprod].bool == 0) memcpy(&BUFFER[pprod], e, sizeof(edit));
      pprod = (pprod + 1) % BUFFSIZE;
      sem_post(&s);
      sem_post(&elements);
} 

// Edit a file and if correct returns 1 else 0
void* EditDB(void *arg){
  while(1){
      sem_wait(&elements);
      sem_wait(&s);
      edit Edit;
      memcpy(&Edit, &BUFFER[pcons], sizeof(edit));
      BUFFER[pcons].bool = 0; // para indicar que la posición está vacia
      pcons= (pcons + 1) % BUFFSIZE;
      if(Edit.bool == -2){
          // printf("[EditDB] last element\n");
          sem_post(&s);
          sem_post(&spaces);
          return NULL;
      }
      int start = Edit.start;
      int end = Edit.end;
      char* token = Edit.token;
      int fend = 0;
      // printf("[other]path: %s start: %d end: %d token: %s\n", dbin,start,end,token);
      FILE *f;
  //CONTROL
      if((f = fopen(dbin, "rb")) == NULL){
          printf("[EditDB] Error opening file\n");
          sem_post(&s);
          sem_post(&spaces);
          return NULL;
      }
      fseek(f, 0L, SEEK_END);
      fend = ftell(f);
      fseek(f, 0L, SEEK_SET);
      if( end > fend)
          end = fend;
      if(start > end || start < 0){
          printf("[EditDB] Incorrect params or/and overlaod\n");
          sem_post(&s);
          sem_post(&spaces);
          return NULL;
      }
      char a[start+1],b[fend-end+1];
  //COPY
      fread(a, start, 1, f);
      fseek(f, (long)end, SEEK_SET);
      fread(b, fend-end, 1, f);
      fclose(f);       
      if((f = fopen(dbin, "wb")) == NULL){
          printf("[EditDB] Error opening file\n");
          sem_post(&s);
          sem_post(&spaces);
          return NULL;
      }
      fwrite(a, start, 1, f);
      fwrite(token, strlen(token), 1, f);
      fwrite(b, fend-end, 1, f);  
      fclose(f);
      nbook = saveDB();
      sem_post(&s);
      sem_post(&spaces); 
  }
}

//sets the actual date plus w as weeks ahead.
void setDate(copy *c,int w){
    long now = time(NULL)+w*7*24*3600;
    struct tm tm = *localtime(&now);
    c->date.year = tm.tm_year + 1900;
    c->date.month = tm.tm_mon + 1;
    c->date.day =  tm.tm_mday;      
}

//Verifies and Saves argv in global varibles
int params(int argc, char** argv){
  int pipe, database;
  if(argc != 5 ){
    return -1;
  }
  for (size_t i = 1; i < 5; i+=2)
  {
    if(strlen(argv[i]) > 2 ){
      return -1;
    }
    if(strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-f") != 0  ){
      return -1;
    }
    if(strcmp(argv[i], "-p") == 0){
      pipe = i;
    }
    if(strcmp(argv[i], "-f") == 0){
      database = i;
    }    
  }
  
  thePipe = malloc(strlen(argv[pipe+1])+1); 
  strcpy(thePipe,argv[pipe+1]);
  dbin = malloc(strlen(argv[database+1])+1); 
  strcpy(dbin,argv[database+1]);
  return 0;
}

//return the pipe if exist else 0
int pipeInt(char *PipeName,Pipe *array ){
  int a = -1;
  for (size_t i = 0; i < MAXPIPES; i++)
  {
    if(array[i].number > -1){
      if(strcmp(array[i].id, PipeName) == 0){
        a = array[i].number;
      }
    }
  }
  if(a == -1){
    for (size_t i = 0; i < MAXPIPES; i++){
      if(array[i].number == -1){
        a = open(PipeName, O_WRONLY);
        if (a == -1) {
          perror("[pipeInt] ");
          break;      
        }
        strcpy(array[i].id, PipeName);
        array[i].number = a;
        nPipes++;
        printf("[pipeInt] Pipe Guardado Name:%s Number:%d\n",array[i].id,array[i].number);
        break;
      }
    }
  }
  return a;
}

//Gets the pipe out of the equation when it sents 600 status
// if fine returns 1 if not found 0
int pipeOut(char *PipeName,Pipe *array ){
  int a = 0;
  for (size_t i = 0; i < MAXPIPES; i++)
  {
    if(array[i].number > -1){
      if(strcmp(array[i].id, PipeName) == 0){
        array[i].number = -1;
        strcpy(array[i].id, "");
        a = 1;
        nPipes--;
        printf("[pipeOut] %s\n",array[i].id); 
      }
    }
  }
  return 0;
}