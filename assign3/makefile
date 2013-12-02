CC = gcc

LIBS = -lpthread\
	/home/users/cse533/Stevens/unpv13e/libunp.a\
	
FLAGS = -g -O2

CFLAGS = -I /home/users/cse533/Stevens/unpv13e/lib

all: client_dozhang server_dozhang odr_dozhang

server_dozhang: server.o
	${CC} -o server_dozhang server.o  ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c


client_dozhang: client.o get_hw_addrs.o
	${CC} -o client_dozhang client.o get_hw_addrs.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c

odr_dozhang: odr.o get_hw_addrs.o
	${CC} -o odr_dozhang odr.o get_hw_addrs.o ${LIBS}
odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm server.o client.o odr.o client_dozhang server_dozhang odr_dozhang get_hw_addrs.o
