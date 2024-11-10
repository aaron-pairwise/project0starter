// utils.c
#include "utils.h"
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>


// Makes a file descriptor non-blocking
int make_non_blocking(int fd) {
   // Get fd flags:
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("error getting fd flags");
        return -1;
    }
    // Set fd flag:
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("error setting fd flag");
        return -1;
    }
    return 0;
}

packet create_packet(uint32_t ack, uint32_t seq, uint16_t length, uint8_t flags, uint8_t unused, char* payload) {
   packet pkt = {0};
   pkt.ack = htonl(ack);
   pkt.seq = htonl(seq);
   pkt.length = htons(length);
   pkt.flags = flags;
   pkt.unused = unused;
   memcpy(pkt.payload, payload, length);
   return pkt;
}
int send_packet(int sockfd, packet pkt, struct sockaddr_in addr) {
   print_diag(&pkt, SEND);
   return sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
}
uint32_t get_random_seq() {
   return rand() % (UINT32_MAX / 2 + 1);
}


// Diagnostic messages

void print_diag(packet* pkt, int diag) {
    switch (diag) {
    case RECV:
        fprintf(stderr, "RECV");
        break;
    case SEND:
        fprintf(stderr, "SEND");
        break;
    case RTOS:
        fprintf(stderr, "RTOS");
        break;
    case DUPA:
        fprintf(stderr, "DUPS");
        break;
    }

    bool syn = pkt->flags & 0b01;
    bool ack = pkt->flags & 0b10;
    fprintf(stderr, " %u ACK %u SIZE %hu FLAGS ", ntohl(pkt->seq),
            ntohl(pkt->ack), ntohs(pkt->length));
    if (!syn && !ack) {
        fprintf(stderr, "NONE");
    } else {
        if (syn) {
            fprintf(stderr, "SYN ");
        }
        if (ack) {
            fprintf(stderr, "ACK ");
        }
    }
    fprintf(stderr, "\n");
}