#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>   
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server.h"
#include "socket.h"
#include "util.h"

#define CLIENT_MAX 10
#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 10

void send_to_clients_except(arg_s thread_arg, char * msg)
{
  int i;
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message back to the sender or to a null socket.
    if(thread_arg->socket_clients[i] != 0 && i != thread_arg->idx){ 
      write(thread_arg->socket_clients[i] , msg , strlen(msg) + 1);
    }
  }
}

void send_to_clients(arg_s thread_arg, char * msg)
{
  int i;
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message back to a null socket.
    if(thread_arg->socket_clients[i] != 0){ 
      write(thread_arg->socket_clients[i] , msg , strlen(msg) + 1);
    }
  }
}

void free_thread_rsc(arg_s thread_arg)
{
  // Close the old socket and set it to 0 in the socket_clients tab.
  close(thread_arg->socket_clients[thread_arg->idx]);
  thread_arg->socket_clients[thread_arg->idx] = 0;
  // Update the number of client.
  pthread_mutex_lock(thread_arg->counter_lock);
  (*thread_arg->counter_client)--;
  pthread_mutex_unlock(thread_arg->counter_lock);
  // Free the allocated space.
  free(thread_arg->pseudo);
  free(thread_arg);
}

void * handle_client(void * arg)
{
  arg_s thread_arg = (arg_s) arg;
  int read_size;
  char buffer[BUF_SIZ];
  char client_msg[BUF_SIZ+PSEUDO_MAX_SIZE];

  if(get_client_pseudo(thread_arg->socket_clients[thread_arg->idx], thread_arg->pseudo) != 0){
    printf("Error no pseudo received from the client %d\n", thread_arg->idx);
    pthread_exit(NULL);
  }  

  snprintf(client_msg, BUF_SIZ, "%s has joined the chat.", thread_arg->pseudo);
  send_to_clients_except(thread_arg, client_msg);

  // Receive a message from the client.
  while((read_size = read(thread_arg->socket_clients[thread_arg->idx] , buffer , BUF_SIZ)) > 0 ){
    // printf("Server received: %s\n", buffer);
    // Don't send empty message.
    // This was also checked in the client but the server perform a second check.
    if(!is_empty(buffer)){
      // Add the client pseudo to the message.
      snprintf(client_msg, PSEUDO_MAX_SIZE, "%s: ", thread_arg->pseudo);
      strncat(client_msg, buffer, BUF_SIZ);  
      send_to_clients(thread_arg, client_msg);
    }
  }
  
  if(read_size == 0) {
    printf("Client number %d has disconnected\n", thread_arg->idx + 1);
    snprintf(client_msg, BUF_SIZ, "%s has left the chat.", thread_arg->pseudo);
    send_to_clients_except(thread_arg, client_msg);
  }
  else if(read_size == -1){
    perror("read failed");
  }
  
  free_thread_rsc(thread_arg);
  pthread_exit(NULL);
}

int get_client_idx(int * tab)
{
  int i;
  for(i = 0; i < CLIENT_MAX; i++){
    // if it is a free space.
    if(tab[i] == 0){ 
      return i;
    }
  }
  return -1;
}

int get_client_pseudo(int socket_client, char * buffer)
{

  if(recv(socket_client , buffer , PSEUDO_MAX_SIZE, 0) <= 0){
    return -1;
  }
  return 0;
}

int accept_client(int socket_server, int * socket_clients, int * counter_client, pthread_mutex_t * counter_lock)
{
  int idx;
  int socket_acc;
  pthread_t thread_client;
  arg_s thread_arg = NULL;
  
  printf("Server waiting for incoming connection...\n");
  
  // Accept the connection.
  socket_acc = accept(socket_server, NULL , NULL);
  if(socket_acc == -1){
    perror ("accept()");
    return -1;
  }
  
  // Get an ID for the client.
  idx = get_client_idx(socket_clients);
  if(idx < 0){
    printf("accept_client() error: client ID < 0\n");
    return -1;
  }

  // Update the client counter.
  pthread_mutex_lock(counter_lock);
  (*counter_client)++;
  pthread_mutex_unlock(counter_lock);
  
  // Set the data for the client.  
  thread_arg =  (arg_s) malloc(sizeof(struct thread_arg_s));
  thread_arg->socket_clients = socket_clients;
  thread_arg->idx = idx;
  thread_arg->counter_client = counter_client;
  thread_arg->counter_lock = counter_lock;
  thread_arg->pseudo = malloc(PSEUDO_MAX_SIZE * sizeof(char));
  
  // Save the client fd into the server fd tab.
  socket_clients[idx] = socket_acc;
 
  
  // Start a thread that will handle the client.
  if(pthread_create(&thread_client, NULL, handle_client, (void*) thread_arg)){
    perror("pthread_create()");
    return -1;
  }

  if(pthread_detach(thread_client) != 0){
    perror("pthread_detach");
    return -1;
  }
  printf("Client #%d is connected on slot %d\n", *counter_client, idx);
  return 0;
}      


int main(int argc, char * argv[])
{

  int socket_server, port, counter_client;
  int * socket_clients = NULL;
  pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

  if(argc < 2){
    printf("Bad usage: %s [port_number]\n", argv[0]);
    return -1;
  }
  
  counter_client = 0;
  port = atoi(argv[1]);
  socket_clients = calloc(CLIENT_MAX, sizeof(int *));
  socket_server = create_socket_server(port);
  
  
  if(socket_server == -1){
    perror("create_socket_server()");
    return -1;
  }
  
  while(1){
    if(counter_client < CLIENT_MAX){
      if(accept_client(socket_server, socket_clients, &counter_client, &counter_lock) < 0){
	return -1;
      }
    }  
  }
  return 0;
}

