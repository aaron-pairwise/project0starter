#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


int make_non_blocking(int fd) {
   // Get fd flags:
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    // Set fd flag:
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}


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

   while(true) {
      /* 5. Listen for response from server */
      int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 
                              // socket  store data  how much
                                 0, (struct sockaddr*) &serveraddr, 
                                 &serversize);
      if (bytes_recvd > 0) {
         write(1, server_buf, bytes_recvd);
      }
      // Execution will stop here until `BUF_SIZE` is read or termination/error
      // Error if bytes_recvd < 0 :(
      //  if (bytes_recvd < 0) return errno;
      // Print out data

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
