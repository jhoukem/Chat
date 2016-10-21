#ifndef CLIENT
#define CLIENT

int handle_input(int socket_client);
void handle_listen(int socket_client);
int send_pseudo(int socket_client, char * pseudo);

#endif
