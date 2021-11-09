all: buscador central

central: central.o empleados.h
	gcc -o central central.o -lpthread

central.o: central.c empleados.h
	gcc -c central.c -lpthread

buscador: buscador.o empleados.h
	gcc -o buscador buscador.o

buscador.o: buscador.c empleados.h
	gcc -c buscador.c
