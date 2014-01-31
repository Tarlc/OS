#include <linux/syscalls.h>
#include <linux/kernel.h>

asmlinkage long sys_basic_syscall(int pid, int* pArray){

  for (int i = 0; i < 4; i++)
    pArray[i] = 0;
  if (!kill(pid)){
    task_struct *pid_info = find_task_by_vpid((pid_t) pid);
    
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
