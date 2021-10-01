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

//main functions
int queriesManager(int qc,query []);
int readQueries(int ap, query []);

//subfunctions
int fileEdit(char* path,int start,int end, char* token);
int readBook(char *str, book *);
int resolveDB(query *q, copy *c);
int readCopy(char *str, copy *c);


int main (int argc, char **argv)
{
//CONTROL
    if(argc != 3){//./e pipe db
        printf ("Bad request: Check Documentation\n");
        exit(1);
    }

  int tp,ap, pid,bytes; //THEPIPE = tp A PIPE = ap
  query queries[MAXQUERIES];//BUFFER


//OPEN PIPE
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  
  unlink("ThePipe");
  if (mkfifo ("ThePipe", fifo_mode) == -1) {
     perror("[mkfifo]");
     exit(1);
  }
  
  tp = open ("ThePipe", O_RDONLY);
  printf ("Pipe Opening\n");

//READ QUERIES
    // query counter
    int qc= readQueries(tp,queries);
  //queryManager
    queriesManager(qc, queries);

    /* toca usar un hilo  */
//RESPOND QUERIES
    queries[0].status = 200;
    for (size_t i = 0; i < qc; i++)
    {
      ap = open(queries[i].pipe, O_WRONLY);
      if (ap == -1) {
        perror("A Pipe");
        printf(" Se volvera a intentar despues\n");
	      sleep(5);        
      }
      printf ("Open Apipe\n");
      write (ap, &queries[i], sizeof(query));
    }
    unlink("ThePipe");
    exit(0);
}


int queriesManager(int qc,query *qv){
  int qd = 0;//query done!
  int founded = 0;
  book b;
  copy c;
  size_t size = 100;
  char* sample;
  FILE *fi, *fo;
  fi = fopen("dbin", "r");//recuerda cambiar el argv
  sample = (char *) malloc (size);
  while (!feof(fi))  {
    getline(&sample,&size,fi);
    readBook(sample,&b);
    for (size_t v = 0; v < qc; v++)
    {
      if(strcmp(b.name,(qv+v)->book) == 0   && b.ISBN == (qv+v)->ISBN){//comparar 2 strings
        founded = 1;
        printf("%s\t%d\t%d", b.name,b.ISBN,b.copies);
      }
    }
    for (size_t j = 0; j < b.copies && !feof(fi) ; j++)
    {
      getline(&sample,&size,fi);
      readCopy(sample,&c);
      if(founded){
        printf("%d,%c,%d-%d-%d\n",c.index,c.state,(c.date.day),(c.date.month),(c.date.year));
        printf("Lo logramos\n");
        founded = 0;
      }
    }    
    qd++;
  }
  fclose(fi);
  return 0 ;
}

int readQueries(int tp, query *qv){
    int qc= 0, bytes;
    while (qc != 1)  {//!feof(tp)
        bytes = read (tp, &qv[qc], sizeof(query));
        printf("Type:%c\tName:%s\tISBN:%d\tStatus:%d\tPipeName:%s\n",  qv[qc].type,qv[qc].book,qv[qc].ISBN,qv[qc].status,qv[qc].pipe);
        qc++;
        if (bytes == -1) {
            perror("[Reading Queries]");
            exit(1);
        }
    }
    return qc;
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

//WRITE
    if((f = fopen(path, "wb")) == NULL){
        printf("[fileEdit] Error opening file\n");
        return -1;
    }
    fwrite(a, start, 1, f);
    fwrite(token, strlen(token), 1, f);
    fwrite(b, fend-end, 1, f);    
    fclose(f);
    return 0;
}


int resolveDB(query *q, copy *c){
  if(q->type == 'S' && c->state == 'D'){
    c->state = 'P';
    // c->date
    q->status = 201;
  }else
  if(q->type == 'R' && c->state == 'P'){
    q->status = 202;
  }else
  if(q->type == 'D' && c->state == 'P'){
    c->state = 'D';
    q->status = 202;
  }else{
    q->status = 404;
    return -1;
  }
  return 0;
}
