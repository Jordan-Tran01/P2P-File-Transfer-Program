#include <sys/socket.h>
#include <package.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

void serialize_packet(const btide_packet *packet, uint8_t *buffer) {
    memcpy(buffer, &packet->msg_code, sizeof(packet->msg_code));
    memcpy(buffer + sizeof(packet->msg_code), &packet->error, sizeof(packet->error));
    memcpy(buffer + sizeof(packet->msg_code) + sizeof(packet->error), &packet->pl.data, DATA_MAX);
}

void deserialize_packet(const uint8_t *buffer, btide_packet *packet) {
    memcpy(&packet->msg_code, buffer, sizeof(packet->msg_code));
    memcpy(&packet->error, buffer + sizeof(packet->msg_code), sizeof(packet->error));
    memcpy(&packet->pl.data, buffer + sizeof(packet->msg_code) + sizeof(packet->error), DATA_MAX);
}

void send_packet(int sockfd, const btide_packet* packet) {
    uint8_t buffer[PAYLOAD_MAX];
    serialize_packet(packet, buffer);
    send(sockfd, buffer, PAYLOAD_MAX, 0);
}

// void receive_packet(int sockfd, btide_packet* packet) {
//     uint8_t buffer[PAYLOAD_MAX + 4];
//     recv(sockfd, buffer, PAYLOAD_MAX + 4, 0);
//     deserialize_packet(buffer, packet);
// }

void receive_packet(int sockfd, btide_packet* packet) {
    uint8_t buffer[PAYLOAD_MAX];
    int bytes_received = recv(sockfd, buffer, PAYLOAD_MAX, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Peer has closed the connection\n");
        } else {
            perror("Receive failed");
        }
        return;
    }

    deserialize_packet(buffer, packet);
}

void send_ack(int socket_fd) {
    btide_packet ack_packet;
    ack_packet.msg_code = PKT_MSG_ACK;
    ack_packet.error = 0;  // No error
    memset(ack_packet.pl.data, 0, PAYLOAD_MAX);  // Clear payload

    // Assuming you have a function like send_packet to handle packet sending
    send_packet(socket_fd, &ack_packet);
}