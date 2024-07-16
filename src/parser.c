#include "config/config.h"
#include "net/packet.h"
#include "bytetide/btide.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// function to handle CONNECT command
void handle_connect(char command[], char ip[], int *port, Config *cfg)
{
    // check for leading spaces
    char remainder;
    if (command[7] != ' ' || command[8] == ' ' || command[8] == '\0')
    {
        printf("Invalid Input\n");
        return;
    }
    // Attempt to parse IP and port from the command
    // make sure no trailing spaces
    if (sscanf(command + 8, "%127[^:]:%d%c", ip, port, &remainder) == 3 
        && (remainder == '\n' || remainder == '\r' || remainder == '\0'))
    {
        // printf("C: Attempting to connect to %s on port %d\n", ip, *port);
        // attempted to connect same port as server
        if (*port == cfg->port && strcmp(ip, "127.0.0.1") == 0)
        {
            // printf("C: Error: Attempted to connect to
            //  server port %d\n", *port);
            printf("Unable to connect to request peer\n");
            return;
        }

        int sockfd;
        // check if already connected
        if (is_already_connected(ip, *port) == -1)
        {
            sockfd = connect_to_peer(ip, *port);
        }
        else
        {
            printf("Already connected to peer\n");
            return;
        }

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(*port);
        if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
        {
            printf("Invalid address/Address not supported\n");
            return;
        }

        // connection successfull so save peer to list
        if (sockfd >= 0)
        {
            // wait for ACP packet
            // printf("C: Waiting for ACP packet\n");
            struct btide_packet packet;
            if (read(sockfd, &packet, sizeof(packet)) > 0 
                && packet.msg_code == PKT_MSG_ACP)
            {
                // Send ACK after receiving ACP
                send_ack_packet(sockfd);
                printf("Connection established with peer\n");
                add_peer(sockfd, servaddr, cfg);
            }
            else
            {
                // printf("C: Failed to receive ACP\n");
                close(sockfd);
                sockfd = -1;
                printf("Unable to connect to request peer\n");
            }
        }
        else
        {
            printf("Unable to connect to request peer\n");
        }
    }
    else
    {
        printf("Missing address and port argument\n");
    }
    // debug
    // printf("%s\n", ip);
    // printf("%d\n", port);
    // printf("%c\n", remainder);
}

// function to handle DISCONNECT command
void handle_disconnect(char command[], char ip[], int *port, 
pthread_mutex_t *peer_list_lock, int *peer_count, Peer *peer_list)
{
    char remainder;
    if (command[10] != ' ' || command[11] == ' ' || command[11] == '\0')
    {
        printf("Invalid Input\n");
        return;
    }
    // Attempt to parse IP and port from the command
    // make sure no trailing spaces
    if (sscanf(command + 11, "%127[^:]:%d%c", ip, port, &remainder) == 3 
        && (remainder == '\n' || remainder == '\r' || remainder == '\0'))
    {
        // printf("C: Attempting to disconnect %s on port %d\n", ip, *port);

        int found = 0;
        // critical section, sending DSN packets, removing PEERS
        pthread_mutex_lock(peer_list_lock);
        for (int i = 0; i < *peer_count; i++)
        {
            char connected_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peer_list[i].address.sin_addr, 
                connected_ip, INET_ADDRSTRLEN);

            if (strcmp(connected_ip, ip) == 0 
                && ntohs(peer_list[i].address.sin_port) == *port)
            {
                // send dsn packet
                send_dsn_packet(peer_list[i].socket);
                close(peer_list[i].socket);
                pthread_join(peer_list[i].id, NULL);
                // Swap last peer into the one removed so no holes
                peer_list[i] = peer_list[*peer_count - 1];
                (*peer_count)--;
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(peer_list_lock);

        if (found)
        {
            printf("Disconnected from peer\n");
        }
        else
        {
            printf("Unknown peer, not connected\n");
        }
    }
    else
    {
        printf("Missing address and port argument\n");
    }
}
