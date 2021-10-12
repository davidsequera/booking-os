// Conexion Structures

#define MAXQUERIES 100000
#define MAXPIPE 30
#define MAXNAME 39
#define MAXCOPIES 500

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
  char pipe[MAXPIPE];
} query;