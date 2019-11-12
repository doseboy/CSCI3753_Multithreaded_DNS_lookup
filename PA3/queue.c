#include <stdlib.h>
#include "queue.h"

int q_init(queue* q, int size)
{
	int i;

	if (size>0) {q->max = size;}
	else {q->max = 50;}
	q->arr = malloc(sizeof(q_node) * (q->max));
	if (!(q->arr)) {return -1;}
	for (i=0; i < q->max; i++)
	{
		q->arr[i].memory = NULL;
	}
	q->front = 0;
	q->rear  = 0;
	return q->max;
}

int q_is_empty(queue* q)
{
	/*conditional to find out if q is empty
		-returns 1 (True) for empty ass q
		-returns 0 (False) for full ass q
	*/

	if ((q->front == q->rear) && (q->arr[q->front].memory == NULL)) {return 1;}
	else{return 0;}
}

int q_is_full(queue* q)
{
	if ((q->front == q->rear) && (q->arr[q->front].memory != NULL)) {return 1;}
	else{return 0;}
}

int enqueue(queue* q, void* element)
{
	if (q_is_full(q)) {return -1;}
	q->arr[q->rear].memory = element;
	q->rear = ((q->rear+1) % q->max);
	return 0;
}

void* dequeue(queue* q)
{
	void* d_element; //dequeued element
	if (q_is_empty(q)){return NULL;}
	d_element = q->arr[q->front].memory;
	q->arr[q->front].memory = NULL;
	q->front = ((q->front+1) % q->max);
	return d_element;
}

void q_free(queue* q)
{
	while(!q_is_empty(q))
	{
		dequeue(q);
	}
	free(q->arr);
}


