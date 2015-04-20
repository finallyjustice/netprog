/** File:		tpool.c
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

/* implementation file for thread pool */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "tpool.h"

static tpool_t *tpool = NULL;

int tpool_create(int max_thr)
{
	int c;
	tpool = (tpool_t *)malloc(sizeof(tpool_t));
	if(tpool == NULL)
	{
		printf("%s: tpool malloc failed\n", __FUNCTION__);
		return -1;
	}

	tpool->max_thr = max_thr;

	tpool->tid = (pthread_t *)malloc(sizeof(pthread_t)*max_thr);
	if(tpool->tid == NULL)
	{
		printf("%s: tid malloc failed\n", __FUNCTION__);
		return -1;
	}
	
	tpool->pid = (int *)malloc(sizeof(int)*max_thr);
	if(tpool->pid == NULL)
	{
		printf("%s: pid malloc failed\n", __FUNCTION__);
		return -1;
	}

	tpool->queue_head = NULL;
	tpool->queue_tail = NULL;

	if(pthread_mutex_init(&tpool->queue_mutex, NULL) != 0)
	{
		printf("%s: pthread_mutex_init failed, errno: %d, error: %s\n",
				__FUNCTION__, errno, strerror(errno));
		return -1;
	}

	if(pthread_cond_init(&tpool->queue_ready, NULL) != 0)
	{
		printf("%s: pthread_cond_init failed, errnoL %d, error: %s\n",
				__FUNCTION__, errno, strerror(errno));
		return -1;
	}

	for(c=0; c<max_thr; c++)
	{
		//create new threads
		if(pthread_create(&tpool->tid[c], NULL, do_thread, &c) != 0)
		{
			printf("%s: pthread_create failed, errno: %d, error: %s\n", 
					__FUNCTION__, errno, strerror(errno));
			return -1;
		}
	}

	return 1;
}

void * do_thread(void *arg)
{
	tpool_job_t *job;
	printf("Init a new thread\n");

	while(1)
	{
		sleep(10);
		pthread_mutex_lock(&tpool->queue_mutex);
		
		while(tpool->queue_head == NULL)
		{
			pthread_cond_wait(&tpool->queue_ready, &tpool->queue_mutex);
		}

		job = tpool->queue_head;
		tpool->queue_head = tpool->queue_head->next;
		if(tpool->queue_head == NULL) tpool->queue_tail=NULL;

		pthread_mutex_unlock(&tpool->queue_mutex);
		
		printf("Process a job\n");
		job->routine(job->arg);
		free(job);
	}
}

int tpool_add_job(void* (*routine)(void *), void *arg)
{
	tpool_job_t *job;

	if(!routine)	
	{
		printf("%s: Invalid routine\n", __FUNCTION__);
		return -1;
	}

	job = (tpool_job_t *)malloc(sizeof(tpool_job_t));
	if(job == NULL)
	{
		printf("%s: malloc failed\n", __FUNCTION__);
		return -1;
	}

	job->routine = routine;
	job->arg = arg;
	job->next = NULL;

	pthread_mutex_lock(&tpool->queue_mutex);

	if(tpool->queue_head == NULL)
	{
		printf("Add init thread\n");
		tpool->queue_head = job;
		tpool->queue_tail = job;
	}
	else
	{
		printf("Add new thread\n");
		tpool->queue_tail->next = job;
		tpool->queue_tail = job;
	}

	pthread_cond_signal(&tpool->queue_ready);
	pthread_mutex_unlock(&tpool->queue_mutex);

	return 1;
}
