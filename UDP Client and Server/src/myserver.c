#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096

int main(int argc, char **argv) {
 
  if(argc != 2){
    printf("Needs one argument: (./myserver <port number>)\n");
    exit(1);
  }

  // Referenced Network Programming Textbook
  int sock, bytes_received,port;
  char buffer[1024];
  port = atoi(argv[1]);

  struct sockaddr_in servaddr, cliaddr;
  bzero(&servaddr, sizeof(servaddr));

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Can't create socket");
        exit(1);
    }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Can't bind to socket");
        exit(1);
    }


    socklen_t cliaddr_len;
    printf("Waiting for data...\n");
    while (1) {
        cliaddr_len = sizeof(cliaddr);
        bytes_received = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) &cliaddr, &cliaddr_len);
        if (bytes_received < 0) {
            printf("Error: Can't receive data\n");
            exit(1);
        }

        // Echo the received data back to the client
        if (sendto(sock, buffer, bytes_received, 0, (struct sockaddr *) &cliaddr, cliaddr_len) < 0) {
            printf("Error: Can't send data\n");
            exit(1);
        }
    }

    close(sock);
   
  return 0;
}
