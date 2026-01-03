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
#include <inttypes.h>


#define MSS 1400
#define HEADER_SIZE (sizeof(uint32_t) + sizeof(uint16_t))
#define MAX_BUF (MSS + HEADER_SIZE)

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

void send_ack(uint32_t ack_seq) {
    uint32_t net_ack = ack_seq;
    if (sendto(s, &net_ack, sizeof(net_ack), 0, (struct sockaddr*)&si_other, slen) == -1)
        perror("sendto() ack failed");
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    FILE *fp;
    slen = sizeof(si_other);
    uint32_t expected_seq = 0;

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
    if (!(fp = fopen(destinationFile, "wb")))
        diep("fopen");

    /* Now receive data and send acknowledgements */    
    printf("Receiver started. Waiting for packets...\n");
    uint8_t buf[MAX_BUF];
    ssize_t recv_len;
    while (1) {
        recv_len = recvfrom(s, buf, MAX_BUF, 0, (struct sockaddr*)&si_other, &slen);
        if (recv_len < 0)
            diep("recvfrom");

        // Parse header
        uint32_t seq_num;
        uint16_t payload_len;
        memcpy(&seq_num, buf, sizeof(uint32_t));
        memcpy(&payload_len, buf + sizeof(uint32_t), sizeof(uint16_t));
       

        uint8_t *payload = buf + HEADER_SIZE;

        // In-order packet
        if (seq_num == expected_seq) {
            fwrite(payload, 1, payload_len, fp);
            expected_seq++;
            printf("ack_num sent is %"PRIu32"\n",expected_seq);
            send_ack(expected_seq);
        } else {
            // Out-of-order packet; re-ACK last received
            send_ack(expected_seq);
        }

        if (payload_len != MSS) {
            printf("End of transmission. Closing file.\n");
            break;
        }
    }

    fclose(fp);
    close(s);
    printf("%s received.\n", destinationFile);
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
