#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/types.h>
#include <unistd.h>


void handle_sig(int sig)
{
  if (sig == SIGINT){
    endwin();
    exit(0);
  }
}

void init_sig()
{
  
  struct sigaction sa;
  sa.sa_handler = handle_sig;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa ,NULL) == -1){
    perror("sigaction(SIGINT)");
  }
}
