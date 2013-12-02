/** File:		echo_cli.c
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
#include <stdlib.h>

void str_clib(FILE *fp, int sockfd, int pfd);

int main(int argc, char **argv)
{
	char *spfd=argv[2];
	int pfd=atoi(spfd);	

	Writen(pfd,"Child is Running.\n",strlen("Child is Running.\n"));

	printf("ECHO -- %s\n", argv[1]);
	printf("INPUT MESSAGE BELOW:\n\n");

	int sockfd;
	struct sockaddr_in servaddr;

	sockfd=Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(ECHO_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	int connstatus=connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	if(connstatus==-1)
	{
		Writen(pfd,"Server is Closed!\n",strlen("Server is Closed!\n"));
		return 1;
	}

	str_clib(stdin, sockfd, pfd);
	
	exit(0);

	
}

void
str_clib(FILE *fp, int sockfd, int pfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	char buf[MAXLINE];

	int maxfdp1,stdineof;
	fd_set rset;

	int n;

	stdineof=0;
	FD_ZERO(&rset);

	for( ; ; )
	{
		if(stdineof==0)
			FD_SET(fileno(fp),&rset);

		FD_SET(sockfd,&rset);

		maxfdp1=max(fileno(fp),sockfd)+1;
		Select(maxfdp1,&rset,NULL,NULL,NULL);

		if(FD_ISSET(sockfd,&rset))
		{
			if((n=Readline(sockfd, buf, MAXLINE))==0)
			{
				if(stdineof==1)
				{
					sprintf(buf,"%s","Normal Exit!\n");
					Writen(pfd,buf,strlen(buf));
					close(pfd);
					return;
				}
				else
				{
					sprintf(buf,"%s","Server Terminated Prematurely!\n");
					Writen(pfd,buf,strlen(buf));
					close(pfd);
					return;
				}
			}
			Write(fileno(stdout),buf,n);
			Writen(pfd,buf,strlen(buf)+1);
			
		}

		if(FD_ISSET(fileno(fp),&rset))
		{
			if((n=Read(fileno(fp),buf,MAXLINE))==0)
			{
				stdineof=1;
				Shutdown(sockfd,SHUT_WR);
				FD_CLR(fileno(fp),&rset);
				continue;
			}
			Writen(sockfd,buf,n);
		}
	}
}
