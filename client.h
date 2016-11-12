#ifndef CLIENT
#define CLIENT



struct thread_arg_c
{
  WINDOW * chat, *input; // The windows.
  int socket; // The socket to the server.
  int * line; // The line to print the text in the chat.
  char * pseudo; // The client pseudo.
  int * stop_flag; // Should the input thread stop listening.
};
typedef struct thread_arg_c * arg_c;

void * handle_input(void * arg);
void * handle_listen(void *arg);
int send_pseudo(int socket_client, char * pseudo);
void draw_borders(WINDOW *screen);
void add_to_chat(arg_c thread_arg, char * text);
int get_input(WINDOW *input, WINDOW *chat, char **buffer, int *stop_flag);
#endif
