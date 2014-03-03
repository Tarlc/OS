#ifndef __WEB_QUEUE_H__
#define __WEB_QUEUE_H__

// pointer to _queue struct
typedef struct _queue *queue;

// allocates space for a queue with a max number of elements of array_size
queue create_queue(int array_size);

// free's queue
void free_queue(queue q);

// enqueues an element to the queue if there is space and returns 1.
// otherwise it returns 0
int enqueue(queue q, int elem);

// dequeues an element from the queue if there is one and returns 1.
// otherwise it returns 0. Returns the queue element to the given
// parameter ret
int dequeue(queue q, int* ret);

// returns 0 if it isn't empty and 1 if it is
int queue_is_empty(queue q);

// returns 0 if it isn't full and 1 if it is
int queue_is_full(queue q);

#endif
