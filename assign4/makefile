CC = gcc

LIBS = -lpthread\
	/home/users/cse533/Stevens/unpv13e/libunp.a\
	
FLAGS = -g -O2

CFLAGS = -I /home/users/cse533/Stevens/unpv13e/lib

all: tour_dozhang arp_dozhang

tour_dozhang: tour.o get_hw_addrs.o
	${CC} -o tour_dozhang tour.o  get_hw_addrs.o ${LIBS}
tour.o: tour.c
	${CC} ${CFLAGS} -c tour.c

arp_dozhang: arp.o get_hw_addrs.o
	${CC} -o arp_dozhang arp.o get_hw_addrs.o ${LIBS}
arp.o: arp.c
	${CC} ${CFLAGS} -c arp.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c
	
clean:
	rm tour_dozhang arp_dozhang tour.o arp.o get_hw_addrs.o
