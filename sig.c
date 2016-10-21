#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void handle_sig(int sig)
{ 
  if (sig == SIGCHLD){
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0){
      if (WIFSIGNALED(status)){
	printf("Child killed by signal: %d\n", WTERMSIG(status));
      }
    }
  }
  exit(0);
}

void init_sig()
{
  
  struct sigaction sa;
  sa.sa_handler = handle_sig;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa ,NULL) == -1){
    perror("sigaction(SIGCHLD)");
  }
}
