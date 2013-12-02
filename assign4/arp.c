/** File:		arp.c
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
void printCache();

char host_ip[SIZE_IP];
unsigned char host_mac[6];
unsigned char broad_mac[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
struct arp_pair macTable[10];
char ip_array[10][SIZE_IP];
struct arp_cache cache[10];
unsigned short arpProtocol;

int main(int argc, char **argv)
{
	
	getAllIP();

	int count;
	int count_if=0;
	int count_vm;
	int num_interface=0;

	arpProtocol=htons(ARP_PROTOCOL);
	
	struct hwa_info *hwa;
	struct hwa_info *hwahead;
	struct sockaddr *sa;
	char *ptr;
	int i;
	int prflag;
	
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
		}
	}
	
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(strcmp(hwa->if_name, "eth0")==0 || (hwa->ip_alias)==IP_ALIAS)
		{
			num_interface++;
		}
		
	}
	
	//after we get the number of interface 
	struct arp_pair pairs[num_interface];
	int alias[num_interface];
	printf("\n");

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(strcmp(hwa->if_name, "eth0")!=0 && (hwa->ip_alias)!=IP_ALIAS)
		{
			continue;
		}

		if((hwa->ip_alias)==IP_ALIAS)
		{
			alias[count_if]=1;
		}
		
		if ( (sa = hwa->ip_addr) != NULL)
		{
			sprintf(pairs[count_if].ipaddr, "%s", Sock_ntop_host(sa, sizeof(*sa)));
		}

		prflag = 0;
		i = 0;
		
		for(count=0;count<6;count++)
		{
			pairs[count_if].hwaddr[count]=hwa->if_haddr[count];	
		}
		
		do 
		{
			if (hwa->if_haddr[i] != '\0') 
			{
				prflag = 1;
				break;
			}
			
		} while (++i < IF_HADDR);
		
		count_if++;
	}

	free_hwa_info(hwahead);

	printf("############ Matching pair ############\n");
	for(count_if=0;count_if<num_interface;count_if++)
	{
		printf("\n");
		printf("  < %s , ", pairs[count_if].ipaddr);
		
		for(count=0;count<6;count++)
		{
			printf("%.2x", pairs[count_if].hwaddr[count] & 0xff);

			if(count!=5)
			{
				printf(":");
			}
		}
		printf(" >");
		if(alias[count_if]==1)
		{
			printf(" ALIAS Interface");
		}
		printf("\n");
	}
	printf("\n");
	printf("#######################################\n");
	
	for(count_vm=0;count_vm<10;count_vm++)
	{
		cache[count_vm].status=0;
		cache[count_vm].waiting=0;
	}
	
	//PF_PACKET socket
	int pfSockfd;
	pfSockfd=Socket(PF_PACKET, SOCK_RAW, htons(ARP_PROTOCOL));
	
	struct sockaddr_ll bindAddr;
	bzero(&bindAddr, sizeof(bindAddr));
	bindAddr.sll_family=AF_PACKET;
	bindAddr.sll_protocol=htons(ARP_PROTOCOL);
	bindAddr.sll_halen=6;
	bindAddr.sll_ifindex=2;

	Bind(pfSockfd, (SA *)&bindAddr, sizeof(bindAddr));
	
	//Unix Domain Socket
	int udSockfd;
	udSockfd=Socket(AF_LOCAL, SOCK_STREAM, 0);
	
	struct sockaddr_un servaddr;
	unlink(ARP_WELPATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family=AF_LOCAL;
	strcpy(servaddr.sun_path, ARP_WELPATH);
	
	Bind(udSockfd, (SA *)&servaddr, sizeof(servaddr));
	Listen(udSockfd, 100);
	
	fd_set rset;
	int maxfdp1;

	FD_ZERO(&rset);
	
	for( ; ; )
	{
		FD_ZERO(&rset);
		
		FD_SET(pfSockfd, &rset);
		FD_SET(udSockfd, &rset);
	
		maxfdp1=max(pfSockfd, udSockfd)+1;
		int n;
		n=select(maxfdp1, &rset, NULL, NULL, NULL);
		
		if(n<0)
		{
			continue;
		}
		
		if(FD_ISSET(pfSockfd, &rset))
		{
			void *recvline=(void *)Malloc(14+sizeof(struct arpMes));
			int n=Recvfrom(pfSockfd, recvline, 14+sizeof(struct arpMes), 0, NULL, NULL);
			
			struct arpMes *arpmsg=(struct arpMes *)(recvline+14);
			
			int identification=arpmsg->identification;
			
			if(identification!=ARP_IDENTIFICATION)
			{
				continue;
			}
			
			int arpType=arpmsg->op;
			
			//request
			if(arpType==1)
			{
				if(getVMNum(host_ip)==getVMNum(arpmsg->target_ip))
				{
					printf("ARP at Node vm%d received ", getVMNum(host_ip)+1);
					printf("[");
					printf("Mac_DST:");
					int count_mac;
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpmsg->target_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Mac_SRC:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpmsg->sender_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					
					printf(",Protocl:%u", ntohs(*(unsigned short*)(recvline+12)));
					printf(",Type:req");
					printf(",Sender_Eth:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpmsg->sender_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Sender_IP:%s", arpmsg->sender_ip);
					printf(",Target_Eth:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpmsg->target_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Target_IP:%s", arpmsg->target_ip);
					printf("]\n");
					
					//Store and update cache
					int vm_id=getVMNum(arpmsg->sender_ip);
					sprintf(cache[vm_id].ipaddr, "%s", arpmsg->sender_ip);
					for(count_mac=0;count_mac<6;count_mac++)
					{
						cache[vm_id].hwaddr[count_mac]=arpmsg->sender_mac[count_mac];
					}
					cache[vm_id].sll_ifindex=2;
					cache[vm_id].status=1;
					
					printCache();
					
					//printf("prepare reply\n");
					//it is for me
					int ppro=htons(ARP_PROTOCOL);
					void *sendline=(void *)Malloc(14+sizeof(struct arpMes));
					bzero(sendline, sizeof(*sendline));
					memcpy((void *)sendline, (void *)(arpmsg->sender_mac), 6);
					memcpy((void *)(sendline+6), (void *)host_mac, 6);
					memcpy((void *)(sendline+12), (void *)&ppro, 2);
					
					struct arpMes *arpRep=(struct arpMes *)(sendline+14);
					arpRep->op=2;
					memcpy((void *)(arpRep->sender_mac), (void *)host_mac, 6);
					memcpy((void *)(arpRep->sender_ip), (void *)host_ip, SIZE_IP);
					memcpy((void *)(arpRep->target_mac), (void *)(arpmsg->sender_mac), 6);
					memcpy((void *)(arpRep->target_ip), (void *)(arpmsg->sender_ip), SIZE_IP);
					arpRep->identification=ARP_IDENTIFICATION;
					
					struct sockaddr_ll uniAddr;
					bzero(&uniAddr, sizeof(uniAddr));
					uniAddr.sll_family=AF_PACKET;
					uniAddr.sll_protocol=htons(ARP_PROTOCOL);
					uniAddr.sll_halen=6;
					uniAddr.sll_ifindex=2;
					uniAddr.sll_addr[0]=arpmsg->sender_mac[0];
					uniAddr.sll_addr[1]=arpmsg->sender_mac[1];
					uniAddr.sll_addr[2]=arpmsg->sender_mac[2];
					uniAddr.sll_addr[3]=arpmsg->sender_mac[3];
					uniAddr.sll_addr[4]=arpmsg->sender_mac[4];
					uniAddr.sll_addr[5]=arpmsg->sender_mac[5];
					uniAddr.sll_addr[6]=0x00;
					uniAddr.sll_addr[7]=0x00;
					
					Sendto(pfSockfd, sendline, 14+sizeof(struct arpMes), 0, (SA *)&uniAddr, sizeof(uniAddr));
					
					printf("ARP at Node vm%d Sent ", getVMNum(host_ip)+1);
					printf("[");
					printf("Mac_DST:");
					count_mac;
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpmsg->sender_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Mac_SRC:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", host_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					
					printf(",Protocl:%u", ntohs(*(unsigned short*)(sendline+12)));
					printf(",Type:rep");
					printf(",Sender_Eth:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpRep->sender_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Sender_IP:%s", arpRep->sender_ip);
					printf(",Target_Eth:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", arpRep->target_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(",Target_IP:%s", arpRep->target_ip);
					printf("]\n");
					continue;
				}
				else
				{
					int count_mac;
					//Store and update cache
					int vm_id=getVMNum(arpmsg->sender_ip);
					if(cache[vm_id].status==1)
					{
						sprintf(cache[vm_id].ipaddr, "%s", arpmsg->sender_ip);
						for(count_mac=0;count_mac<6;count_mac++)
						{
							cache[vm_id].hwaddr[count_mac]=arpmsg->sender_mac[count_mac];
						}
						cache[vm_id].sll_ifindex=2;
						
						printCache();
					}
					continue;
				}
			}
			//reply
			if(arpType==2)
			{
				int vm_id=getVMNum(arpmsg->sender_ip);
				
				int count_mac;
				
				printf("ARP at Node vm%d received ", getVMNum(host_ip)+1);
				printf("[");
				printf("Mac_DST:");
				count_mac;
				for(count_mac=0;count_mac<6;count_mac++)
				{
					printf("%.2x", arpmsg->target_mac[count_mac] & 0xff);
					if(count_mac!=5)
					{
						printf(":");
					}
				}
				printf(",Mac_SRC:");
				for(count_mac=0;count_mac<6;count_mac++)
				{
					printf("%.2x", arpmsg->sender_mac[count_mac] & 0xff);
					if(count_mac!=5)
					{
						printf(":");
					}
				}
					
				printf(",Protocl:%u", ntohs(*(unsigned short*)(recvline+12)));
				printf(",Type:rep");
				printf(",Sender_Eth:");
				for(count_mac=0;count_mac<6;count_mac++)
				{
					printf("%.2x", arpmsg->sender_mac[count_mac] & 0xff);
					if(count_mac!=5)
					{
						printf(":");
					}
				}
				printf(",Sender_IP:%s", arpmsg->sender_ip);
				printf(",Target_Eth:");
				for(count_mac=0;count_mac<6;count_mac++)
				{
					printf("%.2x", arpmsg->target_mac[count_mac] & 0xff);
					if(count_mac!=5)
					{
						printf(":");
					}
				}
				printf(",Target_IP:%s", arpmsg->target_ip);
				printf("]\n");
				
				if(cache[vm_id].waiting!=1)
				{
					continue;
				}
				memcpy((void *)(cache[vm_id].hwaddr), (void *)(arpmsg->sender_mac), 6);
				
				void *reline=(void *)Malloc(sizeof(struct hwaddr));
				struct hwaddr *ptrHWaddr=(struct hwaddr *)reline;
				
				ptrHWaddr->sll_ifindex=2;
				
				for(count_mac=0;count_mac<6;count_mac++)
				{
					ptrHWaddr->sll_addr[count_mac]=cache[vm_id].hwaddr[count_mac];
				}
				Writen(cache[vm_id].sockfd, reline, sizeof(struct hwaddr));
				Close(cache[vm_id].sockfd);
				cache[vm_id].waiting=0;
				cache[vm_id].status=1;
				
				printCache();
				
				continue;
			}
		}
		
		if(FD_ISSET(udSockfd, &rset))
		{
			int udConfd;
			udConfd=accept(udSockfd, NULL, NULL);
			
			char ip_query[SIZE_IP];
			
			int n=Read(udConfd, ip_query, MAXLINE);
			if(n==0)
			{
				Close(udConfd);
				continue;
			}
			ip_query[n]=0;
			printf("\nTour at vm%d ask for %s\n", getVMNum(host_ip)+1, ip_query);
			
			//if known 
			int vm_id=getVMNum(ip_query);
			printCache();
			
			if(cache[vm_id].status==1)
			{
				printf("Relavent ARP entry is in cache\n");
				void *reline=(void *)Malloc(sizeof(struct hwaddr));
				struct hwaddr *ptrHWaddr=(struct hwaddr *)reline;
				ptrHWaddr->sll_ifindex=2;
				//memcpy((void *)(ptrHWaddr->sll_addr), (void *)(cache[vm_id].hwaddr), 6);
				int count_mac;
				for(count_mac=0;count_mac<6;count_mac++)
				{
					ptrHWaddr->sll_addr[count_mac]=cache[vm_id].hwaddr[count_mac];
				}
				Writen(udConfd, reline, sizeof(struct hwaddr));
				//printf("Reply for Tour\n");
				Close(udConfd);
				continue;
			}
			printf("No entry in Cache. Send request on network\n");
			//if not known
			int ppro=htons(ARP_PROTOCOL);
			void *sendline=(void *)Malloc(14+sizeof(struct arpMes));
			bzero(sendline, sizeof(*sendline));
			memcpy((void *)sendline, (void *)broad_mac, 6);
			memcpy((void *)(sendline+6), (void *)host_mac, 6);
			memcpy((void *)(sendline+12), (void *)&ppro, 2);
			
			struct arpMes *arpReq=(struct arpMes *)(sendline+14);
			arpReq->op=1;
			memcpy((void *)(arpReq->sender_mac), (void *)host_mac, 6);
			memcpy((void *)(arpReq->sender_ip), (void *)host_ip, SIZE_IP);
			memcpy((void *)(arpReq->target_mac), (void *)broad_mac, 6);
			memcpy((void *)(arpReq->target_ip), (void *)ip_query, SIZE_IP);
			arpReq->identification=ARP_IDENTIFICATION;
			
			struct sockaddr_ll broadAddr;
			bzero(&broadAddr, sizeof(broadAddr));
			broadAddr.sll_family=AF_PACKET;
			broadAddr.sll_protocol=htons(ARP_PROTOCOL);
			broadAddr.sll_halen=6;
			broadAddr.sll_ifindex=2;
			broadAddr.sll_addr[0]=0xff;
			broadAddr.sll_addr[1]=0xff;
			broadAddr.sll_addr[2]=0xff;
			broadAddr.sll_addr[3]=0xff;
			broadAddr.sll_addr[4]=0xff;
			broadAddr.sll_addr[5]=0xff;
			broadAddr.sll_addr[6]=0x00;
			broadAddr.sll_addr[7]=0x00;
			
			Sendto(pfSockfd, sendline, 14+sizeof(struct arpMes), 0, (SA *)&broadAddr, sizeof(broadAddr));
			
			printf("ARP at Node vm%d Sent ", getVMNum(host_ip)+1);
			printf("[");
			printf("Mac_DST:");
			int count_mac;
			for(count_mac=0;count_mac<6;count_mac++)
			{
				printf("%.2x", broad_mac[count_mac] & 0xff);
				if(count_mac!=5)
				{
					printf(":");
				}
			}
			printf(",Mac_SRC:");
			for(count_mac=0;count_mac<6;count_mac++)
			{
				printf("%.2x", host_mac[count_mac] & 0xff);
				if(count_mac!=5)
				{
					printf(":");
				}
			}
					
			printf(",Protocl:%u", ntohs(*(unsigned short*)(sendline+12)));
			printf(",Type:req");
			printf(",Sender_Eth:");
			for(count_mac=0;count_mac<6;count_mac++)
			{
				printf("%.2x", arpReq->sender_mac[count_mac] & 0xff);
				if(count_mac!=5)
				{
					printf(":");
				}
			}
			printf(",Sender_IP:%s", arpReq->sender_ip);
			printf(",Target_Eth:");
			for(count_mac=0;count_mac<6;count_mac++)
			{
				printf("%.2x", arpReq->target_mac[count_mac] & 0xff);
				if(count_mac!=5)
				{
					printf(":");
				}
			}
			printf(",Target_IP:%s", arpReq->target_ip);
			printf("]\n");

			sprintf(cache[vm_id].ipaddr, "%s", ip_query);
			cache[vm_id].sll_ifindex=2;
			cache[vm_id].sockfd=udConfd;
			cache[vm_id].status=0;
			cache[vm_id].waiting=1;
			
			//char sendline[MAXLINE];
			//sprintf(sendline,"%s","hello\0");
			//Writen(udConfd, sendline, MAXLINE);
			
			//Close(udConfd);
			
			continue;
		}
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

void printCache()
{
	int count;
	int count_mac;
	printf("\n############ ARP Cache ###############\n");
	for(count=0;count<10;count++)
	{
		if(cache[count].status==1)
		{
			printf("[ %s, ", cache[count].ipaddr);
			for(count_mac=0;count_mac<6;count_mac++)
			{
				printf("%.2x", cache[count].hwaddr[count_mac] & 0xff);
				if(count_mac!=5)
				{
					printf(":");
				}
			}
			printf(" ]\n");
		}
	}
	printf("######################################\n\n");
}
