#include "config/config.h"
#include <pthread.h>
#include <net/packet.h>

void send_dsn_packet(int socket);

void send_pog_packet(int socket);

void send_acp_packet(int socket);

void send_ack_packet(int socket);

void send_png_packet(int socket);

void send_req_packet(int socket, const char *identifier, 
    const char *chunk_hash, uint32_t offset, uint32_t size);

void send_res_packet(int socket, const char *identifier, 
    const char *chunk_hash, uint32_t offset, const char* buffer, 
    uint16_t read, uint16_t error);

void handle_peers(pthread_mutex_t peer_list_lock, 
    int* peer_count, Peer *peer_list);

void *handle_connection(void *arg);

int connect_to_peer(const char *ip, int port);
