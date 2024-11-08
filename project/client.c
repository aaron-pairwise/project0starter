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
   /* Setup Stuff */
   char* hostname = argv[1];
   int port = atoi(argv[2]);
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   struct sockaddr_in serveraddr;
   serveraddr.sin_family = AF_INET; // use IPv4
   if (strcmp(hostname, "localhost") == 0)
      serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
   else
      serveraddr.sin_addr.s_addr = inet_addr(hostname);
   int SEND_PORT = port;
   serveraddr.sin_port = htons(SEND_PORT); // Big endian
   if (make_non_blocking(sockfd) < 0 || make_non_blocking(STDIN_FILENO) < 0) {
        return errno;
   }

   /* 4. Network State */
   int BUF_SIZE = 1024;
   socklen_t serversize = sizeof(socklen_t); // Temp buffer for recvfrom API
   int handshake_stage = 0;
   uint32_t SEQ = 0;


   while(true) {
      /* 5. Listen for response from server */
      packet pkt = {0}; 
      int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*), &server_addr, &s);

      /* 5. Initial Handshake */
      if (handshake_stage < 2) {
         if (handshake_stage == 0) {
            /* Send a SYN packet from client */
            SEQ = get_random_seq();
            packet syn_pkt = create_packet(0, SEQ, 0, 0b10000000, 0, "");
            send_packet(sockfd, syn_pkt, serveraddr);
            handshake_stage++;
         }
         if (handshake_stage == 1) {
            /* Expect a SYN-ACK packet from server */
            if (bytes_recvd <= 0) {
               continue;
            }
            bool syn = pkt.flags & 1;
            bool ack = (pkt.flags >> 1) & 1;
            if (!syn || !ack) {
               write(1, "Error: Expected syn and ack flags 1\n", 36);
               return 1;
            }
            if (pkt.ack != SEQ + 1) {
               write(1, "Error: Expected Ack for SEQ + 1\n", 24);
               return 1;
            }
            SEQ = ntohl(pkt.ack);
            handshake_stage++;
         }
         if (handshake_stage == 2) {
            /* Send an ACK packet from client */
            packet ack_pkt = create_packet(SEQ, SEQ, 0, 0b01000000, 0, "");
            send_packet(sockfd, ack_pkt, serveraddr);
            handshake_stage++;
         }
      }

      /* 5. Read data from server */
      if (bytes_recvd > 0) {
         uint32_t ack = ntohl(pkt.ack);
         uint32_t seq = ntohl(pkt.seq);
         uint16_t length = ntohs(pkt.length);
         uint8_t flags = pkt.flags;
         uint8_t unused = pkt.unused;
         write(1, payload, length);
      }

      /* 5. Send data to server */
      char server_buf[BUF_SIZE];
      int bytes_read = read(STDIN_FILENO, server_buf, BUF_SIZE);
      if (bytes_read > 0)
      {
         packet pkt = create_packet(0, 0, bytes_read, 0, 0, server_buf);
         int did_send = send_packet(sockfd, pkt, serveraddr);
         if (did_send < 0)
            return errno;
      }
   }

    /* 6. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}
