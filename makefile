all: receptor solicitante clean

receptor: receptor.o booking.h
	gcc -o receptor receptor.o

receptor.o: receptor.c booking.h
	gcc -c receptor.c

solicitante: solicitante.o booking.h
	gcc -o solicitante solicitante.o

solicitante.o: solicitante.c booking.h
	gcc -c solicitante.c

clean:
	rm -f *.o *~ 
