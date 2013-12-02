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

#include "unp.h"
#include "hw_addrs.h"

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <time.h>

#define SIZE_BUF 4096
#define SIZE_VM 10
#define SIZE_IP 16
#define SIZE_FIXSOURCE 84

#define RT_PROTOCOL 253
#define ARP_PROTOCOL 0x1029
#define ARP_IDENTIFICATION 1217
#define IP_IDENTIFICATION 19918

#define MULTIADDR "234.234.1.29"
#define MULTIPORT 2119

#define ARP_WELPATH "/arp_wellknown211"

struct arp_pair
{
	char ipaddr[SIZE_IP];
	char hwaddr[6];
};

struct hwaddr
{
	int sll_ifindex;
	unsigned short sll_hatype;
	unsigned short sll_halen;
	unsigned short sll_addr[6];
};

struct arp_cache
{
	char ipaddr[SIZE_IP];
	char hwaddr[6];
	int sll_ifindex;
	unsigned short sll_hatype;
	int sockfd;
	int status;
	int waiting;
};

struct arpMes
{
	int op;
	char sender_mac[6];
	char sender_ip[SIZE_IP];
	char target_mac[6];
	char target_ip[SIZE_IP];
	int identification;
};
