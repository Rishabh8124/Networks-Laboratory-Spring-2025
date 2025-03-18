// Name - S.Rishabh
// Roll number - 22CS10058

#ifndef KTP_STRUCTURE
#define KTP_STRUCTURE

#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SOCK_KTP                100
#define UNIQUE_NUMBER           728
#define BIND_SHARE_NUMBER       725
#define SEM_UNIQUE_KEY          728725
#define MAX_SOCKET_COUNT        5
#define MESSAGE_SIZE            512
#define TIMEOUT_HALF            100000
#define BIND_TIMEOUT            10000

// Timeout for sending thread is 200 ms, if 5 seconds need to be implemented, change the macro TIMEOUT_HALF to 2500000

// swnd structure
struct swnd {
    int unack[10];
    int receive_size;
    int window_size;
    int retransmit_nospace;
    int seq;
    int sb_start;
    int sb_end;
};

typedef struct swnd sswnd;

// rwnd structure
struct rwnd {
    int seq_map[10];
    int window_size;
    int last_message;
    int rb_start;
    int rb_end;
};

typedef struct rwnd rrwnd;

// Shared memory data structure
struct shared_memory_ktp {
    int isfree;
    int proc_id;
    int sockfd;
    struct sockaddr_in source;
    struct sockaddr_in destination;
    char send_buffer[10][MESSAGE_SIZE];    
    char receive_buffer[10][MESSAGE_SIZE]; 
    sswnd swnd;
    rrwnd rwnd;
    struct timeval last_send;
    struct timeval close_start;

    int actual_count;
    int send_count;
};

typedef struct shared_memory_ktp shmktp;

#endif