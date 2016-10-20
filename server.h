#ifndef SERV
#define SERV

struct thread_arg
{   
  int idx; // Client ID
  int *counter_client; // Clients count
  int *socket_clients; // Other clients ID
  char *pseudo; // Client human readable id
  pthread_mutex_t * counter_lock; // Mutex to modify the counter

};
typedef struct thread_arg * arg_t;

void * handle_client(void * arg);
int get_client_idx(int * tab);
void send_to_clients_except(arg_t thread_arg, char * msg);
void free_thread_rsc(arg_t thread_arg);
int get_client_pseudo(int socket_client, char * buffer);
int accept_client(int socket_server, int * socket_clients, int * counter_client, pthread_mutex_t * counter_lock);

#endif
