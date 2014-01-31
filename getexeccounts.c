#include <unistd.h>
#include <linux/kernel.h>
#include <stdio.h>
#include <linux/sched.h>

int getExecCounts(int pid, int* pArray){

  if(syscall(314, pid, pArray))
    return 0;
  else{
    printf("LIBRARY FUNCTION FAILED\n");
    return -1;
  }
}

