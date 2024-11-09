// utils.h
#ifndef UTILS_H
#define UTILS_H

// Define your structs
#define MSS 1012 // MSS = Maximum Segment Size (aka max length)
#define WINDOW_SIZE 20
#define FILE_BUF_SIZE 1024
typedef struct {
	uint32_t ack;
	uint32_t seq;
	uint16_t length;
	uint8_t flags;
	uint8_t unused;
	uint8_t payload[MSS];
} packet;

// Declare utility functions
int make_non_blocking(int fd);
packet read_packet(int sockfd, struct sockaddr_in addr)
packet create_packet(uint32_t ack, uint32_t seq, uint16_t length, uint8_t flags, uint8_t unused, char* payload)
int send_packet(int sockfd, packet pkt, struct sockaddr_in addr)
uint32_t get_random_seq()


#endif // UTILS_H
