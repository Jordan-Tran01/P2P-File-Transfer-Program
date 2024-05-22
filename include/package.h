#include <stdint.h>
#include <stddef.h>

#define PAYLOAD_MAX 4096
#define DATA_MAX 4092
#define PKT_MSG_ACK 0x0C
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00

union btide_payload {
    uint8_t data[DATA_MAX];
};

typedef struct {
    uint16_t msg_code;
    uint16_t error;
    union btide_payload pl;
} btide_packet;

#define MIN_IDENT 20

void send_packet(int sockfd, const btide_packet* packet);
void receive_packet(int sockfd, btide_packet* packet);
void send_ack(int socket_fd);