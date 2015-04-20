FLAGS  := -g -Wall
CC     := g++
LIB    := libreactor.a

all: time_server time_client http_server

time_server: reactor.cc event_demultiplexer.cc time_server.cc
	$(CC) $(FLAGS) reactor.cc event_demultiplexer.cc http_server.cc -o $@

time_client: reactor.cc event_demultiplexer.cc time_client.cc
	$(CC) $(FLAGS) reactor.cc event_demultiplexer.cc time_client.cc -o $@

http_server: reactor.cc event_demultiplexer.cc http_server.cc
	$(CC) $(FLAGS) reactor.cc event_demultiplexer.cc http_server.cc -o $@

clean:
	rm -rf time_server time_client http_server
