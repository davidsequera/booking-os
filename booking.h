// Conexion Structures

#define MAXQUERIES 100000
#define MAXPIPES 30
#define MAXPIPENAME 20
#define MAXNAME 39
#define MAXCOPIES 500
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
} book;
typedef struct net {
  char type;
  char book[MAXNAME];
  int ISBN;
  int status;
  char pipe[MAXPIPENAME];
} query;
typedef struct pipe{
  int number;
  char name[MAXPIPENAME];
}Pipe;
typedef struct fileEditArg{
    int edit;
    char path[MAXNAME];
    int start;
    int end;
    char token[MAXNAME];
} editArg;