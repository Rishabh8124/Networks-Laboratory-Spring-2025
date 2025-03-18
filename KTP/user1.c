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

    server.sin_port = htons(10000);
    client.sin_port = htons(5000);

    int bind_status = k_bind(sock, &server, &client);
    if (bind_status) {
        printf("Bind Error\n");
        exit(0);
    }

    char mess[512];

    FILE * f = (FILE *) fopen("testfile.txt", "r");

    while(fgets(mess, 511, f)) {
        mess[511] = 0;
        while(k_sendto(sock, mess, sizeof(mess), 0, &client, sizeof(client)) < 0);
        bzero(mess, 512);
    }

    fclose(f);

    bzero(mess, sizeof(mess));
    sprintf(mess, "END");
    while(k_sendto(sock, mess, sizeof(mess), 0, &client, sizeof(client)) < 0);
    k_close(sock);
}