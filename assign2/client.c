/** File:		clien.c
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
#include <setjmp.h>

int check_local(char *ipaddr, struct socket_content *socket_info, int len, char *IPclient, int *flag);
struct in_addr bitwise(struct in_addr ip,struct in_addr mask);
int match(struct in_addr *cli_ip, struct in_addr *ser_ip);
static void *read_from(void *);

int port_num;
struct client_configuration client_conf;

struct payload **cli_win_buf;

static sigjmp_buf jmpwait;
static void wait_sgl(int signo);

int main(int argc, char **argv)
{
	printf("\n");

	int count;
	
	FILE *in_fp;
	in_fp=fopen("client.in","r");

	if(in_fp==NULL)
	{
		printf("No client.in file!\n");
	}
	
	char ch_ipaddr[MAXLINE];
	char ch_serv_port[MAXLINE];
	char ch_filename[MAXLINE];
	char ch_cli_win[MAXLINE];
	char ch_seed[MAXLINE];
	char ch_probability[MAXLINE];
	char ch_mean[MAXLINE];
	
	int int_serv_port;
	int int_cli_win;
	int int_seed;
	float float_probability;
	int int_mean;
	
	//retrieve ip address
	fgets(ch_ipaddr, MAXLINE, in_fp);
	int len_ip=strlen(ch_ipaddr);
	char ipaddr[len_ip];
	for(count=0;count<len_ip-1;count++)
		ipaddr[count]=ch_ipaddr[count];
	ipaddr[len_ip-1]='\0';
	
	//retrieve server port
	fgets(ch_serv_port, MAXLINE, in_fp);
	int_serv_port=atoi(ch_serv_port);
	
	//retrieve file name
	fgets(ch_filename, MAXLINE, in_fp);
	int len_fn=strlen(ch_filename);
	char filename[len_fn];
	for(count=0;count<len_fn-1;count++)
		filename[count]=ch_filename[count];
	filename[len_fn-1]='\0';
	
	//retrieve window size
	fgets(ch_cli_win, MAXLINE, in_fp);
	int_cli_win=atoi(ch_cli_win);

	//retrieve seed value
	fgets(ch_seed, MAXLINE, in_fp);
	int_seed=atoi(ch_seed);

	//retrieve probability
	fgets(ch_probability, MAXLINE, in_fp);
	float_probability=atof(ch_probability);

	//retrieve mean
	fgets(ch_mean, MAXLINE, in_fp);
	int_mean=atoi(ch_mean);

	fclose(in_fp);

	printf("Configuration File: \n");
	printf("  Server IP     : %s\n", ipaddr);
	printf("  Server Port   : %d\n", int_serv_port);
	printf("  File Name     : %s\n", filename);
	printf("  Recv Win Size : %d\n", int_cli_win);
	printf("  Seed Value    : %d\n", int_seed);
	printf("  Probability   : %f\n", float_probability);
	printf("  Mean          : %d\n", int_mean);
	printf("\n");

	client_conf.serv_ip=ipaddr;
	client_conf.serv_port=int_serv_port;
	client_conf.filename=filename;
	client_conf.cli_win_size=int_cli_win;
	client_conf.seed=int_seed;
	client_conf.probability=float_probability;
	client_conf.recv_mean=int_mean;
	
	struct ifi_info	*ifi;
	struct ifi_info *ifihead;

	struct sockaddr_in *sa;
	struct sockaddr_in *sa1;

	struct socket_content socket_info[MAXLINE];

	int sockfd;
	
	int num_if=0;
	
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next) 
	{
		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		socket_info[num_if].sockfd=sockfd;		

		if ( (sa = (struct sockaddr_in *)ifi->ifi_addr) != NULL)
		{	
			socket_info[num_if].ip=sa->sin_addr;
			printf("IP Address  : %s\n",Sock_ntop_host(sa, sizeof(*sa)));
		}
		
		if ( (sa1 = (struct sockaddr_in *)ifi->ifi_ntmaddr) != NULL)
		{
			socket_info[num_if].mask=sa1->sin_addr;
			printf("Network mask: %s\n",Sock_ntop_host(sa1, sizeof(*sa1)));
		}
		
		socket_info[num_if].subnet= bitwise(socket_info[num_if].ip, socket_info[num_if].mask);	
		
		num_if++;
	}

	printf("\n");

	char IPclient[MAXLINE];
	int flag_local;
	int result;

	result=check_local(ipaddr, socket_info, num_if, IPclient, &flag_local);

	char IPserver[MAXLINE];
	sprintf(IPserver,"%s",ipaddr);

	printf("\nIPCLIENT: %s\n", IPclient);
	printf("IPSERVER: %s\n\n", IPserver);

	int new_sockfd;

	new_sockfd=Socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in IPCliaddr;
	bzero(&IPCliaddr, sizeof(IPCliaddr));
	IPCliaddr.sin_family=AF_INET;
	IPCliaddr.sin_port=htons(0);
	Inet_pton(AF_INET, IPclient, &IPCliaddr.sin_addr);

	Bind(new_sockfd, (SA *)&IPCliaddr, sizeof(IPCliaddr));

	struct sockaddr_in myCliaddr;
	socklen_t len_myCliaddr;

	len_myCliaddr=sizeof(myCliaddr);
	if(getsockname(new_sockfd, (SA *)&myCliaddr, &len_myCliaddr) < 0 )
	{
		printf("getsockname error!\n");
		exit(1);
	}

	char ch_PortClient[MAXLINE];
	sprintf(ch_PortClient, "%i", ntohs(myCliaddr.sin_port));
	printf("Client Ephemeral Port: %s\n", ch_PortClient);

	struct sockaddr_in IPServaddr;
	bzero(&IPServaddr, sizeof(IPServaddr));
	IPServaddr.sin_family=AF_INET;
	IPServaddr.sin_port=htons(client_conf.serv_port);
	Inet_pton(AF_INET, IPserver, &IPServaddr.sin_addr);

//	Connect(new_sockfd, (SA *)&IPServaddr, sizeof(IPServaddr));

	char msg_cliport[MAXLINE];
	sprintf(msg_cliport, "%s", ch_PortClient);
	Sendto(new_sockfd, msg_cliport, MAXLINE, 0, (SA *)&IPServaddr, sizeof(IPServaddr));

	char msg_servport[MAXLINE];
	Recvfrom(new_sockfd, msg_servport, MAXLINE, 0, NULL, NULL);
	printf("Server Well Known Port : %d\n", client_conf.serv_port);
	printf("Server Ephemeral Port  : %s\n", msg_servport);

	int PortServer=atoi(msg_servport);
	struct sockaddr_in finalServaddr;
	bzero(&finalServaddr, sizeof(finalServaddr));
	finalServaddr.sin_family=AF_INET;
	finalServaddr.sin_port=htons(PortServer);
	Inet_pton(AF_INET, IPserver, &finalServaddr.sin_addr);

	Connect(new_sockfd, (SA *)&finalServaddr, sizeof(finalServaddr));

	if(result==0)
	{
		Write(new_sockfd, "1", MAXLINE);
	}
	else if(result==1)
	{
		if(flag_local==1)
		{
			Write(new_sockfd, "2", MAXLINE);
		}
		else if(flag_local==2)
		{
			Write(new_sockfd, "3", MAXLINE);
		}
	}
	
	//if server is host
	if(result==0){
		int sockfd_cli;

		strcpy(IPserver,"127.0.0.1");
		strcpy(IPclient,"127.0.0.1");
		printf("Server is on the same host \n");
		printf("IPserver is  %s\n",ipaddr);
		printf("IPclient is  %s\n",IPclient);
	}

	//If server is local
	if(result==1)
	{
		if(flag_local==1)
		{
			int sockfd_cli;
			printf("Server host is local \n");
			printf("IPserver is  %s\n",ipaddr);
			printf("IPclient is  %s\n",IPclient);
		}
	
	//If server is remote
		else if(flag_local==2)
		{
			int sockfd_cli;
			printf("Server host is not local \n");
			printf("IPserver is  %s\n",ipaddr);
			printf("IPclient is  %s\n",IPclient);
		}
	}

	Write(new_sockfd, client_conf.filename, MAXLINE);

	char recvnumpack[MAXLINE];
	Read(new_sockfd, recvnumpack, MAXLINE);

	int num_packet=atoi(recvnumpack);

	printf("\n\n");

	for( ; ; )
	{
		struct payload recvpayload;

		Read(new_sockfd, &recvpayload, SIZE_PAYLOAD);

		if(((struct payload)recvpayload).sequence==-1)
		{
			printf("\nFinal\n");
			break;
		}

		if(do_drop()==1)
		{
			printf("\n########Packet Drop!##########\n");
			continue;
		}

		struct ackheader ackhead;

		ackhead.ts=((struct payload)recvpayload).ts;
		ackhead.ack=((struct payload)recvpayload).sequence+1;

		Write(new_sockfd, &ackhead, MAXLINE);

		printf("%s", ((struct payload)recvpayload).message);
	}
	
	
	Signal(SIGALRM, wait_sgl);

	struct ackheader ackfin;

	ackfin.ack=-2;
	
	Write(new_sockfd, &ackfin, MAXLINE);

	alarm(4);

	if(sigsetjmp(jmpwait, 1)!=0)
	{
		close(new_sockfd);
		printf("CLOSE_WAIT\n");
		exit(1);
	}

	Read(new_sockfd, &ackfin, MAXLINE);

	close(new_sockfd);
	printf("CLOSE Socket!\n");

}
int match(struct in_addr *cli_ip, struct in_addr *ser_ip)
{
	uint32_t a=cli_ip->s_addr;
	uint32_t b=ser_ip->s_addr;

	int count=0;

	uint32_t c=a^b;

	while(c!=0)
	{
		if(c & 1 ==1)
		{
			count++;
		}

		c=c>>1;
	}

	return count;
}

int check_local(char *ipaddr, struct socket_content *socket_info, int num_if, char *IPclient, int *flag)
{
	int count_if;
	int match_num;
	int num=1000; //representing max value

	struct in_addr ip_addr;
	struct in_addr ip_subaddr;

	inet_pton(AF_INET, ipaddr, &ip_addr);
	*flag=0;
	
	for(count_if=0; count_if<num_if; count_if++)
	{
		//on same host
		if(strcmp(ipaddr,inet_ntoa(socket_info[count_if].ip))==0)
		{
			return 0;
		}
			
		//on local
		ip_subaddr = bitwise(ip_addr,socket_info[count_if].mask);
		
		if(strcmp(inet_ntoa(ip_subaddr),inet_ntoa(socket_info[count_if].subnet))==0)
		{
			*flag=1;

			match_num=match(&(socket_info[count_if].ip), &ip_addr);
			
			if(num>match_num)
			{
				num=match_num;
				strcpy(IPclient, inet_ntoa(socket_info[count_if].ip));
			}
		}
	}

	if(*flag!=1)
	{
		*flag=2;
	}
	
	if(*flag==1 || *flag==2)
	{
		if(strcmp(IPclient,"127.0.0.1")==0)
		{
			strcpy(IPclient, inet_ntoa(socket_info[1].ip));
		}
	}

	return 1;
}

struct in_addr bitwise(struct in_addr ip,struct in_addr mask)
{
	struct in_addr subnet;
	in_addr_t subnet_addr;

	subnet.s_addr=ip.s_addr & mask.s_addr;
	
	return subnet;
}

int do_drop()
{
	float r=drand48();
	if(client_conf.probability > r)
	{
		return 0;
	}
	return 1;
}

static void wait_sgl(int signo)
{
	siglongjmp(jmpwait, 1);
}
