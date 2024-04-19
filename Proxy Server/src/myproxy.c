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
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <netdb.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 4096
#define BUFSIZE 1200

pthread_mutex_t mutex;

struct thread_arg {
    int client_sock;
    struct sockaddr_in client_addr;
    char* forb_file;
    char* log_file;
};

// void handle_sigint(int sig) {

// }


void *client_handler(void *arg) {
    
    pthread_mutex_lock(&mutex);
    time_t current_time;
    struct tm* time_info;
    char time_string[30];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H:%M:%S.000Z", time_info);



    
    struct thread_arg *targ = (struct thread_arg *)arg;
    int client_sock = targ->client_sock;
    struct sockaddr_in client_addr = targ->client_addr;
     char*forb1 = targ->forb_file;
    char *log1 = targ->log_file;


    // create SSL context
    const SSL_METHOD* meth = SSLv23_client_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);
    if (!ctx) {
        perror("Error creating SSL context");
    }

    // create an SSL connection
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        perror("Error Creating SSL connection");
    }

    //printf("Proxy server started on port %d\n", listport);

    // open forbiddenlist
    FILE *forb = fopen(forb1,"r");
    if (forb == NULL) {
        printf("Can't open file to read\n");
    }

    FILE *log = fopen(log1,"a");
    if (log == NULL) {
        printf("Can't open file to write\n");
    }

        
        char buf[4096];
        char request[4096];
        memset(buf, 0, 4096);
        int n;

        // Read the incoming request from client
        n = recv(client_sock, buf, 4095, 0);

        // split up HTTP request
        strcpy(request,buf);
        printf("Buff: \n%s",buf);
        char* method = strtok(buf, " ");
        char* url = strtok(NULL, " ");
        char* http_version = strtok(NULL, "\r\n");
        char* host = "";
        char* token;
        char *ip = inet_ntoa(client_addr.sin_addr);
        int port = (int)ntohs(client_addr.sin_port);
        char line[4096];
        char clientip[1024];
        strcpy(clientip,ip);

        // split by line to get hostname
        token = strtok(NULL, "\r\n");
        while (token != NULL) {
            if (strncmp(token, "Host:", 5) == 0) {
            host = token + 6; // skip "Host: "
            break;
            }
            token = strtok(NULL, "\r\n");
        }
        
        printf("Method: %s\n", method);
        printf("URL: %s\n", url);
        printf("HTTP version: %s\n", http_version);

        printf("Host: %s\n", host);
        printf("IP address is: %s\n", ip);
        printf("port is: %d\n", port);

        
        char forbidden_sites[1000][MAX_LINE];
        int num_forbidden_sites = 0;

        // parse through forbiddenlist file
        while (fgets(line, MAX_LINE, forb) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            strcpy(forbidden_sites[num_forbidden_sites], line);
            num_forbidden_sites++;
            if(num_forbidden_sites > 1000){
                printf("Too many forbidden sites");
                exit(-1);
            }
        }

        // Check if the last line was stored completely
        if (num_forbidden_sites > 0 && strlen(forbidden_sites[num_forbidden_sites-1]) == MAX_LINE-1) {
            char remaining[MAX_LINE];
            fgets(remaining, MAX_LINE, forb);
            remaining[strcspn(remaining, "\r\n")] = '\0';
            strcpy(forbidden_sites[num_forbidden_sites+1], remaining);
            num_forbidden_sites++;
            if(num_forbidden_sites > 1000){
                printf("Too many forbidden sites\n");
                exit(-1);
            }
        }

        //printf("Host: %s\n", host);

        int error;
        //char* hoster = "www.google.com";
        struct addrinfo hints_1, *res_1;
        memset(&hints_1, '\0', sizeof(hints_1));
        error = getaddrinfo(host, "80", &hints_1, &res_1);
        //printf ("\nError: %i\n", error);

        // create socket for server
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Can't create socket");
            exit(1);
        }

        char firstline[1024];
        struct hostent *server = gethostbyname(host);
        if (server == NULL) {
        // Unable to resolve domain name
        send(client_sock, "HTTP/1.1 502 Bad Gateway\r\n\r\n", strlen("HTTP/1.1 502 Bad Gateway\r\n\r\n"), 0);
        sprintf(firstline, "%s %s \"%s %s %s\" 502 %d\r\n", time_string,clientip, method,url, http_version,n);
        close(client_sock);
        exit(-1);
        }

        char *dest_ip = inet_ntoa(*((struct in_addr *) server->h_addr));
        //printf("Dest IP: %s\n",dest_ip);

         // Connect to the server using hostname and port number
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(dest_ip);
        server_address.sin_port = htons(443);
        

        if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
            perror("Error connecting to destination server");
            send(client_sock, "HTTP/1.1 504 Gateway Timeout\r\n\r\n", strlen("HTTP/1.1 504 Gateway Timeout\r\n\r\n"), 0);
            sprintf(firstline, "%s %s \"%s %s %s\" 504 %d\r\n", time_string,clientip, method,url, http_version,n);
            close(client_sock);
            exit(-1);
        }

        // Initialize the SSL connection
        SSL_set_fd(ssl, sockfd);
        if (SSL_connect(ssl) < 0) {
            perror("Error establishing SSL connection");
            exit(-1);
        }

        //printf("\n%s",request);
        // Check if the host is in the forbidden sites array
        bool check = true;
        for (int i = 0; i < num_forbidden_sites; i++) {
            if ((strcmp(forbidden_sites[i], host) == 0) || (strcmp(forbidden_sites[i], dest_ip) == 0)) {
                printf("This is forbidden: %s\n", host);
                check = false;
                break;
            }
        }

        // sprintf(firstline, "%s %s \"%s %s %s\" 403 %d", time_string,clientip, method,url, http_version,n);
        // printf("%s\n",firstline);
        // fprintf(log,"%s",firstline);
        // fclose(forb);
        // fclose(log);

        //     struct timeval timeout;
        // timeout.tv_sec = 10;  // 10 second timeout
        // timeout.tv_usec = 0;
        // if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        //     printf("Error setting socket receive timeout\n");
        //     return NULL;
        // }

        if(check == false){
            char response[1024];
            strcpy(response,"HTTP/1.1 403 Forbidden\r\n\r\nHTTP/1.1 403 Forbidden\r\n");
            response[strlen(response)] = '\0';
            send(client_sock, response, strlen(response), 0);
            sprintf(firstline, "%s %s \"%s %s %s\" 403 %d\r\n", time_string,clientip, method,url, http_version,n);
            //printf("%s",firstline);
            fprintf(log,"%s",firstline);
        } else if((strcmp(method,"GET") != 0) && (strcmp(method,"HEAD") != 0)){
            char response[1024];
            strcpy(response,"HTTP/1.1 501 Not Implemented\r\n\r\nHTTP/1.1 501 Not Implemented\r\n");
            response[strlen(response)] = '\0';
            send(client_sock, response, strlen(response), 0);
            sprintf(firstline, "%s %s \"%s %s %s\" 501 %d\r\n", time_string,clientip, method,url, http_version,n);
            //printf("%s",firstline);
            fprintf(log,"%s",firstline);

        } else {

        char http_request[4096];
        memset(http_request, 0, sizeof(http_request));
        sprintf(http_request, "%s %s %s\r\nHost: %s\r\n\r\n", method, url, http_version, host);

        // Send the request to the destination server
        SSL_write(ssl, request, strlen(request));

        // Receive the response from the destination server
        char response_buf[1000000];
        int bytes_received = 0;
        int total_bytes_received = 0;

        while ((bytes_received = SSL_read(ssl, response_buf + total_bytes_received, 1000000 - 1 - total_bytes_received)) > 0 ) {
            total_bytes_received += bytes_received;
        }

        if (bytes_received < 0) {
            perror("Error receiving response from destination server");
            exit(-1);
        }

        response_buf[total_bytes_received] = '\0';

        
        printf("Received response from destination server:\n%s\n", response_buf);
        // Forward the response to the client
        send(client_sock, response_buf, strlen(response_buf), 0);
    
        sprintf(firstline, "%s %s \"%s %s %s\" 200 %d\r\n", time_string,clientip, method,url, http_version,total_bytes_received );
        //printf("%s",firstline);
        fprintf(log,"%s",firstline);
  
        
        }
         fclose(forb);
        fclose(log);
        close(sockfd);
        close(client_sock);
        SSL_shutdown(ssl);    // shut down the SSL connection
        SSL_free(ssl);        // free the SSL object
        SSL_CTX_free(ctx);    // free the SSL context
        rewind(forb); // Move file pointer to beginning of fil
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);

}

int main(int argc, char **argv) {
    // ./myproxy listen_port forbidden_sites_file_path access_log_file_path
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // signal(SIGINT, handle_sigint);
    // for(int i = 0; i < argc;i++){
    //     printf("Hi %d\n",i);
    // }
    if ((argc < 4) || (argc > 4)) {
        printf("Usage: %s listen_port forbidden_sites_file_path access_log_file_path", argv[0]);
        return 1;
    }

    int sock,listport;  
    listport = atoi(argv[1]);

    // Create a listening socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Can't create socket");
            exit(1);
        }
    // Set socket options to reuse the address and port
    // int optval = 1;
    // if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
    //     error("Error setting socket options");
    // }

    // Bind the socket to a local address and port
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listport);
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("Can't bind to socket");
            exit(1);
        }

     // Start listening for incoming connections
    if (listen(sock, 10) == -1) {
        perror("Error listening on socket");
    }

     const char *f = argv[2];
    if (access(f, F_OK) == -1) {
        // Path doesn't exist, create it
        if (mkdir(f, 0777) == -1) {
            perror("mkdir");
            exit(1);
        }
    }
    const char *l = argv[3];
    if (access(l, F_OK) == -1) {
        // Path doesn't exist, create it
        if (mkdir(l, 0777) == -1) {
            perror("mkdir");
            exit(1);
        }
    }
    while (1) {
        
        // Accept incoming connections
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("Error accepting connection");
            exit(-1);
        }
        pthread_t tid;
        struct thread_arg targ;
        targ.client_sock = client_sock;
        targ.client_addr = client_addr;
        targ.forb_file = argv[2];
        targ.log_file = argv[3];
        
        if (pthread_create(&tid, NULL, client_handler, &targ) != 0) {
            perror("Error creating thread");
            close(client_sock);
        }

    }
    close(sock);
  return 0;
}
