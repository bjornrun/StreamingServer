#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <fcntl.h>
#include <poll.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include <sched.h>

#include "netio.h"
#include "StreamingServer.h"

/**
 * Maximum packet size, normally much less.
 */
#define RECV_PORT  4343
#define SEND_PORT  4344

#define MAX_ID   1000
#define OK_COOKIE 0x2938

BufEntry session[MAX_ID];

void * input_thread(void *arg) {
    const int header_size = sizeof(int);
    char packet[PACKET_SIZE];
    mrecv_t *receiver = recv_open(RECV_PORT);

    msend_t *sender;
    int send_port;
    struct sockaddr_in send_addr;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    puts("starting input thread");

    while (1) {
        int packet_len = recv_msg(receiver, packet, PACKET_SIZE, 1);

        Header* hdr = (Header*)  packet;

        int cookie = ntohs(hdr->cookie);
        int id = ntohs(hdr->id);
        long seq = ntohl(hdr->seq);
        
        printf("sizeof(long)=%d\n", (int)sizeof(long));

        printf("Got packet (cookie=%d, id=%d, len=%d seq=%ld)\n", cookie, id, packet_len, seq);

        if (cookie == OK_COOKIE && id >= 0 && id < MAX_ID && packet_len > sizeof(Header)) {
            printf("OK\n");
            printf("head=%d, tail=%d sizeof(Header)=%d\n", session[id].head, session[id].tail, sizeof(Header));
            
            if (session[id].head != session[id].tail) {
                printf("head=%d session[id].seq[session[id].head]=%d\n", session[id].head, session[id].seq[session[id].head]);
                if(session[id].seq[session[id].head] >= seq) {
                    printf("dublicates!\n");
                    continue;
                }   
            }
            int next_head = (session[id].head + 1) % MAX_BUFS;
            if (next_head == session[id].tail) {
                printf("next_head == session[id].tail\n");
                int next_tail = (session[id].tail + 1) % MAX_BUFS;
                session[id].tail = next_tail;
                if (session[id].buf[next_tail] != NULL) {
                    free(session[id].buf[next_tail]);
                    session[id].buf[next_tail] = NULL;
                }
            }
            if (session[id].buf[next_head] != NULL) {
                printf("1session[id].buf[next_head] != NULL\n");
                free(session[id].buf[next_head]);
                session[id].buf[next_head] = NULL;
            }
            session[id].buf[next_head] = (char*) malloc(packet_len - sizeof(Header));
            if (session[id].buf[next_head] != NULL) {
                printf("2session[id].buf[next_head] != NULL\n");
                for (int i = 0; i < packet_len - sizeof(Header); i++) printf("%02X ", packet[sizeof(Header) + i]);
                printf("\n");
                memcpy(session[id].buf[next_head], &packet[sizeof(Header)], packet_len - sizeof(Header));
                session[id].len[next_head] = packet_len - sizeof(Header);
                session[id].seq[next_head] = seq;
                time(&session[id].last_access);
                session[id].head = next_head;
                
            } else
            {
                printf("Out of memory!");
            }
        }


    }
}

void * send_thread(void *arg) {
    const int header_size = sizeof(int);
    char packet[PACKET_SIZE];
    msend_t *sender = send_open(SEND_PORT);

    struct sockaddr_in send_addr;
    
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    puts("starting send thread");
    
    while (1) {
        int packet_len = recv_send_msg(sender, packet, PACKET_SIZE, 1);
        
        Header* hdr = (Header*)  packet;
        
        int cookie = ntohs(hdr->cookie);
        int id = ntohs(hdr->id);
        unsigned long seq = ntohl(hdr->seq);
        
        printf("Got send packet (cookie=%d, id=%d, len=%d seq=%ld)\n", cookie, id, packet_len, seq);
        if (cookie == OK_COOKIE && id >= 0 && id < MAX_ID && packet_len == sizeof(Header)) {
            printf("OK\n");
            if (session[id].head == session[id].tail) {
                printf("session[id].head == session[id].tail\n");
                long msg = 0;
                send_msg(sender, &msg, sizeof(msg));
            } else
            if (seq > session[id].seq[session[id].head]) {
                printf("seq > session[id].seq[session[id].head]\n");
                long msg = session[id].seq[session[id].head];
                send_msg(sender, &msg, sizeof(msg));
            } else
            if (seq < session[id].seq[session[id].tail]) {
                printf("seq < session[id].seq[session[id].tail]\n");
                long msg = session[id].seq[session[id].tail];
                send_msg(sender, &msg, sizeof(msg));
            } else {
                int next = session[id].tail;
                bool sent = false;
                do {
                    if (session[id].seq[next] == seq) {
                        printf("session[id].seq[next] == seq\n");
                        for (int i = 0; i < session[id].len[next]; i++) printf("%02X ", session[id].buf[next][i]);
                        printf("\n");
                        send_msg(sender, session[id].buf[next], session[id].len[next]);
                        sent = true;
                        break;
                    }
                    if (session[id].seq[next] > seq) {
                        printf("session[id].seq[next] > seq\n");
                        long msg = session[id].seq[next];
                        send_msg(sender, &msg, sizeof(msg));
                        sent = true;
                        break;
                    }
                    if (next == session[id].head) break;
                    next = (next + 1) % MAX_BUFS;
                } while(1);
                if (!sent) {
                    printf("!sent\n");
                    long msg = session[id].seq[session[id].head];
                    send_msg(sender, &msg, sizeof(msg));
                }
            }
        }
    }
}


int main(int argc, const char* argv[]) {
    for (int i = 0; i < MAX_ID; i++) {
        session[i].head = 0;
        session[i].tail = 0;
        for (int j = 0; j < MAX_BUFS; j++) {
            session[i].buf[j] = NULL;
            session[i].len[j] = 0;
            session[i].seq[j] = 0;
        }
    }

    pthread_t send_id;
    pthread_create(&send_id, NULL, send_thread, 0);

    
    pthread_t input_id;
    pthread_create(&input_id, NULL, input_thread, 0);
    pthread_join(input_id, NULL);


  return 0;
}