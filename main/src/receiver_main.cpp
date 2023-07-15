/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

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
#include "mytcp.h"

void diep(char *s) {
    perror(s);
    exit(1);
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
	/* bind local port */
    struct sockaddr_in si_me, si_other;
    int sockfd;
    socklen_t si_len = sizeof si_other;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
    puts("port bound");

	/* open the file to write */
    FILE *fp;
    fp = fopen(destinationFile, "wb");
    if (fp == NULL) {
	    puts("Could not open file to write.");
	    exit(1);
    }

    struct seg recvbuf;
    struct ack sendbuf;
    struct seg window[WINDOW_SIZE], *seg_ptr;
    char map[WINDOW_SIZE] = {0};
    int base = 0;
    int end = -1;
    int i;
    int recv_num;

    // int drop = 1, drop_seq = 150;
    while (1) {
	    recv_num = recvfrom(sockfd, &recvbuf, sizeof recvbuf, 0, (struct sockaddr *)(&si_other), &si_len);
// 	    if (recvbuf.seq == drop_seq && drop) {
// 		    printf("drop pack %d\n", drop_seq);
// 		    drop = 1;
// 		    drop_seq += 200;
// 	            continue;
// 	    }
	    if (recvbuf.seq < base) {
		    sendbuf.seq = base;
		    sendto(sockfd, &sendbuf, sizeof sendbuf, 0, (sockaddr *)&si_other, sizeof si_other);
		    continue;
	    } else {
		    memcpy(window + (recvbuf.seq % WINDOW_SIZE), &recvbuf, sizeof(recvbuf));
		    map[recvbuf.seq % WINDOW_SIZE] = 1;
		    if (recvbuf.eot ==  1) end = recvbuf.seq;
		    printf("receive pack %d\n", recvbuf.seq);
	    }
	    if (recvbuf.seq == base) {
		    i = base;
		    while (map[i % WINDOW_SIZE] == 1) {
			    seg_ptr = window + (i % WINDOW_SIZE);
			    fwrite(seg_ptr->data, 1, seg_ptr->len, fp);
			    map[i % WINDOW_SIZE] = 0;
			    i++;
		    }
		    base = i;
		    sendbuf.seq = base;
		    sendto(sockfd, &sendbuf, sizeof sendbuf, 0, (sockaddr *)&si_other, sizeof si_other);
		    if (end > -1 && base > end)
			    break;
	    } else {
		    sendbuf.seq = base;
		    sendto(sockfd, &sendbuf, sizeof sendbuf, 0, (sockaddr *)&si_other, sizeof si_other);
	    }
    }

    fclose(fp);
    close(sockfd);
    printf("%s received.\n", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

