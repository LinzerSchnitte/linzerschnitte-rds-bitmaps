//util.c
//LinzerSchnitte Entangled Sparks
//Josh Gardiner
//2014 07 12
//Copyright (C) Josh Gardiner 2014
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "util.h"



void print_warranty(void)
{
    fprintf(stderr,"\n");
    fprintf(stderr,"Disclaimer of Warranty.\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY\n");
    fprintf(stderr,"APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT\n");
    fprintf(stderr,"HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY\n");
    fprintf(stderr,"OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,\n");
    fprintf(stderr,"THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n");
    fprintf(stderr,"PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM\n");
    fprintf(stderr,"IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF\n");
    fprintf(stderr,"ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n");
    exit(EXIT_FAILURE);
}

void print_conditions(void)
{
    fprintf(stderr,"\n");
    fprintf(stderr,"see the License file in the directory or see <http://www.gnu.org/licenses/");
    fprintf(stderr,"\n");
    exit(EXIT_FAILURE);
}

void print32bits(uint32_t n)
{
    unsigned int i;
    i = 1<<(sizeof(n) * 8 - 1);

    while (i > 0) {
        if (n & i)
            fprintf(stderr,"1");
        else
            fprintf(stderr,"0");
        i >>= 1;
    }
    fprintf(stderr,"\n");
}

void print64bits(uint64_t n)
{
    uint64_t i;
    i = (uint64_t)1<<(sizeof(n) * 8 - 1);

    while (i > 0) {
        if (n & i)
            fprintf(stderr,"1");
        else
            fprintf(stderr,"0");
        i >>= 1;
    }
    fprintf(stderr,"\n");
}

double int2double_time(uint32_t seconds, uint32_t useconds)
{
    double value;
    value = (double)seconds + (double)useconds/USECS_PER_SEC;
    return value;
}

typedef struct queueRDS {
  RDS_Command contents[QUEUE_SIZE];
  int front;
  int count;
} queueRDS;


RDS_Queue QueueCreate(void)
{
    RDS_Queue queue;

    queue = (RDS_Queue)malloc(sizeof(queueRDS));

    queue->front = 0;
    queue->count = 0;

    return queue;
}

void QueueEnter(RDS_Queue queue, RDS_Command command)
{
    int elementPosition;

    elementPosition = (queue->front + queue->count)% QUEUE_SIZE; // warp
    queue->contents[elementPosition] = command;

    queue->count++;
    // check if queue is full 
    assert(queue->count < QUEUE_SIZE+1);

}

void QueueInsert(RDS_Queue queue, RDS_Command command, int elementPosition)
{
    assert (elementPosition >= 0 && elementPosition< QUEUE_SIZE);
    queue->contents[elementPosition]= command;
    queue->count++;
//    queue->contents[elementPosition].status==DIRTY;
}


RDS_Command QueueDelete(RDS_Queue queue)
{
    // Need to 
    RDS_Command command;
    printf("\t\tQ LEN: %d\n", queue->count);

    command = queue->contents[queue->front];

    queue->front++;
    queue->front %= QUEUE_SIZE;
    queue->count--;
    return command;
}

int QueueEmpty(RDS_Queue queue)
{
    if ( queue->count==0 )
    {
        return 1;
    } else 
    {
        return 0;
    }
}
