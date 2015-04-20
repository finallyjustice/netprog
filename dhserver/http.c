/** File:		http.c
 ** Author:		Dongli Zhang
 ** Contact:	dongli.zhang0129@gmail.com
 **
 ** Copyright (C) Dongli Zhang 2013
 **
 ** This program is free software;  you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY;  without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 ** the GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program;  if not, write to the Free Software 
 ** Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* implementation of http server based on thread pool */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "tpool.h"

#define BUF_LEN				1028
#define SERV_PORT			80
#define PEND_CONNECTIONS	100

const static char http_error_hdr[]	= "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n"; 
const static char http_error_html[] = "<html><body><h1>FILE NOT FOUND</h1></body></html>";
const static char http_html_hdr[]	= "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"; 
const static char http_image_hdr[]  = "HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\n\r\n"; 

//send the requested file to client
int http_send_file(char *filename, int sockfd)
{
	char send_buf[BUF_LEN];

	int fd;
	fd = open(filename, O_RDONLY, S_IREAD | S_IWRITE);
	if(fd == -1)
	{
		printf("File %s not found - sending an HTTP 404\n", filename);
		write(sockfd, http_error_hdr, strlen(http_error_hdr));
		write(sockfd, http_error_html, strlen(http_error_html));
	}
	else
	{
		printf("File %s is being sent\n", filename);
		
		if(strstr(filename, ".jpg")!=NULL || strstr(filename, ".gif")!=NULL)
			write(sockfd, http_image_hdr, strlen(http_image_hdr));
		else
			write(sockfd, http_html_hdr, strlen(http_html_hdr));

		int buf_len = 1;
		while(buf_len > 0)
		{
			buf_len = read(fd, send_buf, BUF_LEN);
			if(buf_len > 0)
			{
				write(sockfd, send_buf, buf_len);
			}
		}

	}

	close(fd);
	close(sockfd);
	return 0;
}

void serve(int sockfd)
{
	char buf[BUF_LEN];
	read(sockfd, buf, BUF_LEN);
	if(!strncmp(buf, "GET", 3))
	{
		char *file = buf+5;
		char *space = strchr(file, ' ');
		*space = '\0';
		http_send_file(file, sockfd);
	}
	else
	{
		printf("unsupported request\n");
		return;
	}
}

void *process_request(void *arg)
{
	int sockfd = (int)arg;
	serve(sockfd);
}
	
int main(int argc, char **argv)
{
	int sockfd;
	int subfd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char *client_ip;
	int client_port;
	int addr_len;

	if(tpool_create(16) != 1)
	{
		printf("create tpool failed\n");
		return 1;
	}
	sleep(3);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		printf("Cannot create socket\n");
		return 1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)))
	{
		perror("Cannot bind port\n");
		return 1;
	}

	listen(sockfd, PEND_CONNECTIONS);

	for(;;)
	{
		addr_len = sizeof(client_addr);
		subfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
		//printf("New request\n");
		client_ip = inet_ntoa(client_addr.sin_addr);
		client_port = ntohs(client_addr.sin_port);
		printf("New request from %s:%d\n", client_ip, client_port);
		tpool_add_job(process_request, subfd);
	}

	return 0;	
}
