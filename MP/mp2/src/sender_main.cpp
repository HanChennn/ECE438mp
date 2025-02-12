#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <queue>
#include <cmath>
#include <iostream>
#define MSS 1000

#define RTT 20000
#define MAX_Seq 100000
#define BUFFER_SIZE 512

#define SLOW_START 0
#define CONGESTION_AVOID 1
#define FAST_RECOVERY 2
#define FIN_WAIT 3

using namespace std;

struct sockaddr_in si_other;
int s, slen;
FILE* fp;
int flag_time_out, flag_new_ack;

float cwnd;                         
int ssthread, count_duplicate_acks;                 // maintain the # of duplicated acks
unsigned long long int remain_byte;
int congestion_state;                          // 0 - slow start, 1 - congestion avoid, 2 - fast recovery
int sequence_num;
int state;             

typedef struct packet{
    int data_size, seq_num, ack_num, msg_type;  // 0 - DATA, 1 - ACK, 2 - FIN, 3 - FINACK
    char data[MSS];
    void update(int data_size_, int seq_num_, int ack_num_, int msg_type_){
        data_size = data_size_;
        seq_num = seq_num_;
        ack_num = ack_num_;
        msg_type = msg_type_;
    }
}Packet;

queue<Packet> buf_queue;         // buffer for pkgs that have read but not yet send
queue<Packet> wait_queue;        // buffer for pkgs that have sent and waiting for ACKs


void diep(char *s) {perror(s);exit(1);}

void read_data(){
    Packet seg;
    int num = 0;
    if(remain_byte < (BUFFER_SIZE - buf_queue.size())* MSS){
        num =  (remain_byte-1) / MSS + 1;
    }else{
        num = BUFFER_SIZE - buf_queue.size();
    }

    int bytes_to_read, out;
    char buf[MSS];
    for(int i=0; i<num || remain_byte>0; i++){
        bytes_to_read = (remain_byte>MSS)?MSS:remain_byte;

        memset(seg.data, 0, MSS);
        memset(buf, 0, MSS);
        if((out = fread(buf, sizeof(char), bytes_to_read, fp)) < 0){
            // printf("fread error.");
            fprintf(stderr, "fread error in read_data\n");
            exit(1);
        }
        memcpy(seg.data, buf, MSS);
        seg.update(out, sequence_num, seg.ack_num, 0);
        // seg.data_size = out;
        // seg.msg_type = 0;
        // seg.seq_num = sequence_num;
        buf_queue.push(seg);
        sequence_num += out;
        remain_byte -= out;
    }
    return;
}

void send_data(){
    Packet seg;
    int max_send, num_sent;
    if( (cwnd - wait_queue.size()*MSS) > (buf_queue.size()*MSS) ) max_send = ceil( (cwnd - wait_queue.size()*MSS)/MSS );
    else max_send = buf_queue.size();

    for(int i=0; i < max_send && buf_queue.empty()==false; i++){
        seg = buf_queue.front();
        if( (num_sent = sendto(s, &seg, sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
            fprintf(stderr, "send data error\n");
            exit(1);
        }
        wait_queue.push(seg);
        buf_queue.pop();
    }
    if(remain_byte>0) read_data();
    return;
}

void handle_timeout(){
    ssthread = (cwnd/2 >= 4096*MSS)?cwnd/2:4096*MSS;
    cwnd = MSS;
    count_duplicate_acks = 0;
    flag_time_out = 0;

    queue<Packet> temp_queue = wait_queue;
    int sent, queue_size;

    queue_size = (BUFFER_SIZE/4 > wait_queue.size())? wait_queue.size()/2:BUFFER_SIZE/4;
    for(int i=0; i<queue_size; i++){
        if( (sent = sendto(s, &(temp_queue.front()), sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
            fprintf(stderr, "send data error\n");
            exit(1);
        }
        temp_queue.pop();
    }
    return;
}

void duplicate(){
    ssthread = cwnd / 2;
    if (ssthread < MSS) ssthread = MSS;
    cwnd = ssthread + 3*MSS;
    count_duplicate_acks = 0;
    state = FAST_RECOVERY;
    int sent;
    if( (sent = sendto(s, &(wait_queue.front()), sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
        fprintf(stderr, "send data error\n");
        exit(1);
    }
    printf("CONGESTION: ? -> fast_recovery\n");
    return;
}

void handle_congestion(){
    switch(state){
        case 0:
            if(flag_time_out){handle_timeout();return;}
            if(flag_new_ack){
                count_duplicate_acks = 0;
                cwnd += MSS;
                if(cwnd >= ssthread){
                    state = CONGESTION_AVOID;
                    printf("CONGESTION: slow_start -> congestion_avoidance\n");
                }
                flag_new_ack = 0;
            }else{
                if((++count_duplicate_acks) ==3)duplicate();
            }
            break;
        case 1:
            if(flag_time_out){
                handle_timeout(); state = SLOW_START;
                printf("CONGESTION: congestion_avoidance -> slow_start\n");
                return;
            }
            if(flag_new_ack){
                cwnd += floor(MSS*1.0 / cwnd)*MSS;
                count_duplicate_acks = 0;
                flag_new_ack = 0;
            }else{
                if((++count_duplicate_acks) ==3)duplicate();
            }
            break;
        case 2:
            if(flag_time_out){
                handle_timeout(); state = SLOW_START;
                printf("CONGESTION: fast_recovery -> slow_start\n");
                return;
            }
            if(flag_new_ack){
                cwnd = ssthread;
                count_duplicate_acks = 0;
                flag_new_ack = 0;
                state = CONGESTION_AVOID;
                printf("CONGESTION: fast_recovery -> congestion_avoidance\n");
            }else cwnd += MSS;
            break;
        default:
            break;
    }return;
}

void end_connection(){
    Packet end, recv;
    int sent, num_recv;
    // end.msg_type = 2;
    // end.data_size = 0;
    end.update(0, end.seq_num, end.ack_num, 2);
    memset(end.data, 0, MSS);
    if( (sent = sendto(s, &(end), sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
        fprintf(stderr, "FIN send data error\n");
        exit(1);
    }
    while(1){
        if( (num_recv = recvfrom(s, &recv, sizeof(Packet), 0, (struct sockaddr*)&si_other, (socklen_t*)&slen)) == -1){
            if(errno != EAGAIN || errno != EWOULDBLOCK){
                perror("can not receive FINACK\n");
                exit(1);
            }else{
                printf("FINACK recv timeout\n");
                if( (sent = sendto(s, &(end), sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
                    fprintf(stderr, "FIN send data error\n");
                    exit(1);
                }
            }
        }else{
            if(recv.msg_type == 3){
                printf("receive FINACK\n");
                break;
            }
        }
    }return;
}

void handle_queue(){
    Packet temp;
    int num_send, num_recv;
    while(buf_queue.empty()==false || wait_queue.empty()==false){
        if( (num_recv = recvfrom(s, &temp, sizeof(Packet), 0, (struct sockaddr*)&si_other, (socklen_t*)&slen)) == -1){
            printf("recvfrom fail\n");
            if(errno != EAGAIN || errno != EWOULDBLOCK){
                perror("can not receive ack\n");
                exit(2);
            }else{
                printf("resend package, sequence_num = %d",wait_queue.front().seq_num);
                if( (num_send = sendto(s, &(wait_queue.front()), sizeof(Packet), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1){
                    fprintf(stderr, "send data error\n");
                    exit(1);
                }
                flag_time_out = 1;
                handle_congestion();
            }
        }else{
            if(temp.msg_type == 1){  //type 1 = ACK
                if(temp.ack_num == wait_queue.front().seq_num) handle_congestion();
                else{
                    while(wait_queue.empty()==false && wait_queue.front().seq_num < temp.ack_num){
                        flag_new_ack = 1;
                        handle_congestion();
                        wait_queue.pop();
                    }
                    if(remain_byte > 0){read_data();send_data();}
                }
            }
        }
    }return;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.\n");
        exit(1);
    }

	/* Determine how many bytes to transfer */
    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


	/* Send data and receive acknowledgements on s*/
    ////////////////////
    ssthread = 4096 * MSS;
    cwnd = MSS;
    sequence_num = 0;
    count_duplicate_acks = 0;
    state = SLOW_START;
    remain_byte = bytesToTransfer;

    ////////////////////
    struct timeval RTT_TO;
    RTT_TO.tv_sec = 0;
    RTT_TO.tv_usec = RTT;
    if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &RTT_TO, sizeof(RTT_TO))<0){
        fprintf(stderr, "Error setting socket timeout\n");
        exit(1);
    }

    read_data();
    send_data();
    handle_queue();
    end_connection();

    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}


