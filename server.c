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
#define CMD_HELP "/help"
#define CMD_WHO "/who"
#define CMD_QUIT "/quit"

// Mutex to modify the shared ressources between threads.
pthread_mutex_t shared_rsc_lock = PTHREAD_MUTEX_INITIALIZER; 

int client_connected(CLIENT client)
{
  return client.socket != 0 && client.pseudo != NULL;
}

void send_to_clients_except(CLIENT *clients, CLIENT *client_exception, char * msg)
{
  int i;
  // Lock the shared ressources.
  pthread_mutex_lock(&shared_rsc_lock);
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message back to the sender or to a null socket.
    if(client_connected(clients[i]) && &clients[i] != client_exception){ 
      if(write(clients[i].socket , msg , strlen(msg) + 1) < 0){
	perror("write to client except: ");
      }
    }
  }
  pthread_mutex_unlock(&shared_rsc_lock);
}

void send_to_clients(CLIENT *clients, char * msg)
{
  int i;
  // Lock the shared ressources.
  pthread_mutex_lock(&shared_rsc_lock);
  // Send the message to all the clients.
  for(i = 0; i < CLIENT_MAX; i++){
    // Don't send the message to a null socket.
    if(client_connected(clients[i])){ 
      if(write(clients[i].socket , msg , strlen(msg) + 1) < 0){
      perror("write to client: ");
      }
    }
  }
  pthread_mutex_unlock(&shared_rsc_lock);
}

void free_thread_rsc(arg_s thread_arg, int fflag)
{
  CLIENT *client;
  client = thread_arg->clients+thread_arg->current_client_id;  

  // Lock the shared ressources.
  pthread_mutex_lock(&shared_rsc_lock);

  // Close the client socket, and set it to 0.
  close(client->socket);
  client->socket = 0;

  // Free the client pseudo and set it to NULL.
  if(fflag){
    client->pseudo = NULL;
    free(client->pseudo);
  }

  // Update the number of client.
  (*thread_arg->counter_client)--;

  pthread_mutex_unlock(&shared_rsc_lock);

  // flush all the stdout for the log file.
  fflush(stdout);
}

int is_command(char *msg)
{
  return '/' == *msg;
}

void process_command(char *msg, CLIENT *clients, int current_client_id, int *quit_flag)
{
  int i, send;
  char buffer[BUF_SIZ] = {0};
  send = 1;

  if(strcmp(msg, CMD_HELP) == 0){
    snprintf(buffer, BUF_SIZ, "List of the command availaible on this server: /help, /who, /quit.");
  } else if(strcmp(msg, CMD_WHO) == 0){
  
    snprintf(buffer, BUF_SIZ, "Client(s) connected: ");   
    for(i = 0; i < CLIENT_MAX; i++){
      if(client_connected(clients[i])){
	snprintf(buffer + strlen(buffer), BUF_SIZ - (strlen(buffer) + 1), "%s, ", clients[i].pseudo);
      }
    }
    // Remove the last comma.
    buffer[strlen(buffer) - 2] = '\0';
  } else if(strcmp(msg, CMD_QUIT) == 0){
    *quit_flag = 1;
    send = 0;
  } else {
    snprintf(buffer, BUF_SIZ, "Error unknown command.");
  }
  if(send){
    if(write(clients[current_client_id].socket, buffer, strlen(buffer) + 1) < 0){
      perror("write client command: ");
    }
  }
}

void * handle_client(void * arg)
{
  arg_s thread_arg = (arg_s) arg;
  int read_size, ret, quit_flag;
  char buffer[BUF_SIZ];
  char client_msg[BUF_SIZ+PSEUDO_MAX_SIZE];
  CLIENT *client;
  quit_flag = 0;
  client = &thread_arg->clients[thread_arg->current_client_id];
 
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
    if(write(client->socket, buffer , strlen(buffer) + 1) < 0){
      perror("write cleint error msg: ");
    }
    printf("Disconnecting the client %d. Thread %d stop.\n", thread_arg->current_client_id + 1, thread_arg->current_client_id);
    free_thread_rsc(thread_arg, 0);
    pthread_exit(NULL);
  } 

  // Send the status to the client.
  snprintf(buffer, BUF_SIZ, CONNECTION_SUCCESS);
  if(write(client->socket ,buffer , strlen(buffer) + 1) < 0){
    perror("write client status: ");
  }
  
  printf("Client %d identified successfully and joined the chat\n", thread_arg->current_client_id + 1);
  snprintf(client_msg, BUF_SIZ, "%s has joined the chat.", client->pseudo);
  send_to_clients_except(thread_arg->clients, client, client_msg);

  // Receive a message from the client.
  while(!quit_flag && (read_size = read(client->socket , buffer , BUF_SIZ)) > 0 ){
    if(DEBUG) {
      printf("Server received: %s\n", buffer);
    }    
    // Don't send empty message. This was also checked in the client but the server perform a second check.
    if(is_empty(buffer)){
      continue;
    }
 
    if(is_command(buffer)){
      process_command(buffer, thread_arg->clients, thread_arg->current_client_id, &quit_flag);
    }
    // It is a simple message.
    else {
      // Add the client pseudo to the message.
      snprintf(client_msg, PSEUDO_MAX_SIZE, "%s: ", client->pseudo);
      strncat(client_msg, buffer, BUF_SIZ);  
      send_to_clients(thread_arg->clients, client_msg);
    }
  }

  if(read_size == 0 || quit_flag) {
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
  int i, id;
  id = -1;
  // Lock the shared ressources.
  pthread_mutex_lock(&shared_rsc_lock);

  for(i = 0; i < CLIENT_MAX; i++){
    // if it is a free space.
    if(clients[i].socket == 0){ 
      id = i;
      break;
    }
  }
  pthread_mutex_unlock(&shared_rsc_lock);
  return id;
}

int is_pseudo_free(char *pseudo, CLIENT *clients, int client_id)
{
  int i, status;
  status = 1;
  // Lock the shared ressources.
  pthread_mutex_lock(&shared_rsc_lock);
 
  for(i = 0; i < CLIENT_MAX; i++){
    // If the pseudo is already taken.
    if(i != client_id){
     
      if(client_connected(clients[i]) && (strcmp(clients[i].pseudo, pseudo) == 0)){
	status = 0;
	break;
      }
    }
  }

  pthread_mutex_unlock(&shared_rsc_lock);
  return status;
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
  
  // If everything is ok.  (+1 stand here for the null byte)
  client->pseudo = malloc(sizeof(char) * (strlen(pseudo_tmp) + 1));
  strncpy(client->pseudo,  pseudo_tmp, strlen(pseudo_tmp) + 1);
  return 0;
}

int accept_client(int socket_server, CLIENT *clients, int * counter_client)
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
  pthread_mutex_lock(&shared_rsc_lock);
  (*counter_client)++;
    
  // Set the data for the client.  
  thread_arg = malloc(sizeof(struct thread_arg_s));
  thread_arg->current_client_id = id;
  thread_arg->clients = clients;
  thread_arg->counter_client = counter_client;
  
  // Save the client socket.
  clients[id].socket = socket_acc;
 
  pthread_mutex_unlock(&shared_rsc_lock);
  
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

  if(argc < 2){
    printf("Bad usage: %s [port_number]\n", argv[0]);
    return -1;
  }
  
  counter_client = 0;
  port = atoi(argv[1]);
  clients = malloc(CLIENT_MAX * sizeof(struct CLIENT));
  for(i = 0; i < CLIENT_MAX; i++){
    clients[i].socket = 0;
    clients[i].pseudo = NULL;
  }
  socket_server = create_socket_server(port);
  
  
  if(socket_server == -1){
    perror("create_socket_server()");
    return -1;
  }
  
  while(1){
    if(counter_client < CLIENT_MAX){
      if(accept_client(socket_server, clients, &counter_client) < 0){
	return -1;
      }
    }  
  }
  return 0;
}

