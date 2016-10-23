#ifndef SOCK
#define SOCK

int hostname_to_ip(char * hostname, char * ip);
int create_socket_client();
int create_socket_server(int port);

#endif

