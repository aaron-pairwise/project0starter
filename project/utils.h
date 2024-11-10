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

// utils.h
#ifndef UTILS_H
#define UTILS_H

// Define your structs
#define MSS 1012 // MSS = Maximum Segment Size (aka max length)
#define WINDOW_SIZE 200
// #define FILE_BUF_SIZE 1024
#define FILE_BUF_SIZE 1012 // making it the same as MSS to avoid overflow
#define CLIENT_SEQ 42
#define SERVER_SEQ 5000
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
packet create_packet(uint32_t ack, uint32_t seq, uint16_t length, uint8_t flags, uint8_t unused, char* payload);
int send_packet(int sockfd, packet pkt, struct sockaddr_in addr);
uint32_t get_random_seq();


#endif // UTILS_H
