#ifndef NETPKT_H
#define NETPKT_H

#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PAYLOAD_MAX (4092)

#define PKT_MSG_ACK 0x0c
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00

typedef struct
{
    int socket;
    pthread_t id;
    struct sockaddr_in address;
} Peer;

union btide_payload
{
    uint8_t data[PAYLOAD_MAX];
};

struct btide_packet
{
    uint16_t msg_code;
    uint16_t error;
    union btide_payload pl;
};

#endif
