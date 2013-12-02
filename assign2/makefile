CC = gcc

LIBS = -lresolv -lc -lnsl -lsocket -lpthread -lm\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a\
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I /home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: client server

server: server.o get_ifi_info_plus.o rtt.o
	${CC} -o server server.o get_ifi_info_plus.o rtt.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c


client: client.o get_ifi_info_plus.o
	${CC} -o client client.o get_ifi_info_plus.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c

rtt.o: rtt.c
	${CC} ${CFLAGS} -c rtt.c

clean:
	rm server.o client.o get_ifi_info_plus.o
