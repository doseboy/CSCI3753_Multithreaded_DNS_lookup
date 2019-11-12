#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

typedef struct q_node
{
	void*	memory;
} q_node;

typedef struct queue_struct
{
	q_node*	arr;
	int	front;
	int	rear;
	int	max;
} queue;

int	q_init(queue* q, int size);
int	q_is_empty(queue* q);
int	q_is_full(queue* q);
int	enqueue(queue* q, void* memory);
void*	dequeue(queue* q);
void	q_free(queue* q);

#endif
