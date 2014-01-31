#include <linux/syscalls.h>
#include <linux/kernel.h>

asmlinkage long sys_basic_syscall(int pid, int* pArray){
  printk("TESTING BASIC SYSCALL");
  return 0;
}
