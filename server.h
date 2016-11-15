#ifndef SERV
#define SERV

struct CLIENT
{
  char *pseudo;
  int socket;
};
typedef struct CLIENT CLIENT;

struct thread_arg_s
{   
  int current_client_id; // The id of the client to handle.
  CLIENT *clients; // Others client.
  int *counter_client; // Client counter.
  pthread_mutex_t *counter_lock; // Mutex to modify the counter.

};
typedef struct thread_arg_s * arg_s;

void * handle_client(void *arg);
int get_client_id(CLIENT *clients);
void send_to_clients_except(CLIENT *clients, CLIENT *client_exception, char * msg);
void send_to_clients(CLIENT *clients, char *msg);
void free_thread_rsc(arg_s thread_arg, int fflag);
int is_pseudo_free(char *pseudo, CLIENT *clients, int id_client);
int identify_client(CLIENT *client, int id_client);
int accept_client(int socket_server, CLIENT *clients, int *counter_client, pthread_mutex_t *counter_lock);

#endif
