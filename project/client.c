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
   char* hostname = argv[1];
   int port = atoi(argv[2]);

   /* 1. Create socket */
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                  // use IPv4  use UDP

   /* 2. Construct server address */
   struct sockaddr_in serveraddr;
   serveraddr.sin_family = AF_INET; // use IPv4
   //  serveraddr.sin_addr.s_addr = INADDR_ANY;
   // Check for localhost
   if (strcmp(hostname, "localhost") == 0)
      serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
   else
      serveraddr.sin_addr.s_addr = inet_addr(hostname);

    // Set sending port
    int SEND_PORT = port;
    serveraddr.sin_port = htons(SEND_PORT); // Big endian

   // Make socket and input non-blocking:
    if (make_non_blocking(sockfd) < 0 || make_non_blocking(STDIN_FILENO) < 0) {
        return errno;
    }

   /* 4. Create buffer to store incoming data */
   int BUF_SIZE = 1024;
   char server_buf[BUF_SIZE];
   socklen_t serversize = sizeof(socklen_t); // Temp buffer for recvfrom API
   int handshake_stage = 0;
   uint32_t SEQ = 0;

   while(true) {
      /* 5. Listen for response from server */
      // int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 
      //                         // socket  store data  how much
      //                            0, (struct sockaddr*) &serveraddr, 
      //                            &serversize);
      // if (bytes_recvd > 0) {
      //    write(1, server_buf, bytes_recvd);
      // }
      packet pkt = {0}; 
      int bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*), &server_addr, &s);

      /* 5. Initial Handshake */
      if (handshake_stage < 2) {
         if (handshake_stage == 0) {
            /* Send a SYN packet from client */
            packet syn_pkt = create_packet(0, get_random_seq(), 0, 0b10000000, 0, "");
            send_packet(sockfd, syn_pkt, serveraddr);
            handshake_stage++;
         }
         if (handshake_stage == 1) {
            /* Expect a SYN-ACK packet from server */
            packet pkt = read_packet(sockfd, serveraddr);
            bool syn = pkt.flags & 1;
            bool ack = (pkt.flags >> 1) & 1;
            if (!syn || !ack) {
               write(1, "Error: Expected syn and ack flags 1\n", 36);
               return 1;
            }
            handshake_stage++;
         }
         if (handshake_stage == 2) {
            /* Send an ACK packet from client */
            packet ack_pkt = create_packet(0, get_random_seq(), 0, 0b01000000, 0, "");
            send_packet(sockfd, ack_pkt, serveraddr);
            handshake_stage++;
         }
      }

      /* 5. Send data to server */
      int bytes_read = read(STDIN_FILENO, server_buf, BUF_SIZE);
      if (bytes_read > 0)
      {
         int did_send = sendto(sockfd, server_buf, bytes_read,
                               // socket  send data   how much to send
                               0, (struct sockaddr *)&serveraddr,
                               // flags   where to send
                               sizeof(serveraddr));
         if (did_send < 0)
            return errno;
      }
   }

    /* 6. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}
