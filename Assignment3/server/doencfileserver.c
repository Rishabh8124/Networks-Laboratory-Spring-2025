#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {

	int	sockfd, newsockfd ; /* Socket descriptors */
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(5000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	} else {
		printf("Server ready...\n\n");
	}

	listen(sockfd, 0);

	while (1) {	// Keep server running till eternity
		// Accept new connection to client
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		if (fork() == 0) {
			// Create a child process to handle the client connection so that server can handle other clients parallely
			close(sockfd);
			int cond = 1;

			while(cond) {	// Single client can encrypt multple files
				for(i=0; i < 100; i++) buf[i] = '\0';
				
				// Create filenames as mentioned
				char addr[20], file_name[50], enc_file_name[50];
				inet_ntop(AF_INET, &cli_addr.sin_addr, addr, 20);
				sprintf(file_name, "%s.%d.txt\n", addr, ntohs(cli_addr.sin_port));
				sprintf(enc_file_name, "%s.%d.txt.enc\n", addr, ntohs(cli_addr.sin_port));

				int j=1;
				char encrytion_key[26];
				recv(newsockfd, encrytion_key, 27, 0);

				// Receive file and store the original copy
				FILE * fp1 = (FILE *) fopen(file_name, "w");
				printf("Hello %s\n", encrytion_key);
				while(j) {
					recv(newsockfd, buf, 100, 0);
					for(i=0; i<100; i++) {
						if (buf[i] == '0') {
							j=0;
							break;
						}

						if (buf[i] != '\0') fprintf(fp1, "%c", buf[i]);
						buf[i] = '\0';
					}
				}
				fclose(fp1);
				
				// Encrypt the file and store in a different file
				fp1 = (FILE *) fopen(file_name, "r");
				FILE * fp = (FILE *) fopen(enc_file_name, "w");
				for(int i=0; i<100; i++) buf[i] = '\0';
				buf[0] = '0';

				while(1) {
					fgets(buf, 100, fp1);
					if (strcmp(buf, "") == 0) break;
					for (int i=0; i<100; i++) {
						if (buf[i]>='A' && buf[i]<='Z') fprintf(fp, "%c", encrytion_key[buf[i]-'A']);
						else if (buf[i]>='a' && buf[i]<='z') fprintf(fp, "%c", encrytion_key[buf[i]-'a']+32);
						else if (buf[i] != '\0') fprintf(fp, "%c", buf[i]);
						buf[i] = '\0';
					}
				}

				fclose(fp1);
				fclose(fp);
				
				// Send the file to client
				fp = (FILE *) fopen(enc_file_name, "r");
				buf[0] = '0';

				while(strcmp(buf, "")) {
					for(int i=0; i<100; i++) buf[i] = '\0';
					fgets(buf, 100, fp);
					send(newsockfd, buf, strlen(buf) + 1, 0);
				}

				for(int i=0; i<100; i++) buf[i] = '\0';
				strcpy(buf, "0");
				send(newsockfd, buf, strlen(buf) + 1, 0);

				printf("Encrypted file sent successfully\n\n");

				// Receive status from client to close the connection or start encrypting a new file
				buf[0] = '\0';
				recv(newsockfd, buf, 100, 0);

				cond = (buf[0] != '0');
			}

			printf("CONNECTION CLOSED\n\n");
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}

	close(sockfd);
	return 0;
}