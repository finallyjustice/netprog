/** File:		client.c
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

int msg_recv(int sockfd, char *message, char *ip_src, unsigned short *port_src, char *ip_dest, unsigned short *port_dest);
void msg_send(int sockfd, SA *servaddr, socklen_t servlen, char *ip_src, char *ip_dest, int port_dest, char *message, int flag);
static void recv_alarm(int);
int getVMNum(char *ip_addr);
void getAllIP();

char ip_array[10][SIZE_IP];

int main(int argc, char **argv)
{
	getAllIP();

	int count;
	
	for( ; ; )
	{
		printf("\nPlease Choose the Server From VM1 to VM10.\n");
		printf("(1 - 10):");

		int choose;

		scanf("%d", &choose);

		int vm_dest=choose;
		struct hostent *hp;
		
		char ip_src[INET_ADDRSTRLEN];
		char ip_dest[INET_ADDRSTRLEN];

		int result;
		
		char namestr[10];
		
		for(count=0;count<5;count++)
		{
			namestr[count]='\0';
		}

		sprintf(namestr, "vm%d", choose);

		hp=gethostbyname(namestr);

		char **pptr=hp->h_addr_list;
		Inet_ntop(hp->h_addrtype, *pptr, ip_dest, sizeof(ip_dest));
		
		int sockfd;
	
		struct sockaddr_un servaddr;
		struct sockaddr_un cliaddr;

		sockfd=Socket(AF_LOCAL, SOCK_DGRAM, 0);

		char temp_file[]="tmp_XXXXXX";
		mkstemp(temp_file);

		unlink(temp_file);
		
		bzero(&cliaddr, sizeof(cliaddr));
		cliaddr.sun_family=AF_LOCAL;
		strcpy(cliaddr.sun_path, temp_file);
	
		Bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

		bzero(&servaddr, sizeof(servaddr));
		servaddr.sun_family=AF_LOCAL;
		strcpy(servaddr.sun_path, ODR_PATH);
	
		//client ip vm
		struct hwa_info *hwa;
		struct hwa_info *hwahead;

		struct sockaddr *sa;
		char *ptr;

		for(hwahead=hwa=Get_hw_addrs(); hwa!=NULL;hwa=hwa->hwa_next)
		{
			//we only care about eth0
			if(strcmp(hwa->if_name, "eth0")==0)
			{
				sa=hwa->ip_addr;
				sprintf(ip_src, "%s", Sock_ntop_host(sa, sizeof(*sa)));
			}
		}
		
		//msg_send
		char message[32];
		for(count=0;count<32;count++)
		{
			message[count]='\0';
		}
		sprintf(message, "%s", "\0");

		//char send_packet[50];
		//Sendto(sockfd, send_packet, 50, 0, (SA *)&servaddr, sizeof(servaddr));
		msg_send(sockfd, (SA *)&servaddr, sizeof(servaddr), ip_src, ip_dest, SERV_WKPORT, message, 0);

		char *new_message=(char *)Malloc(32);
		for(count=0;count<32;count++)
		{
			new_message[count]='\0';
		}
		
		char new_ipsrc[SIZE_IP];
		char new_ipdest[SIZE_IP];
		unsigned short new_portsrc;
		unsigned short new_portdest;
		
		result=msg_recv(sockfd, new_message, new_ipsrc, &new_portsrc, new_ipdest, &new_portdest);
		
		if(result==-1)
		{
			printf("##RECV# client at node vm%d timeout on response from vm%d.\n", getVMNum(ip_src)+1, getVMNum(ip_dest)+1);
			
			msg_send(sockfd, (SA *)&servaddr, sizeof(servaddr), ip_src, ip_dest, SERV_WKPORT, message, 1);

			result=msg_recv(sockfd, new_message, new_ipsrc, &new_portsrc, new_ipdest, &new_portdest);

			if(result==-1)
			{
				printf("##RECV# client at node vm%d timeout on response from vm%d.\n", getVMNum(ip_src)+1, getVMNum(ip_dest)+1);
				unlink(temp_file);
				continue;
			}
		}
		
		unlink(temp_file);
	}
}

int  msg_recv(int sockfd, char *message, char *ip_src, unsigned short *port_src, char *ip_dest, unsigned short *port_dest)
{
	void *recv_packet=(void *)Malloc(SIZE_SENDMESSAGE);

	char m_ip_src[INET_ADDRSTRLEN];
	char m_ip_dest[INET_ADDRSTRLEN];
	unsigned short m_port_src;
	unsigned short m_port_dest;
	
	//Timeout Part
	int n;
	Sigfunc *sigfunc;
	sigfunc=Signal(SIGALRM, recv_alarm);
	alarm(TIMEOUT);
	
	if((n=recvfrom(sockfd, recv_packet, SIZE_SENDMESSAGE, 0, NULL, NULL)) < 0)
	{
		if(errno==EINTR)
		{
			errno=ETIMEDOUT;
		}
	}
	alarm(0);
	Signal(SIGALRM, sigfunc);

	if(n<0)
	{
		return -1;
	}

	//printf("TYPE: %d\n", *(int *)recv_packet);
	//printf("SIP :%s\n", (char *)(recv_packet+4));
	//printf("SPOT:%d\n", *(unsigned short *)(recv_packet+20));
	//printf("DIP :%s\n", (char *)(recv_packet+22));		
	//printf("DPOT:%d\n", *(unsigned short *)(recv_packet+38));
		
	sprintf(m_ip_src, "%s", (char *)(recv_packet+4));
	sprintf(m_ip_dest, "%s", (char *)(recv_packet+22));
	m_port_src=*(unsigned short *)(recv_packet+20);
	m_port_dest=*(unsigned short *)(recv_packet+38);
	sprintf(message, "%s", (char *)(recv_packet+48));

	sprintf(ip_src, "%s", m_ip_src);
	sprintf(ip_dest, "%s", m_ip_dest);
	*port_src=m_port_src;
	*port_dest=m_port_dest;

	printf("##RECV# client at node vm%d recieved from vm%d. Time: %s\n.", getVMNum(m_ip_src)+1, getVMNum(m_ip_dest)+1, message);
	
	return 1;
}

void msg_send(int sockfd, SA *servaddr, socklen_t servlen, char *ip_src, char *ip_dest, int port_dest, char *message, int flag)
{
	//printf("SRC IP    : %s\n", ip_src);
	//printf("DEST IP   : %s\n", ip_dest);
	//printf("DEST PORT : %d\n", port_dest);
	//printf("MESSAGE   : %s\n", message);
	//printf("FLAG      : %d\n", flag);
	
	char src_name[10];
	char dest_name[10];

	
	
	int type=2;
	int hop_cnt=0;
	int len=32;
	
	unsigned short port_src=1033;
	
	void *send_packet=(void *)Malloc(SIZE_SENDMESSAGE+4);

	memcpy((void *)send_packet, (void *)&type, SIZE_TYPE);
	memcpy((void *)(send_packet+4), (void *)ip_src, SIZE_IP);

	memcpy((void *)(send_packet+20), (void *)&port_src, SIZE_PORT);
	
	memcpy((void *)(send_packet+22), (void *)ip_dest, SIZE_IP);
	memcpy((void *)(send_packet+38), (void *)&port_dest, SIZE_PORT);
	memcpy((void *)(send_packet+40), (void *)&hop_cnt, SIZE_HOP);
	memcpy((void *)(send_packet+44), (void *)&len, SIZE_LEN);
	memcpy((void *)(send_packet+48), (void *)message, 32);
	memcpy((void *)(send_packet+80), (void *)&flag, 4);
	
	Sendto(sockfd, send_packet, SIZE_SENDMESSAGE+4, 0, servaddr, servlen);

	printf("##SEND# Client at node vm%d sending request to server at vm%d.\n", getVMNum(ip_src)+1, getVMNum(ip_dest)+1);
}

static void recv_alarm(int signo)
{
	return;	
}

int getVMNum(char *ip_addr)
{
	int count;

	for(count=0;count<10;count++)
	{
		if(strcmp(ip_addr, ip_array[count])==0)
		{
			return count;
		}
	}

	return -1;
}

void getAllIP()
{
	int count;
	char name[30];
	struct hostent *hp;
	
	for(count=0;count<10;count++)
	{
		sprintf(name, "vm%d", count+1);
		hp=gethostbyname(name);
		char **pptr=hp->h_addr_list;
		Inet_ntop(hp->h_addrtype, *pptr, ip_array[count], sizeof(ip_array));
	}
}
