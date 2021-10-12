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
#include "booking.h" 

//main functions
int queryDB(query *);
int readQuery(int ap, query *q);

//subfunctions
int fileEdit(char* path,int start,int end, char* token);
int readBook(char *str, book *);
int readCopy(char *str, copy *c);
int queryLogic(query *q, copy *c);
void setDate(copy *c,int w); 

//Global varibles
int paramc; char *thePipe; char * dbin;

int main (int argc, char** argv)
{
//CONTROL
  if(argc != 3){// ./e pipe db
    printf ("Bad request: Check out Documentation\n");
    exit(1);
  } 

  int tp,ap, pid,bytes; //THEPIPE = tp A PIPE = ap
  query queries[MAXQUERIES];//BUFFER
  thePipe = malloc(strlen(argv[1])+1); 
  strcpy(thePipe,argv[1]);
   dbin = malloc(strlen(argv[2])+1); 
  strcpy(dbin,argv[2]);




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
    queryDB(&queries[qc]);
    ap = open(queries[qc].pipe, O_WRONLY);
    if (ap == -1) {
      perror("A Pipe");
      printf(" Se volvera a intentar despues\n");
      sleep(5);        
    }
    // printf ("Open pipe\n");
    write (ap, &queries[qc], sizeof(query));
    qc++;
  } while (status != 600);//end everything
  

//CLOSE PIPE AND EXIT
    unlink(thePipe);
    free(thePipe);
    free(dbin);
    exit(0);
}


int queryDB(query *q){
  int start = -1, end = -1;//query done!
  int correctBook = 0,correctCopy = 0 , founded = 0, dbWrite = 0;//booleans
  book b;
  copy c;
  size_t size = 0;
  char* sample, *outCopy;
  FILE *fi;

  fi = fopen(dbin, "r");//recuerda cambiar el argv
  sample = (char *) malloc (size);
  outCopy = (char *) malloc (size);
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
  if(founded){
    sprintf(outCopy, "%d,%c,%d-%d-%d\n",c.index,c.state,(c.date.day),(c.date.month),(c.date.year));
    printf("%d\t%d\t%s\n",start,end,outCopy);
    dbWrite = fileEdit(dbin,start,end,outCopy);
    if(!dbWrite){
      q->status = 500;//Internal Server Error
    }
  }
  free(sample);
  free(outCopy);
  return 0 ;
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
    setDate(c,1);

  }else
  if(q->type == 'D' && c->state == 'P'){
    c->state = 'D';
    q->status = 202;//Returned
    setDate(c,0);
  }else{
    q->status = 401;//No Match means fail
    return 0;
  }
  return 1;
}

// Edit a file and if correct returns 1 else 0
int fileEdit(char* path,int start,int end, char* token){
    int fend = 0;
    FILE *f;//CONTROL
    if((f = fopen(path, "rb")) == NULL){
        printf("[fileEdit] Error opening file\n");
        return -1;
    }
    fseek(f, 0L, SEEK_END);
    fend = ftell(f);
    fseek(f, 0L, SEEK_SET);

    if( end > fend)
        end = fend;
    if(start > end || start < 0){
        printf("[fileEdit] Incorrect params or/and overlaod\n");
        return -1;
    }
    char a[start+1],b[fend-end+1];
//COPY
    fread(a, start, 1, f);
    fseek(f, (long)end, SEEK_SET);
    fread(b, fend-end, 1, f);
    fclose(f);

           
    if((f = fopen(path, "wb")) == NULL){
        printf("[fileEdit] Error opening file\n");
        return -1;
    }
    fwrite(a, start, 1, f);
    fwrite(token, strlen(token), 1, f);
    fwrite(b, fend-end, 1, f);    
    fclose(f);
    return 1;
}

//sets the actual date plus w as weeks ahead.
void setDate(copy *c,int w){
    long now = time(NULL)+w*7*24*3600;
    struct tm tm = *localtime(&now);
    c->date.year = tm.tm_year + 1900;
    c->date.month = tm.tm_mon + 1;
    c->date.day =  tm.tm_mday;       
}