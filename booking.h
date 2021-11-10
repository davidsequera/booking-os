// Conexion Structures

#define MAXCOPIES 50
#define MAXQUERIES 100
#define MAXPIPES 30
#define MAXNAME 39
#define MAXTHREADS 20

typedef struct zeit {
  int day;
  int month;
  int year;
} Date;
typedef struct cop {
  int start;
  int end;
  int index;
  char state;
  Date date;
} copy;
typedef struct reg {
  char name[MAXNAME];
  int ISBN;
  int copies;
  copy Copies[MAXCOPIES];
} book;
typedef struct net {
  char type;
  char book[MAXNAME];
  int ISBN;
  int status;
  char pipe[MAXNAME];
} query;
typedef struct pipe{
  char id[MAXNAME];
  int number;
  int queries;
  query Queries[MAXQUERIES];
}Pipe;
typedef struct fileEditArg{
    int bool;
    int start;
    int end;
    char token[MAXNAME];
} edit;