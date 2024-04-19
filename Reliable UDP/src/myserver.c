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
#define WINDOW_SIZE 10
#define BUFSIZE 1024

int main(int argc, char **argv) {
 
  if(argc != 3){
    printf("Needs one argument: (./myserver <port number> <droppedcount>)\n");
    exit(1);
  }

  //int drop = atoi(argv[2]);
  // Referenced Network Programming Textbook
  int sock, bytes_received,port;
  char buffer[1024];
  port = atoi(argv[1]);

  // drop = random % 100 < percentage

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

  char file[1024];
  socklen_t cliaddr_len;
  cliaddr_len = sizeof(cliaddr);
  //int seq_num;
  char temp[1024];
  int seq_num = 0;
  
  bytes_received = recvfrom(sock, file, 200, 0, (struct sockaddr *) &cliaddr, &cliaddr_len); // Receive outfile name
  //printf("File: %s\n",file);

  FILE *fp = fopen(file,"wb");

    printf("Waiting for data...\n");
    while (1) {
      bytes_received = recvfrom(sock, buffer, 200, 0, (struct sockaddr *)&cliaddr, &cliaddr_len); // Recieve packet from client
       
    memcpy(&seq_num, buffer, sizeof(seq_num)); // read the sequence number from the buffer
    int data_size = bytes_received - sizeof(seq_num); // calculate the size of the data in the buffer
    fwrite(buffer + sizeof(seq_num), 1, data_size, fp); // write the data to the file
    //printf("WRITE: %d and SN: %d and DATA: %s \n",writer,seq_num,buffer + 4);
    memset(buffer, 0, 200); // clear the buffer
        memcpy(temp,&seq_num,sizeof(seq_num));
        // Echo the ACK to the client
        if (sendto(sock, temp, sizeof(seq_num), 0, (struct sockaddr *) &cliaddr, cliaddr_len) < 0) {
            printf("Error: Can't send data\n");
            exit(1);
        }
    }

    // if (rand() % 100 < drop) {
    //     char buf[100];
    //     time_t now = time(NULL);
    //     strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    //    if(packet.type == Data){
    //          printf(stderr,%s DROPPED %s %d, buf,DATA,packet.sequence number)
    // } else {
    //       printf(stderr,%s DROPPED %s %d, buf,DATA,packet.sequence number)
    // }
    

    close(sock);
   
  return 0;
}
