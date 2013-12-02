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

#include <math.h>
#include <stdlib.h>
#include "unp.h"
#include "unpifiplus.h"
#include "unpthread.h"
#include "unprtt.h"
//#include "parameter.h"

#define SIZE_PAYLOAD 512
#define LEN_MESSAGE 504

struct socket_content
{
	int sockfd;
	struct in_addr ip;
	struct in_addr mask;
	struct in_addr subnet;	
};

struct server_configuration
{
	int serv_port;
	int serv_win_size;
};

struct client_configuration
{
	char *serv_ip;
	int serv_port;
	char *filename;
	int cli_win_size;
	int seed;
	float probability;
	int recv_mean;
};

struct payload
{
	int sequence;
	uint32_t ts;
	char message[504];
};

struct ackheader
{
	int ack;
	uint32_t ts;
	int win_size;
};
