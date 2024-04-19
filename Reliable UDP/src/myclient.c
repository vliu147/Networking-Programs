#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 4096
#define BUFSIZE 1200

int main(int argc, char **argv) {

    int port = atoi(argv[2]);
    char *address = argv[1];
    int mtu = atoi(argv[3]);

    int sock, bytes_sent, bytes_received, read_count;
    struct sockaddr_in servaddr;

    if (port < 1024 || port > 65536){
        printf("Invalid port number\n");
        exit(1);
    }

    if (mtu > 32000){
        printf("Invalid MTU");
        exit(1);
    }

    if (argc != 7){
        printf("Needs 6 arguments: (./myclient <server address> <server port> <mtu> <wdwsize> <infile path> <outfile path>)\n");
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  // Create the socket
        perror("Can't create socket");
        exit(1);

         }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET,address, &servaddr.sin_addr) < 0){
        perror("Can't connect to server address\n");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    FILE *fp = fopen(argv[5], "rb");         // open input file
    if (fp == NULL) {
        printf("Can't open file to read\n");
    }

    // Use getenv("HOME")
    //char *file = argv[6];
    char file_path[256];
    char *home_dir = getenv("HOME");
    
    if (argv[6][0] != '/') {
        // printf("It has a directory");
        strncpy(file_path, argv[6], 256);
    } else {
        //printf("No FILE PATH");
        // home directory
        snprintf(file_path, 256, "%s/%s", home_dir, argv[6]);
    }

    FILE *fp2 = fopen(file_path, "wb");                // open outout file
    if (fp2 == NULL) {
        printf("Can't open file to write\n");
        fprintf(stderr, "Error opening file %s: %s\n", file_path, strerror(errno));
        return -1;
    }



    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // treat number as string

    sendto(sock, file_path, strlen(file_path) + 1, 0, (struct sockaddr *) &servaddr, sizeof(servaddr)); // Send outfile name to server
    printf("File Size: %d\n",fileSize);
    char buffer[fileSize]; 
    char SNbuffer[fileSize];   
    int seq_num = 0;
    int ack_num = 0;
    
    time_t current_time;
    struct tm* time_info;
    char time_string[30];

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H:%M:%S.000Z", time_info);

    while ((read_count = fread(buffer, 1, mtu-sizeof(seq_num), fp)) > 0) {     // Send the data to server
        // memcpy(buffer + read_count, *seq_num, sizeof(seq_num));
        // read_count += sizeof(seq_num);
        memcpy(SNbuffer, &seq_num, sizeof(seq_num)); // Add sequence number to each packet
        memcpy(SNbuffer + sizeof(seq_num),buffer, mtu-sizeof(seq_num));
        // memset(SNbuffer + sizeof(seq_num) + read_count, 0, mtu - sizeof(seq_num) - read_count);
        bytes_sent = sendto(sock, SNbuffer, mtu, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); // send packet
        //printf("Bytes sent: %d",bytes_sent);

        if (bytes_sent < 0) {
            perror("Can't send file\n");
            return 1;
        }
       



        bytes_received = recvfrom(sock, buffer, mtu, 0, NULL, NULL); // Receive response from server
        memcpy(&ack_num, buffer, sizeof(ack_num)); // read the ack number from the buffer
        // printf("%s READ %d and SN %d and Sent %d and RCV %d\n",time_string,read_count,seq_num,bytes_sent,bytes_received);
        printf("%s DATA %d\n",time_string,seq_num);
        printf("%s ACK %d\n",time_string,ack_num);
        if (bytes_received < 0) {
            perror("Can't receive response\n");
            return 1;
        }
        seq_num++;
        // fwrite(buffer, 1, bytes_received, fp2);
    }
    //     for (int i = 0; i < WINDOW_SIZE; i++) {
    //     window[i] = -1;
    //         }
    //     for (int i = 0; i < MAX_SEQ_NUM; i++) {
    //     ack_received[i] = false;
    //         }

    // if (seq_num < ack_num + WINDOW_SIZE) {
    //     window[seq_num % WINDOW_SIZE] = seq_num;

    //     if (sendto(client_socket, &packet, sizeof(packet), 0, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
    //         // Handle error
    //     }
    //     seq_num = (seq_num + 1) % MAX_SEQ_NUM;
    // }

    // Slide the window
    // while (ack_received[ack_num]) {
    //     ack_received[ack_num] = false;
    //     window[ack_num % WINDOW_SIZE] = -1;
    //     ack_num = (ack_num + 1) % MAX_SEQ_NUM;

    fclose(fp);
    fclose(fp2);

    close(sock);

    printf("File sent and received\n");
    return 0;
}
//     fseek(fp, 0, SEEK_END);

