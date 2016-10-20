#include <stdio.h>
#include "socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>   
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLIENT_MAX 10
#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 15


pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct thread_arg thread_arg;
struct thread_arg
{
  char *pseudo;
  int idx; // Client ID
  int *counter_client; // Clients count
  int *socket_clients; // Other clients ID
   // Client's nickname
  int * osef;
};



void * handle_client(void * arg)
{
  printf("in\n");
  thread_arg * thread_arg = (struct thread_arg*) arg;
  int i, read_size;
  char buffer[BUF_SIZ];
  char client_msg[BUF_SIZ+PSEUDO_MAX_SIZE];

  printf("handle_client: thread_arg->pseudo = %p\n", thread_arg->pseudo);
  
  //Receive a message from client
  while((read_size = recv(thread_arg->socket_clients[thread_arg->idx] , client_msg , BUF_SIZ , 0)) > 0 ){
    //snprintf(client_msg, PSEUDO_MAX_SIZE, "%s: ", thread_arg->pseudo);
    // strncat(client_msg, buffer, BUF_SIZ);
    //Send the message back to all the client
    for(i = 0; i < CLIENT_MAX; i++){
      // Don't send the message back to the sender or to a null socket.
      if(thread_arg->socket_clients[i] != 0 && i != thread_arg->idx){ 
	write(thread_arg->socket_clients[i] , client_msg , strlen(client_msg) + 1);
      }
    }
  }
  
  if(read_size == 0) {
    printf("Client number %d has disconnected\n", thread_arg->idx + 1);
  }
  else if(read_size == -1){
    perror("recv failed");
  }
    
  // Free the old socket in the socket_clients tab.
  close(thread_arg->socket_clients[thread_arg->idx]);
  thread_arg->socket_clients[thread_arg->idx] = 0;
  pthread_mutex_lock(&counter_lock);
  (*thread_arg->counter_client)--;
  pthread_mutex_unlock(&counter_lock);
  // Free the allocated space
  free(thread_arg);
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

  if(recv(socket_client , buffer , PSEUDO_MAX_SIZE , 0) <= 0){
    return -1;
  }
  buffer[PSEUDO_MAX_SIZE + 1] = 0;
  return 0;
}

int accept_client(int socket_server, int * socket_clients, int * counter_client)
{
  int idx;
  int socket_acc;
  pthread_t thread_client;
  thread_arg * thread_arg = NULL;
  char pseudo[PSEUDO_MAX_SIZE];
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

  if(get_client_pseudo(socket_acc, pseudo) != 0){
    printf("Error no pseudo received from the client\n");
    return 1;
  }
  
  // Update the client counter.
  pthread_mutex_lock(&counter_lock);
  (*counter_client)++;
  pthread_mutex_unlock(&counter_lock);
  
  // Set the data for the client.  
  thread_arg =  (struct thread_arg *) malloc(sizeof(thread_arg));
  thread_arg->socket_clients = socket_clients;
  thread_arg->idx = idx;
  thread_arg->counter_client = counter_client;
  thread_arg->pseudo = pseudo;

  printf("accept_client: &pseudo = %p || thread_arg->pseudo = %p\n", &pseudo, thread_arg->pseudo);

  
  // Save the client fd into the server fd tab.
  socket_clients[idx] = socket_acc;
  
  // Start a thread that will handle the client.
  if(pthread_create(&thread_client, NULL, handle_client, (void*) thread_arg)){
    perror("pthread_create()");
    return -1;
  }
  sleep(2);
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
      if(accept_client(socket_server, socket_clients, &counter_client) < 0){
	return -1;
      }
    }  
  }
  return 0;
}

