#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <utils.h>


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
   int handshake_stage = 0;
   uint32_t SEQ = 0;

   // Network Loop:
   while (true) {
      /* 5. Listen for data from clients */
      packet pkt = {0}; 
      int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*), &server_addr, &s);
      
      /* 5. Initial Handshake */
      if (bytes_recvd > 0 && handshake_stage < 2) {
         if (handshake_stage == 0) {
            /* Expect a SYN packet from client, respond with a similar one. */
            bool syn = pkt.flags & 1;
            bool ack = (pkt.flags >> 1) & 1;
            if (!syn || ack) {
               write(1, "Error: Unexpected handshake 1 flag from client 0\n", 23);
               return 1;
            }
            SEQ = ntohl(pkt.seq);
            packet syn_ack = create_packet(SEQ + 1, get_random_seq(), 0, 0b11000000, 0, "");
            send_packet(sockfd, syn_ack, clientaddr);
            handshake_stage++;
            continue;
         }
         else if (handshake_stage == 1) {
            /* This packet may include payload, so let it thru. */
            handshake_stage++;
         }
      }

      /* 5. Read data from client */
      if (bytes_recvd > 0) {
         uint32_t ack = ntohl(pkt.ack);
         uint32_t seq = ntohl(pkt.seq);
         uint16_t length = ntohs(pkt.length);
         uint8_t flags = pkt.flags;
         uint8_t unused = pkt.unused;
         char* payload = pkt.payload;
         write(1, payload, length);
      }

      /* 7. Send data back to client */
      if (!client_connected) {
         continue;
      }
      char server_buf[BUF_SIZE];
      int bytes_read = read(STDIN_FILENO, server_buf, BUF_SIZE);
      if (bytes_read > 0)
      {
         packet pkt = create_packet(0, 0, bytes_read, 0, 0, server_buf);
         int did_send = send_packet(sockfd, pkt, clientaddr);
         if (did_send < 0)
            return errno;
      }
   }

    /* 8. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}

