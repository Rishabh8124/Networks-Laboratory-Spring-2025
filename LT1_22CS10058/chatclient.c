// Name: Rishabh
// Roll Number: 22CS10058

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define STDIN 0
#define PORT 5000

int main() {
    struct sockaddr_in server_addr, cliaddr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Socket creation failed\n");
        exit(0);
    }

    if ((connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
        printf("Connection Failed\n");
        exit(0);
    }
    
    // Select Initalizations
    int maxfd;
    fd_set fds;
    char buff[1000];
    
    // Client address used later for printing
    char cli_addr[30], cli[20];
    int size;
    getpeername(sockfd, (struct sockaddr *) &cliaddr, (int *) &size);
    inet_ntop(AF_INET, &cliaddr.sin_addr, cli, 20);

    sprintf(cli_addr, "%s:%d", cli, ntohs(cliaddr.sin_port));

    while(1) {      // Game starts
        FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		FD_SET(STDIN, &fds);

        maxfd = (sockfd > STDIN) ? sockfd : STDIN;
        select(maxfd+1, &fds, NULL, NULL, NULL);

        // Standard input
        if (FD_ISSET(STDIN, &fds)) {
            int num;
            read(STDIN, buff, 100);
            send(sockfd, buff, strlen(buff), 0);
            num = atoi(buff);
            printf("Client %s Number %d sent to server\n", cli_addr, num);
        }

        // Input from server
        if (FD_ISSET(sockfd, &fds)) {
            bzero(buff, sizeof(buff));
            
            if (recv(sockfd, buff, 1000, 0) > 0) {
                printf("\nReceived the following text from the server:\n%s", buff);
                fflush(stdout);
            } else break;
        }
    }

    close(sockfd);
}