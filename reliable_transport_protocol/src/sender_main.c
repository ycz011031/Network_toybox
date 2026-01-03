/* 
 * File:   sender_main.c
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <inttypes.h>

#define MSS 1400
#define MAX_L 128 //max allowed in flight un-ack packets
#define SST 64
#define TIMEOUT_MS 100

struct sockaddr_in si_other;
int s, slen;

typedef struct{
    uint32_t seq_num;
    uint16_t payload_size;
} send_header_t;

typedef struct{
    send_header_t header;
    uint8_t payload[MSS];
}send_packet_t;

void diep(char *s) {
    perror(s);
    exit(1);
}

int send_packet(uint32_t seq_num, uint16_t payload_len, const uint8_t *payload){
    if (payload_len > MSS){
        fprintf(stderr, "payload length exceeds maximum");
        return -1;
    }
    uint8_t packet[sizeof(uint32_t) + sizeof(uint16_t)+MSS];
    memcpy(packet,&seq_num,sizeof(uint32_t));
    memcpy(packet+sizeof(uint32_t),&payload_len,sizeof(uint16_t));
    memcpy(packet + sizeof(uint32_t) + sizeof(uint16_t),payload,payload_len);
    int packet_len= sizeof(uint32_t)+sizeof(uint16_t)+payload_len;
    if (sendto(s, packet, packet_len, 0, (struct sockaddr *)&si_other, slen) < 0) {
        perror("sendto() failed");
        return -1;
    }
    printf("seq_num sent is %" PRIu32 "\n",seq_num);
    return 0;
}

void update_stamp(struct timeval stamps[MAX_L], int map[MAX_L], int slot, uint32_t seq_num) {
    map[slot] = seq_num;
    gettimeofday(&stamps[slot], NULL);
}

int receive_ack(int sock, struct sockaddr_in *si_other, socklen_t *slen) {
    uint32_t ack_seq;
    ssize_t recv_len = recvfrom(sock, &ack_seq, sizeof(ack_seq), 0,
                                (struct sockaddr *)si_other, slen);
    if (recv_len < 0) {
        perror("recvfrom() failed for ACK");
        return -1;
    } else if (recv_len != sizeof(ack_seq)) {
        fprintf(stderr, "Invalid ACK size: %zd\n", recv_len);
        return -1;
    }
    printf("seq_num ack is %"PRIu32"\n",ack_seq);
    return ack_seq;
}

bool is_head_timed_out(struct timeval stamps[MAX_L], int map[MAX_L], uint32_t cw_head) {
    int slot = cw_head % MAX_L;

    if (map[slot] != (int)cw_head)
        return false; // this slot has moved on, no timeout for this seq_num

    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed_ms = (now.tv_sec - stamps[slot].tv_sec) * 1000
                      + (now.tv_usec - stamps[slot].tv_usec) / 1000;
    return elapsed_ms >= TIMEOUT_MS;
}
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
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

    //header generation
    uint32_t cw_head = 0, cw_tail = 0;
    double cw_size = 1.0;   // in packets, slow start starts at 1
    int sst = SST;
    uint8_t state = 0;       // 0 = slow start, 1 = congestion avoidance, 2 = fast recovery
    int dupack = 0;

    struct timeval stamps[MAX_L];
    int map[MAX_L];
    for (int i = 0; i < MAX_L; i++) map[i] = -1;

    uint8_t buffer[MAX_L][MSS]; // payload buffer

    int total_packets = (bytesToTransfer + MSS - 1) / MSS; //roudning up for the number of packets needed to transmit all data
    printf("total packets is %d\n" ,total_packets);
    while (true){
        // --- Send packets ---
        while (cw_tail - cw_head < (int)cw_size && cw_tail < total_packets) {
            int slot = cw_tail % MAX_L;

            // Read payload from file
            size_t payload_len = MSS;
            if ((cw_tail + 1) * MSS > bytesToTransfer)
                payload_len = bytesToTransfer - cw_tail * MSS;
            fread(buffer[slot], 1, payload_len, fp);

            send_packet(cw_tail, payload_len, buffer[slot]);
            update_stamp(stamps, map, slot, cw_tail);

            cw_tail++;
        }

        // --- Receive ACKs ---
        fd_set read_fds;
        struct timeval tv = {0, 1000}; // 1ms timeout
        FD_ZERO(&read_fds);
        FD_SET(s, &read_fds);
        int ret = select(s + 1, &read_fds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(s, &read_fds)) {
            int ack_seq = receive_ack(s, &si_other, &slen);
            if (ack_seq >= 0) {
                if ((uint32_t)ack_seq > cw_head) {
                    cw_head = ack_seq;
                    dupack = 0;
                    // --- Update CW size ---
                    if (state == 0) { 
                        cw_size++;
                        if (cw_size >= sst) {
                            state = 1; 
                        }
                    } else if (state == 1) { 
                        cw_size += 1.0 / cw_size;
                    }
                    if (cw_size > MAX_L)
                        cw_size = MAX_L;

                } else {
                    // Duplicate ACKs
                    dupack++;
                    if (dupack == 3) { // Fast retransmit
                        int slot = cw_head % MAX_L;
                        size_t payload_len = MSS;
                        if ((cw_head + 1) * MSS > bytesToTransfer)
                            payload_len = bytesToTransfer - cw_head * MSS;

                        send_packet(cw_head, payload_len, buffer[slot]);
                        gettimeofday(&stamps[slot], NULL); // refresh timestamp

                        sst = cw_size / 2;
                        cw_size = sst; 
                        if (cw_size < 1) cw_size = 1;
                        state = 2; 
                        dupack = 0;
                    }
                }
            }
        }
        // --- Check timeout ---
        if (is_head_timed_out(stamps, map, cw_head)) {
            int slot = cw_head % MAX_L;
            size_t payload_len = MSS;
            if ((cw_head + 1) * MSS > bytesToTransfer)
                payload_len = bytesToTransfer - cw_head * MSS;

            send_packet(cw_head, payload_len, buffer[slot]);
            gettimeofday(&stamps[slot], NULL);
            sst = cw_size / 2;
            cw_size = 1.0;
            state = 0;
            dupack = 0;
        }

        if (cw_head >= total_packets){
            printf("All packets sent and ACKed. Closing socket.\n");
            fclose(fp);
            break;           
        }
    }
    



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


