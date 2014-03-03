#include <stdlib.h>
#include <stdio.h>
#include "web_queue.h"

// implement queue with an array

struct _queue{
  int array_size;
  int* array;
  int front;
  int back;
  int size;
};

queue create_queue(int array_size){
  queue ret = (queue)malloc(sizeof(struct _queue));
  ret->array_size = array_size;
  ret->array = (int*)malloc(sizeof(int) * array_size);
  ret->front = 0;
  ret->back = 0;
  ret->size = 0;
  return ret;
}

void free_queue(queue q){
  free(q->array);
  free(q);
}

// enqueues an element to the queue. returns on 0 fail and 1 on success
int enqueue(queue q, int elem){
  if (queue_is_full(q))
    return 0;

  q->array[q->back] = elem;
  q->back = (q->back + 1) % q->array_size;
  q->size++;
  return 1;
}

// dequeue an element from the queue to ret.
// returns 0 on fail and 1 on success
int dequeue(queue q, int* ret){
  if (queue_is_empty(q))
    return 0;
  
  *ret = q->array[q->front];
  q->front = (q->front + 1) % q->array_size;
  q->size--;
  return 1;
}

// returns 0 is it isn't empty, and 1 if it is empty
int queue_is_empty(queue q){
  if (q->size == 0)
    return 1;
  else
    return 0;
}

// returns 0 is it isn't full, and 1 if it is full
int queue_is_full(queue q){
  if (q->size == q->array_size)
    return 1;
  else
    return 0;
}
