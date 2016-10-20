#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "socket.h"

#define BUF_SIZ 1024
#define PSEUDO_MAX_SIZE 10


int handle_input(int socket_client)
{
  char msg[BUF_SIZ];
  // Keep communicating with the server.
  while(1){
    printf("\nEnter a message : ");
    fgets(msg, BUF_SIZ, stdin);
    //Send some data
    if(write(socket_client, msg , strlen(msg) + 1) < 0){
      perror("send");
      printf("Failed to send message: \"%s\"\n", msg);
      return 1;
    }
  }
  printf("out\n");
  return 0;
}


int handle_listen(int socket_client)
{
  char buffer[BUF_SIZ + PSEUDO_MAX_SIZE];
 
  while(1){
    // Receive a reply from the server.
    if(read(socket_client, buffer, BUF_SIZ) <= 0){
      printf("\nConnection closed");
      break;
    }
    printf("\n%s", buffer);
  }
  return 0;
}

int send_pseudo(int socket_client, char * pseudo)
{
  if(send(socket_client, pseudo , PSEUDO_MAX_SIZE, 0) < 0){
    return -1;
  }
  return 0;
}


int main(int argc, char * argv[])
{

  int socket_client, port;
  char * hostname;
  char * pseudo;
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
  printf("Connected to %s\n", hostname);

  if(send_pseudo(socket_client, pseudo) < 0){
    printf("Can't send the pseudo to the server\n");
    return -1;
  }
  
  switch(fork()){
  case -1 : perror("fork()"); return -1;
  case 0: handle_listen(socket_client); exit(0); break;
  default: handle_input(socket_client); break;
  }
  wait(NULL);

  return 0;
}
