#include "ktp.h"

int main() {
    // Socket creation and close
    int sock = k_socket(AF_INET, SOCK_KTP, 0);
    if (sock < 0) {
        printf("No sockets found\n");
        exit(0);
    }

    struct sockaddr_in server, client;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr("127.0.0.1");

    server.sin_port = htons(5000);
    client.sin_port = htons(10000);

    int bind_status = k_bind(sock, &server, &client);
    if (bind_status) {
        printf("Bind Error\n");
        exit(0);
    }

    char buff[MESSAGE_SIZE];

    FILE * f = fopen("receive.txt", "w");
    int i = 0;

    while(1) {
        while(k_recvfrom(sock, buff, 512, 0, &client, sizeof(client))<0);
        if (strcmp(buff, "END") == 0) break;
        fprintf(f, "%s", buff);
    }

    fclose(f);

    printf("Over\n");
    k_close(sock);
}