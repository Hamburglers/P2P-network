#include <arpa/inet.h>
#include <config/config.h>

void *server_function(void *arg);

void *client_function(void *arg);

void *handle_connection(void *arg);

void add_peer(int socket, struct sockaddr_in address, Config *cfg);

void remove_peer(int socket);

void send_acp_packet(int socket);

void send_ack_packet(int socket);

void send_dsn_packet(int socket);

void *handle_connection(void *arg);

int connect_to_peer(const char *ip, int port);

int is_already_connected(const char *ip, int port);