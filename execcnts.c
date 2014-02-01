#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "getexeccounts.h"

int main (int argc, char **argv){

  if (argc == 1){
    printf("USAGE: No arguments\n");
    exit(1);
  }

  int parent_id = getpid();
  printf("My process ID: %d\n", parent_id);
  int before[4], after[4];
  
  // get the parents initial info before making a child
  int results = getExecCounts(parent_id, before);
  if (results != 0){
      printf("Error: failed to get process information\n");
      exit(-1);
  }

  int pid = fork();
  if (pid == 0){
    //child
    results = execvp(argv[1], argv+1); // skip the execcnts argument
    if (results == -1){
      printf("Error: %s not found\n", argv[1]);
      exit(-1);
    }
  }
  else{
    // parent
    wait(&results);
    int results = getExecCounts(parent_id, after);
    if (results != 0){
      printf("Error: failed to get process information2");
      exit(-1);
    }
    
    // print difference in before and after
    printf("fork\t%d\n", after[0] - before[0] - 1); // account for fork in parent to make child
    printf("vfork\t%d\n", after[1] - before[1]);
    printf("execve\t%d\n", after[2] - before[2] - 1); // account for execve in parent to make child
    printf("clone\t%d\n", after[3] - before[3]);
    
    return 0;
  }
}
