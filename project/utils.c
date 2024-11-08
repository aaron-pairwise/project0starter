// utils.c
#include "utils.h"
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

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

packet read_packet(int sockfd, struct sockaddr_in addr) {
   packet pkt = {0};
   recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, sizeof(addr));
   return pkt;
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
   return sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, sizeof(addr));
}
uint32_t get_random_seq() {
   return rand();
}
