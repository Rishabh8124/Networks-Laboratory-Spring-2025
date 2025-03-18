// Name - Rishabh
// Roll number - 22CS10058

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8000

char buff[50];

// Equation solver
int solve_equation() {
	int index = 5;
	int n1 = 0, n2 = 0;
	char op = ' ';

	sscanf(buff, "Task:%d %c %d", &n1, &op, &n2);

	int result;
	switch(op) {
		case '+':
			result = n1 + n2;
			break;
		case '-':
			result = n1 - n2;
			break;
		case '*':
			result = n1 * n2;
			break;
		case '/':
			if (n2 != 0) result = n1 / n2;
			else result = 1e9;
			break;
	}

	return result;
}

int main(int argc, char * argv[]) {
	/* Command Line Argument -
		-> 1 - Proper functioning client which solves 5 tasks
		-> 2 - No request/response sent to server after accepting the connection for 10 seconds (Server closes the connection after 5 seconds of idleness)
		-> 3 - Send GET_TASK and do not respond for 10 seconds
		-> 4 - Send GET_TASK and send exit (Terminating before responding)
		-> 5 - Send 10 GET_TASK requests (Server accepts only consecutive 5 repeated GET_TASK requests)*/

	int type = 1;
	if (argc == 2) {
		type = atoi(argv[1]);
	}

	// Initializations
	int	sockfd;
	struct sockaddr_in	serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(SERVER_PORT);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Not able to connect\n");
		exit(0);
	}

	// Client socket made non blocking for I/O (after conecting with server)
	int flags = fcntl(sockfd, F_GETFL);

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        printf("Cannot set flags\n");
        exit(0);
    }

	if (type == 1) {	// Proper client
		for(int i=0; i<5; i++) {
			sleep(1);
			bzero(buff, sizeof(buff));
			strcpy(buff, "GET_TASK");
			send(sockfd, buff, 50, 0);
			sleep(1);

			int x = recv(sockfd, buff, 50, 0);
			printf("%s\n", buff);
			if (x == -1) {
				printf("Sleeping\n");
				continue;
			}

			if (buff[0] == 'T') {
				int result = solve_equation();
				bzero(buff, sizeof(buff));
				sprintf(buff, "RESULT %d", result);
				send(sockfd, buff, 50, 0);
			} else if (strcmp(buff, "REPEAT")) break;
		}
	} else if (type == 2) {	// No meesage for 10 seconds
		sleep(10);
	} else if (type == 3) {	// Send GET_TASK and do not respond
		sleep(1);
		bzero(buff, sizeof(buff));
		strcpy(buff, "GET_TASK");
		send(sockfd, buff, 50, 0);
		sleep(10);
	} else if (type == 4) {	// Send GET_TASK and exit (TERMINATION without responding)
		sleep(1);
		bzero(buff, sizeof(buff));
		strcpy(buff, "GET_TASK");
		send(sockfd, buff, 50, 0);
	} else if (type == 5) {	// Consecutive 10 GET_TASK without responding
		for(int i=0; i<10; i++) {
			sleep(1);
			bzero(buff, sizeof(buff));
			strcpy(buff, "GET_TASK");
			send(sockfd, buff, 50, 0);
		}
	}

	send(sockfd, "exit", 5, 0);

	close(sockfd);

}