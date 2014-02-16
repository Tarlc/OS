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
sthread_t dead_thread;
int initted = 0;



/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/

void sthread_user_init(void) {
  //initialize the whole system
  
  thread_queue = sthread_new_queue();
  dead_thread_queue = sthread_new_queue();
  dead_thread = NULL; 
  
  
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
  
  sthread_start_func_t func = active_thread->start_routine_ptr;
  sthread_exit((func)(active_thread->args));
  exit(0);
  
  
  
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
void free_dead_threads(void){
  while (!sthread_queue_is_empty(dead_thread_queue)){
    sthread_t temp = sthread_dequeue(dead_thread_queue);
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
  
  sthread_t new = malloc(sizeof(sthread_t));
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
  /*
    if(dead_thread_queue == NULL){
    dead_thread_queue = sthread_new_queue();
    }
    
    if(dead_thread != NULL){
    if(sthread_queue_is_empty(dead_thread_queue)){
    sthread_enqueue(dead_thread_queue,dead_thread);
    } else{
    dead_thread_queue = sthread_new_queue();
    sthread_enqueue(dead_thread_queue,dead_thread);
    }
    dead_thread = NULL;
    }
    
    if(sthread_queue_is_empty(thread_queue)){
    sthread_free_queue(thread_queue);
    exit(0);
    } else{
    while(!sthread_queue_is_empty(active_thread->join_queue)){
    sthread_enqueue(thread_queue,sthread_dequeue(active_thread->join_queue));
    }
    sthread_free_queue(active_thread->join_queue);
    dead_thread = active_thread;
    if(!sthread_queue_is_empty(dead_thread_queue)){
    sthread_enqueue(dead_thread_queue,dead_thread);
    }else{
    dead_thread_queue = sthread_new_queue();
    sthread_enqueue(dead_thread_queue,dead_thread);
    }
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(dead_thread->saved_ctx,active_thread->saved_ctx);
    }*/
  
  /*if (dead_thread != NULL) {
  // free the memory of the last dead thread
  sthread_free_ctx(dead_thread->saved_ctx);
  free(dead_thread);
  dead_thread = NULL;
  }
  
  if(sthread_queue_is_empty(thread_queue)){
  // the last thread terminated, exit the process
  sthread_free_queue(thread_queue);
  exit(0);
  } else {
  // switch to the next executable thread
  dead_thread = active_thread;
  active_thread = sthread_dequeue(thread_queue);
  sthread_switch(dead_thread->saved_ctx, active_thread->saved_ctx);
  }*/
  
  // if no threads are waiting on this thread, simply kill it



  //---------------------------------------------------------------



  // THIS SHOULD WORK, BUT FREEING active_thread BEFORE CONTEXT SWITCH FAILS
    
  // FREEING THREAD

  sthread_t dead_thread = active_thread;

  // there aren't any threads waiting for this thread to terminate

  // there are threads waiting on this thread to terminate
  while (!sthread_queue_is_empty(active_thread->join_queue)){
    sthread_t temp = sthread_dequeue(active_thread->join_queue);

    // add ret to the thread's waiting on this thread
    temp->return_value = ret;

    sthread_enqueue(thread_queue, temp);
  }

  /*
  // add this thread to the dead_thread_queue
  sthread_enqueue(dead_thread_queue, active_thread);
  */
  // SWITCHING TO NEW THREAD

  if(!sthread_queue_is_empty(thread_queue)){      
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(dead_thread->saved_ctx, active_thread->saved_ctx); 
  }
  else {
    sthread_free_queue(thread_queue);
    while(!sthread_queue_is_empty(dead_thread_queue)){
      sthread_t temp = sthread_dequeue(dead_thread_queue);
      sthread_free_ctx(temp->saved_ctx);
      free(temp);
    }
    sthread_free_queue(thread_queue);
    //    exit(0);
  }

}
  


  //-------------------------------------------------------------------------
  /*
// otherwise make all waiting threads ready and put current thread
// on dead_threads queue
  
//  free_dead_threads();


  if (dead_thread != NULL) {
    // free the memory of the last dead thread 
    sthread_free_ctx(dead_thread->saved_ctx);
    free(dead_thread);
    dead_thread = NULL;
  }

  // if the dead_thread had any thread's waiting for it, put them
  // on the ready queue
  while (!sthread_queue_is_empty(active_thread->join_queue)){
    sthread_t temp = sthread_dequeue(active_thread->join_queue);
    sthread_enqueue(thread_queue, temp);
  }


  if(sthread_queue_is_empty(thread_queue)){
    // the last thread terminated, exit the process
    sthread_free_queue(thread_queue);
    exit(0);
  } else {
    // switch to the next executable thread
    dead_thread = active_thread;
    //    sthread_enqueue(dead_thread_queue, dead_thread);
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(dead_thread->saved_ctx, active_thread->saved_ctx);
    }
}

*/
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
  }
  //return t->return_value;
  //return NULL;
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
  sthread_t holder;
  sthread_queue_t waiting_threads;
};

sthread_mutex_t sthread_user_mutex_init() {
  sthread_mutex_t ret = (sthread_mutex_t)malloc(sizeof(_sthread_mutex));
  ret->lock = 0;
  ret->waiting_threads = sthread_new_queue();
  return ret;
}

void sthread_user_mutex_free(sthread_mutex_t lock) {
  sthread_free_queue(lock->waiting_threads);
  free(lock);
}

void sthread_user_mutex_lock(sthread_mutex_t lock) {
  if (lock == 0){
    lock->holder = active_thread;
    lock = 1;
  }
  else {
    while (lock == 1
    
    
}

void sthread_user_mutex_unlock(sthread_mutex_t lock) {
  if (sthread_queue_is_empty(lock->waiting_threads))
    lock->lock = 0;
  else
    lock->holder = sthread_dequeue(lock->waiting_threads);
}


struct _sthread_cond {
  /* Fill in condition variable structure */
  sthread_queue_t waiting_threads;
};

sthread_cond_t sthread_user_cond_init(void) {
  sthread_cond_t ret = (sthread_cond_t)malloc(sizeof(_sthread_cond));
  ret->waiting_threads = sthread_new_queue();
}

void sthread_user_cond_free(sthread_cond_t cond) {\
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


}
