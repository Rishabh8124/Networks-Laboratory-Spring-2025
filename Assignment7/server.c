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
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h> 
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

int sock;
uint16_t pid;

void * announcer_thread() {

    int tot_header = sizeof(struct iphdr) + sizeof(struct cldp_header);
    char packet[1024];
    struct iphdr *ip = (struct iphdr *)packet;
    struct cldp_header *ch = (struct cldp_header *) (packet + sizeof(struct iphdr));
    bzero(packet + tot_header, sizeof(packet) - (tot_header));

    ip->version = 4;
    ip->ihl = (sizeof(struct iphdr))/4;
    ip->ttl = 64;
    ip->protocol = 253;  // Custom protocol
    ip->saddr = inet_addr(IP_SELF);
    ip->daddr = inet_addr("255.255.255.255");
    ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

    ch->msg_type = 1;
    ch->payload_len = 0;
    ch->txn_id1 = (uint16_t) pid;

    struct sockaddr_in dst;
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = ip->daddr;

    while(1) {
        sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dst, sizeof(dst));
        sleep(5);
    }

    pthread_exit(NULL);
}

void get_sysinfo_as_string(char *buffer, size_t buffer_size) {
    struct sysinfo info;

    // Get system information
    if (sysinfo(&info) != 0) {
        snprintf(buffer, buffer_size, "Error retrieving system info.");
        return;
    }

    // Uptime (convert seconds to days, hours, minutes, seconds)
    int days = info.uptime / 86400;
    int hours = (info.uptime % 86400) / 3600;
    int minutes = (info.uptime % 3600) / 60;
    int seconds = info.uptime % 60;

    // Format system information into the buffer
    snprintf(buffer, buffer_size,
        "System Uptime: %d days, %d hours, %d minutes, %d seconds\n"
        "Load Averages: 1 min: %.2f, 5 min: %.2f, 15 min: %.2f\n"
        "Total RAM: %.2f MB\n"
        "Free RAM: %.2f MB\n"
        "Shared RAM: %.2f MB\n"
        "Buffered RAM: %.2f MB\n"
        "Total Swap: %.2f MB\n"
        "Free Swap: %.2f MB\n"
        "Number of Processes: %d",
        days, hours, minutes, seconds,
        info.loads[0] / 65536.0,
        info.loads[1] / 65536.0,
        info.loads[2] / 65536.0,
        info.totalram * info.mem_unit / (1024.0 * 1024),
        info.freeram * info.mem_unit / (1024.0 * 1024),
        info.sharedram * info.mem_unit / (1024.0 * 1024),
        info.bufferram * info.mem_unit / (1024.0 * 1024),
        info.totalswap * info.mem_unit / (1024.0 * 1024),
        info.freeswap * info.mem_unit / (1024.0 * 1024),
        info.procs);
}

int main() {
    sock = socket(AF_INET, SOCK_RAW, 253);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));
    
    // Announce
    pid = getpid();

    pthread_t announce;
    pthread_create(&announce, NULL, announcer_thread, NULL);
    
    char buffer[1024];
    struct sockaddr_in src;
    socklen_t len = sizeof(src);

    int tot_header = sizeof(struct iphdr) + sizeof(struct cldp_header);
    
    int bytes;
    
    while (1) {
        bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&src, &len);

        struct iphdr *ip = (struct iphdr *)buffer;
        struct cldp_header *ch = (struct cldp_header *) (buffer + sizeof(struct iphdr));
        char *payload = buffer + ip->ihl * 4 + sizeof(struct cldp_header);
        
        if (ch->msg_type != 2 || ch->txn_id1 != pid) continue;
        
        printf("Received QUERY from - %s\n", inet_ntoa(src.sin_addr));
        
        if (strcmp(payload, "CPU LOAD") == 0) {
            bzero(buffer + tot_header, sizeof(buffer) - (tot_header));
            get_sysinfo_as_string(payload, sizeof(buffer) - (tot_header));
        } else if (strcmp(payload, "SYSTEM TIME") == 0) {
            bzero(buffer + tot_header, sizeof(buffer) - (tot_header));
            
            struct timeval tv;
            struct tm *tm_info; 
            gettimeofday(&tv, NULL);
            
            tm_info = localtime(&tv.tv_sec);
            strftime(payload, sizeof(buffer) - (tot_header), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            bzero(buffer + tot_header, sizeof(buffer) - (tot_header));
            gethostname(buffer + tot_header, sizeof(buffer) - (tot_header));
        }
        
        ch->msg_type = 3;
        ip->daddr = ip->saddr;
        ip->saddr = inet_addr(IP_SELF);
        ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

        struct sockaddr_in dst;
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = ip->daddr;

        ch->payload_len = strlen(payload);

        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dst, sizeof(dst));

        // char packet[1024];
        // ip = (struct iphdr *)packet;
        // strcpy(packet + sizeof(struct iphdr), "Hello!");
        // ip->version = 4;
        // ip->ihl = (sizeof(struct iphdr) + sizeof(struct cldp_header))/4;
        // ip->ttl = 64;
        // ip->protocol = 253;  // Custom protocol
        // // ip->saddr = inet_addr("10.145.42.126");
        // // ip->daddr = inet_addr("10.145.42.126");
        // ip->saddr = inet_addr("127.0.0.1");
        // ip->daddr = inet_addr("127.0.0.1");

        // struct sockaddr_in dst;
        // dst.sin_family = AF_INET;
        // dst.sin_addr.s_addr = ip->daddr;

        // sendto(sock, packet, sizeof(struct iphdr) + strlen("Hello!"), 0, (struct sockaddr *)&dst, sizeof(dst));
    }

    close(sock);
}