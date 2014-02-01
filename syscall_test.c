#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv){

  char **temp = (char**)malloc(sizeof(char*)*2);
  temp[0] = "date";
  temp[1] = NULL;
  int res;
  for (int i = 0; i < 4; i++){
    pid_t pid = fork();
    if (pid == 0){
      printf("hello\n");
      //child
      res = execvp("date", temp);
      if (res == -1){
	printf("Error: %s not found\n", "date");
	exit(-1);
      }
    } else {
      //parent                                                                        
      wait(&res);
    }
  }


  for (int i = 0; i < 5; i++){
    pid_t pid = vfork();
    if (pid == 0){
      //child
      printf("hello2\n");
      // only run execvp once for the 4 forks
      res = execvp("date", temp);
      if (res == -1){
	printf("Error: %s not found\n", "date");
	exit(-1);
      }
    } else {
      //parent                                                                        
      wait(&res);
    }
  }

  // infinite loop to keep process running
  while (1) {}

  return 0;
}
