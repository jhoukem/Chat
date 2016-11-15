#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

struct termios saved_attributes;

char *strstrip(char *s)
{
    size_t size;
    char *end;

    size = strlen(s);

    if (!size)
        return s;

    end = s + size - 1;
    while (end >= s && isspace(*end))
        end--;
    *(end + 1) = '\0';

    while (*s && isspace(*s))
        s++;

    return s;
}

void echo_on()
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void echo_off()
{
  struct termios tattr;

  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (EXIT_FAILURE);
    }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (echo_on);

  /* Set the funny terminal modes. */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON | ECHO);	/* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

int is_empty(char * buffer)
{
  int i;
  for(i = 0; buffer[i] != 0; i++){
    if(buffer[i] != '\n' && buffer[i] != ' '){
      return 0;
    }
  }
  return 1;
}

void clear_stdin(){
  int c = 0;
  while ((c = getchar()) != '\n' && c != EOF);
}

int get_line(char *buffer, int buffer_size){
  
  char *new_line = NULL;
  
  if(fgets(buffer, buffer_size, stdin) != NULL){    
    if((new_line = strchr(buffer, '\n')) != NULL){
      *new_line = '\0';    
    } else {
      clear_stdin();
    }
    return 0;

  } else {
    clear_stdin();
  }
  return 1;
}
