#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/sched.h>

asmlinkage long sys_basic_syscall(int pid, int* pArray){

  pArray[0] = 0;
  pArray[1] = 0;
  pArray[2] = 0;
  pArray[3] = 0;

  if (sys_kill(pid, 0)){
    struct task_struct *pid_info = find_task_by_vpid((pid_t) pid);
    
    pArray[0] = pid_info->numFork;
    pArray[1] = pid_info->numVfork;
    pArray[2] = pid_info->numExecve;
    pArray[3] = pid_info->numClone;
    printk("Basic Syscall was successful\n");
    return 0;
  }

  else{
    printk("Basic Syscall was successful\n");    
    return -1;
  }
}
