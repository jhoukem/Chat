#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <ncurses.h>
#include <pthread.h>
#include "socket.h"
#include "client.h"
#include "util.h"
#include "sig.h"

#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 10

void add_to_chat(arg_c thread_arg, char * text)
{

  mvwprintw(thread_arg->chat, *thread_arg->line, 1, text);

  // Set the line position.
  if((*thread_arg->line) >= thread_arg->chat->_maxy){
    wscrl(thread_arg->chat, 1);
  } else {
    // will stop increment at the window maxy
    (*thread_arg->line)++; 
  }
  // Display the updated screen.
  wrefresh(thread_arg->chat);
}

void draw_line(WINDOW *screen)
{
  int i;
  for (i = 0; i < screen->_maxx; i++) {
    mvwprintw(screen, 0, i, "_");
  }
}

void draw_borders(WINDOW *screen)
{
  int x, y, i;
  getmaxyx(screen, y, x);
   // 4 corners
  mvwprintw(screen, 0, 0, "+");
  mvwprintw(screen, y - 1, 0, "+");
  mvwprintw(screen, 0, x - 1, "+");
  mvwprintw(screen, y - 1, x - 1, "+");
  // sides 
  for (i = 1; i < (y - 1); i++) {
    mvwprintw(screen, i, 0, "|");
    mvwprintw(screen, i, x - 1, "|");
  }
  // top and bottom
  for (i = 1; i < (x - 1); i++) {
    mvwprintw(screen, 0, i, "_");
    mvwprintw(screen, y - 1, i, "-");
  }
}


void * handle_input(void * arg)
{
  arg_c input_arg = (arg_c) arg; 
  char msg[BUF_SIZ];
  char input_title[] = "Enter a message";
  
  while(!(*input_arg->stop)){
    // Clear screen.
    wclear(input_arg->input);
    draw_line(input_arg->input);
       
    // Center the window title.  
    mvwprintw(input_arg->input, 0, (input_arg->input->_maxx/2) - (strlen(input_title)/2), input_title);
    // Display the updated screen.
    wrefresh(input_arg->input);

    // Get the user input.
    mvwgetnstr(input_arg->input, 1, 0, msg, BUF_SIZ);
    if(!is_empty(msg)){
      //Send the user message.
      if(write(input_arg->socket, msg , strlen(msg) + 1) < 0){
	// Failed to send.
	pthread_exit(0);
      }
    }
  }
  pthread_exit(0);
}


void * handle_listen(void * arg)
{
  char buffer[BUF_SIZ + PSEUDO_MAX_SIZE];
  char chat_title[] = "Chat";
  arg_c listen_arg = (arg_c) arg;

  // Enable the scrolling.
  idlok(listen_arg->chat, TRUE);
  scrollok(listen_arg->chat, TRUE);
 
  draw_line(listen_arg->chat);
  // Center the window title.
  mvwprintw(listen_arg->chat, 0, (listen_arg->chat->_maxx/2) - (strlen(chat_title)/2), chat_title);
  wrefresh(listen_arg->chat);
  while(1){    
    // Receive a reply from the server.
    if(read(listen_arg->socket, buffer, BUF_SIZ) <= 0){
      // Connection closed.
      *listen_arg->stop = 1;
      pthread_exit(0);
    }
    // Print the text received to the chat windows
    add_to_chat(listen_arg, buffer);
  }
}


int send_pseudo(int socket_client, char * pseudo)
{
  if(send(socket_client, pseudo, PSEUDO_MAX_SIZE, 0) < 0){
    return -1;
  }
  return 0;
}


int main(int argc, char * argv[])
{

  int socket_client, port, input_size, parent_x, parent_y, stop, line;
  char *hostname, *pseudo;
  pthread_t thread_input, thread_listen;
  arg_c thread_arg;
  struct sockaddr_in server;
  
  if(argc < 4){
    printf("Bad usage: %s [host_address] [port_number] [pseudo]\n", argv[0]);
    return -1;
  }

  // Set the server to connect parameters.
  hostname = argv[1];
  port = atoi(argv[2]);
  pseudo = argv[3];
  server.sin_addr.s_addr = inet_addr(hostname);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  
  // Create client's socket.
  socket_client = create_socket_client();

  // Connect to the remote server.
  if(connect(socket_client , (struct sockaddr *)&server , sizeof(server)) < 0){
    perror("Connection failed");
    return 1;
  }

  // Send the pseudo to the server.
  if(send_pseudo(socket_client, pseudo) < 0){
    printf("Can't send the pseudo to the server\n");
    return -1;
  }

  // Set the ncurses parameters.
  initscr();
  start_color();
  
  // Get the maximum window dimensions.
  getmaxyx(stdscr, parent_y, parent_x);
  line = 1;
  stop = 0;
  input_size = 4;
  
  // Set up the windows.
  WINDOW *chat = newwin(parent_y - input_size, parent_x, 0, 0);
  WINDOW *input = newwin(input_size, parent_x, parent_y - input_size, 0);
  
  // Set the thread struct parameters.
  thread_arg = malloc(sizeof(struct thread_arg_c));
  thread_arg->input = input;
  thread_arg->chat = chat;
  thread_arg->socket = socket_client;
  thread_arg->line = &line;
  thread_arg->pseudo = pseudo;
  thread_arg->stop = &stop;
  init_sig();
  
  // Start a thread that will handle the client input.
  if(pthread_create(&thread_input, NULL, handle_input, (void*) thread_arg)){
    perror("pthread_create()");
    return -1;
  }
  
  // Start a thread that will listen to the server .
  if(pthread_create(&thread_listen, NULL, handle_listen, (void*) thread_arg)){
    perror("pthread_create()");
    return -1;
  }
  
  pthread_join(thread_listen, NULL);
  pthread_join(thread_input, NULL);
  
  delwin(chat);
  delwin(input);
  free(thread_arg);
  endwin();
  printf("The connection was closed by the server\n");
  
  return 0;
}
