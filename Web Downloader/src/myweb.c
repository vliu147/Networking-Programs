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
  // FILE *output;
  // output = fopen("output.dat", "w");// 

  // int counter;
	// for(counter=1; counter<argc; counter++){
	// 	fprintf(output,"%s\n",argv[counter]);
  // }
  char *address;
  char *ip;
  char *website;
  int portnum;
  website = argv[1];      // First arg Ex. www.example.com
  ip = argv[2];           // Second arg Ex. 93.184.216.34:80/index.html
  portnum = 80;           // Port number

  int h = 0;              // Check for -h flag
  if(argc == 4){
    if(strcmp(argv[3],"-h")==0 || strcmp(argv[3],"-H")==0 ){
    h = 1;
    }
  }

  int client_socket;                       // Create the Socket
  client_socket = socket(AF_INET, SOCK_STREAM,0);
  if(client_socket < 0){
    fprintf(stderr, "Error: Can't open socket\n");
    return -1;
  }

  char *temp;
  char *path;
  char *holder = strdup(ip);
  char *holder2 = strdup(ip);
  int temp2;
  if(strchr(ip,'/') == NULL){              // Checks for path
    path = "/";
  } else {
  temp2 = strchr(ip,'/') - argv[2];        // Get position of '/'
  path = &holder[temp2];                   // Get path name
  }               

  temp2 = strchr(ip,':') - ip + 1;         // Get position of ':'
  if(strchr(ip,':') == NULL){              // Check if there is port# or not
    address = strtok(holder2,"/");         // Get the address Ex. 93.184.216.34/
  } else {
    address = strtok(holder2,":");         // Get the address Ex. 93.184.216.34:
    temp = strtok(&holder2[temp2],"/");    // Get port number
    portnum = atoi(temp);
  //temp2 = strchr(ip,'/') - ip + 1;
  }

  if(portnum < 0 || portnum > 65535){
    fprintf(stderr, "Error: Invalid port number\n");
    return -1;
  }
  // printf("\n Path = %s\n",path);
  // printf("\n Website: %s\n",website);          
  // printf("\n Address: %s\n",address);
  // printf("\n Port Number: %d\n",portnum);
  // printf("\n H flag: %d\n",h);
  
  struct sockaddr_in remote_address;
  remote_address.sin_family = AF_INET;
  remote_address.sin_port = htons(80);
  inet_pton(AF_INET,address, &remote_address.sin_addr);

  if(connect(client_socket, (struct sockaddr *) &remote_address, sizeof(remote_address))<0){ //0 = okay, -1 = error
    fprintf(stderr, "Error: Can't connect\n");
    return -1;
  }

  char request[4096]; 
  char response[MAXLINE];

if(h==1){                            // If there is -h flag, use HEAD
  sprintf(request,"HEAD %s HTTP/1.0\r\nHost: %s \r\n\r\n",path,website);
  int sent = send(client_socket,request, sizeof(request),0);
  if(sent < 0){
    fprintf(stderr, "Error: Can't write to socket\n");
    return -1;
  }
  //printf("\n Sent: %d\n",sent);
  
  int recvd = recv(client_socket, response, sizeof(response),0); // receive data from server
  if(recvd < 0){
    fprintf(stderr, "Error: Can't read from socket\n");
    return -1;
  }
  //printf("\n Recvd: %d\n",recvd);

  printf("%s\n",response);

} else {
 
  FILE *output;               // Open the file to write to

  output = fopen("output.dat", "w");// 

  sprintf(request,"GET %s HTTP/1.0\r\nHost: %s \r\n\r\n",path,website);
  int sent = write(client_socket,request, sizeof(request));
  if(sent < 0){
    fprintf(stderr, "Error: Can't write to socket\n");
    return -1;
  }
  //printf("\n Sent: %d\n",sent);
  
  int n;
 
 while ((n = read(client_socket, request, MAXLINE)) > 0) {
        request[n] = 0; /* null terminate */

        // Index of the "\r\n\r\n" marker
        char *body_start = strstr(request, "\r\n\r\n");

        if (body_start) {
            body_start +=4; //null-terminate the headers
            fprintf(output, "%s", body_start);
            break;
        } 
    }
while ((n = read(client_socket, request, MAXLINE)) > 0) {
        fwrite(request, sizeof(char), n, output);
    }


  //  printf("\nResponse from the server: \n%s\n",response);
  //  fputs(response,output);
  fclose(output);
}

  //fclose(output);
  close(client_socket);
  return 0;
}
