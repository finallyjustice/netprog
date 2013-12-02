/** File:		hw_addr.h
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

/* Our own header for the programs that need hardware address info. */

#include <stdio.h>
#include <sys/socket.h>

#define	IF_NAME 16	/* same as IFNAMSIZ    in <net/if.h> */
#define	IF_HADDR 6	/* same as IFHWADDRLEN in <net/if.h> */
#define	IP_ALIAS 1	/* hwa_addr is an alias */

struct hwa_info 
{
  char if_name[IF_NAME];	/* interface name, null terminated */
  char if_haddr[IF_HADDR];	/* hardware address */
  int if_index;		/* interface index */
  short ip_alias;		/* 1 if hwa_addr is an alias IP address */
  struct sockaddr  *ip_addr;	/* IP address */
  struct hwa_info  *hwa_next;	/* next of these structures */
};


/* function prototypes */
struct hwa_info	*get_hw_addrs();
struct hwa_info	*Get_hw_addrs();
void free_hwa_info(struct hwa_info *);

