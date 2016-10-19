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

typedef struct serv_arg serv_arg;
struct serv_arg
{
  int idx;
  int * socket_clients;
};



void * handle_client(void * arg)
{
  serv_arg * thread_arg = (serv_arg *) arg;
  int i, read_size;
  char client_msg[BUF_SIZ];
  
  //Receive a message from client
  while((read_size = recv(thread_arg->socket_clients[thread_arg->idx] , client_msg , BUF_SIZ , 0)) > 0 ){
    printf("I read: %s\n", client_msg);
    /* //Send the message back to all the client
    for(i = 0; i < CLIENT_MAX; i++){
      // Don't send to the sender or to a null fd.
      if(i != thread_arg->idx && i != 0){ 
	write(thread_arg->socket_clients[i] , client_msg , strlen(client_msg));
	}
      }*/
  }
  
  if(read_size == 0) {
    printf("Client number %d has disconnected\n", thread_arg->idx);
  }
  else if(read_size == -1){
    perror("recv failed");
  }

  // Free the allocated space

  // Remove the old socket.
  //thread_arg->socket_clients[thread_arg->idx] = 0;*/
}

int get_client_idx(int * tab)
{
  int i = -1;
  for(i = 0; i < CLIENT_MAX; i++){
    if(tab[i] == 0){ // if it is a free space.
      return i;
    }
  }
  return i;
}

void init_thread_arg(serv_arg * arg, int ** socket_clients)
{
  int i;
  for(i = 0; i < CLIENT_MAX; i++){
    arg[i].socket_clients = *socket_clients;
  }  
}

int main(int argc, char * argv[])
{

  int socket_server, port, socket_acc, counter_client, idx;
  int * socket_clients = NULL;
  pthread_t * thread_clients = NULL;
  serv_arg * thread_arg = NULL;

  if(argc < 2){
    printf("Bad usage: %s [port_number]\n", argv[0]);
    return -1;
  }

  counter_client = 1;
  port = atoi(argv[1]);
  socket_clients = calloc(CLIENT_MAX, sizeof(int *));
  thread_clients = malloc(sizeof(pthread_t *) * CLIENT_MAX);
  thread_arg = malloc(sizeof(serv_arg *) * CLIENT_MAX);
  socket_server = create_socket_server(port);
  init_thread_arg(thread_arg, &socket_clients);


  if(socket_server == -1){
    perror("create socket server");
    return -1;
  }
  
  while(counter_client <= CLIENT_MAX){
    printf("Server waiting for incoming connection...\n");
    
    socket_acc = accept(socket_server, NULL , NULL);
    if(socket_acc == -1){
      perror ("accept()");
      return -1;
    }

    idx = get_client_idx(socket_clients);
    socket_clients[idx] = socket_acc;
    thread_arg[idx].idx = idx;

    if(pthread_create(&thread_clients[idx], NULL, handle_client, (void*) &thread_arg[idx])){
      perror("pthread_create()");
      exit(1);
    }
    printf("Client number %d connected on slot %d\n", counter_client++, idx);
  }       
  return 0;
}
