#ifndef CLIENT
#define CLIENT



struct thread_arg_c
{
  WINDOW * chat, *input; // The windows.
  int socket; // The socket to the server.
  int * line; // The line to print the text in the chat.
  char * pseudo; // The client pseudo.
  int * stop; // SHould the input continue listening
};
typedef struct thread_arg_c * arg_c;

void * handle_input(void * arg);
void * handle_listen(void *arg);
int send_pseudo(int socket_client, char * pseudo);
void draw_borders(WINDOW *screen);

#endif
