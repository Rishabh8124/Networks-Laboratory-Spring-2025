#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {

	// Initializations
	int	sockfd;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[100];
	char filename[100], encrypt[50], enc_file[110];
	char ans[100];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(5000);

	// Single TCP connection for a client, closed only when the user stops encrypting
	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	while(strcmp(ans, "No")) {	// Encrypt files till the user stops
		while(1) {	// Get a valid filename
			printf("Enter filename: ");
			scanf("%s", filename);
			FILE * temp = (FILE *) fopen (filename, "r");
			if (temp != NULL) {
				break;
			} else {
				printf("NOTFOUND %s. Enter a valid filename\n\n", filename);
			}
		}

		FILE * fp = (FILE *) fopen(filename, "r");

		// DEFPRTVWLMZAYGHQSIUJXKBCNO
		while(1) {	// Get a valid encryption key
			printf("Enter encryption key (CAPITALS): ");
			scanf("%s", buf);
			if (strlen(buf) == 26) {
				int c = 1;
				for (int i=0; i<26; i++) {
					if (buf[i] < 'A' || buf[i] > 'Z') c = 0;
				}
				if (c) break;
			}
			printf("Enter a valid encryption key\n\n");
		}
		
		// Send the file content
		while(strcmp(buf, "")) {
			send(sockfd, buf, strlen(buf) + 1, 0);

			for(int i=0; i<100; i++) buf[i] = '\0';
			fgets(buf, 100, fp);
		}

		// Since the file contains only alphabets, 0 is used to mark the end of the file
		for(int i=0; i<100; i++) buf[i] = '\0';
		strcpy(buf, "0");
		send(sockfd, buf, strlen(buf) + 1, 0);

		// Create a file to store the encrypted version
		sprintf(enc_file, "%s.enc", filename);
		fp = (FILE *) fopen(enc_file, "w");
		int j = 1;
		while(j) {
			recv(sockfd, buf, 100, 0);
			for(i=0; i<100; i++) {
				if (buf[i] == '0') {
					j=0;
					break;
				}

				if (buf[i] != '\0') fprintf(fp, "%c", buf[i]);
				buf[i] = '\0';
			}
		}
		fclose(fp);

		// Ask the user for exit/new file encrypt
		printf("\nFile encryption completed\n\nOriginal file: %s\nEncrypted file: %s\n", filename, enc_file);
		printf("\nDo you want to encrypt another file ('No' to exit):");
		scanf("%s", ans);

		// Inform the server, for a new file or to close the connection.
		if (strcmp(ans, "No")) {
			buf[0] = '1';
		} else {
			buf[0] = '0';
		}

		send(sockfd, buf, strlen(buf) + 1, 0);
	}

	close(sockfd);

	return 0;
}