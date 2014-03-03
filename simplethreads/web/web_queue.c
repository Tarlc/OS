/* Implements queue abstract data type. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "web_queue.h"

/* Each link in the queue stores a queue_element and
 * a pointer to the next link in the queue. */
typedef struct _queue_link {
  queue_element* elem;
  struct _queue_link* next;
} queue_link;

/* This is the actual implementation of the queue struct that
 * is declared in queue.h. */
struct _queue {
  queue_link* head;
};

queue* queue_create() {
  queue* q = (queue*) malloc(sizeof(queue));

  q->head = NULL;
  return q;
}

/* Private */
static queue_link* queue_new_element(queue_element* elem) {
  queue_link* ql = (queue_link*) malloc(sizeof(queue_link));

  ql->elem = elem;
  ql->next = NULL;

  return ql;
}

void queue_append(queue* q, queue_element* elem) {
  assert(q != NULL);

  // fixed bug where appending to an empty list segfaulted by
  // checking to see if the list was empty or not
  if (q->head == NULL)
    q->head = queue_new_element(elem);

  else{
    // Find the last link in the queue.
    queue_link* cur;
   
    for (cur = q->head; cur->next; cur = cur->next) {}
    
    // Append the new link.
    cur->next = queue_new_element(elem);
  }
}

bool queue_remove(queue* q, queue_element** elem_ptr) {
  queue_link* old_head;

  assert(q != NULL);
  assert(elem_ptr != NULL);
  if (queue_is_empty(q)) {
    return false;
  }

  *elem_ptr = q->head->elem;
  old_head = q->head;
  q->head = q->head->next;
  
  // fixed memory leak by freeing the old_head
  free(old_head);

  return true;
}

bool queue_is_empty(queue* q) {
  assert(q != NULL);
  return q->head == NULL;
}

/* private */
static bool queue_count_one(queue_element* elem, queue_function_args* args) {
  size_t* count = (size_t*) args;
  *count = *count + 1;
  return true;
}

size_t queue_size(queue* q) {
  size_t count = 0;
  queue_apply(q, queue_count_one, &count);
  return count;
}

bool queue_apply(queue* q, queue_function qf, queue_function_args* args) {
  assert(q != NULL && qf != NULL);

  if (queue_is_empty(q))
    return false;
  /*
  for (queue_link* cur = q->head; cur; cur = cur->next) {
    if (!qf(cur->elem, args))
      break;
      }*/
  queue_link* cur = q->head;
  while (cur){
   if (!qf(cur->elem, args))
      break;
   cur = cur->next;
  } 


  return true;
}

void queue_reverse(queue* q){
  // lists of size 0 or 1 are already reversed
  if (q->head == NULL || q->head->next == NULL)
    return;

  queue_link* prev = NULL;
  queue_link* current = q->head;
  queue_link* next = q->head->next;

  // conceptually, pop elements off the current stack and push them
  // onto a "new" stack until the old stack is empty
  while (next != NULL){
    current->next = prev;
    prev = current;
    current = next;
    next = current->next;
  }

  // update q's head to be the new list
  current->next = prev;
  q->head = current;
}

void queue_sort(queue* q, queue_compare qc){
  // lists of size 0 or 1 are already sorted
  if (q->head == NULL || q->head->next == NULL)
    return;

  // sort the first two elements
  if (qc(q->head, q->head->next) > 0){
    queue_link* temp = q->head;
    q->head = q->head->next;
    temp->next = q->head->next;
    q->head->next = temp;
  }

  queue_link* sorted = q->head->next; // last element of the sorted elements
  queue_link* next_to_insert, *current;
  while (sorted->next != NULL){
    next_to_insert = sorted->next;
    // next value to be inserted is greater than the largest sorted value,
    // it is already sorted
    if (qc(sorted, next_to_insert) <= 0){
      sorted = sorted->next;
      continue;
    }
    else {
      // skip over next_to_insert since it must be before the last sorted element
      sorted->next = next_to_insert->next;
      // the next value to be inserted is less than the head of the queue
      if (qc(q->head, next_to_insert) > 0){
	next_to_insert->next = q->head;
	q->head = next_to_insert;
      }
      // the next value to be inserted is somewhere in the middle of the
      // sorted elements
      else {
	current = q->head;
	while (current->next != sorted){
	  if (qc(current->next, next_to_insert) > 0){
	    next_to_insert->next = current->next;
	    current->next = next_to_insert;
	    break;
	  }
	  else
	    current = current->next;
	}
      }
    }
  }
}
