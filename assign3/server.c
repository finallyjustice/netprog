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

void msg_recv(int sockfd, char *message, char *ip_src, unsigned short *port_src, char *ip_dest, unsigned short *port_dest);
void msg_send(int sockfd, SA *servaddr, socklen_t servlen, char *ip_src, char *ip_dest, int port_dest, char *message, int flag);
int getVMNum(char *ip_addr);
void getAllIP();

char ip_array[10][SIZE_IP];

int main(int argc, char **argv)
{
	getAllIP();

	int count;

	int sockfd;
	struct sockaddr_un servaddr;

	sockfd=Socket(AF_LOCAL, SOCK_DGRAM, 0);
	
	unlink(SERV_WKPATH);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family=AF_LOCAL;
	strcpy(servaddr.sun_path, SERV_WKPATH);

	Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

	char *message=(char *)Malloc(32);
	char ip_src[INET_ADDRSTRLEN];
	char ip_dest[INET_ADDRSTRLEN];
	unsigned short port_src;
	unsigned short port_dest;
	
	for( ; ; )
	{
		msg_recv(sockfd, message, ip_src, &port_src, ip_dest, &port_dest);
		//printf("%s\n", ip_src);
		//printf("%d\n", port_src);
		//printf("%s\n", message);

		struct sockaddr_un cliaddr;
		bzero(&cliaddr, sizeof(cliaddr));
		cliaddr.sun_family=AF_LOCAL;
		strcpy(cliaddr.sun_path, ODR_PATH);

		char *new_message=(char *)Malloc(32);
		for(count=0;count<32;count++)
		{
			new_message[count]='\0';
		}
		time_t ticks;
		ticks=time(NULL);
		sprintf(new_message, "%.24s\r\n", ctime(&ticks));
		
		msg_send(sockfd, (SA *)&cliaddr, sizeof(cliaddr), ip_dest, ip_src, port_src, new_message, 0);

		printf("##SEND# server at node vm%d responding to request from vm%d.\n", getVMNum(ip_src)+1, getVMNum(ip_dest)+1);
	
	}
	unlink(SERV_WKPATH);
}

void msg_recv(int sockfd, char *message, char *ip_src, unsigned short *port_src, char *ip_dest, unsigned short *port_dest)
{
	void *recv_packet=(void *)Malloc(SIZE_SENDMESSAGE);

	char m_ip_src[INET_ADDRSTRLEN];
	char m_ip_dest[INET_ADDRSTRLEN];
	unsigned short m_port_src;
	unsigned short m_port_dest;
	
	Recvfrom(sockfd, recv_packet, SIZE_SENDMESSAGE, 0, NULL, NULL);

	//printf("TYPE: %d\n", *(int *)recv_packet);
	//printf("SIP :%s\n", (char *)(recv_packet+4));
	//printf("SPOT:%d\n", *(unsigned short *)(recv_packet+20));
	//printf("DIP :%s\n", (char *)(recv_packet+22));		
	///printf("DPOT:%d\n", *(unsigned short *)(recv_packet+38));
		
	sprintf(m_ip_src, "%s", (char *)(recv_packet+4));
	sprintf(m_ip_dest, "%s", (char *)(recv_packet+22));
	m_port_src=*(unsigned short *)(recv_packet+20);
	m_port_dest=*(unsigned short *)(recv_packet+38);
	sprintf(message, "%s", (char *)(recv_packet+48));

	sprintf(ip_src, "%s", m_ip_src);
	sprintf(ip_dest, "%s", m_ip_dest);
	*port_src=m_port_src;
	*port_dest=m_port_dest;

}
void msg_send(int sockfd, SA *servaddr, socklen_t servlen, char *ip_src, char *ip_dest, int port_dest, char *message, int flag)
{
	//printf("SRC IP    : %s\n", ip_src);
	//printf("DEST IP   : %s\n", ip_dest);
	//printf("DEST PORT : %d\n", port_dest);
	//printf("MESSAGE   : %s\n", message);
	//printf("FLAG      : %d\n", flag);
	
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

