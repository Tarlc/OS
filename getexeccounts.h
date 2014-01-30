#ifndef __GET_EXEC_COUNTS_H__
#define __GET_EXEC_COUNTS_H__

// takes a process id and a pointer to a 4 element int array and
// returns the number of times fork, vfork, execve, and clone
// were called for that process in the 4 element array (in that order).
// Returns 0 for success and non-zero for failure
int getExecCounts(int pid, int* pArray);

#endif
