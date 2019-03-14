#include "mysr.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

void mysr_init_sender(struct mysr_sender* mysr_sender, char* ip, int port, int N, int timeout){
        mysr_sender->sd = socket(AF_INET, SOCK_DGRAM, 0);

        memset(&(mysr_sender->server_addr), 0, sizeof(mysr_sender->server_addr));
        (mysr_sender->server_addr).sin_family = AF_INET;
        inet_pton(AF_INET, ip, &(mysr_sender->server_addr.sin_addr));
        mysr_sender->server_addr.sin_port=htons(port);

        mysr_sender->tv.tv_sec = timeout;
        mysr_sender->tv.tv_usec = 0;
        mysr_sender->window_size = N;
}

int mysr_send(struct mysr_sender* mysr_sender, unsigned char* buf, int len){
        //Data pack
        struct MYSR_Packet data;
        data.protocol[0] = 's';
        data.protocol[1] = 'r';
        data.type = 0xA0;
        data.seqNum = 0;
        data.length = sizeof(data) + len;

        struct MYSR_Packet end;
        end.protocol[0] = 's';
        end.protocol[1] = 'r';
        end.type = 0xA2;
        end.seqNum = 0;
        end.length = sizeof(end);

        struct MYSR_Packet ACK;

        int return_size = 0;
        int acked_packet = 0;
        int base = 0;
        int window_end = base + mysr_sender->window_size - 1;
        int packetNum = len;
        if (window_end > packetNum){
                window_end = packetNum;
        }
        if (mysr_sender->window_size > packetNum){
                mysr_sender->window_size = packetNum - 1;
        }
        int *ACKed = (int *)malloc(sizeof(int) * packetNum);
        int *sent = (int *)malloc(sizeof(int) * packetNum);
        memset(ACKed, 0, sizeof(int));
        memset(sent, 0, sizeof(int));
        int size = sizeof(mysr_sender->server_addr);
        int i;
        //Send base + window_size - 1 packets
        for(i = 0; i <= window_end; i++){
                //send file
                data.seqNum = i;
                sendto(mysr_sender->sd, &data, sizeof(data), 0, &(mysr_sender->server_addr), sizeof(mysr_sender->server_
addr));
                sendto(mysr_sender->sd, (unsigned char *)&buf[data.seqNum], sizeof(buf[data.seqNum]), 0, &(mysr_sender->
server_addr), sizeof(mysr_sender->server_addr));
                sent[i] = 1;
        }

        while(acked_packet < packetNum){
                //Set timer for the base
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(mysr_sender->sd, &fds);
                int ready;
                struct timeval timeout;
                timeout.tv_sec = mysr_sender->tv.tv_sec;
                timeout.tv_usec = 0;

                if((ready = select((mysr_sender->sd) + 1, &fds, NULL, NULL, &timeout)) < 0){
                        printf("%s",strerror(errno));
                }
                if(ready>0){
                        //recv ACK
                        if(recvfrom(mysr_sender->sd, (struct MYSR_Packet *)&ACK, sizeof(ACK), 0, &(mysr_sender->server_a
ddr), &size) < 0){

                        }else{
                                ACKed[ACK.seqNum] = 1;
                                acked_packet++;
                        }

                        //Move the window
                        if (base == ACK.seqNum){
                                if(window_end > packetNum){
                                        window_end = packetNum - 1;
                                }
                                int window_full = 1;
                                for(i = base; i <= window_end; i++){
                                        if (ACKed[i] == 0){
                                                base = i;
                                                window_end = base + mysr_sender->window_size - 1;
                                                if (window_end > packetNum){
                                                        window_end = packetNum - 1;
                                                }
                                                window_full = 0;
                                                break;
                                        }
                                }
                                if (window_full == 1){
                                        base = window_end + 1;
                                        window_end = base + mysr_sender->window_size - 1;
                                        if (window_end > packetNum){
                                                window_end = packetNum;
                                        }
                                        if (base >= packetNum){
                                                base = packetNum;
                                        }
                                }
                        }
                        //send file
                        for (i = base; i <= window_end; i++){
                                data.seqNum = i;
                                if(sent[i] == 0 && window_end < packetNum){
                                        sendto(mysr_sender->sd, (struct MYSR_Packet *)&data, sizeof(data), 0, &(mysr_sen
der->server_addr), sizeof(mysr_sender->server_addr));
                                        sendto(mysr_sender->sd, (unsigned char *)&(buf[data.seqNum]), sizeof(buf[data.se
qNum]), 0, &mysr_sender->server_addr, sizeof(mysr_sender->server_addr));
                                        sent[i] = 1;
                                }
                        }

                }else{
                        //timeout
                        //send again
                        data.seqNum = base;
                        sendto(mysr_sender->sd, (struct MYSR_Packet *)&data, sizeof(data), 0, &(mysr_sender->server_addr
), sizeof(mysr_sender->server_addr));
                        sendto(mysr_sender->sd, (unsigned char *)&(buf[data.seqNum]), sizeof(buf[data.seqNum]), 0, &(mys
r_sender->server_addr), sizeof(mysr_sender->server_addr));
                        sent[i] = 1;
                }
        }

        struct MYSR_Packet recv_end;
        int isend = 0;
        sendto(mysr_sender->sd, (struct MYSR_Packet *)&end, sizeof(end), 0, &(mysr_sender->server_addr), sizeof(mysr_sen
der->server_addr));
        while(1){
                end.seqNum = 0;
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(mysr_sender->sd, &fds);
                int ready;
                struct timeval timeout;
                timeout.tv_sec = mysr_sender->tv.tv_sec;
                timeout.tv_usec = 0;
                if((ready = select((mysr_sender->sd) + 1, &fds, NULL, NULL, &timeout)) < 0){
                        printf("%s",strerror(errno));
                }
                if(FD_ISSET(mysr_sender->sd, &fds)){
                        recvfrom(mysr_sender->sd, (struct MYSR_Packet *)&recv_end, sizeof(recv_end), 0, &(mysr_sender->s
erver_addr), &size);
                        return return_size;
                }else{
                        if (isend > 3){
                                return 0;
                        }
                        isend++;
                        sendto(mysr_sender->sd, (struct MYSR_Packet *)&end, sizeof(end), 0, &(mysr_sender->server_addr),
 sizeof(mysr_sender->server_addr));
                }
        }
}

void mysr_close_sender(struct mysr_sender* mysr_sender){
        struct MYSR_Packet end;
        end.protocol[0] = 's';
        end.protocol[1] = 'r';
        end.type = 0xA2;
        end.seqNum = 1;
        end.length = sizeof(end);

        struct MYSR_Packet ACK;
        int sent = 0;
        sendto(mysr_sender->sd, (struct MYSR_Packet *)&end, sizeof(end), 0, &(mysr_sender->server_addr), sizeof(mysr_sen
der->server_addr));
        sent++;
        while(1){
                end.seqNum = 1;
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(mysr_sender->sd, &fds);
                struct timeval timeout;
                timeout.tv_sec = mysr_sender->tv.tv_sec;
                timeout.tv_usec = 0;

                select((mysr_sender->sd) + 1, &fds, NULL, NULL, &timeout);

                struct MYSR_Packet recv_end;
                if(FD_ISSET(mysr_sender->sd, &fds)){
                        recvfrom(mysr_sender->sd, (struct MYSR_Packet *)&recv_end, sizeof(recv_end), 0, &(mysr_sender->s
erver_addr), sizeof(mysr_sender->server_addr));
                        close(mysr_sender->sd);
                        break;
                }else{
                        if(sent >= 3){
                                printf("Error: No ACK for EndPacket.\n");
                                close(mysr_sender->sd);
                                break;
                        }
                        sendto(mysr_sender->sd, (struct MYSR_Packet *)&end, sizeof(end), 0, &(mysr_sender->server_addr),
 sizeof(mysr_sender->server_addr));
                        sent++;
                }
        }

}

void mysr_init_receiver(struct mysr_receiver* mysr_receiver, int port){
        mysr_receiver->sd = socket(AF_INET, SOCK_DGRAM, 0);

        memset(&(mysr_receiver->address), 0, sizeof(mysr_receiver->address));
        mysr_receiver->address.sin_family = AF_INET;
        mysr_receiver->address.sin_port = htons(12345);
        mysr_receiver->address.sin_addr.s_addr = htonl(INADDR_ANY);

        mysr_receiver->port = port;

        bind(mysr_receiver->sd, (struct sockaddr *)&(mysr_receiver->address), sizeof(struct sockaddr));
}

int mysr_recv(struct mysr_receiver* mysr_receiver, unsigned char* buf, int len){
        //Data pack
        struct MYSR_Packet ACK;
        ACK.protocol[0] = 's';
       ACK.type = 0xA1;
        ACK.seqNum = 0;
        ACK.length = sizeof(ACK);

        struct MYSR_Packet end;
        end.protocol[0] = 's';
        end.protocol[1] = 'r';
        end.type = 0xA2;
        end.seqNum = 0;
        end.length = sizeof(end);

        int return_size = 0;
        int tmp;
        struct MYSR_Packet data;
        int packet_size = 0;
        int address_size = sizeof(mysr_receiver->address);

        int isend = 0;
        while (1){
                if((tmp = recvfrom(mysr_receiver->sd, (struct MYSR_Packet*)&data, sizeof(struct MYSR_Packet), 0, (struct
 sockaddr *)&mysr_receiver->address, &address_size)) >= 0){
                        if (data.type == 0xA0){
                                if ((tmp = recvfrom(mysr_receiver->sd, (unsigned char *)&buf[data.seqNum], sizeof(buf[da
ta.seqNum]), 0 ,(struct sockaddr *)&mysr_receiver->address, &address_size)) < 0 ){
                                        printf("%s\n", strerror(errno));
                                }else{
                                        return_size +=tmp;
                                        ACK.seqNum = data.seqNum;

                                        sendto(mysr_receiver->sd, (struct MYSR_Packet *)&ACK, sizeof(ACK), 0, (struct so
ckaddr *)&mysr_receiver->address, sizeof(mysr_receiver->address));
                                }
                        }else if ((data.type == 0xA2) && (data.seqNum == 0)){
                                sendto(mysr_receiver->sd, (struct MYSR_Packet *)&end, sizeof(end), 0 ,(struct sockaddr *
)&mysr_receiver->address, address_size);
                                return return_size;
                        }else if ((data.type == 0xA2) && (data.seqNum == 1)){
                                sendto(mysr_receiver->sd, (struct MYSR_Packet *)&ACK, sizeof(ACK), 0 ,(struct sockaddr *
)&mysr_receiver->address, address_size);

                                memset(&(mysr_receiver->address), 0, sizeof(mysr_receiver->address));
                                mysr_receiver->address.sin_family = AF_INET;
                                mysr_receiver->address.sin_port = htons(12345);
                                mysr_receiver->address.sin_addr.s_addr = htonl(INADDR_ANY);

                                bind(mysr_receiver->sd, (struct sockaddr *)&(mysr_receiver->address), sizeof(struct sock
addr));
                        }
                }
        }
}

void mysr_close_receiver(struct mysr_receiver* mysr_receiver) {
                close(mysr_receiver->sd);
}