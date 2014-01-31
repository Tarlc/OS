#define __SYS_NUM_EXEC_COUNTS__ 314
#include <unistd.h>
#include <sys/syscall.h>

int getExecCounts(int pid, int* pArray){
  return syscall(__SYS_NUM_EXEC_COUNTS__, pid, pArray);
}

