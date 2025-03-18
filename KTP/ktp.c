#include "ktp.h"

#define ENOSOCK 1000
#define ENOTBOUND 1001
#define ENOSPACE 1002
#define ENOMESSAGE 1003
#define ELARGEMESSAGE 1004

void semlock(int semid, int semnum) {
    struct sembuf p = {semnum, -1, SEM_UNDO};
    semop(semid, &p, 1);
}

void semunlock(int semid, int semnum) {
    struct sembuf p = {semnum, 1, SEM_UNDO};
    semop(semid, &p, 1);
}

int k_socket(int __domain, int __type, int __protocol) {

    // Check for free space in shared memory
    int sockid = -1;

    // Connect to shared memory
    int shmid = shmget(UNIQUE_NUMBER, MAX_SOCKET_COUNT*sizeof(shmktp), 0777);
    int semid = semget(SEM_UNIQUE_KEY, MAX_SOCKET_COUNT*2, 0777);
    shmktp * addr = (shmktp *) shmat(shmid, NULL, 0);

    // Initialize the shared memory
    for (int i=0; i<MAX_SOCKET_COUNT; i++) {
        semlock(semid, i);
        if (addr[i].isfree == 1) {
            sockid = i;
            break;
        } else {
            semunlock(semid, i);
        }
    }

    // Initialize socket details
    if (sockid > -1) {
        addr[sockid].isfree = 0;
        addr[sockid].proc_id = getpid();
        bzero(&addr[sockid].destination, sizeof(struct sockaddr));
        
        addr[sockid].swnd.window_size = 10;
        addr[sockid].swnd.receive_size = 10;
        addr[sockid].swnd.sb_start = 9;
        addr[sockid].swnd.sb_end = 9;
        addr[sockid].swnd.seq = 1;
        addr[sockid].swnd.retransmit_nospace = 0;

        addr[sockid].rwnd.last_message = 0;
        addr[sockid].rwnd.rb_start = 9;
        addr[sockid].rwnd.rb_end = 9;
        addr[sockid].rwnd.window_size = 10;

        addr[sockid].actual_count = 0;
        addr[sockid].send_count = 0;

        for(int i=0; i<10; i++) {
            addr[sockid].swnd.unack[i] = 0;
            addr[sockid].rwnd.seq_map[i] = 0;
            bzero(&addr[sockid].send_buffer[i], sizeof(addr[sockid].send_buffer[i]));
            bzero(&addr[sockid].receive_buffer[i], sizeof(addr[sockid].receive_buffer[i]));
        }

        gettimeofday(&addr[sockid].last_send, NULL);
        semunlock(semid, sockid);
    } else {
        errno = ENOSOCK;
    }

    shmdt(addr);

    return sockid;
}

int k_bind(int sockid, struct sockaddr_in * source, struct sockaddr_in * destination) {
    int shmid = shmget(UNIQUE_NUMBER, MAX_SOCKET_COUNT*sizeof(shmktp), 0777);
    int bind_shmid = shmget(BIND_SHARE_NUMBER, MAX_SOCKET_COUNT*sizeof(int), 0777);
    int semid = semget(SEM_UNIQUE_KEY, MAX_SOCKET_COUNT*2, 0777);
    
    shmktp * addr = (shmktp *) shmat(shmid, NULL, 0);
    int * bind_addr = (int *) shmat(bind_shmid, NULL, 0);
    
    semlock(semid, sockid);
    semlock(semid, sockid+MAX_SOCKET_COUNT);

    addr[sockid].source = *source;
    addr[sockid].destination = *destination;
    bind_addr[sockid] = getpid();
    
    semunlock(semid, sockid+MAX_SOCKET_COUNT);
    semunlock(semid, sockid);

    while(bind_addr[sockid] == getpid()) usleep(20000);
    int status = bind_addr[sockid];

    shmdt(bind_addr);
    shmdt(addr);
    return status;
}

int k_sendto(int sockid, char message[], int message_len, int flags, struct sockaddr_in * destination, int sock_len) {
    int shmid = shmget(UNIQUE_NUMBER, MAX_SOCKET_COUNT*sizeof(shmktp), 0777);
    int semid = semget(SEM_UNIQUE_KEY, MAX_SOCKET_COUNT*2, 0777);
    shmktp * addr = (shmktp *) shmat(shmid, NULL, 0);
    
    semlock(semid, sockid);
    if (addr[sockid].destination.sin_addr.s_addr != destination->sin_addr.s_addr || addr[sockid].destination.sin_port != destination->sin_port) {
        errno = ENOTBOUND;
        shmdt(addr);
        semunlock(semid, sockid);
        return -1;
    }

    if (addr[sockid].swnd.window_size == 0) {
        errno = ENOSPACE;
        shmdt(addr);
        semunlock(semid, sockid);
        return -1;
    }

    if (strlen(message) > 512) {
        errno = ELARGEMESSAGE;
        shmdt(addr);
        semunlock(semid, sockid);
        return -1;
    }

    int index = (addr[sockid].swnd.sb_end + 1)%10;
    addr[sockid].swnd.sb_end = index;
    bzero(&addr[sockid].send_buffer[index], MESSAGE_SIZE);
    strcpy(addr[sockid].send_buffer[index], message);
    addr[sockid].swnd.window_size--;
    addr[sockid].swnd.unack[index] = 1;
    addr[sockid].actual_count++;

    shmdt(addr);
    semunlock(semid, sockid);
    return message_len;
}

int k_recvfrom(int sockid, char * buff, int len, int flags, struct sockaddr_in * destination, int size) {
    int shmid = shmget(UNIQUE_NUMBER, MAX_SOCKET_COUNT*sizeof(shmktp), 0777);
    int semid = semget(SEM_UNIQUE_KEY, MAX_SOCKET_COUNT*2, 0777);
    shmktp * addr = (shmktp *) shmat(shmid, NULL, 0);

    semlock(semid, sockid);
    
    if (addr[sockid].rwnd.window_size == 10) {
        errno = ENOMESSAGE;
        shmdt(addr);
        semunlock(semid, sockid);
        return -1;
    }
    
    int index = (addr[sockid].rwnd.rb_start + 1)%10;
    addr[sockid].rwnd.rb_start = index;
    addr[sockid].rwnd.window_size++;
    
    strcpy(buff, addr[sockid].receive_buffer[index]);

    bzero(addr[sockid].receive_buffer[index], MESSAGE_SIZE);

    shmdt(addr);
    semunlock(semid, sockid);
    return 0;
}

void k_close(int sockid) {
    int shmid = shmget(UNIQUE_NUMBER, MAX_SOCKET_COUNT*sizeof(shmktp), 0777);
    int bind_shmid = shmget(BIND_SHARE_NUMBER, MAX_SOCKET_COUNT*sizeof(int), 0777);
    int semid = semget(SEM_UNIQUE_KEY, MAX_SOCKET_COUNT*2, 0777);

    int * bind_addr = (int *) shmat(bind_shmid, NULL, 0);
    shmktp * addr = shmat(shmid, NULL, 0);

    semlock(semid, sockid+MAX_SOCKET_COUNT);

    bind_addr[sockid] = -10;

    semunlock(semid, sockid+MAX_SOCKET_COUNT);

    shmdt(addr);
    shmdt(bind_addr);
}