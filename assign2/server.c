/** File:		server.c
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

#include "network.h"
#include "unprtt.h"
#include <setjmp.h>

int check_local(char *IPclient,struct socket_content *socket_info,int i_len);
struct in_addr bitwise(struct in_addr ip,struct in_addr mask);
void sig_chld(int signo);

void read_from_file(FILE *filedp, int read_length, struct payload *packet);
int getCurrentSeq();
struct payload *create_packet();

struct server_configuration server_conf;
int rttinit=0;
struct rtt_info rttinfo;

static void sig_alrm(int signo);
static sigjmp_buf jmpbuf;
static sigjmp_buf jmpfin;

static void final_ack(int signo);

int fin_status=0;

int main(int argc, char **argv)
{
	printf("\n");

	int count;
	int count_ip;
	int count_c;

	int sockfd;

	FILE *in_fp;
	in_fp=fopen("server.in","r");

	if(in_fp==NULL)
	{
		printf("No server.in file!\n");
	}

	//server port in string
	char ch_serv_port[MAXLINE]; 

	//window size in string 
	char ch_serv_win[MAXLINE];

	//server port in int
	int serv_port;

	//window size in int
	int serv_win;

	//retrieve server port
	fgets(ch_serv_port, MAXLINE, in_fp);
	serv_port=atoi(ch_serv_port);

	//retrieve server window size
	fgets(ch_serv_win, MAXLINE, in_fp);
	serv_win=atoi(ch_serv_win);

	fclose(in_fp);

	server_conf.serv_port=serv_port;
	server_conf.serv_win_size=serv_win;
	
	printf("Server Port : %d\n", serv_port);
	printf("Window Size : %d\n", serv_win);
	
	//retrieve IP address list
	printf("\nIP Address Available:\n\n");

	int num_if=0;
	const int on=1;

	struct socket_content socket_info[MAXLINE];
	struct sockaddr_in *sa; //ip address
	struct sockaddr_in *sinptr; //network mask

	struct ifi_info *ifi;
    struct ifi_info *ifihead;

	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1) ; ifi != NULL ; ifi = ifi->ifi_next)
	{
		sockfd= Socket(AF_INET, SOCK_DGRAM, 0);
		Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		socket_info[num_if].sockfd=sockfd;


		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(serv_port);

		Bind(socket_info[num_if].sockfd, (SA *) sa, sizeof(*sa));
		
		//IP Address
		socket_info[num_if].ip=sa->sin_addr;
		printf(" IP Address    : %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));

		//Network mask
		sinptr=(struct sockaddr_in *)ifi->ifi_ntmaddr;
		socket_info[num_if].mask=sinptr->sin_addr;
		printf(" Network Mask  : %s\n", Sock_ntop((SA *) sinptr, sizeof(*sinptr)));

		//caculate subnet Address
		socket_info[num_if].subnet=bitwise(socket_info[num_if].ip, socket_info[num_if].mask);
		
		printf(" Subnet Address: %s\n", inet_ntoa(socket_info[num_if].subnet));
		
		printf("\n");

		num_if++;
	}

	printf("\n");

	Signal(SIGCHLD, sig_chld);

	fd_set rset;

	int maxfd;
	int nready;

	//get max fd
	maxfd=socket_info[0].sockfd;
	for(count=0;count<num_if;count++)
	{
		if(socket_info[count].sockfd>maxfd)
		{
			maxfd=socket_info[count].sockfd;
		}
	}
	
	for( ; ; )
	{
		FD_ZERO(&rset);

		for(count_ip=0; count_ip<num_if; count_ip++)
		{
			FD_SET(socket_info[count_ip].sockfd,&rset);
		}

		if( (nready=select(maxfd+1, &rset, NULL, NULL, NULL)) < 0 )
		{
			//printf("HelloLele\n");
			if (errno == EINTR) 
			{
				nready=0;
				continue;
			}
		}

		for(count_ip=0; count_ip<num_if; count_ip++)
		{

			if(FD_ISSET(socket_info[count_ip].sockfd, &rset))
			{
				struct sockaddr_in s_cliaddr;
				int len_s_cliaddr;

				len_s_cliaddr=sizeof(struct sockaddr);

				char ch_PortClient[MAXLINE];
				Recvfrom(socket_info[count_ip].sockfd, ch_PortClient, MAXLINE, 0, (SA *)&s_cliaddr, &len_s_cliaddr);
				
				char IPclient[MAXLINE];
				int PortClient;

				char IPserver[MAXLINE];
				int PortServer;

				sprintf(IPserver, "%s", inet_ntoa(socket_info[count_ip].ip));
				printf("IPserver : %s\n", IPserver);

				sprintf(IPclient, "%s", inet_ntoa(s_cliaddr.sin_addr));
				PortClient=atoi(ch_PortClient);

				//race condition?
				pid_t pid;

				if( (pid=Fork()) == 0 ) 
				{
					for(count=0;count<num_if;count++)
					{
						if(count!=count_ip)
						{
							close(socket_info[count].sockfd);
						}
					}
					
					int final_sockfd;
					final_sockfd=Socket(AF_INET, SOCK_DGRAM, 0);

					struct sockaddr_in final_servaddr;
					bzero(&final_servaddr, sizeof(final_servaddr));
					final_servaddr.sin_family=AF_INET;
					final_servaddr.sin_port=htons(0);
					Inet_pton(AF_INET, IPserver, &final_servaddr.sin_addr);

					Bind(final_sockfd, (SA *)&final_servaddr, sizeof(final_servaddr));

					struct sockaddr_in final_cliaddr;
					bzero(&final_cliaddr, sizeof(final_cliaddr));
					final_cliaddr.sin_family=AF_INET;
					final_cliaddr.sin_port=PortClient;
					Inet_pton(AF_INET, IPclient, &final_cliaddr.sin_addr);

					Connect(final_sockfd, (SA *)&final_cliaddr, sizeof(final_cliaddr));
					struct sockaddr_in myFinalServaddr;
					socklen_t len_myFinalServaddr;

					len_myFinalServaddr=sizeof(myFinalServaddr);
					if(getsockname(final_sockfd, (SA *)&myFinalServaddr, &len_myFinalServaddr) < 0)
					{
						printf("getsockname error!\n");
						exit(1);
					}

					char ch_finalPortServ[MAXLINE];
					sprintf(ch_finalPortServ, "%i", ntohs(myFinalServaddr.sin_port));
					printf("Server Ephemeral Port: %s\n", ch_finalPortServ);

					Write(final_sockfd, ch_finalPortServ, MAXLINE);
			
					close(socket_info[count_ip].sockfd);

					char recvrange[MAXLINE];
					Read(final_sockfd, recvrange, MAXLINE);
					int int_recvrange=atoi(recvrange);

					if(int_recvrange==1)
					{
						printf("Client is on the host.\n");
					}
					else if(int_recvrange==2)
					{
						printf("Client is on the local.\n");
					}
					else if(int_recvrange==3)
					{
						printf("client is remote.\n");
					}

					char recvfilename[MAXLINE];
					Read(final_sockfd, recvfilename,  MAXLINE);
					printf("File Name: %s\n", recvfilename);

					//start transfer file##########################################
					FILE *prefiledp;
					if(prefiledp=fopen(recvfilename, "r"));

					if(prefiledp==NULL)
					{
						printf("Can't Open File!\n");
						exit(1);
					}
					
					int num_char=0;
					char ch;

					ch=fgetc(prefiledp);
					while(ch!=EOF)
					{
						num_char++;
						ch=fgetc(prefiledp);
					}

					fclose(prefiledp);

					int num_packet;
					if(num_char % (LEN_MESSAGE-1) == 0)
					{
						num_packet=num_char/(LEN_MESSAGE-1);
					}
					if(num_char % (LEN_MESSAGE-1) != 0)
					{
						num_packet=num_char/(LEN_MESSAGE-1)+1;
					}


					char sendnumpack[MAXLINE];
					sprintf(sendnumpack, "%d", num_packet);
					Write(final_sockfd, sendnumpack, MAXLINE);
					
					FILE *filedp;
					filedp=fopen(recvfilename, "r");

					struct payload serv_win_buf[num_packet];
					for(count=0;count<num_packet;count++)
					{
						for(count_c=0;count_c<LEN_MESSAGE-1;count_c++)
						{
							ch=fgetc(filedp);
							if(ch==EOF)
							{
								break;
							}
							serv_win_buf[count].message[count_c]=ch;
						}
						serv_win_buf[count].message[count_c+1]='\0';
					}
				
					int current_seq=0;

					struct ackheader ackhead;

					for( ; ; )
					{
						if(rttinit==0)
						{
							rtt_init(&rttinfo);
							rttinit=1;
							rtt_d_flag=1;
						}

						Signal(SIGALRM, sig_alrm);
						rtt_newpack(&rttinfo);
	                  sendagain:
						serv_win_buf[current_seq].ts=rtt_ts(&rttinfo);
						Write(final_sockfd, &serv_win_buf[current_seq], SIZE_PAYLOAD);
						alarm(rtt_start(&rttinfo));

						if(sigsetjmp(jmpbuf, 1)!=0)
						{
							printf("timeout!\n");
							if(rtt_timeout(&rttinfo) < 0)
							{
								printf("Time Out For Packet: %d\n", serv_win_buf[current_seq].sequence);
								rttinit=0;
							}
							goto sendagain;
						}

						printf("... ");
						Read(final_sockfd, &ackhead, MAXLINE);
						
						alarm(0);

						rtt_stop(&rttinfo, rtt_ts(&rttinfo)-((struct ackheader)ackhead).ts);
						if(current_seq==num_packet-1)
						{
							break;
						}
						current_seq++;
					}

					printf("\nFile Transfer OK!\n\n");

					int num_timeout=0;

					struct payload finload;
					finload.sequence=-1;

					Signal(SIGALRM, final_ack);
					
writeagain:
					Write(final_sockfd, &finload, SIZE_PAYLOAD);

					struct ackheader finack;

					alarm(3);

					int n;
						
					if(sigsetjmp(jmpfin, 1)!=0)
					{
						printf("ACK Timeout!\n");
						num_timeout++;
						goto writeagain;
					}

					if(num_timeout==3)
					{
						printf("Can't Contact Client Anymore!\n");
						close(final_sockfd);
						exit(0);
					}
				
					printf("Waiting for ACK...\n");
					Read(final_sockfd, &finack, MAXLINE);
					
					alarm(0);			

					Write(final_sockfd, &finack, MAXLINE);

		
					close(final_sockfd);

					exit(0);
				}
				
			}
		}
	}
}

int check_local(char *IPclient,struct socket_content *socket_info,int i_len){
	
	int i;
	struct in_addr ip_addr, ip_subaddr;
	inet_pton(AF_INET, IPclient, &ip_addr);

	//if client host is loopback address
	for(i=0;i<i_len;i++){
		if(strcmp(IPclient,inet_ntoa(socket_info[i].ip))==0){	
			return 0;

		}
		
	}
	//if client host is local
	for(i=0;i<i_len;i++){
	
		ip_subaddr = bitwise(ip_addr, socket_info[i].mask);
		if(strcmp(inet_ntoa(ip_subaddr),inet_ntoa(socket_info[i].subnet))==0){
			return 1;
			
		}
	}
	//if client host is remote
	return 2;
	
}

struct in_addr bitwise(struct in_addr ip,struct in_addr mask)
{
	in_addr_t subnet_addr;
	struct in_addr subnet;

	subnet.s_addr=ip.s_addr & mask.s_addr;
	
	return subnet;
}

void sig_chld(int signo)
{
	pid_t pid;
	int stat;
	char buf[MAXLINE];

	while( (pid=waitpid(-1, &stat, WNOHANG)) > 0)
	{
		sprintf(buf, "\nChile %d terminated\n", pid);
		fputs(buf, stdout);
	}

	return;
}

void read_from_file(FILE *filedp, int read_length, struct payload *packet)
{
	int count;

	char ch;

	for(count=0;count<LEN_MESSAGE-1;count++)
	{
		ch=fgetc(filedp);
		if(ch!=EOF)
		{
			packet->message[count]=ch;
		}
		if(ch==EOF)
		{
			break;
		}
	}
}

static void sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

static void final_ack(int signo)
{
	siglongjmp(jmpfin, 1);
}
