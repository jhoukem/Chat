#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>   
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include<arpa/inet.h>
#include <netdb.h>
#include "server.h"
#include "socket.h"
#include "util.h"

// The password to connect to the server.
#define PASSWD "xxx"
#define CLIENT_MAX 10
#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 20
#define MAX_KEY_SIZE 20
#define REC_ERR 1
#define ID_ERR 2
#define PASS_ERR 3
#define ID_NOT_FREE_ERR 4
#define TIME_OUT 5
#define DEBUG 0
#define CONNECTION_SUCCESS "Connection successful"

void send_to_clients_except(CLIENT *clients, CLIENT *client_exception, char * msg)
{
  int i;
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message back to the sender or to a null socket.
    if((clients + i)->socket != 0 && (clients+i) != client_exception){ 
      write((clients + i)->socket , msg , strlen(msg) + 1);
    }
  }
}

void send_to_clients(CLIENT *clients, char * msg)
{
  int i;
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message back to a null client.
    if((clients + i)->socket != 0){ 
      write((clients + i)->socket , msg , strlen(msg) + 1);
    }
  }
}

void free_thread_rsc(arg_s thread_arg, int fflag)
{
  CLIENT *client;
  client = thread_arg->clients+thread_arg->current_client_id;
  

  // Close the client socket
  close(client->socket);
  // Free the client pseudo, and set the client socket to 0.
  if(fflag){
    free(client->pseudo);
  }
  client->socket = 0;

  // Update the number of client.
  pthread_mutex_lock(thread_arg->counter_lock);
  (*thread_arg->counter_client)--;
  pthread_mutex_unlock(thread_arg->counter_lock);

  // flush all the stdout for the log file.
  fflush(stdout);
}

void * handle_client(void * arg)
{
  arg_s thread_arg = (arg_s) arg;
  int read_size, ret;
  char buffer[BUF_SIZ];
  char client_msg[BUF_SIZ+PSEUDO_MAX_SIZE];
  CLIENT *client;
  client = thread_arg->clients+thread_arg->current_client_id;

  if((ret = identify_client(thread_arg->clients, thread_arg->current_client_id)) != 0){
    switch(ret){
    case TIME_OUT:
      snprintf(buffer, BUF_SIZ, "Error: Server timeout.");
      printf("Error: Client time out.\n"); break;
    case REC_ERR: 
      snprintf(buffer, BUF_SIZ, "Error: Server can't read.");
      printf("Error: Can't read from the client.\n"); break;
    case ID_ERR: 
      snprintf(buffer, BUF_SIZ, "No ID send.");
      printf("Error: No ID received from the client.\n"); break;
    case PASS_ERR: 
      snprintf(buffer, BUF_SIZ, "Error: Wrong server password.");
      printf("Error: Wrong password from the client.\n"); break;
    case ID_NOT_FREE_ERR: 
      snprintf(buffer, BUF_SIZ, "Error: The pseudo already exist.");
      printf("Error: The client pseudo already exist.\n"); break;
    }
    // Send the error message to the client.
    write(client->socket ,buffer , strlen(buffer) + 1);
    printf("Disconnecting the client %d. Thread %d stop.\n", thread_arg->current_client_id + 1, thread_arg->current_client_id);
    free_thread_rsc(thread_arg, 0);
    pthread_exit(NULL);
  } 
  // Send the status to the client.
  snprintf(buffer, BUF_SIZ, CONNECTION_SUCCESS);
  write(client->socket ,buffer , strlen(buffer) + 1);
  
  printf("Client %d identified successfully and joined the chat\n", thread_arg->current_client_id + 1);
  snprintf(client_msg, BUF_SIZ, "%s has joined the chat.", client->pseudo);
  send_to_clients_except(thread_arg->clients, client, client_msg);

  // Receive a message from the client.
  while((read_size = read(client->socket , buffer , BUF_SIZ)) > 0 ){
    // printf("Server received: %s\n", buffer);
    // Don't send empty message.
    // This was also checked in the client but the server perform a second check.
    if(!is_empty(buffer)){
      // Add the client pseudo to the message.
      snprintf(client_msg, PSEUDO_MAX_SIZE, "%s: ", client->pseudo);
      strncat(client_msg, buffer, BUF_SIZ);  
      send_to_clients(thread_arg->clients, client_msg);
    }
  }
  
  if(read_size == 0) {
    printf("Client number %d has disconnected\n", thread_arg->current_client_id + 1);
    snprintf(client_msg, BUF_SIZ, "%s has left the chat.", client->pseudo);
    send_to_clients_except(thread_arg->clients, client, client_msg);
  }
  else if(read_size == -1){
    perror("read failed");
  }
  
  free_thread_rsc(thread_arg, 1);
  pthread_exit(NULL);
}

int get_client_id(CLIENT *clients)
{
  int i;
  for(i = 0; i < CLIENT_MAX; i++){
    // if it is a free space.
    if((clients + i)->socket == 0){ 
      return i;
    }
  }
  return -1;
}

int is_pseudo_free(char *pseudo, CLIENT *clients, int client_id)
{
  int i;
  for(i = 0; i < CLIENT_MAX; i++){
    // If the pseudo is already taken.
    if(i != client_id){
      if((clients+i)->socket != 0 && (strcmp((clients+i)->pseudo, pseudo) == 0)){ 
	return 0;
      }
    }
  }
  return 1;
}

int identify_client(CLIENT *clients, int id_client)
{
  int ret = 0;  
  char *key, *pseudo_tmp;
  char buffer[PSEUDO_MAX_SIZE + MAX_KEY_SIZE + 1] = {0};  
  fd_set readfs;
  struct timeval timeout;
  CLIENT *client = (clients + id_client);
  // The server wait the client credential for 20 seconds.
  timeout.tv_sec = 20;
  timeout.tv_usec = 0;
  
  FD_ZERO(&readfs);
  FD_SET(client->socket, &readfs);

  if((ret = select(client->socket + 1, &readfs, NULL, NULL, &timeout)) <= 0){
    return TIME_OUT;
  } 
  else if(FD_ISSET(client->socket, &readfs)) {
    
    if((ret = read(client->socket , buffer , PSEUDO_MAX_SIZE + MAX_KEY_SIZE + 1)) <= 0){
      return REC_ERR;
    }
    if(DEBUG){
      printf("Received string: %s\n", buffer);
    }
    // Check input send by the client.
    pseudo_tmp = strtok(buffer, ":");
    if(DEBUG){
      printf("Received id: %s\n", pseudo_tmp);
    }    
    if(pseudo_tmp == NULL){
      return ID_ERR;
    }
    // Remove the trailling spaces.
    pseudo_tmp = strstrip(pseudo_tmp);
    if(!is_pseudo_free(pseudo_tmp, clients, id_client)){
      return ID_NOT_FREE_ERR;
    }
    key = strtok(NULL, ":");
    if(DEBUG){
      printf("Received passwd: %s\n", key);
    }
    
    if(key == NULL){
      if(strcmp(PASSWD, "") != 0){
	return PASS_ERR;
      }
    } else if(strcmp(key, PASSWD) != 0){
      // If the password is wrong.
      return PASS_ERR;
    }
  }
    
  // If everything is ok
  client->pseudo = malloc(sizeof(char) * strlen(pseudo_tmp));
  strncpy(client->pseudo,  pseudo_tmp, strlen(pseudo_tmp));
  return 0;
}

int accept_client(int socket_server, CLIENT *clients, int * counter_client, pthread_mutex_t * counter_lock)
{
  int id;
  socklen_t client_len;
  int socket_acc;
  pthread_t thread_client;
  arg_s thread_arg = NULL;
  struct sockaddr_in client_address;
  char clntName[INET6_ADDRSTRLEN];
  client_len = sizeof(client_address);
  printf("Server waiting for incoming connection...\n");
  
  // Accept the connection.
  socket_acc = accept(socket_server, (struct sockaddr *)&client_address, &client_len);
  if(socket_acc == -1){
    perror ("accept()");
    return -1;
  }
  
  // Get an ID for the client.
  id = get_client_id(clients);
  if(id < 0){
    printf("accept_client() error: client ID < 0\n");
    return -1;
  }

  // Update the client counter.
  pthread_mutex_lock(counter_lock);
  (*counter_client)++;
  pthread_mutex_unlock(counter_lock);
  
  // Set the data for the client.  
  thread_arg = malloc(sizeof(struct thread_arg_s));
  thread_arg->current_client_id = id;
  thread_arg->clients = clients;
  thread_arg->counter_client = counter_client;
  thread_arg->counter_lock = counter_lock;
  
  // Save the client socket.
  (clients + id)->socket = socket_acc;
 
  
  // Start a thread that will handle the client.
  if(pthread_create(&thread_client, NULL, handle_client, (void*) thread_arg)){
    perror("pthread_create()");
    return -1;
  }

  if(pthread_detach(thread_client) != 0){
    perror("pthread_detach");
    return -1;
  }

  inet_ntop(AF_INET, &client_address.sin_addr.s_addr, clntName, sizeof(clntName));

  printf("Client #%d is connected on slot %d with the ip address: %s on the port %d\n", *counter_client, id, clntName, ntohs(client_address.sin_port));
  return 0;
}      


int main(int argc, char * argv[])
{

  int i, socket_server, port, counter_client;
  CLIENT *clients = NULL;
  pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

  if(argc < 2){
    printf("Bad usage: %s [port_number]\n", argv[0]);
    return -1;
  }
  
  counter_client = 0;
  port = atoi(argv[1]);
  clients = malloc(CLIENT_MAX * sizeof(struct CLIENT));
  for(i = 0; i < CLIENT_MAX; i++){
    clients[i].socket = 0;
  }
  socket_server = create_socket_server(port);
  
  
  if(socket_server == -1){
    perror("create_socket_server()");
    return -1;
  }
  
  while(1){
    if(counter_client < CLIENT_MAX){
      if(accept_client(socket_server, clients, &counter_client, &counter_lock) < 0){
	return -1;
      }
    }  
  }
  return 0;
}

