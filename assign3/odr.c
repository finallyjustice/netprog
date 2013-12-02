/** File:		odr.c
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

unsigned short entry_findPath(char *path_name, struct head_path_port *entry_head);
unsigned short entry_insertPath(char *path_name, int isperm, struct head_path_port *entry_head);
char *getPathByPort(unsigned short port, struct head_path_port *entry_head);
int route_findRoute(char *ip_dest);
void route_updateRoute(char *ip_dest, char *next_hop, int out_ifindex, int hop_cnt, int broadId);
void route_insertRoute(char *ip_dest, char *next_hop, int out_ifindex, int hop_cnt, int broadId);
void printRoutingTable();
unsigned short getRand();
int getBroadcastID();
int getTime();
int getVMNum(char *ip_addr);
void getAllIP();
void insertQueue(int type, int destVM, char *recvline, int len);
int printNumQueue();

int staleness;
void *recv_packet;

struct head_path_port *entry_head;
struct head_queue *hqueue;

int broadcast_id=0;

struct routingTable routing[10];
char ip_array[10][SIZE_IP];

int main(int argc, char **argv)
{
	getAllIP();

	if(argv[1]==NULL)
	{
		printf("Please input staleness parameter.\n");

		return 1;
	}
	
	staleness=atoi(argv[1]);
	//staleness=0;

	//create entry table
	entry_head=(struct head_path_port *)Malloc(sizeof(struct head_path_port));
	entry_head->table=NULL;
	
	struct table_path_port *first_entry=(struct table_path_port *)Malloc(sizeof(struct table_path_port));
	first_entry->port=SERV_WKPORT;
	sprintf(first_entry->pathname, "%s", SERV_WKPATH);
	first_entry->isperm=1;
	first_entry->next=NULL;
	entry_head->table=first_entry;
	
	hqueue=(struct head_queue *)Malloc(sizeof(struct head_queue));
	hqueue->table=NULL;
	
	int count;
	int count_mac;
	int count_if=0;
	int count_ifB;

	struct hwa_info	*hwa;
	struct hwa_info *hwahead;
	
	struct sockaddr	*sa;
	
	char *ptr;
	
	int i;
	int prflag;
	
	//init routing table
	for(count=0;count<10;count++)
	{
		routing[count].status=0;
	}
	
	int num_interface=0;

	char host_ip[SIZE_IP];

	for(hwahead=hwa=Get_hw_addrs(); hwa!=NULL;hwa=hwa->hwa_next)
	{
		if(strcmp(hwa->if_name, "eth0")==0)
		{
			sa=hwa->ip_addr;
			sprintf(host_ip, "%s", Sock_ntop_host(sa, sizeof(*sa)));
			
			break;
		}
	}
	
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(strcmp(hwa->if_name,"lo")==0 || strcmp(hwa->if_name, "eth0")==0)
		{
			continue;
		}

		num_interface++;
	}
	
	//after we get the number of interface 
	struct interface_info if_info[num_interface];
	
	printf("\n");

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(strcmp(hwa->if_name,"lo")==0 || strcmp(hwa->if_name, "eth0")==0)
		{
			continue;
		}

		sprintf(if_info[count_if].if_name, "%s", hwa->if_name);
		
		if ( (sa = hwa->ip_addr) != NULL)
		{
			sprintf(if_info[count_if].ip_addr, "%s", Sock_ntop_host(sa, sizeof(*sa)));
		}

		prflag = 0;
		i = 0;
		
		for(count=0;count<6;count++)
		{
			if_info[count_if].hw_addr[count]=hwa->if_haddr[count];	
		}
		
		do 
		{
			if (hwa->if_haddr[i] != '\0') 
			{
				prflag = 1;
				break;
			}
			
		} while (++i < IF_HADDR);

		if_info[count_if].if_index=hwa->if_index;
		
		count_if++;
	}

	free_hwa_info(hwahead);

	printf("########Interfaces Infomation:########\n");
	for(count_if=0;count_if<num_interface;count_if++)
	{
		printf("\n");
		printf("    Name      : %s\n", if_info[count_if].if_name);
		printf("    IP Addr   : %s\n", if_info[count_if].ip_addr);
		printf("    Index     : %d\n", if_info[count_if].if_index);
		printf("    MAC Addr  : ");
		
		for(count=0;count<6;count++)
		{
			printf("%.2x", if_info[count_if].hw_addr[count] & 0xff);

			if(count!=5)
			{
				printf(":");
			}
		}
		printf("\n");
	}
	printf("\n");
	printf("######################################\n");
	
	printf("\n");
	
	int pfSockfd[num_interface];

	for(count_if=0;count_if<num_interface;count_if++)
	{
		pfSockfd[count_if]=Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

		bzero(&(if_info[count_if].odrPFAddr), sizeof(if_info[count_if].odrPFAddr));
		if_info[count_if].odrPFAddr.sll_family=AF_PACKET;
		if_info[count_if].odrPFAddr.sll_protocol=htons(PROTOCOL_ID);
		if_info[count_if].odrPFAddr.sll_halen=6;
		if_info[count_if].odrPFAddr.sll_ifindex=if_info[count_if].if_index;

		Bind(pfSockfd[count_if], (SA *)&(if_info[count_if].odrPFAddr), sizeof(if_info[count_if].odrPFAddr));
	}

	int udSockfd;
	struct sockaddr_un odrUDAddr;

	udSockfd=Socket(AF_LOCAL, SOCK_DGRAM, 0);

	unlink(ODR_PATH);
	bzero(&odrUDAddr, sizeof(odrUDAddr));
	odrUDAddr.sun_family=AF_LOCAL;
	strcpy(odrUDAddr.sun_path, ODR_PATH);

	Bind(udSockfd, (SA *)&odrUDAddr, sizeof(odrUDAddr));

	//begin the select procedure
	int maxfd;
	int maxfdp1;

	fd_set rset;

	maxfd=pfSockfd[0];
	for(count_if=0;count_if<num_interface;count_if++)
	{
		if(pfSockfd[count_if] > maxfd)
		{
			maxfd=pfSockfd[count_if];
		}
	}

	if(udSockfd > maxfd)
	{
		maxfd=udSockfd;
	}

	maxfdp1=maxfd+1;
	for( ; ; )
	{
		printRoutingTable();
		
		for(count_if=0;count_if<num_interface;count_if++)
		{
			FD_SET(pfSockfd[count_if], &rset);
		}

		FD_SET(udSockfd, &rset);
		
		select(maxfdp1, &rset, NULL, NULL, NULL);

		for(count_if=0;count_if<num_interface;count_if++)
		{
			if(FD_ISSET(pfSockfd[count_if], &rset))
			{
				char recvline[MAXLINE];
				int recv_sock;
				
				Recvfrom(pfSockfd[count_if], recvline, MAXLINE, 0, NULL, NULL);
				recv_sock=pfSockfd[count_if];

				int recv_type=*(int *)(recvline+14);

				if(recv_type==0)
				{	
					//printf("\n##############RECV#RREQ###############\n");
					char send_mac[6];
					char recv_ipsrc[SIZE_IP];
					char recv_ipdest[SIZE_IP];
					int recv_broadId;
					int recv_forceDis;
					int recv_rrepSent;
					int recv_hopCnt;

					for(count_mac=0;count_mac<6;count_mac++)
					{
						send_mac[count_mac]=*(char *)(recvline+6+count_mac);
					}

					memcpy((char *)recv_ipsrc, (char *)(recvline+18), SIZE_IP);
					memcpy((char *)recv_ipdest, (char *)(recvline+34), SIZE_IP);
					recv_broadId=*(int *)(recvline+50);
					recv_forceDis=*(int *)(recvline+54);
					recv_rrepSent=*(int *)(recvline+58);
					recv_hopCnt=*(int *)(recvline+62);
					
					printf("##RECV#: ODR vm%d [RREQ- type:%d, src:%d, dest:%d, bid:%d, force_discovery:%d, rrep_sent:%d, hop:%d, ", getVMNum(host_ip)+1, recv_type, getVMNum(recv_ipsrc)+1, getVMNum(recv_ipdest)+1, recv_broadId, recv_forceDis, recv_rrepSent, recv_hopCnt);
					
					printf("FROM:");
					
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", send_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(", TO:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", if_info[count_if].hw_addr[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf("]\n");
					
					//printf("RECV RREQ:\n");
					//printf("TYPE           : %d\n", recv_type);
					//printf("SRC_IP         : %s\n", recv_ipsrc);
					//printf("DEST_IP        : %s\n", recv_ipdest);
					//printf("Broadcast_ID   : %d\n", recv_broadId);
					//printf("Force_Discover : %d\n", recv_forceDis);
					//printf("RREP_SENT      : %d\n", recv_rrepSent);
					//printf("HOP_CNT        : %d\n", recv_hopCnt);
					//printf("\n");

					int vmid=route_findRoute(recv_ipsrc);
					if(routing[vmid].broadId>recv_broadId)
					{
						break;
					}

					if(routing[vmid].broadId==recv_broadId)
					{
						if((recv_hopCnt+1) >= routing[vmid].hop_cnt)
						{
							break;
						}
					}
			
					if(recv_hopCnt>=15)
					{
						break;
					}
					
					//reqPlrep is used to test whether the condition of rrep_sent is fulfilled.
					int reqPlrep=0;
					int reqPlrepB=0;
					if(vmid==-1)
					{
						reqPlrep=1;
					}
					else
					{
						if(routing[vmid].hop_cnt>(recv_hopCnt+1))
						{
							reqPlrep=1;
						}
					}
					
					if(vmid==-1)
					{
						route_insertRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, recv_broadId);
					}
					else
					{
						if((recv_forceDis==1) || (recv_hopCnt+1<routing[vmid].hop_cnt))
						{
							route_updateRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, recv_broadId);
						}
						else if(recv_hopCnt+1==routing[vmid].hop_cnt)
						{
								
							int update_table=0;
							for(count_mac=0;count_mac<6;count_mac++)
							{
								if(send_mac[count_mac]!=routing[vmid].next_hop[count_mac])
								{
									update_table=1;
									break;
								}
							}
							if(update_table==1)
							{
								route_updateRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, recv_broadId);
							}
						}
					}
					
					int condition;
					
					//RREQ HOST?
					if((strcmp(host_ip, recv_ipdest)==0) && (recv_rrepSent!=1))
					{
						//send back rrep
						condition=1;
					}
					else if((recv_forceDis==1) && (strcmp(host_ip, recv_ipdest)!=0))
					{
						//RREQ UNKNOWN
						condition=2;
					}
					else
					{
						vmid=route_findRoute(recv_ipdest);
						if((vmid!=-1) && (recv_rrepSent!=1))
						{
							//RREQ KNOWN Send back rrep
							condition=1;
						}
						else
						{
							//RREQ UNKNOWN
							condition=2;
						}
					}

					if(condition==1)
					{
						//send back RREP
						void *new_rrep=(void *)Malloc(SIZE_RREP);

						int type=1;
						int force_dis=recv_forceDis;
						int hop_cnt=0;

						memcpy((void *)new_rrep, (void *)&type, SIZE_TYPE);
						memcpy((void *)(new_rrep+4), (void *)recv_ipdest, SIZE_IP);
						memcpy((void *)(new_rrep+20), (void *)recv_ipsrc, SIZE_IP);
						memcpy((void *)(new_rrep+36), (void *)&force_dis, SIZE_TYPE);
						memcpy((void *)(new_rrep+40), (void *)&hop_cnt, SIZE_TYPE);

						struct sockaddr_ll uniAddr;
						void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREP);
						bzero(uniFrame, sizeof(*uniFrame));

						uniAddr.sll_family=AF_PACKET;
						uniAddr.sll_protocol=htons(PROTOCOL_ID);
						uniAddr.sll_halen=6;
						uniAddr.sll_ifindex=if_info[count_if].if_index;

						for(count_mac=0;count_mac<6;count_mac++)
						{
							uniAddr.sll_addr[count_mac]=send_mac[count_mac];
						}
						uniAddr.sll_addr[6]=0xff;
						uniAddr.sll_addr[7]=0xff;

						unsigned short protocol=htons(PROTOCOL_ID);

						memcpy((void *)uniFrame, (void *)send_mac, 6);
						memcpy((void *)(uniFrame+6), (void *)(if_info[count_if].hw_addr), 6);
						memcpy((void *)(uniFrame+12), (void *)&protocol, 2);

						memcpy((void *)(uniFrame+14), (void *)new_rrep, SIZE_RREP);

						Sendto(pfSockfd[count_if], uniFrame, SIZE_HEADFRAME+SIZE_RREP, 0, (SA *)&uniAddr, sizeof(uniAddr));

						printf("##SEND#: ODR vm%d:", getVMNum(host_ip)+1);
						printf("[src ");
						for(count_mac=0;count_mac<6;count_mac++)
						{
							printf("%.2x", if_info[count_if].hw_addr[count_mac] & 0xff);
							if(count_mac!=5)
							{
								printf(":");
							}
						}
						printf(", dest ");
						for(count_mac=0;count_mac<6;count_mac++)
						{
							printf("%.2x", send_mac[count_mac] & 0xff);
							if(count_mac!=5)
							{
								printf(":");
							}
						}
						printf("]\n");
						printf("    ODR msg type %d, src vm%d, dest vm%d, force_discovery %d\n", type, getVMNum(recv_ipdest)+1, getVMNum(recv_ipsrc)+1, force_dis);
                        
						if(reqPlrep==1)
						{
							condition=2;
							reqPlrepB=1;
						}
					}
					if(condition==2)
					{
						//flood out rreq
						void *new_rreq=(void *)Malloc(SIZE_RREQ);
						int new_hopCnt=recv_hopCnt+1;
						
						int new_rrepSent=recv_rrepSent;
						if(reqPlrepB==1)
						{
							new_rrepSent=1;
						}
						
						memcpy((void *)new_rreq, (void *)&recv_type, SIZE_TYPE);
						memcpy((void *)(new_rreq+4), (void *)recv_ipsrc, SIZE_IP);
						memcpy((void *)(new_rreq+20), (void *)recv_ipdest, SIZE_IP);
						memcpy((void *)(new_rreq+36), (void *)&recv_broadId, SIZE_TYPE);
						memcpy((void *)(new_rreq+40), (void *)&recv_forceDis, SIZE_TYPE);
						memcpy((void *)(new_rreq+44), (void *)&new_rrepSent, SIZE_TYPE);
						memcpy((void *)(new_rreq+48), (void *)&new_hopCnt, SIZE_TYPE);
						
						struct sockaddr_ll broadAddr;
						void *broadFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREQ);
						bzero(broadFrame, sizeof(*broadFrame));

						unsigned char broad_mac[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

						for(count_ifB=0;count_ifB<num_interface;count_ifB++)
						{
							if(count_ifB==count_if)
							{
								continue;
							}
							
							broadAddr.sll_family=AF_PACKET;
							broadAddr.sll_protocol=htons(PROTOCOL_ID);
							broadAddr.sll_halen=6;
							broadAddr.sll_ifindex=if_info[count_ifB].if_index;
							broadAddr.sll_addr[0]=0xff;
							broadAddr.sll_addr[1]=0xff;
							broadAddr.sll_addr[2]=0xff;
							broadAddr.sll_addr[3]=0xff;
							broadAddr.sll_addr[4]=0xff;
							broadAddr.sll_addr[5]=0xff;
							broadAddr.sll_addr[6]=0x00;
							broadAddr.sll_addr[7]=0x00;

							unsigned short protocol=htons(PROTOCOL_ID);

							memcpy((void *)broadFrame, (void *)broad_mac, 6);
							memcpy((void *)(broadFrame+6), (void *)(if_info[count_ifB].hw_addr), 6);
							memcpy((void *)(broadFrame+12), (void *)&protocol, 2);
							memcpy((void *)(broadFrame+14), (void *)new_rreq, SIZE_RREQ);

							Sendto(pfSockfd[count_ifB], broadFrame, SIZE_HEADFRAME+SIZE_RREQ, 0, (SA *)&broadAddr, sizeof(broadAddr));
							////printf("Flood RReQ\n");
							printf("##SEND#: ODR vm%d:", getVMNum(host_ip)+1);
							printf("[src ");
							for(count_mac=0;count_mac<6;count_mac++)
							{
								printf("%.2x", if_info[count_ifB].hw_addr[count_mac] & 0xff);
								if(count_mac!=5)
								{
									printf(":");
								}
							}
							printf(",dest ");
							for(count_mac=0;count_mac<6;count_mac++)
							{
								printf("%.2x", broad_mac[count_mac] & 0xff);
								if(count_mac!=5)
								{
									printf(":");
								}
							}
							printf("]\n");
							printf("    ODR msg type %d, src vm%d, dest vm%d, bid %d, force_discovery %d, rrep_sent ,%d, hop %d\n", recv_type, getVMNum(recv_ipsrc)+1, getVMNum(recv_ipdest)+1, recv_broadId, recv_forceDis, new_rrepSent, new_hopCnt);
						}

					}
				}
				
				if(recv_type==1)
				{
					
					//printf("\n##############RECV#RREP###############\n");
					
					char send_mac[6];
					char recv_ipsrc[SIZE_IP];
					char recv_ipdest[SIZE_IP];
					int recv_forceDis;
					int recv_hopCnt;

					for(count_mac=0;count_mac<6;count_mac++)
					{
						send_mac[count_mac]=*(char *)(recvline+6+count_mac);
						//printf("%.2x\n", send_mac[count_mac] & 0xff);
					}

					memcpy((char *)recv_ipsrc, (char *)(recvline+18), SIZE_IP);
					memcpy((char *)recv_ipdest, (char *)(recvline+34), SIZE_IP);
					recv_forceDis=*(int *)(recvline+50);
					recv_hopCnt=*(int *)(recvline+54);

					////printf("RECV RREP:\n");
					////printf("TYPE           : %d\n", recv_type);
					////printf("SRC_IP         : %s\n", recv_ipsrc);
					////printf("DEST_IP        : %s\n", recv_ipdest);
					////printf("Force_Discover : %d\n", recv_forceDis);
					////printf("HOP_CNT        : %d\n", recv_hopCnt);
					
					printf("##RECV#: ODR vm%d [RREP- type:%d, src:%d, dest:%d, force_discovery:%d, hop:%d", getVMNum(host_ip)+1, recv_type, getVMNum(recv_ipsrc)+1, getVMNum(recv_ipdest)+1, recv_forceDis, recv_hopCnt);
					printf(", FROM:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", send_mac[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(", To:");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", if_info[count_if].hw_addr[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf("]\n");
					
					int broadId=-1;
					
					int vmid=route_findRoute(recv_ipsrc);
					if(vmid==-1)
					{
						route_insertRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, broadId);
					}
					else
					{
						if((recv_forceDis==1) || (recv_hopCnt+1<routing[vmid].hop_cnt))
						{
							route_updateRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, broadId);
						}
						else if(recv_hopCnt+1==routing[vmid].hop_cnt)
						{
							
							
							int update_table=0;
							for(count_mac=0;count_mac<6;count_mac++)
							{
								if(send_mac[count_mac]!=routing[vmid].next_hop[count_mac])
								{
									update_table=1;
									break;
								}
							}
							if(update_table==1)
							{
								route_updateRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, broadId);
							}
						
						}
					}
					int condition;
					//RREP HOST?
					if(strcmp(host_ip, recv_ipdest)==0)
					{
						condition=1;
					}
					else if(recv_forceDis==1)
					{
						condition=3;
					}
					else
					{
						int vmid=route_findRoute(recv_ipdest);
						if(vmid!=-1)
						{
							//RREP KNOWN
							condition=2;
						}
						else
						{
							//RREP UNKNOWN
							condition=3;
						}
					}
					//send out all queue messages	
					if(condition==1)
					{
						////printf("Send Out All Queue Information!\n");
						struct table_queue *tqueue;
						struct table_queue *p_tqueue;
						struct table_queue *q_tqueue;
						
						tqueue=hqueue->table;
						
						int qvm;
						while(tqueue!=NULL)
						{
							qvm=tqueue->destVM;
							//printf("QVM:%d, getVMFUN:%d\n", qvm, getVMNum(recv_ipsrc));
							if(getVMNum(recv_ipsrc)==qvm)
							{
								if(tqueue->type==1)
								{
									//send rrep type=1
									struct sockaddr_ll uniAddr;
									void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREP);
									bzero(uniFrame, sizeof(*uniFrame));

									uniAddr.sll_family=AF_PACKET;
									uniAddr.sll_protocol=htons(PROTOCOL_ID);
									uniAddr.sll_halen=6;
									uniAddr.sll_ifindex=routing[qvm].out_ifindex;

									int count_temp;
									for(count_temp=0;count_temp<num_interface;count_temp++)
									{
										if(routing[qvm].out_ifindex==if_info[count_temp].if_index)
										{
											break;
										}
									}

									for(count_mac=0;count_mac<6;count_mac++)
									{
										uniAddr.sll_addr[count_mac]=routing[qvm].next_hop[count_mac];
									}
									uniAddr.sll_addr[6]=0xff;
									uniAddr.sll_addr[7]=0xff;

									unsigned short protocol=htons(PROTOCOL_ID);

									memcpy((void *)uniFrame, (void *)(routing[qvm].next_hop), 6);
									memcpy((void *)(uniFrame+6), (void *)(if_info[count_temp].hw_addr), 6);
									memcpy((void *)(uniFrame+12), (void *)&protocol, 2);

									memcpy((void *)(uniFrame+14), (void *)((tqueue->sequence)+14), SIZE_RREP);

									Sendto(pfSockfd[count_temp], uniFrame, SIZE_HEADFRAME+SIZE_SENDMESSAGE, 0, (SA *)&uniAddr, sizeof(uniAddr));
									
									printf("##SEND#: ODR vm%d :", getVMNum(host_ip)+1);
									printf("[src ");
									for(count_mac=0;count_mac<6;count_mac++)
									{
										printf("%.2x", if_info[count_temp].hw_addr[count_mac] & 0xff);
										if(count_mac!=5)
										{
											printf(":");
										}
									}
									printf(",dest ");
									for(count_mac=0;count_mac<6;count_mac++)
									{
										printf("%.2x", routing[qvm].next_hop[count_mac] & 0xff);
										if(count_mac!=5)
										{
											printf(":");
										}
									}
									printf("]\n");
									printf("    ODR msg type %d, src vm%d, dest vm%d, force_discovery %d\n", 1, getVMNum((char *)(uniFrame+18))+1, getVMNum((char *)(uniFrame+34))+1, *(int *)(uniFrame+50));
									
									if(tqueue==hqueue->table)
									{
										hqueue->table=tqueue->next;
										free(tqueue);
									}
									else
									{
										p_tqueue=hqueue->table;
										q_tqueue=p_tqueue->next;

										while(tqueue!=q_tqueue)
										{
											p_tqueue=p_tqueue->next;
											q_tqueue=q_tqueue->next;
										}

										p_tqueue->next=q_tqueue->next;
										free(q_tqueue);
										tqueue=p_tqueue;
									}
								}
							
								if(tqueue->type==2)
								{	
									//send msg type=2
									struct sockaddr_ll uniAddr;
									void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+(tqueue->len));
									bzero(uniFrame, sizeof(*uniFrame));

									uniAddr.sll_family=AF_PACKET;
									uniAddr.sll_protocol=htons(PROTOCOL_ID);
									uniAddr.sll_halen=6;
									uniAddr.sll_ifindex=routing[qvm].out_ifindex;

									int count_temp;
									for(count_temp=0;count_temp<num_interface;count_temp++)
									{
										if(routing[qvm].out_ifindex==if_info[count_temp].if_index)
										{
											break;
										}
									}

									for(count_mac=0;count_mac<6;count_mac++)
									{
										uniAddr.sll_addr[count_mac]=routing[qvm].next_hop[count_mac];
									}
									uniAddr.sll_addr[6]=0xff;
									uniAddr.sll_addr[7]=0xff;

									unsigned short protocol=htons(PROTOCOL_ID);

									memcpy((void *)uniFrame, (void *)(routing[qvm].next_hop), 6);
									memcpy((void *)(uniFrame+6), (void *)(if_info[count_temp].hw_addr), 6);
									memcpy((void *)(uniFrame+12), (void *)&protocol, 2);

									memcpy((void *)(uniFrame+14), (void *)(tqueue->sequence), tqueue->len);

									Sendto(pfSockfd[count_temp], uniFrame, SIZE_HEADFRAME+SIZE_SENDMESSAGE, 0, (SA *)&uniAddr, sizeof(uniAddr));
									
									printf("##SEND#: ODR vm%d :", getVMNum(host_ip)+1);
									printf("[src ");
									for(count_mac=0;count_mac<6;count_mac++)
									{
										printf("%.2x", if_info[count_temp].hw_addr[count_mac] & 0xff);
										if(count_mac!=5)
										{
											printf(":");
										}
									}
									printf(",dest ");
									for(count_mac=0;count_mac<6;count_mac++)
									{
										printf("%.2x", routing[qvm].next_hop[count_mac] & 0xff);
										if(count_mac!=5)
										{
											printf(":");
										}
									}
									printf("]\n");
									printf("    ODR msg type %d, src vm%d, dest vm%d\n", 2, getVMNum((char *)(uniFrame+18))+1, getVMNum((char *)(uniFrame+36))+1);
									
									if(tqueue==hqueue->table)
									{
										hqueue->table=tqueue->next;
										free(tqueue);
									}
									else
									{
									    p_tqueue=hqueue->table;
										q_tqueue=p_tqueue->next;

										while(tqueue!=q_tqueue)
										{
											p_tqueue=p_tqueue->next;
											q_tqueue=q_tqueue->next;
										}

										p_tqueue->next=q_tqueue->next;
										free(q_tqueue);
										tqueue=p_tqueue;
									}
								}
							}
							tqueue=tqueue->next;
						}
						
					}
					
					if(condition==2)
					{
						//send rrep according to info in routing
						void *new_rrep=(void *)Malloc(SIZE_RREP);

						int type=1;
						int force_dis=0;
						int hop_cnt=recv_hopCnt+1;
						
						int destVM=getVMNum(recv_ipdest);

						memcpy((void *)new_rrep, (void *)&type, SIZE_TYPE);
						memcpy((void *)(new_rrep+4), (void *)recv_ipsrc, SIZE_IP);
						memcpy((void *)(new_rrep+20), (void *)recv_ipdest, SIZE_IP);
						memcpy((void *)(new_rrep+36), (void *)&force_dis, SIZE_TYPE);
						memcpy((void *)(new_rrep+40), (void *)&hop_cnt, SIZE_TYPE);
						
						struct sockaddr_ll uniAddr;
						void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREP);
						bzero(uniFrame, sizeof(*uniFrame));

						uniAddr.sll_family=AF_PACKET;
						uniAddr.sll_protocol=htons(PROTOCOL_ID);
						uniAddr.sll_halen=6;
						uniAddr.sll_ifindex=routing[destVM].out_ifindex;

						for(count_mac=0;count_mac<6;count_mac++)
						{
							uniAddr.sll_addr[count_mac]=routing[destVM].next_hop[count_mac];
						}
						uniAddr.sll_addr[6]=0xff;
						uniAddr.sll_addr[7]=0xff;

						unsigned short protocol=htons(PROTOCOL_ID);

						int outSock;
						int count_out;
						for(count_out=0;count_out<num_interface;count_out++)
						{
							if(if_info[count_out].if_index==routing[destVM].out_ifindex)
							{
								outSock=count_out;
								break;
							}
						}
						
						memcpy((void *)uniFrame, (void *)(routing[destVM].next_hop), 6);
						memcpy((void *)(uniFrame+6), (void *)(if_info[outSock].hw_addr), 6);
						memcpy((void *)(uniFrame+12), (void *)&protocol, 2);
						memcpy((void *)(uniFrame+14), (void *)new_rrep, SIZE_RREP);
						Sendto(pfSockfd[outSock], uniFrame, SIZE_HEADFRAME+SIZE_RREP, 0, (SA *)&uniAddr, sizeof(uniAddr));
						printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
						printf("[src ");
						for(count_mac=0;count_mac<6;count_mac++)							
						{
							printf("%.2x", if_info[outSock].hw_addr[count_mac] & 0xff);
							if(count_mac!=5)
							{
								printf(":");		
							}
						}
						printf(",dest ");
						for(count_mac=0;count_mac<6;count_mac++)
						{
							printf("%.2x", routing[destVM].next_hop[count_mac] & 0xff);
							if(count_mac!=5)			
							{
								printf(":");
							}
						}	
						printf("]\n");
						printf("    ODR msg type %d, src vm%d, dest vm%d, force_discovery %d\n", 1, getVMNum((char *)(uniFrame+18))+1, getVMNum((char *)(uniFrame+34))+1, force_dis);
					
					}
					if(condition==3)
					{
						insertQueue(recv_type, getVMNum(recv_ipdest), recvline, SIZE_HEADFRAME+SIZE_RREP);
						
						//flood out rreq
						void *new_rreq=(void *)Malloc(SIZE_RREQ);
						int new_hopCnt=0;
						
						int rreq_type=0;
						int rreq_broadId=getBroadcastID();
						int rreq_forceDis=recv_forceDis;
						int rreq_rrepSent=0;
						
						memcpy((void *)new_rreq, (void *)&rreq_type, SIZE_TYPE);
						memcpy((void *)(new_rreq+4), (void *)host_ip, SIZE_IP);
						memcpy((void *)(new_rreq+20), (void *)recv_ipdest, SIZE_IP);
						memcpy((void *)(new_rreq+36), (void *)&rreq_broadId, SIZE_TYPE);
						memcpy((void *)(new_rreq+40), (void *)&rreq_forceDis, SIZE_TYPE);
						memcpy((void *)(new_rreq+44), (void *)&rreq_rrepSent, SIZE_TYPE);
						memcpy((void *)(new_rreq+48), (void *)&new_hopCnt, SIZE_TYPE);
						
						struct sockaddr_ll broadAddr;
						void *broadFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREQ);
						bzero(broadFrame, sizeof(*broadFrame));

						unsigned char broad_mac[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

						for(count_ifB=0;count_ifB<num_interface;count_ifB++)
						{
							if(count_ifB==count_if)
							{
								continue;
							}
							
							broadAddr.sll_family=AF_PACKET;
							broadAddr.sll_protocol=htons(PROTOCOL_ID);
							broadAddr.sll_halen=6;
							broadAddr.sll_ifindex=if_info[count_ifB].if_index;
							broadAddr.sll_addr[0]=0xff;
							broadAddr.sll_addr[1]=0xff;
							broadAddr.sll_addr[2]=0xff;
							broadAddr.sll_addr[3]=0xff;
							broadAddr.sll_addr[4]=0xff;
							broadAddr.sll_addr[5]=0xff;
							broadAddr.sll_addr[6]=0x00;
							broadAddr.sll_addr[7]=0x00;

							unsigned short protocol=htons(PROTOCOL_ID);

							memcpy((void *)broadFrame, (void *)broad_mac, 6);
							memcpy((void *)(broadFrame+6), (void *)(if_info[count_ifB].hw_addr), 6);
							memcpy((void *)(broadFrame+12), (void *)&protocol, 2);
							memcpy((void *)(broadFrame+14), (void *)new_rreq, SIZE_RREQ);

							Sendto(pfSockfd[count_ifB], broadFrame, SIZE_HEADFRAME+SIZE_RREQ, 0, (SA *)&broadAddr, sizeof(broadAddr));
							printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
							printf("[src ");
							for(count_mac=0;count_mac<6;count_mac++)
							{
								printf("%.2x", if_info[count_ifB].hw_addr[count_mac] & 0xff);
								if(count_mac!=5)
								{
									printf(":");
								}
							}
							printf(",dest ");
							for(count_mac=0;count_mac<6;count_mac++)
							{
								printf("%.2x", broad_mac[count_mac] & 0xff);
								if(count_mac!=5)
								{
									printf(":");
								}
							}
							printf("]\n");
							printf("    ODR msg type %d, src vm%d, dest vm%d, bid %d, force_discovery %d, rrep_sent %d, hop %d\n", 0, getVMNum((char *)(broadFrame+18))+1, getVMNum((char *)(broadFrame+34))+1, rreq_broadId, rreq_forceDis, rreq_rrepSent, new_hopCnt);
						}	
					}
				}

				if(recv_type==2)
				{
					//printf("\n###########RECV#APPLICATION###########\n");
					char send_mac[6];
					char recv_ipsrc[SIZE_IP];
					unsigned short recv_portsrc;
					char recv_ipdest[SIZE_IP];
					unsigned short recv_portdest;
					int recv_hopCnt;
					int recv_len;

					for(count_mac=0;count_mac<6;count_mac++)
					{
						send_mac[count_mac]=*(char *)(recvline+6+count_mac);
					}

					memcpy((char *)recv_ipsrc, (char *)(recvline+18), SIZE_IP);
					recv_portsrc=*(unsigned short *)(recvline+34);
					memcpy((char *)recv_ipdest, (char *)(recvline+36), SIZE_IP);
					recv_portdest=*(unsigned short *)(recvline+52);
					recv_hopCnt=*(int *)(recvline+54);
					recv_len=*(int *)(recvline+58);

					char recv_message[recv_len];
					memcpy((char *)recv_message, (char *)(recvline+62), recv_len);

					////printf("RECV APP:\n");
					////printf("TYPE: %d\n", recv_type);
					////printf("SRC_IP: %s\n", recv_ipsrc);
					////printf("SRC_PORT: %d\n", recv_portsrc);
					////printf("DEST_IP: %s\n", recv_ipdest);
					////printf("DEST_PORT: %d\n", recv_portdest);
					////printf("HOP_CNT: %d\n", recv_hopCnt);
					////printf("LEN_MESSAGE: %d\n", recv_len);
					////printf("MESSAGE: %s\n", recv_message);
				
					printf("##RECV#: ODR vm%d [Application- type: %d,src:%d, src_port:%d, dest:%d, dest_port:%d]\n",getVMNum(host_ip)+1 ,recv_type, getVMNum(recv_ipsrc)+1, recv_portsrc, getVMNum(recv_ipdest)+1, recv_portdest);
					
					int broadId=-1;
					int vmid=route_findRoute(recv_ipsrc);
					if(vmid==-1)
					{
						//insert the route
						route_insertRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, broadId);
					}
					else
					{
						//update the route
						route_updateRoute(recv_ipsrc, send_mac, if_info[count_if].if_index, recv_hopCnt+1, broadId);
					}
					
					int condition;

					if(strcmp(host_ip, recv_ipdest)==0)
					{
						condition=1;
					}
					else
					{
						condition=2;
					}

					if(condition==1)
					{
						char pname[MAXLINE];
						char *ppname;

						ppname=getPathByPort(recv_portdest, entry_head);
						if(ppname==NULL)
						{
							printf("No Service on Path-Entry Table!\n");
							continue;;
						}
						sprintf(pname, "%s", ppname);

						void *msgsends=(void *)Malloc(SIZE_SENDMESSAGE);
						memcpy((void *)msgsends, (void *)(recvline+14), SIZE_SENDMESSAGE);
						
						struct sockaddr_un servaddrA;
						bzero(&servaddrA, sizeof(servaddrA));
						servaddrA.sun_family=AF_LOCAL;
						strcpy(servaddrA.sun_path, pname);

						int result=sendto(udSockfd, msgsends, SIZE_SENDMESSAGE, 0, (SA *)&servaddrA, sizeof(servaddrA)) ;
						printf("##SEND#: ODR vm%d send to Unix Domain Sock:\n", getVMNum(host_ip)+1);
						printf("    ODR msg src vm%d, dest vm%d\n", getVMNum((char *)(msgsends+4))+1, getVMNum((char *)(msgsends+22))+1);
						if(result==-1)
						{
							printf("Server is Down!\n");
						}
					}
					if(condition==2)
					{	
						vmid=route_findRoute(recv_ipdest);
						if(vmid==-1)
						{
							insertQueue(2, getVMNum(recv_ipdest), (void *)(recvline+14), SIZE_SENDMESSAGE);
							
							//flood out rreq
							void *new_rreq=(void *)Malloc(SIZE_RREQ);
							int new_hopCnt=0;
							
							int rreq_type=0;
							int rreq_broadId=getBroadcastID();
							int rreq_forceDis=0;
							int rreq_rrepSent=0;
							
							memcpy((void *)new_rreq, (void *)&rreq_type, SIZE_TYPE);
							memcpy((void *)(new_rreq+4), (void *)host_ip, SIZE_IP);
							memcpy((void *)(new_rreq+20), (void *)recv_ipdest, SIZE_IP);
							memcpy((void *)(new_rreq+36), (void *)&rreq_broadId, SIZE_TYPE);
							memcpy((void *)(new_rreq+40), (void *)&rreq_forceDis, SIZE_TYPE);
							memcpy((void *)(new_rreq+44), (void *)&rreq_rrepSent, SIZE_TYPE);
							memcpy((void *)(new_rreq+48), (void *)&new_hopCnt, SIZE_TYPE);
							
							struct sockaddr_ll broadAddr;
							void *broadFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREQ);
							bzero(broadFrame, sizeof(*broadFrame));

							unsigned char broad_mac[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

							for(count_ifB=0;count_ifB<num_interface;count_ifB++)
							{
								if(count_ifB==count_if)
								{
									continue;
								}
								
								broadAddr.sll_family=AF_PACKET;
								broadAddr.sll_protocol=htons(PROTOCOL_ID);
								broadAddr.sll_halen=6;
								broadAddr.sll_ifindex=if_info[count_ifB].if_index;
								broadAddr.sll_addr[0]=0xff;
								broadAddr.sll_addr[1]=0xff;
								broadAddr.sll_addr[2]=0xff;
								broadAddr.sll_addr[3]=0xff;
								broadAddr.sll_addr[4]=0xff;
								broadAddr.sll_addr[5]=0xff;
								broadAddr.sll_addr[6]=0x00;
								broadAddr.sll_addr[7]=0x00;

								unsigned short protocol=htons(PROTOCOL_ID);

								memcpy((void *)broadFrame, (void *)broad_mac, 6);
								memcpy((void *)(broadFrame+6), (void *)(if_info[count_ifB].hw_addr), 6);
								memcpy((void *)(broadFrame+12), (void *)&protocol, 2);
								memcpy((void *)(broadFrame+14), (void *)new_rreq, SIZE_RREQ);

								Sendto(pfSockfd[count_ifB], broadFrame, SIZE_HEADFRAME+SIZE_RREQ, 0, (SA *)&broadAddr, sizeof(broadAddr));
								////printf("NEW RREQ DETECT DEST\n");
								printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
								printf("[src ");
								for(count_mac=0;count_mac<6;count_mac++)							
								{
									printf("%.2x", if_info[count_ifB].hw_addr[count_mac] & 0xff);	
									if(count_mac!=5)
									{
										printf(":");		
									}
								}
								printf(",dest ");
								for(count_mac=0;count_mac<6;count_mac++)
								{
									printf("%.2x", broad_mac[count_mac] & 0xff);
									if(count_mac!=5)			
									{
										printf(":");
									}
								}	
								printf("]\n");
								printf("    ODR msg type %d, src vm%d, dest vm%d, bid %d, force_discovery %d, rrep_sent, hop %d\n", 0, getVMNum((char *)(broadFrame+18))+1, getVMNum((char *)(broadFrame+34))+1, rreq_broadId, rreq_forceDis, rreq_rrepSent, new_hopCnt);

							}
						}
						else
						{
							int count_temp;
							for(count_temp=0;count_temp<num_interface;count_temp++)
							{
								if(if_info[count_temp].if_index==routing[vmid].out_ifindex)
								{
									break;
								}
							}
							
							int new_hop=recv_hopCnt+1;
							void *sendMessage=(void *)Malloc(SIZE_SENDMESSAGE);
							memcpy((void *)sendMessage, (void *)(recvline+14), SIZE_SENDMESSAGE);
							memcpy((void *)(sendMessage+40), (void *)&new_hop, SIZE_TYPE);

							struct sockaddr_ll uniAddr;
							void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_SENDMESSAGE);
							bzero(uniFrame, sizeof(*uniFrame));

							uniAddr.sll_family=AF_PACKET;
							uniAddr.sll_protocol=htons(PROTOCOL_ID);
							uniAddr.sll_halen=6;
							uniAddr.sll_ifindex=routing[vmid].out_ifindex;

							for(count_mac=0;count_mac<6;count_mac++)
							{
								uniAddr.sll_addr[count_mac]=routing[vmid].next_hop[count_mac];
							}
							uniAddr.sll_addr[6]=0xff;
							uniAddr.sll_addr[7]=0xff;

							unsigned short protocol=htons(PROTOCOL_ID);

							memcpy((void *)uniFrame, (void *)(routing[vmid].next_hop), 6);
							memcpy((void *)(uniFrame+6), (void *)(if_info[count_temp].hw_addr), 6);
							memcpy((void *)(uniFrame+12), (void *)&protocol, 2);

							memcpy((void *)(uniFrame+14), (void *)sendMessage, SIZE_SENDMESSAGE);

							Sendto(pfSockfd[count_temp], uniFrame, SIZE_HEADFRAME+SIZE_SENDMESSAGE, 0, (SA *)&uniAddr, sizeof(uniAddr));
							
							printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
							printf("[src ");
							for(count_mac=0;count_mac<6;count_mac++)							
							{
								printf("%.2x", if_info[count_temp].hw_addr[count_mac] & 0xff);
								if(count_mac!=5)
								{
									printf(":");		
								}
							}
							printf(",dest ");
							for(count_mac=0;count_mac<6;count_mac++)
							{
								printf("%.2x", routing[vmid].next_hop[count_mac] & 0xff);
								if(count_mac!=5)			
								{
									printf(":");
								}
							}	
							printf("]\n");
							printf("    ODR msg type %d, src vm%d, dest vm%d\n", 2, getVMNum((char *)(uniFrame+18))+1, getVMNum((char *)(uniFrame+34))+1);

						}
					}
				}

				break;
			}
		}

		if(FD_ISSET(udSockfd, &rset))
		{
			////printf("\n###########RECV#Unix#Domain###########\n");
			struct sockaddr_un udServaddr;
			socklen_t len_udServaddr;

			len_udServaddr=sizeof(udServaddr);
		
			recv_packet=(void *)Malloc(SIZE_SENDMESSAGE);
		
			Recvfrom(udSockfd, recv_packet, SIZE_SENDMESSAGE+4, 0, (SA *)&udServaddr, &len_udServaddr);

			int type=*(int *)recv_packet;

			char ip_src[INET_ADDRSTRLEN];
			sprintf(ip_src, "%s", (char *)(recv_packet+4));

			unsigned short port_src=*(unsigned short *)(recv_packet+20);
			//unsigned short port_src;
			
			char ip_dest[INET_ADDRSTRLEN];
			sprintf(ip_dest, "%s", (char *)(recv_packet+22));

			unsigned short port_dest=*(unsigned short *)(recv_packet+38);

			int hop_cnt=*(int *)(recv_packet+40);

			int len=*(int *)(recv_packet+44);
			
			int flag=0;
			flag=*(int *)(recv_packet+80);

			/*
			printf("\n");
			printf("Type     : %d\n", type);
			printf("SRC IP   : %s\n", ip_src);
			printf("SRC PORT : %d\n", port_src);
			printf("DEST IP  : %s\n", ip_dest);
			printf("DEST PORT: %d\n", port_dest);
			*/
			printf("##RECV#: ODR vm%d [RECV from Unix Domain Sock: src:%d, src_port:%d, dest:%d, dest_port:%d, flag:%d]\n", getVMNum(host_ip)+1, getVMNum(ip_src)+1, port_src, getVMNum(ip_dest)+1, port_dest, flag);
	
			port_src=entry_findPath(udServaddr.sun_path, entry_head);
			
			if(port_src==0)
			{
				port_src=entry_insertPath(udServaddr.sun_path, 0, entry_head);
			}

			memcpy((void *)(recv_packet+20), (void *)&port_src, SIZE_PORT);

			struct table_path_port *entry_table;
			entry_table=entry_head->table;
			
			while(entry_table!=NULL)
			{
				entry_table=entry_table->next;
			}

			int vmid=route_findRoute(ip_dest);
			int haveRoute;
			if(vmid==-1)
			{
				haveRoute=0;
			}
			else
			{
				haveRoute=1;
			}

			//int haveRoute=0;
			if(haveRoute==0 || flag==1)
			{
				//create RREQ
				void *new_rreq=(void *)Malloc(SIZE_RREQ);
			
				type=0;
				int broadcast_id=getBroadcastID();
				int rrep_sent=0;
				int force_dis=flag;
				hop_cnt=0;

				memcpy((void *)new_rreq, (void *)&type, SIZE_TYPE);
				memcpy((void *)(new_rreq+4), (void *)ip_src, SIZE_IP);
				memcpy((void *)(new_rreq+20), (void *)ip_dest, SIZE_IP);
				memcpy((void *)(new_rreq+36), (void *)&broadcast_id, SIZE_TYPE);
				memcpy((void *)(new_rreq+40), (void *)&force_dis, SIZE_TYPE);
				memcpy((void *)(new_rreq+44), (void *)&rrep_sent, SIZE_TYPE);
				memcpy((void *)(new_rreq+48), (void *)&hop_cnt, SIZE_TYPE);
			
				//create and send  frame
				struct sockaddr_ll broadAddr;
				void *broadFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREQ);
				bzero(broadFrame, sizeof(*broadFrame));
			
				unsigned char broad_mac[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			
				for(count_if=0;count_if<num_interface;count_if++)
				{
					broadAddr.sll_family=AF_PACKET;
					broadAddr.sll_protocol=htons(PROTOCOL_ID);
					broadAddr.sll_halen=6;
					broadAddr.sll_ifindex=if_info[count_if].if_index;

					broadAddr.sll_addr[0]=0xff;
					broadAddr.sll_addr[1]=0xff;
					broadAddr.sll_addr[2]=0xff;
					broadAddr.sll_addr[3]=0xff;
					broadAddr.sll_addr[4]=0xff;
					broadAddr.sll_addr[5]=0xff;
					broadAddr.sll_addr[6]=0x00;
					broadAddr.sll_addr[7]=0x00;

					unsigned short protocol=htons(PROTOCOL_ID);

					memcpy((void *)broadFrame, (void *)broad_mac, 6);
					memcpy((void *)(broadFrame+6), (void *)(if_info[count_if].hw_addr), 6);
					memcpy((void *)(broadFrame+12), (void *)&protocol, 2);
					memcpy((void *)(broadFrame+14), (void *)new_rreq, SIZE_RREQ);

					Sendto(pfSockfd[count_if], broadFrame, SIZE_HEADFRAME+SIZE_RREQ, 0, (SA *)&broadAddr, sizeof(broadAddr));
					
					printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
					printf("[src ");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", if_info[count_if].hw_addr[count_mac] & 0xff);
						if(count_mac!=5)
						{
							printf(":");
						}
					}
					printf(", dest ");
					for(count_mac=0;count_mac<6;count_mac++)
					{
						printf("%.2x", broad_mac[count_mac] & 0xff);
						if(count_mac!=5)							
						{
							printf(":");
						}
					}
					printf("]\n");
					printf("    ODR msg type %d, src vm%d, dest vm%d, bid %d, force_discovery %d, rrep_sent %d, hop %d\n", type, getVMNum(ip_src)+1, getVMNum(ip_dest)+1, broadcast_id, force_dis, rrep_sent, hop_cnt);
					
					insertQueue(2, getVMNum(ip_dest), recv_packet, SIZE_SENDMESSAGE);
				}
			}

			if(haveRoute==1 && flag!=1)
			{
				struct sockaddr_ll uniAddr;
				void *uniFrame=(void *)Malloc(SIZE_HEADFRAME+SIZE_SENDMESSAGE);
				bzero(uniFrame, sizeof(*uniFrame));

				uniAddr.sll_family=AF_PACKET;
				uniAddr.sll_protocol=htons(PROTOCOL_ID);
				uniAddr.sll_halen=6;
				uniAddr.sll_ifindex=routing[vmid].out_ifindex;

				int count_temp;
				for(count_temp=0;count_temp<num_interface;count_temp++)
				{
					if(routing[vmid].out_ifindex==if_info[count_temp].if_index)
					{
						break;
					}
				}

				for(count_mac=0;count_mac<6;count_mac++)
				{
					uniAddr.sll_addr[count_mac]=routing[vmid].next_hop[count_mac];
				}
				uniAddr.sll_addr[6]=0xff;
				uniAddr.sll_addr[7]=0xff;

				unsigned short protocol=htons(PROTOCOL_ID);

				memcpy((void *)uniFrame, (void *)(routing[vmid].next_hop), 6);
				memcpy((void *)(uniFrame+6), (void *)(if_info[count_temp].hw_addr), 6);
				memcpy((void *)(uniFrame+12), (void *)&protocol, 2);

				memcpy((void *)(uniFrame+14), (void *)recv_packet, SIZE_SENDMESSAGE);

				Sendto(pfSockfd[count_temp], uniFrame, SIZE_HEADFRAME+SIZE_SENDMESSAGE, 0, (SA *)&uniAddr, sizeof(uniAddr));
				printf("##SEND#: ODR vm%d: ", getVMNum(host_ip)+1);
				printf("[src ");
				for(count_mac=0;count_mac<6;count_mac++)							
				{
					printf("%.2x", if_info[count_temp].hw_addr[count_mac] & 0xff);
					if(count_mac!=5)
					{
						printf(":");		
					}
				}
				printf(",dest ");
				for(count_mac=0;count_mac<6;count_mac++)
				{
					printf("%.2x", routing[vmid].next_hop[count_mac] & 0xff);
					if(count_mac!=5)			
					{
						printf(":");
					}
				}	
				printf("]\n");
				printf("    ODR msg type %d, src vm%d, dest vm%d\n", 2, getVMNum((char *)(uniFrame+18))+1, getVMNum((char *)(uniFrame+36))+1);
			}
		}
	}
}

unsigned short  entry_findPath(char *path_name, struct head_path_port *entry_head)
{
	struct table_path_port *entry_table;

	entry_table=entry_head->table;

	while(entry_table!=NULL)
	{
		if(strcmp(path_name, entry_table->pathname) == 0)
		{
			return entry_table->port;
		}
		else
		{
			entry_table=entry_table->next;
		}
	}

	return 0;
}

unsigned short entry_insertPath(char *path_name, int isperm,struct head_path_port *entry_head)
{
	unsigned short  new_port;
	int exist_port;
	
	struct table_path_port *entry_table;

	for( ; ; )
	{
		new_port=getRand();

		//printf("FUN: %d\n", new_port);

		if(new_port==0)
		{
			continue;
		}

		entry_table=entry_head->table;

		exist_port=0;
		while(entry_table!=NULL)
		{
			if(entry_table->port == new_port)
			{
				exist_port=1;
				break;
			}

			entry_table=entry_table->next;
		}

		if(exist_port==0)
		{
			break;
		}
	}

	struct table_path_port *new_entry=(struct table_path_port *)Malloc(sizeof(struct table_path_port));

	new_entry->port=new_port;
	new_entry->timestamp=getTime();
	sprintf(new_entry->pathname, "%s", path_name);
	new_entry->isperm=isperm;
	new_entry->next=NULL;

	if(entry_head->table == NULL)
	{
		entry_head->table=new_entry;
		return new_port;
	}

	entry_table=entry_head->table;
	while(entry_table->next!=NULL)
	{
		entry_table=entry_table->next;
	}

	entry_table->next=new_entry;

	return new_port;
}

char *getPathByPort(unsigned short port, struct head_path_port *entry_head)
{
	struct table_path_port *entry;
	entry=entry_head->table;

	while(entry!=NULL)
	{
		if(entry->port==port)
		{
			return entry->pathname;
		}
		entry=entry->next;
	}

	return NULL;
}

int route_findRoute(char *ip_dest)
{
	int vmid=getVMNum(ip_dest);

	if(routing[vmid].status==0)
	{
		return -1;
	}
	
	if((getTime()-routing[vmid].timestamp) < staleness)
	{
		return vmid;
	}
	else
	{
		routing[vmid].status=0;
	}
	return -1;
}

void route_updateRoute(char *ip_dest, char *next_hop, int out_ifindex, int hop_cnt, int broadId)
{
	int count_mac;
	int vmid=getVMNum(ip_dest);

	sprintf(routing[vmid].ip_dest, "%s", ip_dest);
	for(count_mac=0;count_mac<6;count_mac++)
	{
		routing[vmid].next_hop[count_mac]=next_hop[count_mac];
	}
	routing[vmid].out_ifindex=out_ifindex;
	routing[vmid].hop_cnt=hop_cnt;
	routing[vmid].timestamp=getTime();
	if(broadId!=-1)
	{
		routing[vmid].broadId=broadId;
	}
	routing[vmid].status=1;
}

void route_insertRoute(char *ip_dest, char *next_hop, int out_ifindex, int hop_cnt, int broadId)
{
	int count_mac;
	int vmid=getVMNum(ip_dest);
		
	sprintf(routing[vmid].ip_dest, "%s", ip_dest);	
	for(count_mac=0;count_mac<6;count_mac++)
	{
		routing[vmid].next_hop[count_mac]=next_hop[count_mac];
	}
	routing[vmid].out_ifindex=out_ifindex;
	routing[vmid].hop_cnt=hop_cnt;
	routing[vmid].timestamp=getTime();
	if(broadId!=-1)
	{
		routing[vmid].broadId=broadId;
	}
	routing[vmid].status=1;
}

void printRoutingTable()
{
	int count;
	int count_mac;
	printf("-----------------Routing Table--------------\n");
	for(count=1;count<10;count++)
	{
		if(routing[count].status==0)
		{
			continue;
		}
		
		printf("DEST:%s, ", routing[count].ip_dest);
		printf("NEXT:");
		for(count_mac=0;count_mac<6;count_mac++)
		{
			printf("%0.2x", routing[count].next_hop[count_mac] & 0xff);
			if(count_mac!=5)
			{
				printf(":");
			}
		}
		printf(", ");
		printf("OUT:%d, ", routing[count].out_ifindex);
		printf("HOP:%d, ", routing[count].hop_cnt);
		printf("BROAD:%d, ", routing[count].broadId);
		printf("TIME:%d", getTime()-routing[count].timestamp);
		printf("\n");
	}	
	printf("--------------------------------------------\n");
}

unsigned short getRand()
{
	return rand()%RAND_BASE;
}

int getBroadcastID()
{
	broadcast_id++;
	return broadcast_id;
}

int getTime()
{
	int currentTime;

	currentTime=time(NULL);

	return currentTime;
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

void insertQueue(int type, int destVM, char *recvline, int len)
{
	void *newline;
	if(type==1)
	{
		newline=(void *)Malloc(SIZE_HEADFRAME+SIZE_RREP);
		memcpy((void *)newline, (void *)recvline, SIZE_HEADFRAME+SIZE_RREP);
	}
	if(type==2)
	{
		newline=(void *)Malloc(SIZE_SENDMESSAGE);
		memcpy((void *)newline, (void *)recvline, SIZE_SENDMESSAGE);
	}
	
	struct table_queue *tqueue;
	tqueue=hqueue->table;
	if(tqueue==NULL)
	{
		tqueue=(struct table_queue *)Malloc(sizeof(struct table_queue));
		tqueue->type=type;
		tqueue->destVM=destVM;
		tqueue->sequence=newline;
		tqueue->len=len;
		tqueue->next=NULL;
		hqueue->table=tqueue;
		return;
	}
	while(tqueue->next!=NULL)
	{
		tqueue=tqueue->next;
	}

	struct table_queue *ntqueue;
	ntqueue=(struct table_queue *)Malloc(sizeof(struct table_queue));
	ntqueue->type=type;
	ntqueue->destVM=destVM;
	ntqueue->sequence=newline;
	ntqueue->len=len;
	ntqueue->next=NULL;

	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
}

int printNumQueue()
{
	printf("@@@@@@@@@@@@@@@@@@\n");
	int num=0;
	struct table_queue *tqueue;
	tqueue=hqueue->table;
	while(tqueue!=NULL)
	{
		printf("Type:%d , DestVM:%d.\n", tqueue->type, tqueue->destVM);
		num++;
		tqueue=tqueue->next;
	}
	printf("@@@@@@@@@@@@@@@@@@\n");
	return num;
}
/*
void printQueue()
{
	struct table_queue *tq;
	tq=hqueue->table;
	while(tq!=NULL)
	{
		if(tq->type==1)
		{
			printf("!Type %d, Dest %d, !");
		}
		if(tq->type==2)
		{
			
		}
		printf();
		tq=tq->next;
	}
}*/
