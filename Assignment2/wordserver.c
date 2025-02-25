// Name: Rishabh
// Roll number: 22CS10058
// pcap link: https://drive.google.com/drive/folders/1V5XmE3sE-masRXSnQ7aEtriLcZSISlGk?usp=sharing

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 5000
#define MAXLINE 1000

int main() {
    
    char buffer[1000];      // To send/receive message from receiver and read content from file
    char message[1000];     // To send NOT FOUND message to receiver

    // Network details
    int serverfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));

    // Create a UDP Socket 
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);         
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET;  
   
    // bind server address to socket descriptor 
    bind(serverfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    
    printf("Server Running .........\n");
    
    //receive the datagram 
    len = sizeof(cliaddr);
    int n = recvfrom(serverfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len); //receive message from server 
    buffer[n] = '\0';

    printf("\nReceived request from client to fetch %s\n", buffer);
    FILE * messagefile = (FILE *) fopen(buffer, "r");

    if (messagefile == NULL) {      // File Not found

        printf("\nFile requested by client is not found\n");
        char * message1 = "NOTFOUND ";
        message[0] = '\0';
        strcat(message, message1);
        strcat(message, buffer);
        sendto(serverfd, message, MAXLINE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

    } else {                        // Sending file content to server.

        printf("\nStarting to send file content to client\n");
        do {
            fgets(buffer, 1000, messagefile);
            sendto(serverfd, buffer, MAXLINE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        } while(strcmp(buffer, "FINISH") && strcmp(buffer, "FINISH\n"));

        // File finished marked by keyword "FINISH"
        printf("\nFinished sending file content to client\n");

    }

    close(serverfd);

    return 0;
} 
