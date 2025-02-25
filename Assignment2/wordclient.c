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
#include <time.h>
  
#define PORT 5000 
#define MAXLINE 1000 
  
int main() {    

    srand((unsigned int)time(NULL));

    // Initialization
    char buffer[1000]; 
    char message[] = "22CS10058_File0.txt"; 
    int sockfd, n;
    struct sockaddr_in servaddr; 
    
    message[14] = '0' + rand()%4;

    // Server settings 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET; 
      
    // create datagram socket 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
      
    // Send file name to server
    printf("Client Running .........\n");
    printf("\nSending request to fetch file: %s\n", message);
    sendto(sockfd, message, MAXLINE, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
      
    // waiting for response 
    recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);

    if (strcmp(buffer, "HELLO\n")) {    // File not found

        printf("\n%s\n", buffer);

        // Clear contents of file
        FILE * client = (FILE *) fopen("22CS10058_client.txt", "w");
        fclose(client);

    } else { // File found

        // Creating file after the keyword "HELLO" is found
        FILE * client = (FILE *) fopen("22CS10058_client.txt", "w");

        printf("\nStarting to receive file content from server\n");
        fprintf(client, "%s", buffer);

        while(strcmp(buffer, "FINISH") && strcmp(buffer, "FINISH\n")) {
            recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            fprintf(client, "%s", buffer);
        }

        // Closing the file after the keyword "FINISH" is found
        fclose(client);

        printf("\nFinished receiving file content from server\n");
    }
  
    // close the descriptor 
    close(sockfd); 
} 
