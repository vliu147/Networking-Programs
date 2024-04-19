#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

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

    if (argc != 6){
        printf("Needs 5 arguments: (./myclient <server address> <server port> <mtu> <infile path> <outfile path>)\n");
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

    FILE *fp = fopen(argv[4], "rb");         // open input file
    if (fp == NULL) {
        printf("Can't open file to read\n");
    }

    // Use getenv("HOME")
    //char *file = argv[5];
    char file_path[256];
    char *home_dir = getenv("HOME");
    
    if (argv[5][0] != '/') {
        // printf("It has a directory");
        strncpy(file_path, argv[5], 256);
    } else {
        //printf("No FILE PATH");
        // home directory
        snprintf(file_path, 256, "%s/%s", home_dir, argv[5]);
    }

    FILE *fp2 = fopen(file_path, "wb");                // open outout file
    if (fp2 == NULL) {
        printf("Can't open file to write\n");
        fprintf(stderr, "Error opening file %s: %s\n", file_path, strerror(errno));
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    //int fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // treat number as string
    char buffer[mtu];
    while ((read_count = fread(buffer, 1, mtu, fp)) > 0) {     // Send the data to server
        // memcpy(buffer + read_count, *seq_num, sizeof(seq_num));
        // read_count += sizeof(seq_num);
        bytes_sent = sendto(sock, buffer, read_count, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        //printf("Bytes sent: %d",bytes_sent);

        if (bytes_sent < 0) {
            perror("Can't send file\n");
            return 1;
        }

        bytes_received = recvfrom(sock, buffer, mtu, 0, NULL, NULL); // Receive response from server
        //printf("Bytes received: %d\n",bytes_received);
        if (bytes_received < 0) {
            perror("Can't receive response\n");
            return 1;
        }

        fwrite(buffer, 1, bytes_received, fp2);
    }

    fclose(fp);
    fclose(fp2);

    close(sock);

    printf("File sent and received\n");
    return 0;
}
//     fseek(fp, 0, SEEK_END);
//     int fileSize = ftell(fp);
//     fseek(fp, 0, SEEK_SET);

//     char buffer[fileSize];
//     int len = fread(buffer, 1, fileSize, fp);
//     fclose(fp);

//     // send the file to the server
//     sendto(sock, buffer, len, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));

//     int n = recvfrom(sock, buffer, mtu, 0, NULL, NULL);
//     fwrite(buffer,1,n,fp2);

//     printf("File sent to server\n");

//     close(sock);
//     return 0;
// }
