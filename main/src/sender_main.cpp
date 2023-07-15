/* 
 * File:   receiver_main.cpp
 * Author: yifeis7
           jiahaow4
 *
 * Created on 2022.10.5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
#include <cerrno>
#include "mytcp.h"

#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

void diep(char *s) {
    perror(s);
    exit(1);
}


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
	/* Get destination address */
    struct sockaddr_in si_other;
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

	/* Open the file */
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

	/* Send data and receive acknowledgements */
    // set recefrom timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 30000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    // send packets and receive ACKs
    int total_pack_num = bytesToTransfer/DATA_SIZE + ((bytesToTransfer % DATA_SIZE) > 0);
    struct seg window[WINDOW_SIZE], *seg_ptr;
    int base = 0, old_base = 0;
    int end = 0;
    int packsToRead = MIN(total_pack_num, WINDOW_SIZE);
    struct ack recvbuf;
    int data_len;
    int bytes_read;
    int recv_num;
    int i, n;
    int resend;
    int sent_pack_end = base;
    // var for FSM
    int cwnd = 8;
    int ssthresh = 64;
    int state = 0; // 0 - slow start, 1 - congestion avoidance
    int timeout = 0;
    while (base < total_pack_num) {
	    // load file content into window and set segment header
	    for (i = 0; i < packsToRead; i++) {
		    if ((end + i) == total_pack_num - 1 && (bytesToTransfer % DATA_SIZE) > 0) // the last packet
			    data_len = bytesToTransfer % DATA_SIZE;
		    else
			    data_len = DATA_SIZE;
		    seg_ptr = window + ((end + i) % WINDOW_SIZE);
		    bytes_read = fread(seg_ptr->data, 1, data_len, fp);
		    if (feof(fp)) {
			packsToRead = i + 1;
			data_len = bytes_read;
			total_pack_num = end + packsToRead;
			// puts("End-of-File reached.\n");
		    }
		    seg_ptr->len = data_len;
		    seg_ptr->seq = end + i;
		    if ((end + i) == total_pack_num - 1)
			    seg_ptr->eot = 1;
		    else
			    seg_ptr->eot = 0;
	    }
	    end = end + packsToRead;

	    resend = 0;
	    if (base < sent_pack_end) {
		    resend = 1;
		    sendto(sockfd, window + (base % WINDOW_SIZE), sizeof *seg_ptr, 0, (sockaddr *)&si_other, sizeof si_other);
		    printf("resend pack %d\n", base);
	    }
	    for (i = sent_pack_end; i < MIN(base + cwnd, end); i++) {
		    sendto(sockfd, window + (i % WINDOW_SIZE), sizeof *seg_ptr, 0, (sockaddr *)&si_other, sizeof si_other);
		    // printf("send pack %d\n", i);
	    }
	    old_base = base;
	    n = MAX(0, MIN(base + cwnd, end) - sent_pack_end);
	    sent_pack_end = MAX(sent_pack_end, MIN(base + cwnd, end));
	    timeout = 0;
	    for (i = 0; i < n + resend; i++) {
	            recv_num = recvfrom(sockfd, &recvbuf, sizeof recvbuf, 0, NULL, 0);
		    if (recv_num < 0 && errno == EWOULDBLOCK) {
			    timeout = 1;
			    puts("time out");
			    break;
		    }
		    if (recvbuf.seq > base)
			    base = recvbuf.seq;
		    printf("pack %d ACKed\n", recvbuf.seq);
	    }
	    // update cwnd
	    switch(state) {
	            case 0:
	        	    if (timeout) {
	        		    ssthresh = cwnd/2;
	        		    cwnd = 16;
	        	    } else {
	        		    cwnd = cwnd*2;
	        		    if (cwnd >= ssthresh)
	        			    state = 1;
	        	    }
	        	    break;
	            case 1:
	        	    if (timeout) {
	        		    ssthresh = cwnd/2;
	        		    cwnd = 16;
	        		    state = 0;
	        	    } else {
	        		    cwnd += 8;
	        	    }
	        	    break;
	            default:
	        	    break;
	    }
	    if (cwnd == 0) cwnd = 1;
	    if (cwnd > WINDOW_SIZE) cwnd = WINDOW_SIZE;
	    printf("cwnd %d, ssthresh %d\n", cwnd, ssthresh);
	    packsToRead = MIN(base - old_base, total_pack_num - end);
    }

    fclose(fp);
    printf("Closing the socket\n");
    close(sockfd);
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

