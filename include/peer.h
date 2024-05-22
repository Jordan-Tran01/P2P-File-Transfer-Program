#include <pthread.h>

typedef struct peer_node {
    char *ip;
    int port;
    int socket_fd;
    struct peer_node *next;
} peer_node;

typedef struct package_node {
    char *package_path;
    char *bpkg_path;
    char *ident;
    int complete;
    struct package_node *next;
} package_node;

#define BUFFER_SIZE 4096
#define MAX_RES_DATA 2998

void print_peer_list(peer_node *head);
void free_peer_list(peer_node **head);
void add_peer_to_list(peer_node **head, const char *ip, int port, int socket_fd);
peer_node* find_peer(peer_node *head, const char *ip, int port);

void free_package_list(package_node **head);
void print_package_list(package_node *head);
int remove_package_from_list(package_node **head, const char *ident);
void add_package_to_list(package_node **head, const char *path, const char *bpkg, int complete, const char *ident);
package_node* find_package(package_node *head, const char *identifier);

int disconnect_from_peer(peer_node **head, const char *ip, int port);
int init_server(int port, int max_peers, package_node **package_list, pthread_mutex_t *mutex);
int connect_to_peer(const char* ip, int port);