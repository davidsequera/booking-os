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

//main functions
editArg queryDB(query *);
int readQuery(int ap, query *q);

//subfunctions
void* fileEdit(void *arg);
int readBook(char *str, book *);
int readCopy(char *str, copy *c);
int queryLogic(query *q, copy *c);
void setDate(copy *c,int w); 
int params(int argc, char** argv);
int pipeInt(char *PipeName,Pipe *array );
//Global varibles
char *thePipe; char * dbin;
sem_t sem;
pthread_t hilo;

int main (int argc, char** argv)
{
//CONTROL
  if(params(argc, argv) == -1){// ./e pipe db
    printf ("Bad request: Check out Documentation\n");
    printf ("./receptor -p pipeReceptor -f dbin\n");
    exit(1);
  } 
  sem_init(&sem, 0, 1);
  int tp,ap, pid,bytes; //THEPIPE = tp A PIPE = ap
  query queries[MAXQUERIES];//BUFFER
  Pipe pipes[MAXPIPES];//Buffer
    for(int i= 0; i<MAXPIPES; i++)
      pipes[i].number = -1;
//OPEN PIPE
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  unlink(thePipe);
  if (mkfifo (thePipe, fifo_mode) == -1) {
     perror("[mkfifo]");
     exit(1);
  }
  
  tp = open (thePipe, O_RDONLY);
  printf ("Pipe Opening\n");

//READ AND RESPOND QUERIES
  // query counter and general status
  int qc= 0, status;
  
  do
  {
    readQuery(tp,&queries[qc]); 
    status = queries[qc].status;
    editArg fileArg = queryDB(&queries[qc]);
    if(pthread_create(&hilo, NULL, &fileEdit, &fileArg))
    {
      printf("\n ERROR creating thread");
    }
    ap = pipeInt(queries[qc].pipe, pipes);
    if (ap == -1) {
      printf(" Error: Se volvera a intentar despues\n");
      sleep(5);        
    }
    // printf("PIPE INT: %d \n", ap);
    //Abrir pipe segun arreglo de pipes
    // printf ("Open pipe\n");
    write (ap, &queries[qc], sizeof(query));
    if(pthread_join(hilo, NULL))	/* wait for the thread 1 to finish */
    {
      printf("ERROR joining thread\n");
    }
    qc++;
  } while (status != 600);//end everything
  

//CLOSE PIPE, LINKS THREATH , FREE MEMORY AND EXIT
    unlink(thePipe);
    free(thePipe);
    free(dbin);
    exit(0);
}


editArg queryDB(query *q){
  int start = -1, end = -1;//query done!
  int correctBook = 0,correctCopy = 0 , founded = 0;//booleans
  book b;
  copy c;
  size_t size = 0;
  char* sample, *outCopy;
  editArg fileArg;
    fileArg.edit = 0;
  FILE *fi;

  fi = fopen(dbin, "r");
  sample = (char *) malloc (size);
  outCopy = (char *) malloc (size);
  sem_wait(&sem);
  while (!feof(fi) && !founded)  {
    getline(&sample,&size,fi);
    readBook(sample,&b);
    if(strcmp(b.name,(q)->book) == 0   && b.ISBN == (q)->ISBN){//comparar 2 strings
        correctBook = 1;
        printf("%s\t%d\t%d\n", b.name,b.ISBN,b.copies);
    }else if (!correctBook){
      q->status = 404;
    }
    for (size_t j = 0; j < b.copies && !feof(fi) && !founded ; j++)
    {
      getline(&sample,&size,fi);
      readCopy(sample,&c);
      if(correctBook){
        correctCopy = queryLogic(q,&c);
        if(correctCopy){
          end = ftell(fi);
          start = end-strlen(sample);
          founded = 1;
        }
      }
    }    
  }
  fclose(fi);
  sem_post(&sem);
  if(founded){
    sprintf(outCopy, "%d,%c,%d-%d-%d\n",c.index,c.state,(c.date.day),(c.date.month),(c.date.year));
    // printf("%d\t%d\t%s\n",start,end,outCopy);
    sem_wait(&sem);
    strcpy(fileArg.path,dbin);
    fileArg.start = start;
    fileArg.end = end;
    strcpy(fileArg.token,outCopy);
    // printf("[original]path: %s start: %d end: %d token: %s\n", dbin,start,end,fileArg.token);
    fileArg.edit = 1;
    sem_post(&sem);
  }
  free(sample);
  free(outCopy);
  return fileArg;
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

// Edit a file and if correct returns 1 else 0
void* fileEdit(void *arg){
    sem_wait(&sem);

    editArg fileArg = *(editArg *)arg;

    if(!fileArg.edit){
        // printf("[fileEdit] Error not editable\n");
        sem_post(&sem);
        return arg;
    }

    char* path= fileArg.path;
    int start= fileArg.start;
    int end= fileArg.end;
    char* token = fileArg.token;
    // printf("[other]path: %s start: %d end: %d token: %s\n", dbin,start,end,((editArg *)arg)->token);
    int fend = 0;
    FILE *f;//CONTROL

    if((f = fopen(path, "rb")) == NULL){
        printf("[fileEdit] Error opening file\n");
        sem_post(&sem);
        return arg;
    }
    fseek(f, 0L, SEEK_END);
    fend = ftell(f);
    fseek(f, 0L, SEEK_SET);

    if( end > fend)
        end = fend;
    if(start > end || start < 0){
        printf("[fileEdit] Incorrect params or/and overlaod\n");
        sem_post(&sem);
        return arg;
    }
    char a[start+1],b[fend-end+1];
//COPY
    fread(a, start, 1, f);
    fseek(f, (long)end, SEEK_SET);
    fread(b, fend-end, 1, f);
    fclose(f);       
    if((f = fopen(path, "wb")) == NULL){
        printf("[fileEdit] Error opening file\n");
        sem_post(&sem);
        return arg;
    }
    fwrite(a, start, 1, f);
    fwrite(token, strlen(token), 1, f);
    fwrite(b, fend-end, 1, f);    
    fclose(f);
    sem_post(&sem);
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
      if(strcmp(array[i].name, PipeName) == 0){
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
        strcpy(array[i].name, PipeName);
        array[i].number = a;
        printf("Pipe Guardado Name:%s Number:%d\n",array[i].name,array[i].number);
        break;
      }
    }
  }
  return a;
}