#include<stdio.h>
#include<sys/socket.h>
#include <string.h>
#include<arpa/inet.h>
#include <netdb.h>

int hostname_to_ip(char * hostname , char * ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

int create_socket_client()
{
  int socket_client;
     
  //Create socket
  socket_client = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_client == -1){
    printf("Could not create socket");
  }
  return socket_client;
}

int create_socket_server(int port)
{
  int socket_server ;
  int optval = 1;

  socket_server = socket(AF_INET,SOCK_STREAM, 0);
 
  if (socket_server == -1)
    {
      perror ("socket_server");
      return -1;
    }
  struct sockaddr_in saddr;
  saddr.sin_family=AF_INET; /* Socket ipv4 */
  saddr.sin_port=htons(port); /* Port d ’ écoute */
  saddr.sin_addr.s_addr=INADDR_ANY; /* écoute sur toutes les interfaces */
 
if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
{
	perror ("Can not set SO_REUSEADDR option ");
	return -1;
}
if (bind(socket_server,(struct sockaddr *) &saddr , sizeof(saddr)) == -1)
    {
      perror ( "bind socker_serveur");
      return -1;
      /* traitement de l ’ erreur */
    }

  if(listen(socket_server,10) == -1)
    {
      perror("listen socket_server");
      return -1;
      /* traitement d ’ erreur */
    }
	
  return socket_server;
}




