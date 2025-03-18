#include "ktp_structures.h"
#include "errno.h"

#ifndef KGP_TRANSFER_PROTOCOL
#define KGP_TRANSFER_PROTOCOL

int k_socket(int __domain, int __type, int __protocol);
int k_bind(int sockid, struct sockaddr_in * source, struct sockaddr_in * destination);
int k_sendto(int sockid, char message[], int message_len, int flags, struct sockaddr_in * destination, int sock_len);
int k_recvfrom(int sockid, char * buff, int len, int flags, struct sockaddr_in * destination, int size);
void k_close(int sockid);

#endif