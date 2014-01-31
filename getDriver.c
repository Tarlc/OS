#ifndef __GET_DRIVER_C__
#define __GET_DRIVER_C__

#include <stdio.h>
#include <stdlib.h>
#include "getexeccounts.h"

int main(int argc, char** argv){
  // must have exactly 1 argument
  if (argc != 2){
    fprintf(stderr, "%s expects one PID as the argument\n", argv[0]);
    exit(0);
  }
  
  int temp[4];
  for (int i = 0; i < 4; i++)
    temp[i] = 0;
  int pid = atoi(argv[1]);
  int results = getExecCounts(pid, temp);

  // getExecCounts is successful
  if (results == 0){
    printf("pid %d:\n", pid);
    printf("\t%d\tfork\n", temp[0]);
    printf("\t%d\tvfork\n", temp[1]);
    printf("\t%d\texecve\n", temp[2]);
    printf("\t%d\tclone\n", temp[3]);
    return 0;
  }

  // getExecCounts failed
  else{
    fprintf(stderr, "%s failed to get the process information\n", argv[0]);
    return 1;
  }
}



#endif
