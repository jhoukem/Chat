#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZ 1024



int handle_input(char * pseudo, int socket_client)
{
  char buffer[BUF_SIZ];
  char msg[BUF_SIZ * 2];
  //keep communicating with server
  while(1){
    printf("Enter message : ");
    fgets(buffer, BUF_SIZ, stdin);
    sprintf(msg, "%s: %s", pseudo, buffer);
    //Send some data
    if(send(socket_client, msg , strlen(msg) + 1 , 0) < 0){
      perror("send");
      printf("Failed to send message: \"%s\"\n", msg);
      return 1;
    }
  }
  return 0;
}


int handle_listen(int socket_client)
{
  char buffer[BUF_SIZ];
 
  while(1){
    //Receive a reply from the server
    if(recv(socket_client, buffer, BUF_SIZ, 0) <= 0){
      printf("\nConnection closed\n");
      break;
    }
    printf("\n%s\n", buffer);
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

  // Set server to connect parameters.
  hostname = argv[1];
  port = atoi(argv[2]);
  pseudo = argv[3];
  server.sin_addr.s_addr = inet_addr(hostname);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  
  // Create client's socket.
  socket_client = create_socket_client();

  //Connect to remote server
  if(connect(socket_client , (struct sockaddr *)&server , sizeof(server)) < 0){
    perror("Connection failed");
    return 1;
  }
  printf("Connected to %s\n", hostname);
  
  switch(fork()){
  case -1 : perror("fork"); return -1;
  case 0: handle_listen(socket_client); exit(0); break;
  default: handle_input(pseudo, socket_client); break;
  }
  wait(NULL);

  return 0;
}
