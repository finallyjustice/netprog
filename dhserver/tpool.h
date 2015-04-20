/** File:		tpool.h
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

/* header file for thread pool */

#ifndef __TPOOL_H__
#define __TPOOL_H__

#include <pthread.h>

typedef struct tpool_job
{
	void*			 (*routine)(void *);
	void			 *arg;
	struct tpool_job *next;
}tpool_job_t;

typedef struct tpool
{
	int max_thr;
	pthread_t *tid;
	int *pid;
	tpool_job_t *queue_head;
	tpool_job_t *queue_tail;
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_ready;
}tpool_t;

int tpool_create(int max_thr);
void * do_thread(void *arg);
int tpool_add_job(void* (*routine)(void *), void *arg);

#endif
