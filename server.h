#ifndef SERV
#define SERV

void * handle_client(void * arg);
int get_client_idx(int * tab);
void init_clients_sockets(int * tabt);

#endif
