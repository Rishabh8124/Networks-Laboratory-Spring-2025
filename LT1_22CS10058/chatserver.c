// Name: Rishabh
// Roll Number: 22CS10058

/*
ASSUMPTIONS:
    1) If client sends a message when only one client is present. Server waits for the client enter a new number when
    another client joins without informing the client
    2) Number of clients trying to jopin won't cross 5, Validity has not been checked
    3) Any standard input in server side will result in game close.
*/

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
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed\n");
        exit(0);
    }

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed\n");
        exit(0);
    }

    printf("\nConnection Successful: GAME BEGIN\n");

    // Game Initialization
    int curr_round = 1;
    int num_clients = 0;
    int num_played = 0;
    int clientsockfd[5];
    int round_read[5] = {0, 0, 0, 0, 0};    // Flag to see if the client has already played this round
    int round_value[5] = {0, 0, 0, 0, 0};   // Value given by client in current round

    //  Select Initializations
    int size;                   // Temporary variable used in function calls
    int port;                   // Temporary variable to store port number of winner client
    int maxfd;                  // Parameter for select
    fd_set fds;                 // File Descriptor list
    char buff[1000], cli[20];   // cli -> Temporary variable to store the IPv4 address of the client

    listen(sockfd, 5);

    while (1) {                 // Game Starts
        // Initalize fds for next iteration
        FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		FD_SET(STDIN, &fds);
        
        maxfd = (sockfd > STDIN) ? sockfd : STDIN;

        // Push client addresses in the fds
        for(int i=0; i<num_clients; i++) {
            FD_SET(clientsockfd[i], &fds);
            maxfd = (clientsockfd[i] > maxfd) ? clientsockfd[i] : maxfd;
        }
        
        select(maxfd+1, &fds, NULL, NULL, NULL);

        // Input from user in server side
        if (FD_ISSET(STDIN, &fds)) {
            scanf("%s", buff);
            printf("%s\n", buff);
            break;
        }
        
        // New Connection through server socket
        if (FD_ISSET(sockfd, &fds)) {
            bzero(&cliaddr, sizeof(cliaddr));
            int client_fd = accept(sockfd, (struct sockaddr *) &cliaddr, (int *) &size);
            if (client_fd < 0) {
                printf("Connection Failed\n");
            } else {
                if (num_clients < 5) {
                    getpeername(client_fd, (struct sockaddr *) &cliaddr, (int *) &size);
                    inet_ntop(AF_INET, &cliaddr.sin_addr, buff, 20);
                    port = ntohs(cliaddr.sin_port);
                    printf("\nReceived a new connection from client %s:%d\n", buff, port);
                    clientsockfd[num_clients] = client_fd;
                    num_clients++;

                    bzero(buff, 1000);
                    sprintf(buff, "Send your number for Round %d: ", curr_round);
                    send(client_fd, buff, strlen(buff), 0);
                }
            }
        }
        
        // Check for input from clients
        for(int i=0; i<num_clients; i++) {
            if (FD_ISSET(clientsockfd[i], &fds)) {
                int num;
                bzero(buff, sizeof(buff));

                if (recv(clientsockfd[i], buff, sizeof(buff), 0) > 0) {
                    num = atoi(buff);
                    if (num_clients < 2) {              // Insufficient clients
                        getpeername(clientsockfd[i], (struct sockaddr *) &cliaddr, (int *) &size);
                        inet_ntop(AF_INET, &cliaddr.sin_addr, buff, 20);
                        port = ntohs(cliaddr.sin_port);
                        printf("Insufficient clients, \"%d\" from client %s:%d dropped\n", num, buff, port);
                    } else if (round_read[i] == 1) {    // Client already played
                        bzero(buff, sizeof(buff));
                        sprintf(buff, "Duplicate messages for Round %d are not allowed. Please wait for the results for Round %d and Call for the number for Round %d\n", curr_round, curr_round, curr_round+1);
                        send(clientsockfd[i], buff, strlen(buff), 0);
                    } else {                            // New entry from client
                        num_played++;
                        round_value[i] = num;
                        round_read[i] = 1;

                        if (num_played == num_clients) {    // All clients have played, time for results
                            int winner = 0;
                            for(int i=1; i<num_clients; i++) {
                                if (round_value[i] > round_value[winner]) {
                                    winner = i;
                                }
                            }

                            getpeername(clientsockfd[winner], (struct sockaddr *) &cliaddr, (int *) &size);
                            inet_ntop(AF_INET, &cliaddr.sin_addr, cli, 20);
                            port = ntohs(cliaddr.sin_port);
                            bzero(buff, sizeof(buff));
                            sprintf(buff, "Maximum Number in Round %d is : %d. The number has been received from the client %s:%d\n\nEnter the number for Round %d: ", curr_round, round_value[winner], cli, port, curr_round+1);
                            
                            curr_round++;
                            num_played = 0;

                            for(int i=0; i<num_clients; i++) {
                                round_read[i] = 0;
                                send(clientsockfd[i], buff, strlen(buff), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
}