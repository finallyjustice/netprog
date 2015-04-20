CC = gcc
LIBS = -lpthread

all: http

http: http.o tpool.o
	${CC} http.o tpool.o -o http ${LIBS}

http.o: http.c
	${CC} -c http.c

tpool.o: tpool.c
	${CC} -c tpool.c

clean:
	rm *.o http
