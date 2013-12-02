/** File:		tour.c
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

int getVMNum(char *ip_addr);
void getAllIP();
void source_work(char ip_tour[][SIZE_IP], int num_tour);
void middle_work();
void createRTHeader(void *sendline, int ip_len, char *ip_src, struct sockaddr_in *destAddr);
void writeRTNUM(void *sendline, int num);
void writeRTCurrent(void *sendline, int current);
void writeRTFirstCheck(void *sendline, int *firstCheck);
void writeRTSource(void *sendline, char *ip_src);
void writeRTTour(void *sendline, char ip_tour[][SIZE_IP], int num_tour);
int getRTNUM(void *recvline);
int getRTCurrent(void *recvline);
int getCheckFirst(void *recvline);
void setRTCheckFirst(void *recvline);
void getRTNextIP(void *recvline, char *ip_next);
void getRTSourceIP(void *recvline, char *ip_source);
void getRTPreIP(void *recvline, char *ip_pre);
void getRTIPTour(void *recvline, char ip_tour[][SIZE_IP]);
void getRTCheckFirstList(void *recvline, int *checkFirst);
void ping(char *ip_addr);
int areq(char *ip_addr, struct hwaddr *HWaddr);
void ping_alrm(int signo);
void exit_alrm(int signo);

char host_ip[SIZE_IP];
unsigned char host_mac[6];
char ip_array[10][SIZE_IP];
char ip_ping[SIZE_IP];

int pf_sockfd;
//unsigned char mac_ping[6];
int onPing=0;

int sequence;
int totalRecvPing=0;
int num_first_end=0;
int check_first_end=0;

int change_select=0;

int num_message=0;

int main(int argc, char **argv)
{
	int count;

	getAllIP();

	struct hwa_info *hwa;
	struct hwa_info *hwahead;
	struct sockaddr *sa;

	for(hwahead=hwa=Get_hw_addrs(); hwa!=NULL; hwa=hwa->hwa_next)
	{
		if(strcmp(hwa->if_name, "eth0")==0)
		{
			sa=hwa->ip_addr;
			sprintf(host_ip, "%s", Sock_ntop_host(sa, sizeof(*sa)));
			
			for(count=0;count<6;count++)
			{
				host_mac[count]=hwa->if_haddr[count];	
			}

			break;
		}
	}
	
	if(argc==1)
	{
		printf("\n##### Intermediate Node #####\n\n");
		
		middle_work();

		return 1;
	}
	
	if(argc>1)
	{	
		printf("\n##### Source Node #####\n\n");
		
		int count_tour;
		int num_tour;
		
		num_tour=argc-1;

		char vm_tour[num_tour][SIZE_VM];
		char ip_tour[num_tour][SIZE_IP];
		
		struct hostent *hp_tour;
		char **ptrTemp;

		for(count_tour=0;count_tour<num_tour;count_tour++)
		{
			sprintf(vm_tour[count_tour], "%s", argv[count_tour+1]);
		}
		
		for(count_tour=0;count_tour<num_tour;count_tour++)
		{
			hp_tour=gethostbyname(vm_tour[count_tour]);
			ptrTemp=hp_tour->h_addr_list;
			Inet_ntop(hp_tour->h_addrtype, *ptrTemp, ip_tour[count_tour], sizeof(ip_tour[count_tour]));
		}
		
		source_work(ip_tour, num_tour);
		return 1;
	}
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

void source_work(char ip_tour[][SIZE_IP], int num_tour)
{
	int count_vm;

	int rt_sockfd;

	rt_sockfd=Socket(PF_INET, SOCK_RAW, RT_PROTOCOL);
	int on=1;
	setsockopt(rt_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	int ip_len=SIZE_FIXSOURCE+(num_tour*SIZE_IP);
	
    void *sendline=(void *)Malloc(ip_len);
	bzero(sendline, sizeof(*sendline));

	struct sockaddr_in destAddr;
	destAddr.sin_family=AF_INET;
	destAddr.sin_port=htons(25);
	destAddr.sin_addr.s_addr=inet_addr(ip_tour[0]);
	
	createRTHeader(sendline, ip_len, host_ip, &destAddr);

	writeRTNUM(sendline, num_tour);
	
	int current_tour=1;
	writeRTCurrent(sendline, current_tour);

	int firstCheck[10];
	for(count_vm=0;count_vm<10;count_vm++)
	{
		firstCheck[count_vm]=0;
	}
	writeRTFirstCheck(sendline, firstCheck);

	writeRTSource(sendline, host_ip);

	writeRTTour(sendline, ip_tour, num_tour);

	Sendto(rt_sockfd, sendline, ip_len, 0, (SA *)&destAddr, sizeof(destAddr));
	
	int udp_recvfd;
	udp_recvfd=Socket(AF_INET, SOCK_DGRAM, 0);
	Setsockopt(udp_recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	mcast_set_ttl(udp_recvfd, 1);
	
	int udp_sendfd;
	udp_sendfd=Socket(AF_INET, SOCK_DGRAM, 0);
	Setsockopt(udp_sendfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	mcast_set_ttl(udp_sendfd, 1);
	
	struct sockaddr_in groupAddr;
	bzero(&groupAddr, sizeof(groupAddr));
	groupAddr.sin_family=AF_INET;
	groupAddr.sin_port=htons(MULTIPORT);
	Inet_pton(AF_INET, MULTIADDR, &groupAddr.sin_addr);
				
	Bind(udp_recvfd, (SA *)&groupAddr, sizeof(groupAddr));
	Bind(udp_sendfd, (SA *)&groupAddr, sizeof(groupAddr));
				
	Mcast_join(udp_recvfd, (SA *)&groupAddr, sizeof(groupAddr), NULL, 0);
	
	for( ; ; )
	{
		char recvline[MAXLINE];
		Recvfrom(udp_recvfd, recvline, MAXLINE, 0, NULL, NULL);
		printf("Node vm%d. Received : %s\n", getVMNum(host_ip), recvline);
		
		if(num_message==0)
		{
			num_message=1;
		}
		
		if(num_message==1)
		{
			char *sendline=(char *)Malloc(MAXLINE);
			bzero(sendline, MAXLINE);
			sprintf(sendline, "<<<<< Node vm%d. I am a member of the group >>>>>", getVMNum(host_ip)+1);
			Sendto(udp_sendfd, sendline, strlen(sendline), 0, (SA *)&groupAddr, sizeof(groupAddr));
			
			printf("Node vm%d. Sending: %s\n", getVMNum(host_ip)+1, sendline);
			
			num_message=2;
			
			Signal(SIGALRM, exit_alrm);
				
			alarm(5);
		}
	}
}

void middle_work()
{
	int rt_sockfd;
	rt_sockfd=Socket(PF_INET, SOCK_RAW, RT_PROTOCOL);
	int on=1;
	setsockopt(rt_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	//int pf_sockfd;
	pf_sockfd=Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));

	int pg_sockfd;
	pg_sockfd=Socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	on=1;
	setsockopt(rt_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	
	struct sockaddr_in groupAddr;
	
	int udp_recvfd;
	udp_recvfd=Socket(AF_INET, SOCK_DGRAM, 0);
	Setsockopt(udp_recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	mcast_set_ttl(udp_recvfd, 1);
	
	int udp_sendfd;
	udp_sendfd=Socket(AF_INET, SOCK_DGRAM, 0);
	Setsockopt(udp_sendfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	mcast_set_ttl(udp_sendfd, 1);
	
	fd_set rset;
	int maxfdp1;

	FD_ZERO(&rset);
	int select_result;

	for( ; ; )
	{
		FD_ZERO(&rset);
		FD_SET(rt_sockfd, &rset);
		FD_SET(pg_sockfd, &rset);
		FD_SET(udp_recvfd, &rset);
		
		maxfdp1=max(max(rt_sockfd, pg_sockfd), udp_recvfd)+1;

		select_result=select(maxfdp1, &rset, NULL, NULL, NULL);
	
		//if(select_result<0)
		//{
		//	continue;
		//}
		
		if(change_select==1)
		{
			change_select=0;
			continue;
		}
		
		if(FD_ISSET(rt_sockfd, &rset))
		{
			//receive tour message
			char recvline[SIZE_BUF];
			
			Recvfrom(rt_sockfd, recvline, SIZE_BUF, 0, NULL, NULL);

			struct ip *ipheader=(struct ip *)recvline;
			
			//chack identification
			if(ipheader->ip_id!=IP_IDENTIFICATION)
			{
				continue;
			}
			
			char ip_pre[SIZE_IP];
			getRTPreIP(recvline, ip_pre);

			time_t ticks;
			ticks=time(NULL);

			printf("<%.24s> received source routing packet from <vm%d>\n", ctime(&ticks), getVMNum(ip_pre)+1);

			int is_first=getRTCheckFirst(recvline);
			int num_tour=getRTNUM(recvline);
			int current_tour=getRTCurrent(recvline);
			char ip_next[SIZE_IP];
			
			char ip_source[SIZE_IP];
			getRTSourceIP(recvline, ip_source);
			sprintf(ip_ping, "%s", ip_source);
			//char ip_tour[num_tour][SIZE_IP];
			//int firstList[10];

			//getRTIPTour(recvline, ip_tour);
			//getRTCheckFirstList(recvline, firstList);
			
			//printf("%d-%d-%d-%d-%s\n", ipheader->ip_id, is_first, num_tour, current_tour, ip_next);
			int first_end=0;
			if(is_first==0)
			{
				//printf("is first!!!!!!!!!\n");
				setRTCheckFirst(recvline);
				//start to pinpg
				onPing=1;
				first_end=1;
				Signal(SIGALRM, ping_alrm);
				printf("PING vm%d (%s): 56 data bytes\n", getVMNum(ip_ping)+1, ip_ping);
				ping_alrm(SIGALRM);
				//printf("first time\n");
				//ping(ip_ping);
				//add to multicast
				//struct sockaddr_in groupAddr;
				bzero(&groupAddr, sizeof(groupAddr));
				groupAddr.sin_family=AF_INET;
				groupAddr.sin_port=htons(MULTIPORT);
				Inet_pton(AF_INET, MULTIADDR, &groupAddr.sin_addr);
				
				Bind(udp_recvfd, (SA *)&groupAddr, sizeof(groupAddr));
				Bind(udp_sendfd, (SA *)&groupAddr, sizeof(groupAddr));
				
				Mcast_join(udp_recvfd, (SA *)&groupAddr, sizeof(groupAddr), NULL, 0);
			}

			if(current_tour==num_tour)
			{
				//this is the final UDP multicast
				if(first_end==1)
				{
					check_first_end=1;
				}
				else
				{
					//wait 5 second
					printf("wait five seconds\n");
					onPing=0;
					//printf("sleeping....\n");
					sleep(5);
					//printf("wake uo\n");
					sleep(5);

					printf("Send out message\n");
					char *endMsg=(char *)Malloc(MAXLINE);
					bzero(endMsg, MAXLINE);
					sprintf(endMsg, "<<<<< Thus is node vm%d. Tour has ended. Group members please identify yourselves. >>>>>", getVMNum(host_ip)+1);
					Sendto(udp_sendfd, endMsg, strlen(endMsg), 0, (SA *)&groupAddr, sizeof(groupAddr));
					printf("Node vm%d. Sending : %s\n", getVMNum(host_ip)+1, endMsg);
				}
			}

			first_end=0;

			if(current_tour<num_tour)
			{
				//forward it
				getRTNextIP(recvline, ip_next);
					
				struct sockaddr_in destAddr;
				destAddr.sin_family=AF_INET;
				destAddr.sin_port=htons(25);
				destAddr.sin_addr.s_addr=inet_addr(ip_next);
				
				ipheader->ip_src.s_addr=inet_addr(host_ip);
				ipheader->ip_dst.s_addr=destAddr.sin_addr.s_addr;
				ipheader->ip_len=ntohs(ipheader->ip_len);

				writeRTCurrent(recvline, current_tour+1);

				//printf("%d\n", ipheader->ip_len);
				//printf("%d\n", ipheader->ip_id);
				Sendto(rt_sockfd, recvline, ipheader->ip_len, 0, (SA *)&destAddr, sizeof(destAddr));

				ticks=time(NULL);
				printf("<%.24s> send souce routing packet to <vm%d>\n", ctime(&ticks), getVMNum(ip_next)+1);
			}
		}
		if(FD_ISSET(pg_sockfd, &rset))
		{
			char recvline[MAXLINE];
			Recvfrom(pg_sockfd, recvline, MAXLINE, 0, NULL, NULL);
			//printf("receive ping!####\n");
			
			struct ip *ipheader;
			ipheader=(struct ip *)(recvline);
			
			struct icmphdr *icmphd;
			icmphd=(struct icmphdr *)(recvline+sizeof(struct ip));

			//printf("reply type recved: %d\n", icmphd->un.echo.id);
			
			if(icmphd->type!=ICMP_ECHOREPLY)
			{
				//printf("No reply type\n");
				continue;
			}
			if(icmphd->un.echo.id!=(short)(getpid()&0xffff))
			{
				//printf("Not my reply\n");
				continue;
			}
			
			struct timeval *recvtime=(struct timeval *)Malloc(sizeof(struct timeval));
			Gettimeofday(recvtime, NULL);
			
			struct timeval *sendtime;
			sendtime=(struct timeval *)(recvline+sizeof(struct ip)+sizeof(struct icmphdr));
			
			tv_sub(recvtime, sendtime);
			double rtt=recvtime->tv_sec*1000.0+recvtime->tv_usec/1000.0;
			printf("## %d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n", 64, ip_ping, icmphd->un.echo.sequence, ipheader->ip_ttl, rtt);

			if(check_first_end==1)
			{
				num_first_end++;
			}
			if(num_first_end==5)
			{
				onPing=0;
				//printf("Send out Message!!!!\n");
				
				char *endMsg=(char *)Malloc(MAXLINE);
				bzero(endMsg, MAXLINE);
				sprintf(endMsg, "<<<<< Thus is node vm%d. Tour has ended. Group members please identify yourselves. >>>>>\0", getVMNum(host_ip)+1);
				Sendto(udp_sendfd, endMsg, MAXLINE, 0, (SA *)&groupAddr, sizeof(groupAddr));
				printf("Node vm%d. Sending : %s\n", getVMNum(host_ip)+1, endMsg);
				
				bzero(endMsg, MAXLINE);
				
				check_first_end=0;
				num_first_end=0;
			}
		}
		if(FD_ISSET(udp_recvfd, &rset))
		{
			char *recvline=(char *)Malloc(MAXLINE);
			bzero(recvline, 0);
			Recvfrom(udp_recvfd, recvline, MAXLINE, 0, NULL, NULL);
			
			printf("Node vm%d. Received : %s\n", getVMNum(host_ip)+1, recvline);
			bzero(recvline, 0);
			
			onPing=0;
			
			if(num_message==0)
			{
			num_message=1;
			}
		
			if(num_message==1)
			{
				char *sendline=(char *)Malloc(MAXLINE);
				bzero(sendline, MAXLINE);
				sprintf(sendline, "<<<<< Node vm%d. I am a member of the group >>>>>\0", getVMNum(host_ip)+1);
				Sendto(udp_sendfd, sendline, MAXLINE, 0, (SA *)&groupAddr, sizeof(groupAddr));
			
				printf("Node vm%d. Sending: %s\n", getVMNum(host_ip)+1, sendline);
				bzero(sendline, 0);
			
				num_message=2;
				
				Signal(SIGALRM, exit_alrm);
				
				alarm(5);
			}
		}
	}
}

void createRTHeader(void *sendline, int ip_len, char *ip_src, struct sockaddr_in *destAddr)
{
	struct ip *ipheader;
	ipheader=(struct ip *)sendline;
	memset(sendline, 0, 20);

	ipheader->ip_hl=5;
	ipheader->ip_v=4;
	ipheader->ip_tos=0;
	ipheader->ip_len=ip_len;
	ipheader->ip_id=IP_IDENTIFICATION;
	ipheader->ip_off=0;
	ipheader->ip_ttl=255;
	ipheader->ip_p=RT_PROTOCOL;
	ipheader->ip_sum=0;
	ipheader->ip_src.s_addr=inet_addr(ip_src);
	ipheader->ip_dst.s_addr=destAddr->sin_addr.s_addr;
}

void writeRTNUM(void *sendline, int num)
{
	memcpy((void *)(sendline+sizeof(struct ip)), (void *)&num, sizeof(int));
}

void writeRTCurrent(void *sendline, int current)
{
	memcpy((void *)(sendline+sizeof(struct ip)+sizeof(int)), (void *)&current, sizeof(int));
}

void writeRTFirstCheck(void *sendline, int *firstCheck)
{
	memcpy((void *)(sendline+sizeof(struct ip)+sizeof(int)*2), (void *)firstCheck, sizeof(int)*10);
}

void writeRTSource(void *sendline, char *ip_src)
{
	memcpy((void *)(sendline+sizeof(struct ip)+sizeof(int)*12), (void *)ip_src, SIZE_IP);
}

void writeRTTour(void *sendline, char ip_tour[][SIZE_IP], int num_tour)
{
	int count_tour;

	for(count_tour=0;count_tour<num_tour;count_tour++)
	{
		memcpy((void *)(sendline+sizeof(struct ip)+sizeof(int)*12+SIZE_IP+count_tour*SIZE_IP), (void *)ip_tour[count_tour], SIZE_IP);
	}
}

int getRTNUM(void *recvline)
{
	int num_tour=*(int *)(recvline+sizeof(struct ip));
	return num_tour;
}

int getRTCurrent(void *recvline)
{
	int current_tour=*(int *)(recvline+sizeof(struct ip)+sizeof(int));
	return current_tour;
}

int getRTCheckFirst(void *recvline)
{
	int result;
	result=*(int *)(recvline+sizeof(struct ip)+sizeof(int)*2+sizeof(int)*getVMNum(host_ip));
	return result;
}

void setRTCheckFirst(void *recvline)
{
	int vm_id=getVMNum(host_ip);
	int result=1;
	memcpy((void *)(recvline+sizeof(struct ip)+sizeof(int)*2+sizeof(int)*vm_id), (void *)&result, sizeof(int));
}

void getRTNextIP(void *recvline, char *ip_next)
{
	int current_tour=getRTCurrent(recvline);
	memcpy((void *)ip_next, (void *)(recvline+sizeof(struct ip)+sizeof(int)*12+SIZE_IP+SIZE_IP*current_tour), SIZE_IP);
}

void getRTSourceIP(void *recvline, char *ip_source)
{
	memcpy((void *)ip_source, (void *)(recvline+sizeof(struct ip)+sizeof(int)*12), SIZE_IP);
}

void getRTPreIP(void *recvline, char *ip_pre)
{
	int current_tour=getRTCurrent(recvline);
	if(current_tour==1)
	{
		getRTSourceIP(recvline, ip_pre);
	}
	else
	{
		memcpy((void *)ip_pre, (void *)(recvline+sizeof(struct ip)+sizeof(int)*12+SIZE_IP+SIZE_IP*(current_tour-2)), SIZE_IP);
	}
}

void getRTIPTour(void *recvline, char ip_tour[][SIZE_IP])
{
	int count_tour;
	
	int num_tour=getRTNUM(recvline);

	for(count_tour=0;count_tour<num_tour;count_tour++)
	{
		memcpy((void *)ip_tour[count_tour], (void *)(recvline+sizeof(struct ip)+sizeof(int)*12+SIZE_IP+SIZE_IP*count_tour), SIZE_IP);
		//printf("%s\n", ip_tour[count_tour]);
	}
}

void getRTCheckFirstList(void *recvline, int *checkFirst)
{
	int count_vm;

	for(count_vm=0;count_vm<10;count_vm++)
	{
		checkFirst[count_vm]=*(int *)(recvline+sizeof(struct ip)+sizeof(int)*2+sizeof(int)*count_vm);
		//printf("%d\n", checkFirst[count_vm]);
	}
}

void ping(char *ip_addr)
{
	int count_mac;
	struct hwaddr *pingHWaddr=(struct hwaddr *)Malloc(sizeof(struct hwaddr));
	
	int re=areq(ip_addr, pingHWaddr);
	
	if(re==0)
	{
		printf("Connection in ARP Module Closed, can not check ARP!\n");
		onPing=0;
		return;
	}
	
	if(re==2)
	{
		printf("Call for areq timeout for 3 seconds!\n");
		return;
	}
	
	printf("!!Call areq at Node vm%d for IP %s, result is [ ", getVMNum(host_ip)+1, ip_addr);
	
	printf("hwaddr:");
	for(count_mac=0;count_mac<6;count_mac++)
	{
		printf("%.2x", pingHWaddr->sll_addr[count_mac] & 0xff);
		if(count_mac!=5)
		{
			printf(":");
		}
	}
	printf(",index:2, halen:6");
	
	printf(" ]\n");
	
	//for(count_mac=0;count_mac<6;count_mac++)
	//{
	//	printf("%.2x:", pingHWaddr->sll_addr[count_mac] & 0xff);
	//}
	//printf("\n");
	
	void *sendbuf=(void *)Malloc(SIZE_BUF);

	struct sockaddr_ll pingaddr;
	bzero(&pingaddr, sizeof(pingaddr));
	pingaddr.sll_family=AF_PACKET;
	pingaddr.sll_protocol=htons(ETH_P_IP);
	pingaddr.sll_halen=6;
	pingaddr.sll_ifindex=2;

	pingaddr.sll_addr[0]=pingHWaddr->sll_addr[0];
	pingaddr.sll_addr[1]=pingHWaddr->sll_addr[1];
	pingaddr.sll_addr[2]=pingHWaddr->sll_addr[2];
	pingaddr.sll_addr[3]=pingHWaddr->sll_addr[3];
	pingaddr.sll_addr[4]=pingHWaddr->sll_addr[4];
	pingaddr.sll_addr[5]=pingHWaddr->sll_addr[5];
	pingaddr.sll_addr[6]=0x00;
	pingaddr.sll_addr[7]=0x00;

	unsigned short f_protocol=htons(ETH_P_IP);
	
	struct sockaddr_in destAddr;
	destAddr.sin_family=AF_INET;
	destAddr.sin_port=htons(25);
	destAddr.sin_addr.s_addr=inet_addr(ip_addr);

	struct ip *ipheader;
	ipheader=(struct ip *)(sendbuf+14);
	memset(sendbuf, 0, SIZE_BUF);
	
	ipheader->ip_hl=5;
	ipheader->ip_v=4;
	ipheader->ip_tos=0;
	ipheader->ip_len=htons(sizeof(struct ip)+sizeof(struct icmphdr)+56);
	ipheader->ip_id=htons(10773);
	ipheader->ip_off=0;
	ipheader->ip_ttl=255;
	ipheader->ip_p=IPPROTO_ICMP;
	ipheader->ip_sum=0;
	ipheader->ip_src.s_addr=inet_addr(host_ip);
	ipheader->ip_dst.s_addr=destAddr.sin_addr.s_addr;
	
	struct icmphdr *icmphd;
	icmphd=(struct icmphdr *)(sendbuf+14+sizeof(struct ip));
	
	icmphd->type=ICMP_ECHO;
	icmphd->code=0;
	icmphd->un.echo.id=(short)(getpid()&0xffff);
	//printf("icmp id sent: %d\n", icmphd->un.echo.id);
	icmphd->un.echo.sequence=++sequence;
	
	Gettimeofday((struct timeval *)(sendbuf+14+sizeof(struct ip)+sizeof(struct icmphdr)), NULL);
	
	icmphd->checksum=0;
	icmphd->checksum=in_cksum((unsigned short *)icmphd, 64);

	ipheader->ip_sum=in_cksum((unsigned short *)ipheader, 20);
	
	memcpy((void *)sendbuf, (void *)(pingHWaddr->sll_addr), 6);
	memcpy((void *)(sendbuf+6), (void *)host_mac, 6);
	memcpy((void *)(sendbuf+12), (void *)&f_protocol, 2);
	
	Sendto(pf_sockfd, sendbuf, 84+14, 0, (SA *)&pingaddr, sizeof(pingaddr));

}

int areq(char *ip_addr, struct hwaddr *HWaddr)
{
	int udSockfd;

	udSockfd=Socket(AF_LOCAL, SOCK_STREAM, 0);

	struct sockaddr_un servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family=AF_LOCAL;
	strcpy(servaddr.sun_path, ARP_WELPATH);

	if(connect(udSockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
	{
		return 0;
	}

	Writen(udSockfd, ip_addr, SIZE_IP);
	
	int maxfdp1;
	fd_set rset;
	FD_ZERO(&rset);
	
	int sere;
	
	FD_SET(udSockfd, &rset);
	maxfdp1=udSockfd+1;
	
	struct timeval readtime;
	readtime.tv_sec=3;
	readtime.tv_usec=0;
	
	sere=select(maxfdp1, &rset, NULL, NULL, &readtime);
	
	char recvline[MAXLINE];
	
	if(FD_ISSET(udSockfd, &rset))
	{
		if(Readline(udSockfd, recvline, MAXLINE)==0)
		{
			return 2;
		}
	}
	
	if(sere==0)
	{
		Close(udSockfd);
		return 2;
	}
	struct hwaddr *reHWaddr=(struct hwaddr *)recvline;
	memcpy(HWaddr, reHWaddr, sizeof(struct hwaddr));
	
	Close(udSockfd);
	return 1;
}

void ping_alrm(int signo)
{
	change_select=1;
	if(onPing==1)
	{
		ping(ip_ping);
		alarm(1);
	}

	return;
}

void exit_alrm(int signo)
{
	printf("############ Application End(5 Sec After) ############\n");
	exit(0);
}
