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
   int port = atoi(argv[1]);
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   struct sockaddr_in servaddr;
   servaddr.sin_family = AF_INET; // use IPv4
   servaddr.sin_addr.s_addr = INADDR_ANY; // accept all connections
   int PORT = port;
   servaddr.sin_port = htons(PORT); // Big endian
   int did_bind = bind(sockfd, (struct sockaddr*) &servaddr, 
                        sizeof(servaddr));
   if (did_bind < 0) return errno;
   if (make_non_blocking(sockfd) < 0 || make_non_blocking(STDIN_FILENO) < 0) {
      return errno;
   }

   /* 4. Network State */
   uint32_t SEQ = SERVER_SEQ; // sequence number for outgoing packets
   uint32_t ACK = CLIENT_SEQ + 1; // the next packet to be recieved
   packet send_buffer[WINDOW_SIZE]; // not ordered
   int send_buffer_size = 0;
   packet recieve_buffer[WINDOW_SIZE]; // ordered
   int recieve_buffer_size = 0;

   // Network Loop:
   while(true) {
      /* 5. On Recieve */
      packet pkt = {0}; 
      int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*), &server_addr, &s);
      if (bytes_recvd > 0) {
         bool isAck = (pkt.flags >> 1) & 1;
         if (isAck) {
            // Remove all packets from send_buffer that have been acked:
            uint32_t ack = ntohl(pkt.ack);
            for (int i = 0; i < send_buffer_size; i++) {
               if (ntohl(send_buffer[i].seq) <= ack) {
                  send_buffer[i] = send_buffer[send_buffer_size - 1];
                  send_buffer_size--;
               }
            }
         }
         else if (recieve_buffer_size < WINDOW_SIZE) {
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
                 ACK++;
               } else {
                 new_recieve_buffer[new_recieve_buffer_size++] = recieve_buffer[i];
               }
            }
            memcpy(recieve_buffer, new_recieve_buffer, new_recieve_buffer_size * sizeof(packet));
            recieve_buffer_size = new_recieve_buffer_size;
            // Send ack:
            packet ack_pkt = create_packet(ACK, 0, 0, 0b01000000, 0, "");
            send_packet(sockfd, ack_pkt, serveraddr);
         }
      }

      /* 5. On Send */
      if (handshake_stage < 3) {
         // HANDSHAKE STAGE:
         if (handshake_stage == 0) {
            // Send SYN packet:
            SEQ = get_random_seq();
            packet syn_pkt = create_packet(0, SEQ, 0, 0b10000000, 0, "");
            int did_send = send_packet(sockfd, syn_pkt, serveraddr);
            if (did_send < 0)
               return errno;
            SEQ++;
            handshake_stage++;
         }
      }
      else {
         // NORMAL OPERATION:
         if (send_buffer_size >= WINDOW_SIZE) {
            continue;
         }
         // Read from file:
         char server_buf[FILE_BUF_SIZE];
         int bytes_read = read(STDIN_FILENO, server_buf, FILE_BUF_SIZE);
         if (bytes_read > 0)
         {
            // Send packet:
            packet pkt = create_packet(0, SEQ, bytes_read, 0, 0, server_buf);
            int did_send = send_packet(sockfd, pkt, serveraddr);
            if (did_send < 0)
               return errno;
            // Add to send buffer:
            send_buffer[send_buffer_size++] = pkt;
            // Increase SEQ:
            SEQ++;
         }
      }
   }

   /* 8. You're done! Terminate the connection */     
   close(sockfd);
   return 0;
}

