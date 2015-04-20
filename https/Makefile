CC = g++

all: single_http_server single_https_server

single_http_server: single_http_server.cc
	${CC} single_http_server.cc -o single_http_server

single_https_server: single_https_server.cc
	${CC} single_https_server.cc -o single_https_server -lssl -lcrypto

clean:
	rm -f single_http_server single_https_server


