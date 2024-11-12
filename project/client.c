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
#include <time.h>

int main(int argc, char *argv[]) {
   /* Setup Stuff */
   char* hostname = argv[1];
   int port = atoi(argv[2]);
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   struct sockaddr_in servaddr;
   socklen_t s = sizeof(struct sockaddr_in);
   servaddr.sin_family = AF_INET; // use IPv4
   if (strcmp(hostname, "localhost") == 0)
      servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
   else
      servaddr.sin_addr.s_addr = inet_addr(hostname);
   int SEND_PORT = port;
   servaddr.sin_port = htons(SEND_PORT); // Big endian
   if (make_non_blocking(sockfd) < 0 || make_non_blocking(STDIN_FILENO) < 0) {
        return errno;
   }

   /* 4. Network State */
   uint32_t SEQ = CLIENT_SEQ; // sequence number for outgoing packets
   uint32_t ACK = SERVER_SEQ; // the next packet to be recieved
   packet send_buffer[WINDOW_SIZE]; // not ordered
   int send_buffer_size = 0;
   packet recieve_buffer[WINDOW_SIZE]; // ordered
   int recieve_buffer_size = 0;
   int handshake_stage = 0;
   // 0 = should send packet with SEQ
   // 1 = wait for ACK packet from server
   // 2 = should send ACK packet with SEQ
   // 3 = normal operation

   int dup_acks = 0;
   uint32_t last_ack = 0;
   time_t start_time, current_time;
   time(&start_time);

   while(true) {
      // HANDSHAKE STAGE:
      if (handshake_stage == 0) {
         handshake_stage++;
         // Send SYN packet:
         SEQ = get_random_seq();
         packet syn_pkt = create_packet(0, SEQ, 0, 0b01, 0, "");
         int did_send = send_packet(sockfd, syn_pkt, servaddr);
         if (did_send < 0)
            return errno;
         SEQ++;
      }
      if (handshake_stage == 1) {
         // Recieve SYN-ACK packet:
         packet pkt = {0};
         int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*) &servaddr, &s);
         if (bytes_recvd > 0) {
            print_diag(&pkt, RECV);
            bool isAck = pkt.flags & 0b10;
            bool isSyn = pkt.flags & 0b01;
            uint32_t serverSeq = ntohl(pkt.seq);
            if (isAck && isSyn) {
               // Update ACK:
               ACK = serverSeq + 1;
               // Send ACK packet:
               packet ack_pkt = create_packet(ACK, SEQ, 0, 0b10, 0, "");
               int did_send = send_packet(sockfd, ack_pkt, servaddr);
               if (did_send < 0)
                  return errno;
               handshake_stage++;
               SEQ++;
            }
         }
      }
      if (handshake_stage < 2) continue;
      /* 5. On Recieve */
      packet pkt = {0}; 
      int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*) &servaddr, &s);
      if (bytes_recvd > 0) {
         print_diag(&pkt, RECV);
         bool isAck = pkt.flags & 0b10;
         if (isAck) {
            // Reset timer:
            time(&start_time);
            uint32_t ack = ntohl(pkt.ack);
            int new_size = 0;

            for (int i = 0; i < send_buffer_size; i++) {
               if (ntohl(send_buffer[i].seq) >= ack) {
                  send_buffer[new_size++] = send_buffer[i];  // Retain the packet
               }
            }
            fprintf(stderr, "Shrinking send_buffer from %d to %d\n", send_buffer_size, new_size);

            send_buffer_size = new_size;  // Update buffer size after removal
            // Check for duplicate acks:
            if (ack == last_ack) {
               dup_acks++;
            } else {
               last_ack = ack;
               dup_acks = 0;
            }
         }
         if (ntohl(pkt.length) > 0 && recieve_buffer_size < WINDOW_SIZE) {
            // Check if packet is too old:
            if (ntohl(pkt.seq) < ACK) {
               continue;
            }
            // Check for duplicate packets:
            bool exists = false;
            for (int i = 0; i < recieve_buffer_size; i++) {
               if (ntohl(recieve_buffer[i].seq) == ntohl(pkt.seq)) {
                 exists = true;
                 break;
               }
            }
            if (!exists) {
               // Bubble insert packet into recieve buffer:
               recieve_buffer[recieve_buffer_size] = pkt;
               for (int i = recieve_buffer_size; i > 0 && ntohl(recieve_buffer[i].seq) < ntohl(recieve_buffer[i - 1].seq); i--) {
                 packet temp = recieve_buffer[i];
                 recieve_buffer[i] = recieve_buffer[i - 1];
                 recieve_buffer[i - 1] = temp;
               }
               recieve_buffer_size++;
               // Attempt flush using temp buffer:
               packet new_recieve_buffer[WINDOW_SIZE]; // store packets that are NOT flushed
               int new_recieve_buffer_size = 0;
               for (int i = 0; i < recieve_buffer_size; i++) {
                  if (ntohl(recieve_buffer[i].seq) == ACK) {
                     write(1, recieve_buffer[i].payload, ntohs(recieve_buffer[i].length));
                     fprintf(stderr, "Packet written with SEQ %u\n", ntohl(recieve_buffer[i].seq));
                     ACK += ntohs(recieve_buffer[i].length);
                  } else {
                     new_recieve_buffer[new_recieve_buffer_size++] = recieve_buffer[i];
                  }
               }
               memset(recieve_buffer, 0, WINDOW_SIZE * sizeof(packet));
               memcpy(recieve_buffer, new_recieve_buffer, new_recieve_buffer_size * sizeof(packet));
               recieve_buffer_size = new_recieve_buffer_size;
            }
            // Send ack:
            packet ack_pkt = create_packet(ACK, 0, 0, 0b10, 0, "");
            fprintf(stderr, "Sending ack packet\n");
            // print isAck:
            send_packet(sockfd, ack_pkt, servaddr);
            fprintf(stderr, "rec_buffer: %u, send_buffer: %u\n", recieve_buffer_size, send_buffer_size);
         }
      }

      /* 5. On Send */
      // Resend packets:
      time(&current_time);
      bool overtime = difftime(current_time, start_time) >= 1.0;
      if (dup_acks >= 3 || overtime) {
         // Resend first packet in send buffer (if any):
         if (send_buffer_size > 0) {
            packet pkt = send_buffer[0];
            int did_send = send_packet(sockfd, pkt, servaddr);
            if (did_send < 0)
               return errno;
         }
         // Reset time:
         if (overtime) {
            time(&start_time);
         }
      }

      // Read from file:
      if (send_buffer_size >= WINDOW_SIZE) {
         continue;
      }
      char server_buf[FILE_BUF_SIZE];
      int bytes_read = read(STDIN_FILENO, server_buf, FILE_BUF_SIZE);
      if (bytes_read > 0)
      {
         // Send packet:
         packet pkt = create_packet(0, SEQ, bytes_read, 0, 0, server_buf);
         int did_send = send_packet(sockfd, pkt, servaddr);
         if (did_send < 0)
            return errno;
         // Add to send buffer:
         send_buffer[send_buffer_size++] = pkt;
         // Increase SEQ:
         SEQ += bytes_read;
      }
   }

   /* 6. You're done! Terminate the connection */     
   close(sockfd);
   return 0;
}
