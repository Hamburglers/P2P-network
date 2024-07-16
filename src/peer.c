#include <stdio.h>
#include <pthread.h>
#include "config/config.h"
#include "net/packet.h"
#include <string.h>
#include <unistd.h>

#define MAX_IDENTIFIER_LENGTH 1024
#define MAX_HASH_LENGTH 64
#define MAX_DATA 2998

// sends a DSN packet to the socket
void send_dsn_packet(int socket)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_DSN;

    if (write(socket, &packet, sizeof(packet)) < 0)
    {
        perror("Sending DSN failed");
    }
}

// in response to png packet
void send_pog_packet(int socket)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_POG;

    if (write(socket, &packet, sizeof(packet)) < 0)
    {
        perror("Sending POG failed");
    }
}

// sends a ACP packet to the socket
void send_acp_packet(int socket)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_ACP;
    if (write(socket, &packet, sizeof(packet)) < 0)
    {
        perror("Sending ACP failed");
    }
}

// sends a ACK packet to the socket
void send_ack_packet(int socket)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_ACK;
    if (write(socket, &packet, sizeof(packet)) < 0)
    {
        perror("Sending ACK failed");
    }
}

// sends a PNG packet to the socket
void send_png_packet(int socket)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_PNG;
    if (write(socket, &packet, sizeof(packet)) < 0)
    {
        perror("Sending PNG failed");
    }
}

// sends a REQ packet to the socket
void send_req_packet(int socket, const char *identifier, const char *chunk_hash,
 uint32_t offset, uint32_t size)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_REQ;

    int pos = 0;
    memcpy(packet.pl.data + pos, &offset, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(packet.pl.data + pos, &size, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(packet.pl.data + pos, chunk_hash, strlen(chunk_hash));
    pos += MAX_HASH_LENGTH;
    memcpy(packet.pl.data + pos, identifier, strlen(identifier));
    pos += MAX_IDENTIFIER_LENGTH;
    if (write(socket, &packet, sizeof(packet.msg_code) 
        + sizeof(packet.error) + pos) < 0)
    {
        perror("Sending REQ failed");
    }
}

// for testing with pktval
void write_packet_to_file(const char *filename, const void *packet, 
size_t packet_size)
{
    FILE *file = fopen(filename, "wb");
    if (file)
    {
        fwrite(packet, 1, packet_size, file);
        fclose(file);
    }
    else
    {
        perror("Failed to open file for packet writing");
    }
}

// sends a RES packet to the socket
void send_res_packet(int socket, const char *identifier, const char *chunk_hash,
 uint32_t offset, const char *buffer, uint16_t read, uint16_t error)
{
    struct btide_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.msg_code = PKT_MSG_RES;
    packet.error = error;
    if (error)
    {
        if (write(socket, &packet, sizeof(packet)) < 0)
        {
            perror("Sending REQ failed");
        }
    }
    else
    {
        int pos = 0;
        memcpy(packet.pl.data + pos, &offset, sizeof(uint32_t));
        pos += sizeof(uint32_t);
        memcpy(packet.pl.data + pos, buffer, read);
        pos += MAX_DATA;
        memcpy(packet.pl.data + pos, &read, sizeof(uint16_t));
        pos += sizeof(uint16_t);
        memcpy(packet.pl.data + pos, chunk_hash, MAX_HASH_LENGTH);
        pos += MAX_HASH_LENGTH;
        memcpy(packet.pl.data + pos, identifier, MAX_IDENTIFIER_LENGTH);
        pos += MAX_IDENTIFIER_LENGTH;

        // write_packet_to_file("test.pkt", &packet, sizeof(packet.msg_code) 
        // + sizeof(packet.error) + pos);

        if (write(socket, &packet, sizeof(packet.msg_code)
             + sizeof(packet.error) + pos) < 0)
        {
            perror("Sending REQ failed");
        }
    }
}

// function to handle logic of connecting peers
void handle_peers(pthread_mutex_t peer_list_lock, 
int *peer_count, Peer *peer_list)
{
    pthread_mutex_lock(&peer_list_lock);
    if (*peer_count == 0)
    {
        printf("Not connected to any peers\n");
    }
    else
    {
        printf("Connected to:\n\n");
        for (int i = 0; i < *peer_count; i++)
        {
            printf("%d. %s:%d\n", i + 1,
                   inet_ntoa(peer_list[i].address.sin_addr),
                   ntohs(peer_list[i].address.sin_port));

            // Send a ping packet to each peer
            send_png_packet(peer_list[i].socket);
        }
    }
    pthread_mutex_unlock(&peer_list_lock);
}

// code from resources section with minor modifications -> client.c
int connect_to_peer(const char *ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;

    if (sockfd == -1)
    {
        perror("socket creation failed");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
    {
        // printf("G: Invalid address/Address not supported\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("Connect failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}