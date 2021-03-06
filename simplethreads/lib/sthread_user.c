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
#include <sthread_preempt.h>

struct _sthread {
  void* args;
  int joinable;
  sthread_start_func_t start_routine_ptr;
  sthread_ctx_t *saved_ctx;
  sthread_queue_t join_queue;
  void* return_value;
  int is_actually_dead; // -1 is main, 0 is still runnable, 1 is dead
};

sthread_t active_thread;
sthread_queue_t thread_queue;
sthread_queue_t dead_thread_queue;
sthread_queue_t return_value_queue;
//sthread_t dead_thread;
int initted = 0;


/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/

// free's the dead threads
void free_dead_threads(void);

// wrapper function for starting/exiting threads
void runner(void);

void sthread_user_dispatcher(void);

// takes the old thread and context switches to a new thread
void context_switch(sthread_t old);

void sthread_user_init(void) {
  //initialize the whole system
  
  thread_queue = sthread_new_queue();
  dead_thread_queue = sthread_new_queue();
  return_value_queue = sthread_new_queue();
  //  dead_thread = NULL; 
  
  
  sthread_t main_thread = malloc(sizeof(struct _sthread));
  
  main_thread->joinable = 0;
  main_thread->args = NULL;
  main_thread->start_routine_ptr = NULL;
  main_thread->saved_ctx = sthread_new_blank_ctx();
  main_thread->is_actually_dead = -1; // unique for main thread
  
  // set the main thread as the active thread
  active_thread = main_thread;
  
  initted = 1;

  sthread_preemption_init(sthread_user_dispatcher,10);
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
  new->return_value = NULL;
  new->is_actually_dead = 0;
  int old_interupt_value = splx(HIGH);
  sthread_enqueue(thread_queue,new);
  splx(old_interupt_value);
  return new;
}

void sthread_user_exit(void *ret) {
  //exit current thread
  if (!initted) {
    printf("[sthread_user_exit] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();

  int oldvalue = splx(HIGH);

  // exiting from main thread
  if (active_thread->is_actually_dead == -1){
    // make sure other thread's finish
    while (!sthread_queue_is_empty(thread_queue)){
      sthread_t temp = sthread_dequeue(thread_queue);
      sthread_user_join(temp);
    }

    // free the return_value_queue nodes
    while(!sthread_queue_is_empty(return_value_queue)){
      sthread_dequeue(return_value_queue);
    }

    // free overhead queue's
    sthread_free_queue(thread_queue);
    sthread_free_queue(dead_thread_queue);
    sthread_free_queue(return_value_queue);
  }
  // not main thread
  else {
    // put the dead thread on dead_thread_queue to be freed later
    // and mark the thread as dead/finished
    sthread_enqueue(dead_thread_queue, active_thread);
    active_thread->is_actually_dead = 1;
    
    // put thread's waiting for this thread on ready queue
    while(!sthread_queue_is_empty(active_thread->join_queue)){
      sthread_t temp = sthread_dequeue(active_thread->join_queue);
      //    temp->return_value = ret;
      sthread_enqueue(thread_queue, temp);
    }
    
    // switch to another thread to run
    /*  sthread_t dead_thread = active_thread;
	active_thread = sthread_dequeue(thread_queue);
	sthread_switch(dead_thread->saved_ctx,active_thread->saved_ctx); */
    context_switch(active_thread);
  }
  splx(oldvalue);
}


void* sthread_user_join(sthread_t t) {
  //wait for the specified thread to exit
  if (!initted) {
    printf("[sthread_user_yield] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();
  
  int oldvalue = splx(HIGH);
  if(t->joinable) {
    //sanity check
    if(t->join_queue == NULL){
      t->join_queue = sthread_new_queue();
    }
    // trying to join on main thread (not ok)
    if (t->is_actually_dead == -1){
      printf("ERROR: joining on main thread\n");
      exit(-1);
    }
    // waiting on a thread that is still running
    if (t->is_actually_dead == 0){ 
      sthread_enqueue(t->join_queue, active_thread);
      context_switch(active_thread);
    }
    splx(oldvalue);
    return t->return_value;
  } else {
    printf("not joinable\n");
    splx(oldvalue);
    return NULL;
  }
}


void sthread_user_yield(void) {
  //give up the cpu
  if (!initted) {
    printf("[sthread_user_yield] Library is not initialized.\n");
    exit(-1);
  }

  free_dead_threads();
  if (!sthread_queue_is_empty(thread_queue)){
    int oldvalue = splx(HIGH);
    
    sthread_enqueue(thread_queue, active_thread);
    context_switch(active_thread);
    
    splx(oldvalue);
  }
}

/* Add any new part 1 functions here */
void context_switch(sthread_t old){
  if (!sthread_queue_is_empty(thread_queue)){
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(old->saved_ctx, active_thread->saved_ctx);
  }
}

void runner(void) {
  splx(LOW);
  active_thread->return_value = active_thread->start_routine_ptr(active_thread->args);
  sthread_user_exit(NULL);
  // exit(0);  
}


// Free's all of the threads on the dead_thread_queue
void free_dead_threads(void){
  int oldvalue = splx(HIGH);
  while (!sthread_queue_is_empty(dead_thread_queue)){
    sthread_t temp = sthread_dequeue(dead_thread_queue);
    if (!sthread_queue_is_empty(temp->join_queue)){
      printf("ERROR freeing thread with threads waiting for it\n");
      exit(-1);
    }
    sthread_free_queue(temp->join_queue);
    sthread_free_ctx(temp->saved_ctx);
    //    free(temp);
    sthread_enqueue(return_value_queue, temp); // keep the return value of the dead thread
  }
  splx(oldvalue);
}




void sthread_user_dispatcher(void)
{
  sthread_user_yield();
}


/*********************************************************************/
/* Part 2: Synchronization Primitives                                */
/*********************************************************************/

struct _sthread_mutex {
  /* Fill in mutex data structure */
  lock_t lock;
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

  /*
  while(atomic_test_and_set(&(lock->lock))) {}
  
  if (lock->lock == 0){
    lock->lock = 1;

    atomic_clear(&(lock->lock));
  }
  else {
    sthread_enqueue(lock->waiting_threads, active_thread);

    atomic_clear(&(lock->lock));
  */

  // fails to acquire the lock
  if (atomic_test_and_set(&(lock->lock))){

    int oldvalue = splx(HIGH);
    sthread_enqueue(lock->waiting_threads, active_thread);
    /*
    sthread_t temp = active_thread;
    active_thread = sthread_dequeue(thread_queue);
    sthread_switch(temp->saved_ctx, active_thread->saved_ctx);
    */
    context_switch(active_thread);
    splx(oldvalue);
  }    
}

void sthread_user_mutex_unlock(sthread_mutex_t lock) {
  // no threads waiting for the lock, sets lock to unlocked

  //  while(atomic_test_and_set(&(lock->lock))) {}

  if (sthread_queue_is_empty(lock->waiting_threads))
    atomic_clear(&(lock->lock));
    //lock->lock = 0;
  // thread(s) waiting for the lock, pulls one thread from
  // waiting queue to ready queue implicitly passing the lock
  else{
    int oldvalue = splx(HIGH);
    sthread_t temp = sthread_dequeue(lock->waiting_threads);
    sthread_enqueue(thread_queue, temp);
    splx(oldvalue);

    sthread_user_yield();
  }

  //  atomic_clear(&(lock->lock));
}

struct _sthread_cond {
  /* Fill in condition variable structure */
  sthread_queue_t waiting_threads;
};

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
  int oldvalue = splx(HIGH);
  if (!sthread_queue_is_empty(cond->waiting_threads)){
    sthread_t temp = sthread_dequeue(cond->waiting_threads);
    sthread_enqueue(thread_queue, temp);
  }
  splx(oldvalue);
}

void sthread_user_cond_broadcast(sthread_cond_t cond) {
  int oldvalue = splx(HIGH);
  while (!sthread_queue_is_empty(cond->waiting_threads)){
    sthread_t temp = sthread_dequeue(cond->waiting_threads);
    sthread_enqueue(thread_queue, temp);
  }
  splx(oldvalue);
}



void sthread_user_cond_wait(sthread_cond_t cond,
                            sthread_mutex_t lock) {
  int oldvalue = splx(HIGH);
  sthread_user_mutex_unlock(lock);
  /*
  sthread_t temp = active_thread;
  active_thread = sthread_dequeue(thread_queue);
  sthread_enqueue(cond->waiting_threads, temp);
  sthread_switch(temp->saved_ctx, active_thread->saved_ctx);
  */
  sthread_enqueue(cond->waiting_threads, active_thread);
  context_switch(active_thread);
  sthread_user_mutex_lock(lock);
  splx(oldvalue);
}
