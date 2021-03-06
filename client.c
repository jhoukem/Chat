#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <ncurses.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "socket.h"
#include "client.h"
#include "util.h"
#include "sig.h"


#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 20
#define MAX_KEY_SIZE 20
#define DEBUG 0
#define INPUT_HEIGHT 4
#define INPUT_HEADER "Enter a message"
#define CHAT_HEADER "Chat"
#define CONNECTION_SUCCESS "Connection successful"

pthread_mutex_t ncurse_lock = PTHREAD_MUTEX_INITIALIZER;
time_t lastInput;
int delay_for_afk;
int notify_enabled;

void handle_resize(WINDOW *input, WINDOW *chat)
{
  int new_y, new_x; 
  getmaxyx(stdscr, new_y, new_x);
  wresize(chat, new_y - INPUT_HEIGHT, new_x);
  wresize(input, INPUT_HEIGHT, new_x);
  mvwin(input, new_y - INPUT_HEIGHT, 0);
  mvwin(chat, 0, 0);
  display_header(chat, CHAT_HEADER);
  display_header(input, INPUT_HEADER);
  wrefresh(chat);
  wrefresh(input);
}

int min(int x, int y)
{
  return x < y ? x : y;
}
void rm_ch_from_buffer(WINDOW *input, int x, int y, int width, int nb_ch, char * buffer)
{
  int i, pos;
  pos = x + (width - 1) * (y - 1);
  // We shift to the left all the end of the buffer
  for(i = pos; i < nb_ch - 1; i++){
    buffer[i] = buffer[i + 1];
    mvwprintw(input, min(y, INPUT_HEIGHT - 1), x + (i-pos), (char*)&buffer[i]);
  }
  buffer[i] = ' ';
  mvwprintw(input, min(y, INPUT_HEIGHT - 1), x + (i-pos), (char*)&buffer[i]);
}

void add_ch_to_buffer(WINDOW *input, int ch, int x, int y, int width, int nb_ch, char * buffer)
{
  int i, pos;
  pos = x + (width - 1) * (y - 1);
  
  // Add a char at the end of the buffer.
  if(pos == nb_ch){
    buffer[pos] = ch;
    mvwprintw(input, min(y, INPUT_HEIGHT - 1), x, (char *)&ch);
  }  
  // Shift all the buffer to the right and
  // insert a char at the current position.  
  else {

    for(i = nb_ch; i > pos; i--){
      buffer[i] = buffer[i - 1];
      mvwprintw(input, min(y, INPUT_HEIGHT - 1), i%width , (char*)&buffer[i]);
    }
    buffer[i] = ch;
    mvwprintw(input, min(y, INPUT_HEIGHT - 1), x, (char*)&buffer[i]);
  }
}

int get_input(WINDOW *input, WINDOW *chat, char **buffer, int *stop_flag)
{
  int ch, x, y, nb_ch, width;
  ch = 0;
  x = 0;
  y = 1;
  nb_ch = 0;  
  width = input->_maxx + 1;

  // Clear the buffer.
  *buffer = memset(*buffer, 0, BUF_SIZ);

  pthread_mutex_lock(&ncurse_lock);
  wmove(input, y, x);
  wrefresh(input);
  pthread_mutex_unlock(&ncurse_lock);

  while((ch = wgetch(input)) != '\n'){
    // If the client lost the connexion to the server.
    if(*stop_flag){
      return -1;
    }
       
    if(ch != ERR){
      time(&lastInput);
      pthread_mutex_lock(&ncurse_lock);
      
      if(ch == KEY_LEFT){
	if(x - 1 < 0){
	  if(y > 1){
	    x = width - 1;
	    y--;
	  }
	} else {
	  x--;
	}
      }
      else if(ch == KEY_RIGHT){
	if((x + ((width - 1)*(y-1))) < nb_ch){
	  if((x+1) >= width){
	    y++;
	    x = 0;
	  } else {
	    x++;
	  }	
	}
      }
      else if(ch == KEY_UP){
     
	if(y > 1){
	  /*	if(y == 2){
		wscrl(input, -1);
		}*/
	  y--;
	}
      }
      else if(ch == KEY_DOWN){
	if((nb_ch / (width-1)) >= y ){
	  y++;
	}  
	if(x > nb_ch % (width)){
	  x = nb_ch % (width - 1);
	} 
      }
      else if(ch == KEY_BACKSPACE){
	if(x > 0 || y > 1){	
	  if(x == 0){
	    y--;
	    x = width - 1;
	  } else {
	    x--;
	  }
	  rm_ch_from_buffer(input, x, y, width, nb_ch, *buffer);
	  nb_ch--;
	}
      }
      // Only add the regulars char to the buffer.
      else if(ch > 31 && ch < 127){
	if(nb_ch < BUF_SIZ - 1){
	
	  add_ch_to_buffer(input, ch, x, y, width, nb_ch, *buffer);
	  nb_ch++;
	 
	  if(x + 1 >= width - 1){
	    x = 0;
	    y ++;
	    if(y >= INPUT_HEIGHT){
	      scroll(input);
	      display_header(input, INPUT_HEADER);
	    }
	  } else {
	    x++;
	  }
	}
      }
      else if(ch == KEY_RESIZE){
	handle_resize(input, chat);
	width = input->_maxx + 1;
      }
      if(DEBUG){
	mvwprintw(input, 0, 0, "______________");
	mvwprintw(input, 0, 0, "ch=%d, w=%d", nb_ch, width);
      }
      
      wmove(input, min(y, INPUT_HEIGHT -1), x);
      wrefresh(input);
      pthread_mutex_unlock(&ncurse_lock);
    }
  }
  return 0;
}

void add_to_chat(arg_c thread_arg, char * text)
{

  mvwprintw(thread_arg->chat, *thread_arg->line, 0, text);

  // Set the line position.
  if((*thread_arg->line) >= thread_arg->chat->_maxy){
    scroll(thread_arg->chat);
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

void * handle_input(void * arg)
{
  arg_c input_arg = (arg_c) arg; 
  char * msg = malloc(sizeof(char) * BUF_SIZ);
  
  cbreak();
  noecho();
  nodelay(input_arg->input, TRUE);
  keypad(input_arg->input, TRUE);
  time(&lastInput);

  // Enable the scrolling.
  scrollok(input_arg->input, TRUE);

  while(1){
    pthread_mutex_lock(&ncurse_lock);
    wclear(input_arg->input);
    display_header(input_arg->input, INPUT_HEADER);
    pthread_mutex_unlock(&ncurse_lock);
    
    // Get the user input.
    if(get_input(input_arg->input, input_arg->chat, &msg, input_arg->stop_flag) < 0){
      // The server has closed the socket.
      break;
    }
    if(!is_empty(msg)){
      //Send the user message.
      if(write(input_arg->socket, msg , strlen(msg) + 1) < 0){
	// Failed to send.
	free(msg);
	pthread_exit((void *)-1);
      }
    }
  }
  free(msg);
  pthread_exit((void *)-1);
}


void display_header(WINDOW *win, char *label)
{
  draw_line(win);
  // Center the window title.
  mvwprintw(win, 0, (win->_maxx/2) - (strlen(label)/2), label);
  wrefresh(win);
}

void * handle_listen(void * arg)
{
  char buffer[BUF_SIZ + PSEUDO_MAX_SIZE];
  arg_c listen_arg = (arg_c) arg;

  // Enable the scrolling.
  scrollok(listen_arg->chat, TRUE);
 
  while(1){
    pthread_mutex_lock(&ncurse_lock);
    display_header(listen_arg->chat, CHAT_HEADER);
    pthread_mutex_unlock(&ncurse_lock);
    
    // Receive a reply from the server.
    if(read(listen_arg->socket, buffer, BUF_SIZ) <= 0){
      // Connection closed.
      *listen_arg->stop_flag = 1;
      pthread_exit(0);
    }
    // Print the text received to the chat window
    pthread_mutex_lock(&ncurse_lock);   
    add_to_chat(listen_arg, buffer);
    
    if(notify_enabled && is_absent()){
      send_notify();
    }
    pthread_mutex_unlock(&ncurse_lock);
  }
}

int is_absent()
{
  return (time(NULL) - lastInput) > delay_for_afk; 
}

void send_notify()
{
  char icon_path[520];
  snprintf(icon_path, 520, "%s/msg.png", getcwd(NULL, 512));
  char *argument[] = {"notify-send", "-i", icon_path, "New message", NULL};
  switch(fork()){
  case 0 : execv("/usr/bin/notify-send", argument);
  default : wait(NULL); break;
  }
}

int identify_to_server(int socket_client)
{
  char pseudo[PSEUDO_MAX_SIZE] = {0};
  char key[MAX_KEY_SIZE] = {0};
  char buffer[PSEUDO_MAX_SIZE + MAX_KEY_SIZE + 1] = {0};
  char answer[BUF_SIZ] = {0};
  do{
    printf("Enter your username (%d char max): ", PSEUDO_MAX_SIZE);
    get_line(pseudo, PSEUDO_MAX_SIZE);
  } while(is_empty(pseudo));
  
  printf("Enter the server password:");
  echo_off();
  get_line(key, MAX_KEY_SIZE);
  echo_on();
  printf("\n");
  
  // Concatene the two string together.
  snprintf(buffer, PSEUDO_MAX_SIZE + MAX_KEY_SIZE + 1, "%s:%s", pseudo, key );
  if(write(socket_client, buffer, strlen(buffer) + 1) < 0){
    printf("Cannot write to the server.\n");
    return -1;
  }

  if(read(socket_client, answer, BUF_SIZ) <= 0){
    printf("Cannot read from the server.\n");
    return -1;
  }
  printf("%s\n", answer);

  if(strcmp(answer, CONNECTION_SUCCESS) == 0){
    return 0;
  }
  return -1;
}

int is_integer(char *string)
{
  char c;
  int i;

  for(i = 0; string[i] != 0; i++){
      c = string[i];
      // Allow negative integer.
      if((c < '0' || c > '9') && c != '-'){
          return 0;
      }
  }
  return 1;
}

void handle_arg(int argc, char **argv)
{
  char *options = "-t:h";
  int current_option;
  char help[512];

  snprintf(help, 512, "Usage:\n%s [host_address] [port_number] [options]\n\nThe following options are availables:\n"
  "-h,          Show this help menu\n"
  "-t,          Set the time in seconds before being considered afk (10 sec by default, setting it to -1 disable the notifications)\n", argv[0]);

  delay_for_afk = 15;
  

  if(argc < 3){
    fprintf(stderr, "%s", help);
    exit(-1);
  }
  
  while((current_option = getopt(argc, argv, options)) != -1){
    
    switch(current_option){    
    case 't':
      if(is_integer(optarg)){
	delay_for_afk = atoi(optarg);
      } else {
	fprintf(stderr, "Error: The value for -%c should be an integer.\n", current_option);
	exit(-1);
      }
      break;
    case 'h': printf("%s", help); exit(0);
    }
  }
  if(delay_for_afk > 0 && (access("/usr/bin/notify-send", X_OK) == 0)){
    notify_enabled = 1;
  } else {
    notify_enabled = 0;
  }
}

int main(int argc, char * argv[])
{

  int socket_client, port, parent_x, parent_y, stop, line;
  pthread_t thread_input, thread_listen;
  arg_c thread_arg;
  char ip[100];
  
  struct sockaddr_in server;

  handle_arg(argc, argv);

  // Set the server to connect parameters.
  hostname_to_ip(argv[1], ip);

  port = atoi(argv[2]);
  server.sin_addr.s_addr = inet_addr(ip);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  
  // Create client's socket.
  socket_client = create_socket_client();

  // Connect to the remote server.
  if(connect(socket_client , (struct sockaddr *)&server , sizeof(server)) < 0){
    perror("Connection failed.\n");
    return 1;
  }

  // Send the credential to the server.
  if(identify_to_server(socket_client) < 0){  
    printf("Identification to the server failed.\n");
    return -1;
  }
  
  init_sig();
  // Set the ncurses parameters.
  initscr();
  
  // Get the maximum window dimensions.
  getmaxyx(stdscr, parent_y, parent_x);
  line = 1;
  stop = 0;
  
  // Set up the windows.
  WINDOW *chat = newwin(parent_y - INPUT_HEIGHT, parent_x, 0, 0);
  WINDOW *input = newwin(INPUT_HEIGHT, parent_x, parent_y - INPUT_HEIGHT, 0);
  
  // Set the thread struct parameters.
  thread_arg = malloc(sizeof(struct thread_arg_c));
  thread_arg->input = input;
  thread_arg->chat = chat;
  thread_arg->socket = socket_client;
  thread_arg->line = &line;
  //thread_arg->pseudo = pseudo;
  thread_arg->stop_flag = &stop;
  
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
