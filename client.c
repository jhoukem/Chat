#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include <unistd.h>

#define BUF_SIZ 1024



int main(int argc, char * argv[])
{

  int socket_client, port;
  char * hostname;
  char buffer[BUF_SIZ];
  struct sockaddr_in server;

  if(argc < 3){
    printf("Bad usage: %s [host_address] [port_number]\n", argv[0]);
    return -1;
  }

  // Set server to connect parameters.
  hostname = argv[1];
  port = atoi(argv[2]);

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
  printf("Connected\n");
     
  //keep communicating with server
  while(1){
    printf("Enter message : ");
    gets(buffer);
    
    //Send some data
     if(send(socket_client, buffer , strlen(buffer)+1 , 0) < 0){
    //if(write(socket_client, buffer , strlen(buffer)+1) < 0){    
      printf("Failed to send message: \"%s\"", buffer);
      return 1;
    }
    
    /* //Receive a reply from the server
    if(recv(socket_client, buffer, BUF_SIZ, 0) < 0){
      printf("Nothing received from the server\n");
      break;
      }*/
  }
  
  return 0;
}
