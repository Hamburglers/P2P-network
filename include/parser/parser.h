#include "config/config.h"
#include "net/packet.h"
#include <pthread.h>

void handle_connect(char command[], char ip[], int *port, Config *cfg);

void handle_disconnect(char command[], char ip[], int *port, 
    pthread_mutex_t *peer_list_lock, int *peer_count, Peer *peer_list);