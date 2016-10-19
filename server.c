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

typedef struct thread_arg thread_arg;
struct thread_arg
{
  int idx; // Client ID
  int * counter_client; // Clients count
  int * socket_clients; // Other clients ID
  char* pseudo; // CLient's nickname
};


void * handle_client(void * arg)
{
  thread_arg * thread_arg = (struct thread_arg*) arg;
  int i, read_size;
  char client_msg[BUF_SIZ];
  
  //Receive a message from client
  while((read_size = recv(thread_arg->socket_clients[thread_arg->idx] , client_msg , BUF_SIZ , 0)) > 0 ){
    //printf("Server read: %s\n", client_msg);
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
  *thread_arg->counter_client = *thread_arg->counter_client - 1;

  // Free the allocated space
  free(thread_arg);
  pthread_exit(NULL);
}

int get_client_idx(int * tab)
{
  int i = -1;
  for(i = 0; i < CLIENT_MAX; i++){
    // if it is a free space.
    if(tab[i] == 0){ 
      return i;
    }
  }
  return i;
}

// Should add a thread or a fork ? to join the ending thread.
int main(int argc, char * argv[])
{

  int socket_server, port, socket_acc, counter_client, idx;
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
	printf("Error: client ID < 0\n");
	return -1;
      }
    
      counter_client++;
    
      // Set the data for the client.
      pthread_t thread_client;
      thread_arg * thread_arg =  (struct thread_arg *) malloc(sizeof(thread_arg));
      thread_arg->socket_clients = socket_clients;
      thread_arg->idx = idx;
      thread_arg->counter_client = &counter_client;
      // add pseudo here.

      // Set the client data into the server.
      socket_clients[idx] = socket_acc;

      // Start a thread that will handle the client.
      if(pthread_create(&thread_client, NULL, handle_client, (void*) thread_arg)){
	perror("pthread_create()");
	exit(1);
      }
      if(pthread_detach(thread_client) != 0){
	perror("pthread_detach");
	return -1;
      }
      printf("Client #%d connected on slot %d\n", counter_client, idx);
    }     
  }  
  return 0;
}
