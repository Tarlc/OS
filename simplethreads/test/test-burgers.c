#include "stdio.h"
#include "stdlib.h"
#include "sthread.h"

typedef struct stack {
  int id;
  struct stack *next;
}stack;

static sthread_mutex_t stackLock;
static sthread_cond_t burgerExists;
static int curBurgerNum;
static int maxBurgerNum;
static int burgersEaten;
static stack *top = NULL;


void *cook(void *arg) {
  sthread_mutex_lock(stackLock);
  while(curBurgerNum < maxBurgerNum){
    stack *newBurger = (stack *) malloc(sizeof(stack));
    newBurger->id = curBurgerNum;
    printf("Cooked %d\n", curBurgerNum);
    curBurgerNum++;
    newBurger->next = top;
    top = newBurger;
    sthread_mutex_unlock(stackLock);
    sthread_cond_signal(burgerExists);
    sthread_yield();
    sthread_mutex_lock(stackLock);
  }
  sthread_mutex_unlock(stackLock);
  sthread_cond_broadcast(burgerExists);
  sthread_exit(NULL);
  return (void *) NULL;
}

void *student(void *arg) {
  sthread_mutex_lock(stackLock);
  while(1){
    while(top == NULL && curBurgerNum < maxBurgerNum) sthread_cond_wait(burgerExists, stackLock);
    stack *cur = top;
    if(cur == NULL){
      sthread_mutex_unlock(stackLock);
      sthread_exit(NULL);
    }
    printf("Ate %d\n", cur->id);
    burgersEaten++;
    top = top->next;
    free(cur);
    sthread_mutex_unlock(stackLock);
    sthread_yield();
    sthread_mutex_lock(stackLock);
  }
  sthread_mutex_unlock(stackLock);
  sthread_exit(NULL);
}

int main(int argc, char **argv) {
  if (argc != 4){
    printf("Error: invalid input\n");
    return -1;
  }
  sthread_init();
  stackLock = sthread_mutex_init();
  burgerExists = sthread_cond_init();
  curBurgerNum = 0;
  burgersEaten = 0;
  
  int numCooks = atoi(argv[1]);
  if (numCooks <= 0){
    printf("Error: parameter one must be > 0\n");
    return -1;
  }
  int numStudents = atoi(argv[2]);
  if (numStudents < 0){
    printf("Error: parameter two must be >= 0\n");
    return -1;
  }
  maxBurgerNum = atoi(argv[3]);
  if (maxBurgerNum < 0){
    printf("Error: parameter three must be >= 0\n");
    return -1;
  }
  sthread_t cooks[numCooks];
  sthread_t students[numStudents];
  int i;
  for(i = 0; i < numCooks; i++){
    cooks[i] = sthread_create(cook, NULL, 1);
  }
  for(i = 0; i < numStudents; i++){
    students[i] = sthread_create(student, NULL, 1);
  }
  for(i = 0; i < numCooks; i++){
    sthread_join(cooks[i]);
    free(cooks[i]);
  }
  for(i = 0; i < numStudents; i++){
    sthread_join(students[i]);
    free(students[i]);
  }
  sthread_cond_free(burgerExists);
  sthread_mutex_free(stackLock);
  sthread_exit(0);
  return 0;
}
