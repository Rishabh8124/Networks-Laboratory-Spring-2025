// Name - Rishabh
// Roll number - 22CS10058

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>

#define PORT 2525

int	sockfd, newsockfd ; /* Socket descriptors */
int semid; // Semaphore id

void sig_handle(int signal) {
    close(sockfd);
    semctl(semid, 0, IPC_RMID);
    exit(0);
}

void zombie(int signal) {
    wait(NULL);
}

// Semaphore processes
void semlock(int semid) {
    struct sembuf p = {0, -1, SEM_UNDO};
    semop(semid, &p, 1);
}

void semunlock(int semid) {
    struct sembuf p = {0, 1, SEM_UNDO};
    semop(semid, &p, 1);
}

int main(int argc, char * argv[]) {

    // Command
    if (argc != 2) {
        printf("Run file with command ./mysmtp_server <port>\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    int result = system("mkdir -p mailbox");

    signal(SIGCHLD, zombie);
    signal(SIGINT, sig_handle);

    // Semaphore creation;
    semid = semget(728, 1, IPC_CREAT|IPC_EXCL|0777);
    semctl(semid, 0, SETVAL, 1);

	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		    = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	} else {
		printf("Listening on port %d\n\n", ntohs(serv_addr.sin_port));
	}

	listen(sockfd, 0);

    char garbage[100];

    while (1) {	// Keep server running till eternity
		// Accept new connection to client
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		} else {
            char addr[20];
            inet_ntop(AF_INET, &cli_addr.sin_addr, addr, 20);
            printf("Client connected: %s\n", addr);
        }

        if (fork() == 0) {
            close(sockfd);

            int flags = fcntl(newsockfd, F_GETFL);

            if (fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                printf("Cannot set flags\n");
                exit(0);
            }

            char buff[150];
            char from_address[100];
            char to_address[100];
            char email_id[100];
            char client_id[100];

            int mail_id;
            char filename[100];
            FILE * file;
            char message[10000];
            int data_start = 0;

            int server_state = 0;
            
            while(1) {  // Start receiving messages
                bzero(buff, sizeof(buff));
                int len = recv(newsockfd, buff, 150, 0);
                // if (len != -1) printf("%s\n", buff);
                
                if (len != -1) { // Message received

                    // Quit message
                    if (strcmp(buff, "QUIT") == 0) {
                        bzero(buff, sizeof(buff));
                        strcpy(buff, "200 Goodbye");
                        send(newsockfd, buff, sizeof(buff), 0);
                        usleep(100);
                        printf("Client disconnected\n");
                        break;
                    }
                    
                    // All commands can work only if HELO is executed
                    if (strncmp(buff, "HELO ", 5) == 0) {
                        bzero(client_id, sizeof(client_id));
                        if (sscanf(buff, "HELO %s %s", client_id, garbage) == 1) {
                            server_state = 1;

                            // Reset all variables
                            bzero(from_address, sizeof(from_address));
                            bzero(to_address, sizeof(to_address));
                            bzero(email_id, sizeof(email_id));
                            bzero(message, sizeof(message));

                            printf("HELO received from %s\n", client_id);

                            // Send Acknowledgement
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "200 OK");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        } else {    // Syntax Error
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        }
                    } else if (server_state == 0) {
                        bzero(buff, sizeof(buff));
                        strcpy(buff, "403 FORBIDDEN");
                        send(newsockfd, buff, sizeof(buff), 0);
                        continue;
                    } 
                    
                    // Server state = 1 -> HELO message received
                    if (strncmp(buff, "MAIL FROM: ", 11) == 0) {
                        bzero(from_address, sizeof(from_address));
                        if (sscanf(buff, "MAIL FROM: %s %s", from_address, garbage) == 1) {
                            int pos = 0;
                            while(from_address[pos] != '\0' && from_address[pos] != '@') pos++;

                            if (strcmp(client_id, from_address+pos+1)) {    // Check domain
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "403 FORBIDDEN");
                                send(newsockfd, buff, sizeof(buff), 0);
                                continue;
                            }
                            
                            // Successul state
                            printf("MAIL FROM: %s\n", from_address);
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "200 OK");
                            send(newsockfd, buff, sizeof(buff), 0);
                            server_state = 2;
                            continue;
                        } else {
                            // Syntax error
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        }
                    } else if (strncmp(buff, "RCPT TO: ", 9) == 0) {
                        bzero(to_address, sizeof(to_address));
                        if (sscanf(buff, "RCPT TO: %s %s", to_address, garbage) == 1 && server_state == 2) {
                            // Valid domain check
                            int pos = 0;
                            while(from_address[pos] != '\0' && from_address[pos] != '@') pos++;

                            if (strcmp(client_id, from_address+pos+1)) {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "403 FORBIDDEN");
                                send(newsockfd, buff, sizeof(buff), 0);
                                continue;
                            }

                            printf("RCPT TO %s\n", to_address);
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "200 OK");
                            send(newsockfd, buff, sizeof(buff), 0);
                            server_state = 3;
                            continue;
                        } else if (server_state != 2) {
                            // RCPT TO: should be after MAIL FROM:
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "403 FORBIDDEN");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        } else {
                            // Syntax Error
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        }
                    } 
                    
                    if (data_start && (strncmp(buff, "DATA: ", 6) == 0)) {
                        // Handle data
                        strcat(message, buff+6);
                        fflush(stdout);
                    } else if (strcmp(buff, "DATA") == 0) {
                        // Start receiving data after RCPT TO:
                        if (server_state != 3) {
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "403 FORBIDDEN");
                            send(newsockfd, buff, sizeof(buff), 0);
                            data_start = 2;
                            continue;
                        }

                        bzero(buff, sizeof(buff));
                        strcpy(buff, "200 OK");
                        send(newsockfd, buff, sizeof(buff), 0);
                        data_start = 1;
                        bzero(message, sizeof(message));
                    } else if (strcmp(buff, "END") == 0) {
                        // End of data from client. Store message in file
                        bzero(buff, sizeof(buff));

                        if (data_start == 1) {
                            // Start writing in file
                            semlock(semid);
                            bzero(filename, sizeof(filename));
                            strcpy(filename, "mailbox/");
                            strcat(filename, to_address);
                            strcat(filename, ".txt");
                            file = fopen(filename, "a");
                            
                            time_t current_time;
                            time(&current_time);
                            struct tm * local_time = localtime(&current_time);
                            char date[20];
                            strftime(date, sizeof(date), "%d-%m-%Y", local_time);

                            fprintf(file, "MAIL FROM: %s (%s)\n", from_address, date);
                            if (strlen(message)) fprintf(file, "DATA: %s", message);
                            else fprintf(file, "DATA: \n");
                            fprintf(file, "~\n");
                            fclose(file);
                            semunlock(semid);
                            strcpy(buff, "200 Message stored successfully");
                            printf("Data received, message stored\n");

                            data_start = 0;
                            bzero(from_address, sizeof(from_address));
                            bzero(to_address, sizeof(to_address));
                            bzero(email_id, sizeof(email_id));
                            bzero(message, sizeof(message));
                            server_state = 1;
                            
                        } else {
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                        }

                        send(newsockfd, buff, sizeof(buff), 0);
                    } else if (strncmp(buff, "LIST", 4) == 0) {
                        // Works only after HELO
                        bzero(email_id, sizeof(email_id));  
                        if (sscanf(buff, "LIST %s %s", email_id, garbage) == 1) { 
                            // Check domain   
                            int pos = 0;
                            while(email_id[pos] != '\0' && email_id[pos] != '@') pos++;

                            if (strcmp(client_id, email_id+pos+1)) {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "403 FORBIDDEN");
                                send(newsockfd, buff, sizeof(buff), 0);
                                continue;
                            }

                            bzero(filename, sizeof(filename));
                            strcpy(filename, "mailbox/");
                            strcat(filename, email_id);
                            strcat(filename, ".txt");
                            semlock(semid);
                            file = (FILE *) fopen(filename, "r");

                            if (file != NULL) {
                                printf("Emails of %s retrieved. List sent\n", email_id);
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "200 OK");
                                send(newsockfd, buff, sizeof(buff), 0);

                                int count = 1;
                                int file_state = 1;

                                char file_read[150];
                                bzero(file_read, sizeof(file_read));

                                while(fgets(file_read, 150, file)) {
                                    if (file_state == 1) {
                                        file_state = 0;
                                        bzero(buff, sizeof(buff));
                                        file_read[strlen(file_read)-1] = '\0';
                                        sprintf(buff, "%d: Email from %s", count, file_read+11);
                                        send(newsockfd, buff, 150, 0);
                                        count++;
                                    } else if (strcmp(file_read, "~\n") == 0) file_state = 1;
                                }

                                bzero(buff, sizeof(buff));
                                strcpy(buff, "END");
                                send(newsockfd, buff, 150, 0);

                                fclose(file);
                                semunlock(semid);
                            } else {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "401 Not Found");
                                send(newsockfd, buff, sizeof(buff), 0);
                                semunlock(semid);
                                continue;
                            }
                        } else {
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        }
                    } else if (strncmp(buff, "GET_MAIL ", 9) == 0) {
                        bzero(email_id, sizeof(email_id));  
                        if (sscanf(buff, "GET_MAIL %s %d %s", email_id, &mail_id, garbage) == 2) {
                            int pos = 0;
                            while(email_id[pos] != '\0' && email_id[pos] != '@') pos++;

                            if (strcmp(client_id, email_id+pos+1)) {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "403 FORBIDDEN");
                                send(newsockfd, buff, sizeof(buff), 0);
                                continue;
                            }

                            bzero(filename, sizeof(filename));
                            strcpy(filename, "mailbox/");
                            strcat(filename, email_id);
                            strcat(filename, ".txt");
                            semlock(semid);
                            file = (FILE *) fopen(filename, "r");

                            if (file != NULL) {
                                int count = 1;
                                int file_state = 0;

                                char file_read[150];
                                bzero(file_read, sizeof(file_read));

                                while(fgets(file_read, 150, file)) {
                                    if ((strcmp(file_read, "~\n") == 0) || strcmp(file_read, "~") == 0) {
                                        if (file_state) {
                                            bzero(buff, sizeof(buff));
                                            strcpy(buff, "END");
                                            send(newsockfd, buff, 150, 0);
                                            break;
                                        }
                                        count++;
                                    } else if (count == mail_id) {
                                        bzero(buff, sizeof(buff));
                                        if (file_state == 0) file_state = 1;

                                        if (file_state == 1) {
                                            printf("Email with ID %d of %s sent\n", mail_id, email_id);
                                            bzero(buff, sizeof(buff));
                                            strcpy(buff, "200 OK");
                                            send(newsockfd, buff, sizeof(buff), 0);
                                            file_read[strlen(file_read)-1] = '\0';
                                            char date[20];
                                            bzero(date, sizeof(date));
                                            bzero(email_id, sizeof(email_id));
                                            sscanf(file_read, "MAIL FROM: %s (%s", email_id, date);
                                            date[strlen(date)-1] = '\0';
                                            sprintf(buff, "From: %s\nDate: %s\n", email_id, date);
                                            file_state = 2;
                                        } else if (file_state == 2) {
                                            sprintf(buff, "%s", file_read+6);
                                            file_state = 3;
                                        } else {
                                            sprintf(buff, "%s", file_read);
                                            file_state = 3;
                                        }

                                        send(newsockfd, buff, 150, 0);
                                    }
                                    bzero(file_read, sizeof(file_read));
                                }

                                if (file_state == 0) {
                                    bzero(buff, sizeof(buff));
                                    strcpy(buff, "401 Mail Not Found");
                                    send(newsockfd, buff, sizeof(buff), 0);
                                    semunlock(semid);
                                    continue;
                                }

                                fclose(file);
                                semunlock(semid);
                            } else {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "401 Not Found");
                                send(newsockfd, buff, sizeof(buff), 0);
                                semunlock(semid);
                                continue;
                            }
                        } else {
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "400 Err");
                            send(newsockfd, buff, sizeof(buff), 0);
                            continue;
                        }
                    } else {
                        bzero(buff, sizeof(buff));
                        strcpy(buff, "400 Err");
                        send(newsockfd, buff, sizeof(buff), 0);
                    }
                }
            }
            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }

    close(sockfd);
}