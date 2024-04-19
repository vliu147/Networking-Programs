#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096

int main(int argc, char **argv) {
 
  if(argc != 4){
    printf("Needs one argument: (./myserver <port number> <droppc> <root_folderpath>)\n");
    exit(1);
  }

  // Referenced Network Programming Textbook
  int sock, bytes_received,port;
  char buffer[1024];
  port = atoi(argv[1]);
  char outpath[1024];
  //int drop = atoi(argv[2]);


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
    time_t current_time;
    struct tm* time_info;
    char time_string[30];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H:%M:%S.000Z", time_info);

    socklen_t cliaddr_len;
    int expect_seq = 0;
    char file[1024];
    char total_packets[1024];
    bytes_received = recvfrom(sock, file, 200, 0, (struct sockaddr *) &cliaddr, &cliaddr_len); // Receive outfile name
    bytes_received = recvfrom(sock, total_packets, 200, 0, (struct sockaddr *) &cliaddr, &cliaddr_len); // Receive outfile name
    sprintf(outpath, "%s/%s",argv[3],file);
    //printf("OUT PATH %s\n",outpath);

    FILE *fp2 = fopen(outpath, "wb");    
    if (fp2 == NULL) {
        perror("Can't open file");
        exit(1);
    }
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(cliaddr.sin_port);

    printf("Waiting for data...\n");
    while (1) {
    cliaddr_len = sizeof(cliaddr);
    bytes_received = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) &cliaddr, &cliaddr_len);
    if (bytes_received < 0) {
        printf("Error: Can't receive data\n");
        exit(1);
    }
    int seq;
    memcpy(&seq, buffer, sizeof(int));

    // Check if packet is in order
       
    // if (rand() % 100 < drop_percent) { // Drop packet with probability based on drop percentage
    //     printf("Dropped packet with sequence number %d\n", seq);
    //     continue;
    // }

    // Write packet data to file
    if(seq == expect_seq){           
        fwrite(buffer + sizeof(int), 1, bytes_received - sizeof(int), fp2);        
        //printf("%s\n", buffer + sizeof(int));
            
        int ack = expect_seq;
        printf("%s, %d, %s, %d ACK: %d\n",time_string,port,client_ip,client_port,ack);

        sendto(sock, &ack, sizeof(int), 0, (struct sockaddr *)&cliaddr, cliaddr_len);  
        expect_seq++;      
        } else {
            printf("%s, %d, %s, %d DROP ACK: %d\n",time_string,port,client_ip,client_port,expect_seq);
    }

    //sendto(sock, &expect_seq, sizeof(int), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
    fclose(fp2); 
    fp2 = fopen(outpath, "ab"); 
    if (fp2 == NULL) {
        perror("Can't open file");
        exit(1);
    }
}
}
