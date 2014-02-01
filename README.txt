Tarl Coufal tarlc
Sean Flinn spf3d

The syscall is called basic_syscall.c. It takes an int pid and an int array pArray, which it assumes has at least length four. It sets the first four elements of pArray to zero, then it checks to see if pid is a valid process id for a running process. If not, the syscall returns -1. If it is, the task_struct for the process is found and the first four elements of the pArray are set to the numFork, numVfork, numExecve, and numClone members of the task_struct. The system call then returns 0.

A user program calls the getcounts library, which calls the system call with syscall using the system call number for our system call and the arguments through the file getexeccounts.c.

The internal commands 'cd' and 'exit' must be internal commands because if they were implemented like other commands, then there would be a forked process attempting to either change the parent process directory or exit the parent process. Also, if 'exit' and 'cd' were not internal, they could be modified so that the shell could not exit or change directories. For the '.' command, we chose to interpret the file as direct input. The commands are simply read from a file instead of from the user until the file reaches completion. This means that we did not have to worry about forking a process and the overhead associated with it.
