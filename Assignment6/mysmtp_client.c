// Name - Rishabh
// Roll number - 22CS10058

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_PORT 2525

int main(int argc, char * argv[]) {
    int	sockfd;
	struct sockaddr_in	serv_addr;

    if (argc != 3) {
        printf("Run file with command ./mysmtp_client <IP_Address> <PORT>");
        exit(0);
    }

    char ip[20];
    strcpy(ip, argv[1]);
    int port = atoi(argv[2]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton(ip, &serv_addr.sin_addr);
	serv_addr.sin_port	    = htons(port);

    if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	} else {
        printf("Connected to My_SMTP server\n");
    }

    char buff[150];
    while(1) {
        printf("> ");
        fflush(stdout);
        char garbage;

        bzero(buff, sizeof(buff));
        scanf("%[^\n]s", buff);
        scanf("%c", &garbage);

        // printf("%s\n", buff);
        send(sockfd, buff, sizeof(buff), 0);

        if (strncmp(buff, "DATA", 4) == 0) {
            bzero(buff, sizeof(buff));
            recv(sockfd, buff, sizeof(buff), 0);

            if (strncmp(buff, "200", 3)) {
                printf("%s\n", buff);
                continue;
            }

            printf("Enter your message (end with a single dot '.'), ('~' is not allowed, if used will be ignored), (Max message size - 10000 bytes):\n");
            bzero(buff, sizeof(buff));

            int state = 1, ind = 0;
            char cinput;
            while(1) {
                if (ind == 0) {
                    strcpy(buff, "DATA: ");
                    ind = 6;
                }

                scanf("%c", &cinput);
                
                switch(state) {
                    case 0:
                        buff[ind] = cinput;
                        ind++;
                        if (cinput == '\n') state = 1;
                        break;
                    case 1:
                        if (cinput == '.') state = 2;
                        else {
                            buff[ind] = cinput;
                            ind++;
                            state = 0;
                        }
                        break;
                    case 2:
                        if (cinput == '\n') {
                            state = 3;
                        } else {
                            buff[ind] = '.';
                            buff[ind+1] = cinput;

                            ind+=2;
                            state = 0;
                        }
                        break;
                }

                if ((state == 3) || (ind >= 148)) {
                    send(sockfd, buff, sizeof(buff), 0);
                    ind = 0;
                    bzero(buff, sizeof(buff));
                }

                if (state == 3) {
                    strcpy(buff, "END");
                    send(sockfd, buff, sizeof(buff), 0);
                    break;
                }
            }
        } else if (strncmp(buff, "LIST", 4) == 0) {
            bzero(buff, sizeof(buff));
            recv(sockfd, buff, sizeof(buff), 0);
            printf("%s\n", buff);

            if (strncmp(buff, "200", 3)) continue;

            while(1) {
                bzero(buff, sizeof(buff));
                recv(sockfd, buff, sizeof(buff), 0);
                if (strcmp(buff, "END") == 0) break;
                else printf("%s\n", buff);
            }         
            
            printf("END OF LIST\n");

            continue;
        } else if (strncmp(buff, "GET_MAIL", 8) == 0) {
            bzero(buff, sizeof(buff));
            recv(sockfd, buff, sizeof(buff), 0);
            printf("%s\n", buff);

            if (strncmp(buff, "200", 3)) continue; 

            while(1) {
                bzero(buff, sizeof(buff));
                recv(sockfd, buff, sizeof(buff), 0);
                if (strcmp(buff, "END") == 0) break;
                else {
                    printf("%s", buff);
                    fflush(stdout);
                }
            }           

            continue;
        }

        bzero(buff, sizeof(buff));
        recv(sockfd, buff, sizeof(buff), 0);
        printf("%s\n", buff);

        if (strcmp(buff, "200 Goodbye") == 0) break;
    }
}