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

#define DATAMAXSIZE 1400
#define MAX_BUFSIZE 1000 * 1400

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

/*
    *  Define the segment structure
    *  seq_num: the sequence number of the segment
    *  ack_num: the ack number of the segment
    *  type: the type of the segment
    *  length: the length of the segment
    *  data: the data of the segment
    
    // define the type of segment
    // 0: data segment; 1: ACK segment; 2: FIN segment; 3: FINACK segment

*/

typedef struct segment {
    int length;
    int seq_num;
    int ack_num;
    int type;
    char data[DATAMAXSIZE];

    void update(int seq, int ack, int t, int l){
        seq_num = seq;
        ack_num = ack;
        type = t;
        length = l;
    }
}segment_t;

void UpdateBuffer(char * buffer_receive, short* buffer_record, char * data, int record, int length, int seq_num, FILE * fp){
    for (int i = 0; i < length; i++){
        int index = (seq_num + i) % MAX_BUFSIZE;
        buffer_receive[index] = data[i];
        buffer_record[index] = record;
        if (record == 0 && fp != NULL)
            fwrite(&buffer_receive[index], sizeof(char), 1, fp);
    }
    return;
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep((char *)"socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep((char *)"bind");


	/* Now receive data and send acknowledgements */  
    // open the file
    FILE * fp;
    if ((fp = fopen(destinationFile, "ab")) == NULL){
        diep((char *)"file");
    }
    // set the asking segment
    segment_t * asking_s = (segment_t *)malloc(sizeof(segment_t));

    // set buffers and intialize them
    // 0-> not received; 1-> received
    char buffer_receive[MAX_BUFSIZE];
    short buffer_record[MAX_BUFSIZE];
    for (int i = 0; i < MAX_BUFSIZE; i++){
        buffer_receive[i] = '\0';
        buffer_record[i] = 0;
    }
    // memset(buffer_receive, '\0', MAX_BUFSIZE);
    // memset(asking_s->data, 0, MAX_BUFSIZE);

    int last_asking = 0;
    int index = 0;

    // receive the data
    while(1){
        segment_t * recv_s   = (segment_t *)malloc(sizeof(segment_t));
        if (recvfrom(s, recv_s, sizeof(segment_t), 0, (struct sockaddr*)&si_other, (socklen_t *)&slen) < 0){
            diep("Receive ack");
        }
        
        // if it is data type
        if (recv_s->type == 0){

            // check if it is the one we are asking for
            if (recv_s->seq_num == last_asking){
                // for (int i = 0; i < recv_s->length; i++){
                //     // set index
                //     index = (recv_s->seq_num + i) % MAX_BUFSIZE;
                //     buffer_receive[index] = recv_s->data[i];
                //     buffer_record[index] = 0;
                //     fwrite(&buffer_receive[index], sizeof(char), 1, fp);
                // }
                UpdateBuffer(buffer_receive, buffer_record, recv_s->data, 0, recv_s->length, recv_s->seq_num, fp);
                
                // update the last_asking
                last_asking = recv_s->seq_num + recv_s->length;
                
                // check if there are some data in the buffer
                index = last_asking % MAX_BUFSIZE;
                while(buffer_record[index] == 1){
                    buffer_record[index] = 0;
                    fwrite(&buffer_receive[index], sizeof(char), 1, fp);
                    last_asking++;
                    index = last_asking % MAX_BUFSIZE;
                }
            }   
            // if it is out of order, put it in the buffer, but do not update the last_asking
            else if (recv_s->seq_num > last_asking){
                // for (int i = 0; i < recv_s->length; i++){
                //     index = (recv_s->seq_num + i) % MAX_BUFSIZE;
                //     buffer_receive[index] = recv_s->data[i];
                //     buffer_record[index] = 1;
                // }  
                UpdateBuffer(buffer_receive, buffer_record, recv_s->data, 1, recv_s->length, recv_s->seq_num, NULL);
            }
            // if it is a duplicate, do nothing
            else{
                continue;
            }

            // sending ack back, asking for the next segment
            // update the asking segment
            asking_s->update(0, last_asking, 1, 0);

            // send the ack
            if (sendto(s, asking_s, sizeof(segment_t), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1){
                diep("send ack");
            }
        }

        // if it is fin type 
        else if (recv_s -> type == 2){
            segment_t * finack_s = (segment_t *)malloc(sizeof(segment_t));
            // update the finack segment
            finack_s->update(0, 0, 3, 0);
            if (sendto(s, finack_s, sizeof(segment_t), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1){
                diep((char *)"Send");
            }

            fclose(fp);
            break;
        }
    }
    
    close(s);
	printf("%s received.", destinationFile);
    return;
}


int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

