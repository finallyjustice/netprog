/** File:		tcpechotimesrv.c
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

#include "unp.h"
#include "network.h"

static void *do_echo(void *);
static void *do_time(void *);

int main(int argc, char **argv)
{
	int listenfd_echo;

	struct sockaddr_in servaddr_echo;

	listenfd_echo=Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr_echo, sizeof(servaddr_echo));
	servaddr_echo.sin_family=AF_INET;
	servaddr_echo.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr_echo.sin_port=htons(ECHO_PORT);

	Bind(listenfd_echo, (SA *) &servaddr_echo, sizeof(servaddr_echo));
	Listen(listenfd_echo, LISTENQ);

	int listenfd_time;

	struct sockaddr_in servaddr_time;

	listenfd_time=Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr_time, sizeof(servaddr_time));
	servaddr_time.sin_family=AF_INET;
	servaddr_time.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr_time.sin_port=htons(TIME_PORT);

	Bind(listenfd_time, (SA *) &servaddr_time, sizeof(servaddr_time));
	Listen(listenfd_time, LISTENQ);

	pthread_t tid_echo;
	pthread_t tid_time;

	int *ptr_echo;
	int *ptr_time;

	fd_set rset;
	int maxfdp1;

	FD_ZERO(&rset);

	for( ; ; )
	{	
		FD_ZERO(&rset);
		FD_SET(listenfd_echo, &rset);
		FD_SET(listenfd_time, &rset);
		maxfdp1=max(listenfd_echo, listenfd_time)+1;		

		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd_echo, &rset))
		{
			ptr_echo=Malloc(sizeof(int));
			*ptr_echo=Accept(listenfd_echo, NULL, NULL);
			Pthread_create(&tid_echo, NULL, &do_echo, ptr_echo);
		}
		else if(FD_ISSET(listenfd_time, &rset))
		{
			if(errno==EINTR)
			{
				continue;
			}
			ptr_time=Malloc(sizeof(int));
			*ptr_time=Accept(listenfd_time, NULL, NULL);
			Pthread_create(&tid_time, NULL, &do_time, ptr_time);
		}
		else
		{
			if(errno==EINTR)
			{
				continue;
			}
		}
	}
}

static void *
do_echo(void *arg)
{
	int connfd;

	connfd=*((int *)arg);
	free(arg);

	Pthread_detach(pthread_self());
	str_echo(connfd);
	Close(connfd);
	return(NULL);
}

void
str_echo(int sockfd)
{
	ssize_t n;
	char buf[MAXLINE];

again:
	while( (n=read(sockfd, buf, MAXLINE)) > 0 )
	{
		Writen(sockfd, buf, n);
	}

	if(n==0)
	{
		printf("Client Terminitate!\n");
		return;	
	}

	if(n<0 && errno==EINTR)
	{
		goto again;
	}
	else if(n<0)
	{
		err_sys("str_echo: read error\n");
	}
}

void str_time(int sockfd)
{
	char buff[MAXLINE];
	time_t ticks;

	int maxfdp1;
	fd_set rset;
	FD_ZERO(&rset);	
	
	int sere;
	
	ticks=time(NULL);
	snprintf(buff,sizeof(buff),"%.24s\r\n",ctime(&ticks));
	Write(sockfd,buff,strlen(buff));

	for( ; ; )
	{
		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		maxfdp1=sockfd+1;
		
		struct timeval timev;
		timev.tv_sec=5;
		timev.tv_usec=0;

		sere=select(maxfdp1,&rset,NULL,NULL,&timev);
		if(FD_ISSET(sockfd,&rset))
		{
			if(Readline(sockfd,buff,MAXLINE)==0)
			{
				printf("Client Terminated!\n");
				break;
			}
		}

		if((sere==-1))
		{	
			break;
		}

		if(sere==0)
		{
			ticks=time(NULL);
			snprintf(buff,sizeof(buff),"%.24s\r\n",ctime(&ticks));
			Write(sockfd,buff,strlen(buff));
		}
	}
}

static void *
do_time(void *arg)
{
	int connfd;
	 
	connfd=*((int *)arg);
	free(arg);
	
	Pthread_detach(pthread_self());
	str_time(connfd);
	Close(connfd);
	return(NULL);

}

