/*
 * mysr.h
 */

#ifndef __mysr_h__
#define __mysr_h__

#define MAX_PAYLOAD_SIZE 512

#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

struct MYSR_Packet {
  unsigned char protocol[2];             // protocol string (2 bytes) "sr"
  unsigned char type;                    // type (1 byte)
  unsigned int seqNum;                   // sequence number (4 bytes)
  unsigned int length;                   // length(header+payload) (4 bytes)
  unsigned char payload[MAX_PAYLOAD_SIZE];    // payload data
} __attribute__((packed));

struct mysr_sender {
  int sd; // SR sender socket
  int window_size;
  struct sockaddr_in server_addr;
  struct timeval tv;
  // ... other member variables
};

void mysr_init_sender(struct mysr_sender* mysr_sender, char* ip, int port, int N, int timeout);
int mysr_send(struct mysr_sender* mysr_sender, unsigned char* buf, int len);
void mysr_close_sender(struct mysr_sender* mysr_sender);

struct mysr_receiver {
  int sd; // SR receiver socket
  int window_size;
  struct sockaddr_in address;
  int port;
  // ... other member variables
};

void mysr_init_receiver(struct mysr_receiver* mysr_receiver, int port);
int mysr_recv(struct mysr_receiver* mysr_receiver, unsigned char* buf, int len);
void mysr_close_receiver(struct mysr_receiver* mysr_receiver);

#endif
