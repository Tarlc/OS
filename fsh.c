#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

const char* PROMPT = "CSE451Shell% ";

const int MAX_NUMBER_ARG = 20;

int doCommand(char *line, int exitVal);

int tokenize(char *line, char ***tokens){
  int tokenNum = 0;
  char *token;
  char **newtokens = malloc(MAX_NUMBER_ARG * sizeof(char*));
  if(newtokens == NULL) exit(1);
  token = strtok(line, " \t");
  while (token != NULL && tokenNum < MAX_NUMBER_ARG) {
    newtokens[tokenNum] = token;
    tokenNum++;
    token = strtok(NULL, " \t");
  }
  newtokens[tokenNum] = NULL;
  *tokens = newtokens;
  return tokenNum;
}

int readFromFile(char *filename, int exitVal){
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    printf("Error: file %s could not be opened\n", filename);
    return exitVal;
  }
  char *line;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, f)) != -1) {
    line[read-1] = '\0';
    exitVal = doCommand(line, exitVal);
  }
  free(line);
  fclose(f);
  return exitVal;
}

void cd(char **tokens, int homeDir){
  int res;
  if (homeDir){
    res = chdir(getenv("HOME"));
  } else if(*tokens[1]=='~'){
    res = chdir(getenv("HOME"));
    char *cur;
    cur = tokens[1];
    if (*(cur+1) != '/'){
      return;
    }
    
    while(*(cur+1) != '\0'){
      *cur = *(cur+2);
      cur++;
    }
    res = chdir(tokens[1]);
  } else{
    res = chdir(tokens[1]);
  }
  if (res != 0)
    printf("Error: invalid directory\n");
  return;
}

int execute(char **tokens){
  int res;
  pid_t pid = fork();
  if (pid == 0){
    //child
    res = execvp(tokens[0], tokens);
    if (res == -1){
      printf("Error: %s not found\n", tokens[0]);
      exit(-1);
    }
  } else {
    //parent
    wait(&res);
    return WEXITSTATUS(res);
  }
  return res;
}

int doCommand(char *line, int exitVal){
  char **tokens;
  int tokenNum = tokenize(line, &tokens);
  if (tokenNum > MAX_NUMBER_ARG) {
    printf("Error: too many arguments, must be < %d\n", MAX_NUMBER_ARG);
    free(tokens);
    return exitVal;
  }
  if (tokenNum == 0) {
    free(tokens);
    return exitVal;
  }
  if (strcmp(tokens[0], "exit") == 0) {
    if (tokenNum != 1) {
      int num = atoi(tokens[1]);
      if ((num == 0 && strcmp(tokens[1], "0") != 0) || tokenNum > 2) {
        printf("Error: invalid argument after exit, exiting with previous exit value\n");
      } else {
        exitVal = num;
      }
    }
    free(tokens);
    free(line);
    exit(exitVal);
  }
  if (strcmp(tokens[0], "cd") == 0) {
    cd(tokens, tokenNum==1);
  } else if (strcmp(tokens[0], ".") == 0) {
    if (tokenNum == 1) {
      printf("Error: no filename given\n");
    } else if (tokenNum > 2) {
      printf("Error: too many arguments given\n");
    } else {
      exitVal = readFromFile(tokens[1], exitVal);
    }
  } else {
    exitVal = execute(tokens);
  }
  free(tokens);
  return exitVal;
}

int main() {
  int exitVal = 0;
  while(1) {
    char *line = readline(PROMPT);
    exitVal = doCommand(line, exitVal);
    free(line);
  }
}
