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
void send_notify();
int is_absent();
int identify_to_server(int socket_client);
void draw_borders(WINDOW *screen);
void display_header(WINDOW *win, char *label);
void add_to_chat(arg_c thread_arg, char * text);
void handle_resize(WINDOW *input, WINDOW *chat);
int get_input(WINDOW *input, WINDOW *chat, char **buffer, int *stop_flag);
#endif
