#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_MSG_SIZE 1024
#define MAX_SERVERS 5
#define BUFSIZE 1200

pthread_mutex_t mutex;

struct tread{
    char* add;
    int port;
    int sock;
    char *rootp;
    char *inputf;
    int wdsize;
    int maxtu;
};

void *client_thread(void *arg) {

    pthread_mutex_lock(&mutex);

    struct tread *args = (struct tread *)arg;
    int sockfd = (args->sock);
    if (sockfd < 0) {
        printf("Error creating socket for server %s\n", args->add);
        return NULL;
    }
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("Error setting socket receive timeout\n");
        return NULL;
    }
     if (args->port < 1024 || args->port > 65536){
        printf("Invalid port number\n");
        return NULL;
    }

    if (args->maxtu > 32000){
        printf("Invalid MTU");
        return NULL;
    }

    // char buf[MAX_MSG_SIZE];
    char rootpath[256];

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(args->add);
    servaddr.sin_port = htons(args->port);

    FILE *fp = fopen(args->inputf, "rb"); // open input file to read
    if (fp == NULL) {
        printf("Error opening input file\n");
        exit(1);
    }

    //printf("CLIENT: ADDR %s and PORT %d and FILE %s\n",args->add,args->port,args->rootp);
    // // Send data to server

    strncpy(rootpath,args->rootp,256);
    if(sendto(sockfd,rootpath,strlen(rootpath),0,(struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        printf("Error can't send to server\n");
        exit(1);
    }
     //sprintf(buf, "thread %ld", pthread_self());
     //int num_bytes = sendto(sockfd, buf, strlen(buf), 0, 
     //                       (struct sockaddr *)&servaddr, sizeof(servaddr));
     //if (num_bytes < 0) {
     //    printf("Error sending data from thread %ld\n", pthread_self());
     //}

    
    fseek(fp, 0, SEEK_END);
    int seq = 0;
    int filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    bool done = false;
    char buffer[1024];
    char total[1024];
    int read_count, expected_ack;

    int num_packets_sent = 0;
    int window_size = args->wdsize;
    int mtu = args->maxtu;
    int total_pack = (filesize / (mtu-4)) + ((filesize % mtu) != 0);
    int bytes_sent;
    int reader = 0;
    int bytes_received,ack;
    int basesn;
    int nextsn;

    time_t current_time;
    struct tm* time_info;
    char time_string[30];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H:%M:%S.000Z", time_info);

    struct sockaddr_in sin;
    socklen_t slen;
    short unsigned int port;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = 0;

    bind(sock, (struct sockaddr *)&sin, sizeof(sin)); 
    /* Now bound, get the address */
    slen = sizeof(sin);
    getsockname(sock, (struct sockaddr *)&sin, &slen);
    port = ntohs(sin.sin_port);

    sprintf(total, "%d",total_pack);
    sendto(sockfd,total,strlen(total),0,(struct sockaddr *)&servaddr, sizeof(servaddr));

    while (num_packets_sent < total_pack && !done) {
        // send up to window_size packets at a time
        for (int i = 0; i < window_size && num_packets_sent < total_pack; i++) {
            // Read data from file
            //printf("window and mtu: %d and %d\n",window_size,mtu);
            nextsn = seq + 1;

            read_count = fread(buffer, 1, mtu - sizeof(int), fp);
            if (read_count == 0) {
                done = true;
                break;
            }
            //printf("I and WNDOW SIZE:  %d and %d    NUM PACKET SENT AND TOTAL PACK  %d and %d\n",i,window_size,num_packets_sent,total_pack);
            reader += read_count;
            //printf("reader %d\n",reader);
            //printf("Buffer: %s\n",buffer);
            // Create packet with sequence number
            char packet[BUFSIZE + sizeof(int)];
            memcpy(packet, &seq, sizeof(int));
            memcpy(packet + sizeof(int), buffer, read_count);

            printf("%s, %d, %s, %d DATA %d, %d, %d, %d\n",time_string,port,args->add,args->port,seq,basesn,nextsn,basesn + args->wdsize);
            // Send packet
            bytes_sent = sendto(sockfd, packet, read_count + sizeof(int), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            //printf("Bytes read and sent: %d and %d\n" ,read_count,bytes_sent);

            seq++;
            num_packets_sent++;

            expected_ack = seq - num_packets_sent;   // expected acknowledgement number

        
            bytes_received = recvfrom(sockfd, &ack, sizeof(int), 0, NULL, NULL); // Receive response from server
            //printf("ACK: %d\n",ack);
            if (bytes_received == -1) {
                perror("Error receiving response from server");
                exit(1);
            }

            if (ack >= expected_ack && ack < seq) {  // if the received acknowledgement number is within range, move on to next packet
                num_packets_sent -= ack - expected_ack + 1;
                expected_ack = ack + 1;
                if (num_packets_sent == 0 && done) {
                    break;
                }
            }
            else {  // otherwise, resend the packet with the lowest sequence number that has not yet been acknowledged
                fseek(fp, (ack - expected_ack + 1) * (mtu - sizeof(int)), SEEK_CUR);
                seq = ack + 1;
                num_packets_sent = total_pack - seq + 1;
                printf("Resending packet with seq: %d\n", seq);
                break;
            }
            printf("%s, %d, %s, %d ACK %d, %d, %d, %d\n",time_string,port,args->add,args->port,ack,basesn,nextsn,basesn + args->wdsize);
                basesn = seq;
        }
            
            //printf("seq and num packs sent: %d and %d\n",seq,num_packets_sent);
    }

    close(sockfd);
    free(args->add);  // Free memory allocated for server address
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    

    if (argc < 6) {
        printf("Usage: %s <num_servers> <server_config_file> <mtu> <window_size> <input_file>\n", argv[0]);
        return 1;
    }

    int num_servers = atoi(argv[1]);
    if (num_servers > MAX_SERVERS) {
        printf("Error: too many servers specified\n");
        return 1;
    }
    char *server_config_file = argv[2];
    //int mtu = atoi(argv[3]);
    //int window_size = atoi(argv[4]);
    //char *input_file = argv[5];
  
    pthread_t threads[num_servers];
    FILE *f = fopen(server_config_file, "r"); // read server.config file
    if (f == NULL) {
        printf("Error opening server configuration file\n");
        return 1;
    }
    char server_addr[100];
    int port = 0;
    struct tread args[num_servers];
    int i = 0;
    while (i < num_servers && fscanf(f, "%s %d", server_addr, &port) != EOF) {
        //printf("Server %s and Port %d\n",server_addr,port);
        // Create socket and thread for this server
        args[i].maxtu = atoi(argv[3]);
        args[i].wdsize = atoi(argv[4]);
        args[i].inputf = argv[5];
        args[i].rootp = argv[6];
        args[i].add = malloc(strlen(server_addr) + 1);
        strcpy(args[i].add, server_addr);
        args[i].port = port;
        args[i].sock = socket(AF_INET, SOCK_DGRAM, 0);
if (args[i].sock < 0) {
    printf("Error creating socket for server %s\n", args[i].add);
    return 1;
}

        pthread_create(&threads[i], NULL,client_thread, &args[i]); // Create thread for client
         i++;
}
fclose(f);

// Wait for all threads to finish
for (i = 0; i < num_servers; i++) {
    pthread_join(threads[i], NULL);
}

return 0;
}
