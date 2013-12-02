/** File:		tcpechotimecli.c
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

int checkstatus; //use checkstatus to handle EINTR

void sig_chld(int signo);

int
main(int argc, char **argv)
{
	if(argc!=2)
	{
		err_quit("Please input an IP address.\n");
	}

	//test whether it's ip address
	const char *ipnamestr; 
	struct in_addr ip;
	struct hostent *hp;

	ipnamestr=argv[1];
	char ipaddr[INET_ADDRSTRLEN];//Store Ip Address

	inet_aton(ipnamestr, &ip);

	//Whether it's IP?
	if( (hp=gethostbyaddr((const char*)&ip, sizeof(ip), AF_INET))==NULL )
	{
		//test whether it's hostname?

		if( (hp=gethostbyname((const char*)ipnamestr)) == NULL )
		{
			printf("Unknown Host\n");
			exit(0);
		}
	}

	char **pptr=hp->h_addr_list;
	Inet_ntop(hp->h_addrtype, *pptr, ipaddr, sizeof(ipaddr)); //only use the first IP in IP List

	printf("\nYou are connecting to(HOSTNAME ): %s\n",hp->h_name);
	printf("You are connecting to(IPADDRESS): %s\n",ipaddr);

	//main loop of client
	for( ; ; )
	{
		printf("\nChoose Service: \n");
		printf("1.echo\n");
		printf("2.time\n");
		printf("3.quit\n");

		char choice[2];
		scanf("%s",&choice);
		
		//Choose echo
		if(strcmp("1",choice)==0)
		{
			char buf[1025];
			int nread;

			pid_t childpid;

			int pfd[2];

			pipe(pfd);
			signal(SIGCHLD, sig_chld);

			
			char spfd[10];
			sprintf(spfd,"%d",pfd[1]); //we pass spfd to echo_cli
			
			//in child
			if( (childpid=Fork()) == 0 )
			{	
				close(pfd[0]);

				if( execlp("xterm","xterm","-e","./echo_cli",ipaddr,spfd,(char *)0) < 0 )
					printf("Can't Run Xterm!\n");
			}
			else
			{	
				close(pfd[1]);
				
				int nread;
				char buf_read[MAXLINE+1];
				int maxfdp1;
				int sere=1;				

				fd_set rset;
				FD_ZERO(&rset);
		
				checkstatus=0;

				while(checkstatus==0)
				{				
					maxfdp1=max(pfd[0],fileno(stdin))+1;

					FD_SET(pfd[0], &rset);
					FD_SET(fileno(stdin), &rset);
					
					sere=select(maxfdp1,&rset,NULL,NULL,NULL);
					
					if(FD_ISSET(pfd[0],&rset))
					{
						if((nread=read(pfd[0],buf_read,MAXLINE)!=0))
							printf("Child Received: %s", buf_read);

					}
					if(FD_ISSET(fileno(stdin),&rset))
					{
						if(checkstatus==1)
							break;

						if((nread=read(fileno(stdin),buf_read,MAXLINE))>0)
							fputs("Don't type during the child is running!\n",stdout);
					}
				}
			}
		}
		else if(strcmp("2",choice)==0)
		{
			pid_t childpid;

			int pfd[2];
			pipe(pfd);

			signal(SIGCHLD,sig_chld);

			char spfd[10];
			sprintf(spfd,"%d",pfd[1]);

			if( (childpid=Fork()) == 0 )
			{	
				close(pfd[0]);

				if( execlp("xterm","xterm","-e","./time_cli",ipaddr,spfd,(char *)0) < 0)
					printf("TIME\n");
			}
			else
			{
				close(pfd[1]);
				
				int nread;
				char buf_read[MAXLINE+1];
				int maxfdp1;
				int sere=1;

				fd_set rset;
				FD_ZERO(&rset);

				checkstatus=0;

				while(checkstatus==0)
				{
					maxfdp1=max(pfd[0],fileno(stdin))+1;

					FD_SET(pfd[0],&rset);
					FD_SET(fileno(stdin),&rset);

					select(maxfdp1,&rset,NULL,NULL,NULL);				

					if(FD_ISSET(pfd[0],&rset))
					{
						if((nread=read(pfd[0],buf_read,MAXLINE))!=0)
						{

							printf("Child Received: %s",buf_read);
						}
					}
					else if(FD_ISSET(fileno(stdin),&rset))
					{
						if(checkstatus==1)
						{
							break;
						}

						if((nread=read(fileno(stdin),buf_read,MAXLINE))>0)
						{
							fputs("Don't type during the child is running!\n",stdout);
						}
					}
				}
			}
		}
		else if(strcmp("3",choice)==0)
		{
			printf("QUIT\n");
			exit(0);
		}
		else
		{
			printf("Your input is error!\n");
		}

	}
}

void sig_chld(int signo)
{
	pid_t pid;
	int stat;
	char buf[1024];
	
	while((pid=waitpid(-1, &stat, WNOHANG))>0)
	{
		checkstatus=1;

		sprintf(buf,"Child %d terminated\n",pid);
		fputs(buf,stdout);
	}
	return;
}
