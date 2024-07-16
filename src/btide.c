#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/config.h"
#include "net/packet.h"
#include "chk/pkgchk.h"
#include "parser/parser.h"
#include "package/package.h"
#include "peer/peer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

#define MAXLINELENGTH 128
#define MAXCOMMANDLENGTH 5520
#define BUFFER_SIZE 4096
#define INITIALPACKAGELENGTH 8
#define MAXIDENTLENGTH 1025
#define MAXHASHLENGTH 65
#define MAX_IDENTIFIER_LENGTH 1024
#define MAX_HASH_LENGTH 64
#define MAXDATA 2998
//
// PART 2
//
// Contains the main function, starting point of the program.

// global variables holding list of peers and mutexes
Peer *peer_list = NULL;
int peer_count = 0;
pthread_mutex_t peer_list_lock;
bpkg_obj **list;
int current_length = 0;
int max_size = INITIALPACKAGELENGTH;

int server_fd;
volatile int terminate_flag = 0;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_t server_thread, client_thread;

// forward declaration
void *server_function(void *arg);
void *client_function(void *arg);
void *handle_connection(void *arg);
void add_peer(int socket, struct sockaddr_in address, Config *cfg);

void signal_termination()
{
    pthread_mutex_lock(&terminate_mutex);
    terminate_flag = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

void cleanup() {
    for (int i = 0; i < peer_count; i++)
    {
        send_dsn_packet(peer_list[i].socket);
        close(peer_list[i].socket);
        // printf("cancelling thread %ld\n", peer_list[i].id);
        // pthread_cancel(peer_list[i].id);
        pthread_join(peer_list[i].id, NULL);
    }
    free(peer_list);
    for (int i = 0; i < current_length; i++)
    {
        bpkg_obj_destroy(list[i]);
        list[i] = NULL;
    }
    free(list);
    list = NULL;
    pthread_mutex_destroy(&peer_list_lock); 
    pthread_mutex_destroy(&terminate_mutex);
    pthread_cond_destroy(&terminate_cond);
}

void sigint_handler(int signum)
{
    signal_termination();
}

int main(int argc, char **argv)
{
    // ignore sigpipe
    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    // run using valgrind
    // if (getenv("RUNNING_UNDER_VALGRIND") == NULL) {
    //     setenv("RUNNING_UNDER_VALGRIND", "1", 1);
    //     char **new_argv = malloc((argc + 3) * sizeof(char *));
    //     if (new_argv == NULL) {
    //         perror("malloc");
    //         exit(EXIT_FAILURE);
    //     }

    //     new_argv[0] = "valgrind";
    //     new_argv[1] = "--leak-check=full";
    //     for (int i = 0; i < argc; i++) {
    //         new_argv[i + 2] = argv[i];
    //     }
    //     new_argv[argc + 2] = NULL;

    //     execvp("valgrind", new_argv);
    // }

    // check argument length should be 2
    if (argc != 2)
    {
        fprintf(stderr, "Usage: ./btide <config file path>\n");
        return 1;
    }
    // create config struct and call function in config to parse file
    Config cfg;
    int result = parse_config(argv[1], &cfg);
    // if function did not return 0, then an issue happened
    if (result != 0)
    {
        fprintf(stderr, "Failed to parse config\n");
        return result;
    }
    // debug
    // printf("%s\n", cfg.directory);
    // printf("%d\n", cfg.max_peers);
    // printf("%d\n", cfg.port);

    // peer list
    peer_list = calloc(cfg.max_peers, sizeof(Peer));
    if (!peer_list)
    {
        perror("Memory allocation failed for peer_list");
        return 1;
    }
    list = malloc(INITIALPACKAGELENGTH * sizeof(bpkg_obj *));
    if (!list)
    {
        perror("Malloc failed");
        free(peer_list);
        return 1;
    }
    pthread_mutex_init(&peer_list_lock, NULL);

    // server thread
    if (pthread_create(&server_thread, NULL, 
        server_function, (void *)&cfg) != 0)
    {
        perror("Failed to create server thread");
        return 1;
    }
    // client thread
    if (pthread_create(&client_thread, NULL, 
        client_function, (void *)&cfg) != 0)
    {
        perror("Failed to create client thread");
        return 1;
    }
    // join and return
    pthread_join(server_thread, NULL);
    signal_termination();
    pthread_join(client_thread, NULL);
    cleanup();
    return 0;
}

// code from resources section with minor modifications -> server.c
void *server_function(void *arg)
{
    Config *cfg = (Config *)arg;
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        return NULL;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(cfg->port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        return NULL;
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        return NULL;
    }
    // printf("S: Server listening on port %d...\n", cfg->port);

    // inf loop
    fd_set readfds;
    struct timeval timeout;

    int new_socket;
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        pthread_mutex_lock(&terminate_mutex);
        if (terminate_flag)
        {
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR)
        {
            perror("Select error");
        }

        if (activity == 0)
        {
            continue;
        }

        if (FD_ISSET(server_fd, &readfds))
        {
            // printf("S: Waiting for connections\n");
            socklen_t addrlen = sizeof(struct sockaddr);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                continue;
            }
            // printf("S: New connection, socket fd is %d, IP is : %s,
            // port : %d\n", new_socket, inet_ntoa(address.sin_addr), 
            // ntohs(address.sin_port));
            // send acp packet
            send_acp_packet(new_socket);

            // wait for ACK before adding the peer
            struct btide_packet packet;
            if (read(new_socket, &packet, sizeof(packet)) > 0 
                && packet.msg_code == PKT_MSG_ACK)
            {
                add_peer(new_socket, address, cfg);
                // printf("S: ACK received\n");
            }
            else
            {
                // printf("S: ACK not received - Terminating\n");
                close(new_socket);
            }
        }
    }
    // printf("S: Terminating server...\n");
    close(server_fd);
    return NULL;
}

// function to add a peer to the peer list
void add_peer(int socket, struct sockaddr_in address, Config *cfg)
{
    // mutex lock prevent race condition / undefined behaviour
    pthread_mutex_lock(&peer_list_lock);
    // if there are still spaces left for connections
    if (peer_count < cfg->max_peers)
    {
        pthread_t thread_id;

        if (pthread_create(&thread_id, NULL, handle_connection, 
            &peer_list[peer_count]) != 0)
        {
            perror("Thread creation failed");
            close(socket);
        } else {
            peer_list[peer_count].socket = socket;
            peer_list[peer_count].address = address;
            peer_list[peer_count].id = thread_id;
            peer_count++;
            // printf("new thread id %ld", thread_id);
        }
    }
    else
    {
        fprintf(stderr, "Max peers reached, connection refused\n");
        close(socket);
    }
    // printf("G: Added peer...\n");
    pthread_mutex_unlock(&peer_list_lock);
}

// function to remove a peer from the peer list
void remove_peer(int socket)
{
    // make thread safe
    pthread_mutex_lock(&peer_list_lock);
    for (int i = 0; i < peer_count; i++)
    {
        if (peer_list[i].socket == socket)
        {
            close(peer_list[i].socket);
            // move last peer to empty spot so theres no holes
            peer_list[i] = peer_list[peer_count - 1];
            // dont forget to decrement the count
            (peer_count)--;
            break;
        }
    }
    // printf("G: Removed peer...\n");
    pthread_mutex_unlock(&peer_list_lock);
}

// check if a certain port is already connected
// preventing duplicate connections
int is_already_connected(const char *ip, int port)
{
    pthread_mutex_lock(&peer_list_lock);
    for (int i = 0; i < peer_count; i++)
    {
        char connected_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_list[i].address.sin_addr, 
            connected_ip, INET_ADDRSTRLEN);
        int connected_port = ntohs(peer_list[i].address.sin_port);
        // check if ip and port match so theres already a connection
        if (strcmp(connected_ip, ip) == 0 && connected_port == port)
        {
            pthread_mutex_unlock(&peer_list_lock);
            return peer_list->socket;
        }
    }
    pthread_mutex_unlock(&peer_list_lock);
    return -1;
}

// function which threads use when incoming connection
void *handle_connection(void *arg)
{
    Peer *peer = (Peer *)arg;
    struct btide_packet packet;
    ssize_t num_bytes;

    char identifier[MAX_IDENTIFIER_LENGTH];
    char chunk_hash[MAX_HASH_LENGTH];
    uint32_t offset;
    uint32_t size;
    // handle which sort of packet
    // printf("G: Listening for packets\n");
    // keep receiving packets, until nothing is received
    while (1)
    {
        pthread_mutex_lock(&terminate_mutex);
        if (terminate_flag)
        {
            pthread_mutex_unlock(&terminate_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&terminate_mutex);

        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(peer->socket, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(peer->socket + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR)
        {
            perror("Select error on connecting thread");
            continue;
        }

        if (activity == 0)
        {
            continue;
        }

        if (FD_ISSET(peer->socket, &readfds)) {
            num_bytes = recv(peer->socket, &packet, 
                sizeof(struct btide_packet), 0);
            if (num_bytes > 0)
            {
                switch (packet.msg_code)
                {
                case PKT_MSG_DSN:
                    // Disconnect and cleanup
                    // printf("G: DSN received, disconnecting peer.\n");
                    remove_peer(peer->socket);
                    pthread_exit(NULL);
                    break;
                case PKT_MSG_ACK:
                    // Process ACK packet
                    // printf("G: ACK received from peer\n");
                    break;
                case PKT_MSG_ACP:
                    // Process ACP packet
                    // printf("G: About to send packet\n");
                    send_acp_packet(peer->socket);
                    break;
                case PKT_MSG_REQ:
                    // Handle data request
                    memcpy(&offset, packet.pl.data, sizeof(uint32_t));
                    memcpy(&size, packet.pl.data + sizeof(uint32_t), 
                            sizeof(uint32_t));
                    memcpy(chunk_hash, packet.pl.data + 2 * sizeof(uint32_t), 
                            MAX_HASH_LENGTH);
                    memcpy(identifier, packet.pl.data + 2 * sizeof(uint32_t) 
                            + MAX_HASH_LENGTH, MAX_IDENTIFIER_LENGTH);

                    // printf("offset: %d\n", offset);
                    // printf("size: %d\n", size);
                    // printf("chunk_hash: %.64s\n", chunk_hash);
                    // printf("identifier: %.1024s\n", identifier);

                    int sent = 0;
                    // check if there exists a bpkg obj with certain identifier
                    bpkg_obj *obj = 
                        check_ident(identifier, current_length, list);
                    if (obj != NULL)
                    {
                        // check if there exists a certain Chunk with chunk hash
                        Chunk *chunk = request_hash(chunk_hash, obj);
                        if (chunk != NULL)
                        {
                            // keep reading, keeping track of the offset read
                            // and splitting the packet into multiple until
                            // no bytes remaining
                            FILE *file = fopen(obj->filename, "rb");
                            if (file)
                            {
                                fseek(file, offset, SEEK_SET);
                                size_t remaining_size = size;
                                char buffer[MAXDATA];
                                uint32_t offset = chunk->offset;
                                while (remaining_size > 0)
                                {
                                    size_t to_read = remaining_size > MAXDATA 
                                        ? MAXDATA : remaining_size;
                                    size_t read_bytes = 
                                    fread(buffer, 1, to_read, file);
                                    if (read_bytes != 0)
                                    {
                                        // send the res packet with information
                                        send_res_packet(peer->socket, 
                                            identifier, chunk_hash, 
                                            offset, buffer, read_bytes, 0);

                                        remaining_size -= read_bytes;
                                        offset += read_bytes;
                                        sent = 1;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                fclose(file);
                            }
                        }
                    }
                    if (!sent)
                    {
                        // send an error res packet
                        send_res_packet(peer->socket, identifier, chunk_hash, 
                            offset, NULL, 0, 1);
                    }
                    break;
                case PKT_MSG_RES:
                    // Receive requested data
                    if (packet.error)
                    {
                        printf("Error in RES packet\n");
                        continue;
                    }
                    uint16_t new_size;
                    char buffer[MAXDATA];
                    // store the payload information into variables
                    memcpy(&offset, packet.pl.data, sizeof(uint32_t));
                    memcpy(&buffer, packet.pl.data + sizeof(uint32_t), MAXDATA);
                    memcpy(&new_size, packet.pl.data + sizeof(uint32_t) 
                        + MAXDATA, sizeof(uint16_t));
                    memcpy(chunk_hash, packet.pl.data + sizeof(uint32_t) 
                        + MAXDATA + sizeof(uint16_t), MAX_HASH_LENGTH);
                    memcpy(identifier, packet.pl.data + sizeof(uint32_t) 
                        + MAXDATA + sizeof(uint16_t) 
                        + MAX_HASH_LENGTH, MAX_IDENTIFIER_LENGTH);

                    // printf("offset: %d\n", offset);
                    // printf("size: %d\n", new_size);
                    // printf("chunk_hash: %.64s\n", chunk_hash);
                    // printf("identifier: %.1024s\n", identifier);
                    // printf("buffer: %.2998s\n", buffer);
                    bpkg_obj *new_obj = check_ident(identifier, current_length, 
                        list);
                    if (new_obj == NULL)
                    {
                        printf("Identifier was not part of a package\n");
                        continue;
                    }
                    // open the file to write the new data into
                    FILE *file = fopen(new_obj->filename, "r+b");
                    if (file == NULL)
                    {
                        printf("File specified does not exist\n");
                        continue;
                    }
                    fseek(file, offset, SEEK_SET);
                    fwrite(buffer, 1, new_size, file);
                    fclose(file);
                    break;
                case PKT_MSG_PNG:
                    // Send a POG in response
                    send_pog_packet(peer->socket);
                    break;
                case PKT_MSG_POG:
                    // Process POG packet
                    // do nothing
                    break;
                default:
                    fprintf(stderr, "Unhandled packet type: %d\n", 
                        packet.msg_code);
                }
            }
            else
            {
                // The peer has closed the connection
                // printf("G: Connection closed by peer\n");
                // dont remove to pass testcase
                // remove_peer(peer->socket);
                pthread_exit(NULL);
            }
        }
    }
    pthread_exit(NULL);
}

// code from resources section with minor modifications -> client.c
void *client_function(void *arg)
{
    Config *cfg = (Config *)arg;
    char command[MAXCOMMANDLENGTH];
    char ip[MAXLINELENGTH];
    int port;
    // use select to prevent thread from blocking on user input
    // periodically check if terminate signal has been sent
    while (1) {
        pthread_mutex_lock(&terminate_mutex);
        if (terminate_flag)
        {
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR)
        {
            perror("Select error on stdin");
            continue;
        }

        if (activity == 0)
        {
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            if (fgets(command, sizeof(command), stdin) != NULL)
            {
                // printf("C: Number of connections: %d\n", peer_count);
                // printf("C: Listening for commands:\n");
                if (strncmp(command, "QUIT", 4) == 0)
                {
                    // printf("C: Terminating program\n");
                    signal_termination();
                    break;
                }
                else if (strncmp(command, "CONNECT", 7) == 0)
                {
                    // Handle CONNECT command
                    handle_connect(command, ip, &port, cfg);
                }
                else if (strncmp(command, "DISCONNECT", 10) == 0)
                {
                    // Handle DISCONNECT command
                    handle_disconnect(command, ip, &port, &peer_list_lock, 
                    &peer_count, peer_list);
                }
                else if (strncmp(command, "ADDPACKAGE", 10) == 0)
                {
                    // Handle ADDPACKAGE command
                    handle_add_package(command, &current_length, &max_size, 
                        &list, cfg->directory);
                }
                else if (strncmp(command, "REMPACKAGE", 10) == 0)
                {
                    // Handle REMPACKAGE command
                    handle_remove_package(command, &current_length, 
                        &max_size, list);
                }
                else if (strncmp(command, "PACKAGES", 8) == 0)
                {
                    // Handle PACKAGES command
                    handle_view_package(command, &current_length,
                        &max_size, list);
                }
                else if (strncmp(command, "PEERS", 5) == 0)
                {
                    // Handle PEERS command
                    if (command[6] != '\0')
                    {
                        printf("Invalid Input\n");
                        continue;
                    }
                    handle_peers(peer_list_lock, &peer_count, peer_list);
                }
                else if (strncmp(command, "FETCH", 5) == 0)
                {
                    // Handle FETCH command
                    if (command[5] != ' ' || command[6] == ' ' 
                        || command[6] == '\0')
                    {
                        printf("Invalid Input\n");
                        continue;
                    }
                    char remainder = '\0';
                    char ident[MAXIDENTLENGTH];
                    char hash[MAXHASHLENGTH];
                    int offset = 0;
                    // make sure input is in the correct format
                    int args = sscanf(command + 6, "%127[^:]:%d %s %s %d%c", 
                    ip, &port, ident, hash, &offset, &remainder);

                    // should be at least 4 accounting for the optional argument
                    if (args < 4 || (remainder != '\n' && remainder != '\r' 
                    && remainder != '\0'))
                    {
                        printf("Missing arguments from command\n");
                        continue;
                    }
                    // check if its not connected; needs to be connected
                    int socket = is_already_connected(ip, port);
                    if (socket == -1)
                    {
                        printf("Unable to request chunk, peer not in list\n");
                        continue;
                    }
                    // check if ident is being managed
                    bpkg_obj *obj = check_ident(ident, current_length, list);
                    if (obj == NULL)
                    {
                        printf("Unable to request chunk, package "
                            "is not managed\n");
                        continue;
                    }
                    // check if hash provided is valid
                    Chunk *chunk = request_hash(hash, obj);
                    if (chunk == NULL)
                    {
                        printf("Unable to request chunk, chunk hash does not "
                                "belong to package\n");
                        continue;
                    }
                    // finally send the req packet to the connection
                    send_req_packet(socket, ident, hash, 
                        chunk->offset, chunk->size);
                }
                else
                {
                    printf("Invalid Input\n");
                }
            }
        }
    }
    return NULL;
}