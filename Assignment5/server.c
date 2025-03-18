// Name - Rishabh
// Roll number - 22CS10058

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_LINE 100
#define PORT     8000

int	sockfd, newsockfd ; /* Socket descriptors */

// Global variables
int shmid;
int * addr;
int semid;
int n = 0;

// SIGINT Handler to parent server
void signal_handler(int signal) {
    close(sockfd);

    shmdt(addr);
    shmctl(shmid, IPC_RMID, 0);
    semctl(semid, 0, IPC_RMID);
    exit(0);
}

// Zombie process removal when server receives SIGCHLD
void child_exit(int signal) {
    wait(NULL);
}

// SIGINT Handler to child servers
void child_sig(int signal) {
    int pid = getppid();
    kill(pid, SIGINT);
    exit(0);
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

// Client handling process
int get_task_no() {
    semlock(semid);
    for(int i=1; i<(2*n); i+=2) {
        if (addr[i] == 0) {
            addr[0]--;
            addr[i] = 1;
            semunlock(semid);
            return (i-1)/2;
        }
    }
    semunlock(semid);

    return -1;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGCHLD, child_exit);

    // Input file name
    char filename[100];
    bzero(filename, sizeof(filename));

    printf("Enter filename: ");
    fflush(stdout);
    scanf("%s", filename);

    // Read tasks from file
    FILE * file = fopen(filename, "r");
    char buff[105];

    if (file == NULL) {
        printf("Wrong file name\n");
        kill(getpid(), SIGINT);
    }

    // Tasks
    while(fgets(buff, MAX_LINE, file)) n++;

    fclose(file);

    // Shared memory creation
    shmid = shmget(728, ((n+1)*2-1)*sizeof(int), IPC_CREAT|IPC_EXCL|0777);
    addr = shmat(shmid, NULL, 0);

    semid = semget(725, 1, IPC_CREAT|IPC_EXCL|0777);
    semctl(semid, 0, SETVAL, 1);

    addr[0] = n;

    file = fopen("tasks.txt", "r");
    char tasks[n][MAX_LINE];

    for (int i=0; i<n; i++) {
        bzero(&tasks[i], sizeof(tasks[i]));
        fgets(tasks[i], MAX_LINE, file);
        if (tasks[i][strlen(tasks[i])-1] == '\n') tasks[i][strlen(tasks[i])-1] = '\0';
        addr[1+(i*2)] = 0;
    }

    fclose(file);
    
    // Socket
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

    /* Server socket which accepts requests is not made non blocking 
    because it was specifically mentioned to perform non-blocking IO */

    serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		    = htons(PORT);

    // Binding server to socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	} else {
		printf("Server ready...\n\n");
	}

    listen(sockfd, 5);
    
    // Start sockets
    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd >= 0) {
            printf("\nNew request found\n");

            if (fork() == 0) {
                signal(SIGINT, child_sig);
                close(sockfd);

                // no_answer -> No message from client counter (5 sec)
                // repeat_req -> Number of requests without answer (5 times)
                int no_answer = 0, repeat_req = 0;

                // Set client handling socket to non blocking mode
                int flags = fcntl(newsockfd, F_GETFL);

                if (fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    printf("Cannot set flags\n");
                    exit(0);
                }

                char buff[50];
                int taskno = -1;
                
                // Server process
                while(1) {

                    // Receive message non blocking I/O
                    bzero(buff, sizeof(buff));
                    int sz = recv(newsockfd, buff, 50, 0);
                    if (sz != -1) {
                        buff[sz] = '\0';
                    }
                    
                    // Processing varibales
                    char sub[100];
                    bzero(sub, sizeof(sub));
                    if (strlen(buff) > 6) strncpy(sub, buff, 6);

                    // Handling message from client
                    if (strcmp(buff, "GET_TASK") == 0) { // Send Task
                        no_answer = 0;
                        char mess[150] = "Task:";
                        if (taskno != -1) { // Repeated tasks
                            repeat_req++;
                            bzero(buff, sizeof(buff));
                            strcpy(buff, "REPEAT");
                            send(newsockfd, buff, 50, 0);
                        } else {    // Assign new task
                            taskno = get_task_no();
                            if (taskno == -1) {
                                bzero(buff, sizeof(buff));
                                strcpy(buff, "NO_TASK");
                                send(newsockfd, buff, 50, 0);
                            } else {
                                strcat(mess, tasks[taskno]);
                                send(newsockfd, mess, 50, 0);
                            }
                        }
                    } else if (strcmp(sub, "RESULT") == 0) { // Result
                        int result = 0;

                        if (taskno != -1 && sscanf(buff, "RESULT %d", &result) == 1) {
                            no_answer = 0;
                            repeat_req = 0;
                            
                            semlock(semid);
                            addr[(taskno+1)*2] = result;
                            addr[(taskno+1)*2-1] = 2;

                            // Write to file
                            file = (FILE *) fopen("results.txt", "w");
                            for(int i=0; i<n; i++) {
                                if (addr[(i+1)*2-1] == 2) fprintf(file, "%s = %d\n", tasks[i], addr[(i+1)*2]);
                                else fprintf(file, "%s\n", tasks[i]);
                            }
                            fclose(file);
                            semunlock(semid);
                            printf("Result - %d: %d\n", taskno, result);

                            taskno = -1;
                        } else {
                            no_answer++;
                        }
                    } else if (strcmp(buff, "exit") == 0) {
                        printf("Exit request received. Closing the connection\n");
                        break;
                    } else { // Random message or no message
                        no_answer++;
                        sleep(1);
                    }

                    if (no_answer >= 5) {
                        printf("No message from client for 5 secs. Closing the connection\n");
                        break;
                    }

                    if (repeat_req >= 5) {
                        printf("\n5 repeated requests received from client without answer. Closing the connection\n");
                        break;
                    }
                }

                // Handling exit
                close(newsockfd);
                if (taskno != -1) {
                    addr[1+(taskno*2)] = 0;
                    addr[0]++;
                }
                exit(0);
            }

            close(newsockfd);

        } else {
            sleep(1);
        }
    }

    close(sockfd);

    return 0;
}
