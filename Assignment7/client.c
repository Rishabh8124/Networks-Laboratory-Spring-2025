// Name - Rishabh
// Roll number - 22CS10058

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <netinet/ip.h>

#define IP_SELF "192.168.0.3"

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

struct cldp_header {
    uint8_t msg_type;     // 1 bytes
    uint32_t payload_len; // 4 bytes
    uint16_t txn_id1;     // 2 bytes
    uint16_t txn_id2;     // 2 bytes
};

int main(int argc, char * argv[]) {
    int sock = socket(AF_INET, SOCK_RAW, 253);
    int opt = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));

    int query_number = 0;
    if (argc == 2) {
        query_number = atoi(argv[1]);
        query_number %= 3;
    }

    struct sockaddr_in client_addr;

    int bytes;
    char buffer[1024];
    struct sockaddr_in src;
    socklen_t len = sizeof(src);

    int tot_header = sizeof(struct iphdr) + sizeof(struct cldp_header);

    while (1) {
        bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&src, &len);

        struct iphdr *ip = (struct iphdr *)buffer;
        struct cldp_header *ch = (struct cldp_header *) (buffer + sizeof(struct iphdr));
        char *payload = buffer + ip->ihl * 4;
        
        if (ch->msg_type == 2) continue;

        if (ch->msg_type == 1) {
            bzero(buffer + tot_header, sizeof(buffer) - (tot_header));
            
            ch->msg_type = 2;
            ch->txn_id2 = ((uint16_t) getpid());
            ip->daddr = ip->saddr;
            ip->saddr = inet_addr(IP_SELF);
            ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

            struct sockaddr_in dst;
            dst.sin_family = AF_INET;
            dst.sin_addr.s_addr = ip->daddr;

            char * pt = buffer + tot_header;
            switch(query_number) {
                case 0:
                    strcpy(pt, "CPU LOAD");
                    break;
                case 1:
                    strcpy(pt, "SYSTEM TIME");
                    break;
                case 2:
                    strcpy(pt, "HOSTNAME");
                    break;
            }

            ch->payload_len = strlen(pt);

            sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dst, sizeof(dst));
            printf("%s\n", inet_ntoa(dst.sin_addr));
            printf("Sent request - %d - %d\n",((struct cldp_header *)(buffer + sizeof(struct iphdr)) )-> msg_type, ch->txn_id1);
        } else if ((ch->msg_type == 3) && (ch->txn_id2 == ((uint16_t) getpid()))) {
            char * pt = buffer + tot_header;
            printf("\nResponse received - %s\n", pt);
        }
    }

    close(sock);
}