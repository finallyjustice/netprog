/** File:		network.h
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

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#include "unp.h"
#include "hw_addrs.h"

#define TIMEOUT 3

#define LEN_FRAME 150
#define SIZE_TYPE 4
#define SIZE_IP 16
#define SIZE_PORT 2
#define SIZE_HOP 4
#define SIZE_PACKET 50
#define SIZE_LEN 4

#define SIZE_SENDMESSAGE 80
#define SIZE_RREQ 52
#define SIZE_RREP 44
#define SIZE_HEADFRAME 14

#define SERV_WKPORT 1079
#define SERV_WKPATH "/tmp/welknown_server952"
#define ODR_PATH "/tmp/odr_path952"

#define PROTOCOL_ID 0x8952

#define RAND_BASE 60000

#define PORT_TIMEOUT 60

#define MAX_HOP 15

struct interface_info
{
	char if_name[10];
	char ip_addr[SIZE_IP];
	int if_index;
	char hw_addr[6];
	struct sockaddr_ll odrPFAddr;
};

struct head_path_port
{
	struct table_path_port *table;
};

struct table_path_port
{
	unsigned short port;
	char pathname[30];
	struct table_path_port *next;
	int timestamp;
	int isperm;
};

struct head_routing
{
	struct table_routing *table;
};

struct table_routing
{
	char ip_dest[INET_ADDRSTRLEN];
	char next_hop[6];
	int out_ifindex;
	int hop_cnt;
	int timestamp;
	struct table_routing *next;
};

struct head_qmessage
{
	struct table_qmessage *table;	
};

struct table_qmessage
{
	int type;
	void *message;
};

struct routingTable
{
	char ip_dest[INET_ADDRSTRLEN];
	char next_hop[6];
	int out_ifindex;
	int hop_cnt;
	int timestamp;
	int broadId;
	int status;
};

struct head_queue
{
	struct table_queue *table;
};

struct table_queue
{
	int type;
	int destVM;
	void *sequence;
	int len;
	struct table_queue *next;
};
