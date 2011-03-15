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
#define MAX_ID   1000
#define OK_COOKIE 0x2938

BufEntry buf[MAX_ID];

void add2buf(int id, char* packet, int len)
{
    int next_head = (buf[id].head + 1) % MAX_BUFS;

}

void * input_thread(void *arg) {
    const int header_size = sizeof(int);
    char packet[PACKET_SIZE];
    mrecv_t *receiver = recv_open(4343);

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

        printf("Got packet (cookie=%d, id=%d, len=%d)\n", cookie, id, packet_len);

        if (cookie == OK_COOKIE && id >= 0 && id < MAX_ID) {

        }

        int addr = recv_get_addr(receiver);
         {
                char *c_addr;

                c_addr = inet_ntoa(receiver->addr.sin_addr);

                fprintf(stderr, "from %s\n", c_addr);



            }

    }
}

int main(int argc, const char* argv[]) {


    pthread_t input_id;
    pthread_create(&input_id, NULL, input_thread, 0);
    pthread_join(input_id, NULL);


  return 0;
}