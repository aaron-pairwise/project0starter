#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MSS 1012 // MSS = Maximum Segment Size (aka max length)
typedef struct {
	uint32_t ack;
	uint32_t seq;
	uint16_t length;
	uint8_t flags;
	uint8_t unused;
	uint8_t payload[MSS];
} packet;


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

int main(int argc, char *argv[]) {
   int port = atoi(argv[1]);

    /* 1. Create socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                     // use IPv4  use UDP

    /* 2. Construct our address */
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; // use IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // accept all connections
                            // same as inet_addr("0.0.0.0") 
                                     // "Address string to network bytes"
    // Set receiving port
    int PORT = port;
    servaddr.sin_port = htons(PORT); // Big endian

    /* 3. Let operating system know about our config */
    int did_bind = bind(sockfd, (struct sockaddr*) &servaddr, 
                        sizeof(servaddr));
    // Error if did_bind < 0 :(
    if (did_bind < 0) return errno;

    // Make socket and input non-blocking:
    if (make_non_blocking(sockfd) < 0 || make_non_blocking(STDIN_FILENO) < 0) {
        return errno;
    }


   /* 4. Create buffer to store incoming data */
   int BUF_SIZE = 1024;
   char client_buf[BUF_SIZE];
   struct sockaddr_in clientaddr; // Same information, but about client
   socklen_t clientsize = sizeof(clientaddr);
   bool client_connected = false;

   // Network Loop:
   while (true) {
      /* 5. Listen for data from clients */
      packet pkt = {0}; 
      // int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*), &server_addr, &s);
      int bytes_recvd = recvfrom(sockfd, client_buf, BUF_SIZE, 
                        // socket  store data  how much
                           0, (struct sockaddr*) &clientaddr, 
                           &clientsize);

      /* 5. If first time, setup client */
      if (bytes_recvd > 0 && !client_connected) {
         char* client_ip = inet_ntoa(clientaddr.sin_addr);
         int client_port = ntohs(clientaddr.sin_port); // Little endian
         client_connected = true;
      }

      /* 5. Read data from client */
      if (bytes_recvd > 0) {
         // uint32_t ack = ntohl(pkt.ack);
         // uint32_t seq = ntohl(pkt.seq);
         // uint16_t length = ntohs(pkt.length);
         // uint8_t flags = pkt.flags;
         // uint8_t unused = pkt.unused;
         // char* payload = pkt.payload;
         write(1, client_buf, bytes_recvd);
      }

      /* 7. Send data back to client */
      if (!client_connected) {
         continue;
      }
      char server_buf[BUF_SIZE];
      int bytes_read = read(STDIN_FILENO, server_buf, BUF_SIZE);
      if (bytes_read > 0)
      {
         int did_send = sendto(sockfd, server_buf, bytes_read,
                               // socket  send data   how much to send
                               0, (struct sockaddr *)&clientaddr,
                               // flags   where to send
                               sizeof(clientaddr));
         if (did_send < 0)
            return errno;
      }
   }

    /* 8. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}