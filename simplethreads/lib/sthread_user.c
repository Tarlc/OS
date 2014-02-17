/* Simplethreads Instructional Thread Package
 * 
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 *    You need to implement the routines in this file.
 *
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <sthread.h>
#include <sthread_queue.h>
#include <sthread_user.h>
#include <sthread_ctx.h>
#include <sthread_user.h>

struct _sthread {
  void* args;
  int joinable;
  sthread_start_func_t start_routine_ptr;
  sthread_ctx_t *saved_ctx;
  sthread_queue_t join_queue;
  void* return_value;
};

sthread_t active_thread;
sthread_queue_t thread_queue;
sthread_queue_t dead_thread_queue;
//sthread_t dead_thread;
int initted = 0;



/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/

void sthread_user_init(void) {
  //initialize the whole system
  
  thread_queue = sthread_new_queue();
  dead_thread_queue = sthread_new_queue();
  //  dead_thread = NULL; 
  
  
  sthread_t main_thread = malloc(sizeof(struct _sthread));
  
  main_thread->joinable = 0;
  main_thread->args = NULL;
  main_thread->start_routine_ptr = NULL;
  main_thread->join_queue = sthread_new_queue();
  main_thread->saved_ctx = sthread_new_blank_ctx();
  
  // set the main thread as the active thread
  active_thread = main_thread;
  
  initted = 1;
}

void runner(void) {
  
  void* ret = active_thread->start_routine_ptr(active_thread->args);
  sthread_user_exit(ret);
  // exit(0);
  
  
  
  //active_thread->return_value = active_thread->start_routine_ptr(active_thread->args);
  //if (sthread_queue_is_empty(active_thread->join_queue))
  // sthread_user_exit((void*)0);
  
  /*else{
    sthread_enqueue(dead_threads, active_thread);
    }*/
  //sthread_enqueue(deadthread_queue, active_thread);
  
  //exit(0);
  
}


// Free's all of the threads on the dead_thread_queue
void free_dead_threads(){
  while (!sthread_queue_is_empty(dead_thread_queue)){
    sthread_t temp = sthread_dequeue(dead_thread_queue);
    if (!sthread_queue_is_empty(temp->join_queue)){
      printf("ERROR freeing thread with threads waiting for it\n");
      exit(-1);
    }
    sthread_free_queue(temp->join_queue);
    sthread_free_ctx(temp->saved_ctx);
    free(temp);
  }
}

sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg,
                              int joinable) {
  //create a new thread and make it runnable
  if (!initted) {
    printf("[sthread_user_create] Sthread is not initialized.\n");
    exit(-1);
  }
  
  free_dead_threads();

  if (start_routine == NULL) {
    printf("[sthread_user_create] Start routine is null.\n");
    return NULL;
  }
  
  sthread_t new = malloc(sizeof(struct _sthread));
  new->args = arg;
  new->joinable = joinable;
  new->start_routine_ptr = start_routine;
  new->saved_ctx = sthread_new_ctx(runner);
  new->join_queue = sthread_new_queue();
  sthread_enqueue(thread_queue,new);
  
  return new;
}

void sthread_user_exit(void *ret) {
  //exit current thread
  if (!initted) {
    printf("[sthread_user_exit] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();

  // put the dead thread on dead_thread_queue to be freed later
  sthread_enqueue(dead_thread_queue, active_thread);

  // put thread's waiting for this thread on ready queue
  while(!sthread_queue_is_empty(active_thread->join_queue)){
    sthread_t temp = sthread_dequeue(active_thread->join_queue);
    temp->return_value = ret;
    sthread_enqueue(thread_queue, temp);
  }

  // switch to another thread to run
  sthread_t dead_thread = active_thread;
  active_thread = sthread_dequeue(thread_queue);
  sthread_switch(dead_thread->saved_ctx,active_thread->saved_ctx);
}


void* sthread_user_join(sthread_t t) {
  //wait for the specified thread to exit
  if (!initted) {
    printf("[sthread_user_yield] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();
  
  if(t->joinable) {
    //sanity check
    if(t->join_queue == NULL){
      t->join_queue = sthread_new_queue();
    }
    sthread_t temp = active_thread;
    sthread_enqueue(t->join_queue, active_thread);
    //    active_thread = sthread_dequeue(thread_queue);
    active_thread = t;
    sthread_switch(temp->saved_ctx,active_thread->saved_ctx);
    return active_thread->return_value;
  } else {
    printf("not joinable\n");
    return NULL;
  }
  //return t->return_value;
  //  return NULL;
}


void sthread_user_yield(void) {
  //give up the cpu
  if (!initted) {
    printf("[sthread_user_yield] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();

  if(!sthread_queue_is_empty(thread_queue)){
    sthread_t old_thread = active_thread;
    sthread_enqueue(thread_queue,active_thread);
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(old_thread->saved_ctx,active_thread->saved_ctx);
  }
  // only runnable thread
  else
    return;
}

/* Add any new part 1 functions here */


/*********************************************************************/
/* Part 2: Synchronization Primitives                                */
/*********************************************************************/

struct _sthread_mutex {
  /* Fill in mutex data structure */
  int lock;
  sthread_queue_t waiting_threads;
};

sthread_mutex_t sthread_user_mutex_init() {
  sthread_mutex_t ret = (sthread_mutex_t)malloc(sizeof(struct _sthread_mutex));
  ret->lock = 0;
  ret->waiting_threads = sthread_new_queue();
  return ret;
}

void sthread_user_mutex_free(sthread_mutex_t lock) {
  if (!sthread_queue_is_empty(lock->waiting_threads)){
    printf("Threads are still waiting on the mutex, so it can't be freed\n");
    return;
  }
  else {
    sthread_free_queue(lock->waiting_threads);
    free(lock);
  }
}

void sthread_user_mutex_lock(sthread_mutex_t lock) {
  
  if (lock->lock == 0){
    lock->lock = 1;
  }
  else {
    sthread_t temp = active_thread;
    sthread_enqueue(lock->waiting_threads, temp);
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(temp->saved_ctx, active_thread->saved_ctx);
  }    
}

void sthread_user_mutex_unlock(sthread_mutex_t lock) {
  // no threads waiting for the lock, sets lock to unlocked
  if (sthread_queue_is_empty(lock->waiting_threads))
    lock->lock = 0;
  // thread(s) waiting for the lock, pulls one thread from
  // waiting queue to ready queue implicitly passing the lock
  else{
    sthread_t temp = sthread_dequeue(lock->waiting_threads);
    sthread_enqueue(thread_queue, temp);
  }
}

/*
typedef struct _thread_and_lock {
  sthread_t thread;
  sthread_mutex_t lock;
} *thread_and_lock; 
*/
struct _sthread_cond {
  /* Fill in condition variable structure */
  sthread_queue_t waiting_threads;
};


//*****************************************************************
// NOTE condition variables don't seem to be working even though the
// test passes. Could be a problem with join/part 1 or it could be
// a problem with the condition variables themselves
//*****************************************************************



sthread_cond_t sthread_user_cond_init(void) {
  sthread_cond_t ret = (sthread_cond_t)malloc(sizeof(struct _sthread_cond));
  ret->waiting_threads = sthread_new_queue();
  return ret; 
}

void sthread_user_cond_free(sthread_cond_t cond) {
  sthread_free_queue(cond->waiting_threads);
  free(cond);  
}

void sthread_user_cond_signal(sthread_cond_t cond) {
  if (!sthread_queue_is_empty(cond->waiting_threads)){
    sthread_t temp = sthread_dequeue(cond->waiting_threads);
    sthread_enqueue(thread_queue, temp);
  }
}

void sthread_user_cond_broadcast(sthread_cond_t cond) {
  while (!sthread_queue_is_empty(cond->waiting_threads)){
    sthread_t temp = sthread_dequeue(cond->waiting_threads);
    sthread_enqueue(thread_queue, temp);
  }
}



void sthread_user_cond_wait(sthread_cond_t cond,
                            sthread_mutex_t lock) {
  sthread_user_mutex_unlock(lock);
  sthread_t temp = active_thread;
  active_thread = sthread_dequeue(thread_queue);
  sthread_enqueue(cond->waiting_threads, temp);
  sthread_switch(temp->saved_ctx, active_thread->saved_ctx);
  sthread_user_mutex_lock(lock);
}
