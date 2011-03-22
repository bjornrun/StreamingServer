#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <fcntl.h>
#include <poll.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "netio.h"


/**
 * Open a listening socket on port @a port
 * @param port port number
 * @return a new mrecv_t object.
 */
mrecv_t * recv_open(int port) {
  mrecv_t * mt = (mrecv_t*)malloc(sizeof(mrecv_t));
  if (!mt) {
    perror("malloc");
    return 0;
  }

  mt->fd = socket(AF_INET, SOCK_DGRAM,0);
  if (0 > mt->fd) {
    perror("socket");
    free(mt);
    return 0;
  }
  int reuseaddr = 1;
  if (0 > setsockopt(mt->fd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(int))) {
    perror("setsockopt");
    free(mt);
    return 0;
  }
  memset(&mt->addr, 0, sizeof(mt->addr));
  mt->addr.sin_family = AF_INET;
  mt->addr.sin_addr.s_addr = INADDR_ANY;
  mt->addr.sin_port = htons(port);

  if (0 > bind(mt->fd,(struct sockaddr *) &mt->addr,sizeof(mt->addr))) {
    perror("bind");
    free(mt);
    return 0;
  }

  return mt;

}


/**
 * Get last source address.
 * @param m mrecv_t listening socket.
 * @returns int internet address.
 */
int recv_get_addr(mrecv_t *m) {
  assert(m);
  return m->addr.sin_addr.s_addr;
}

/**
 * Receive message from receiver @a m.
 * @param m listening mrecv_t
 * @param buf a buffer to store the received data in.
 * @param len buffer size
 * @param wait block or not.
 * @returns number of bytes received or EAGAIN if the recv was non blocking.
 */
int recv_msg(mrecv_t *m, void *buf, size_t len, char wait) {
  size_t addrlen = sizeof(m->addr);
  int n;
  if (wait)
     n  = recvfrom(m->fd, buf, len, 0,
                   (struct sockaddr *) &m->addr, (socklen_t *)&addrlen);
  else
     n  = recvfrom(m->fd, buf, len, MSG_DONTWAIT,
                   (struct sockaddr *) &m->addr, (socklen_t *)&addrlen);
  if (0 > n) {
    if (errno == EAGAIN) {
      return EAGAIN;
    }
    printf("%d\n", n);
    perror("recvfrom");
    exit(2);
  }
  return n;
}

/**
 * Open a sending socket.
 * @param port port number.
 * @param dest destination address.
 * @return a new msend_t object or NULL on failure.
 */
msend_t* send_open(int port) {
    int reuseaddr = 1;
    
    msend_t * mt = (msend_t*)malloc(sizeof(msend_t));
    if (!mt) {
        perror("malloc");
        return 0;
    }
    
    mt->fd = socket(AF_INET, SOCK_DGRAM,0);
    if (0 > mt->fd) {
        perror("socket");
        goto error;
    }
    
    
    if (0 > setsockopt(mt->fd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(int))) {
        perror("setsockopt");
        free(mt);
        return 0;
    }
    memset(&mt->addr, 0, sizeof(mt->addr));
    mt->addr.sin_family = AF_INET;
    mt->addr.sin_addr.s_addr = INADDR_ANY;
    mt->addr.sin_port = htons(port);
    
    if (0 > bind(mt->fd,(struct sockaddr *) &mt->addr,sizeof(mt->addr))) {
        perror("bind");
        free(mt);
        return 0;
    }

    
    return mt;
error:
    free(mt);
    return 0;
}

/**
 * Send message.
 * @param m msend_t sender.
 * @param buf buffer to send.
 * @param len buffer size.
 * @returns 1 if the send was successful, 0 otherwise.
 */
int send_msg(msend_t *m, const void *buf, size_t len) {
    assert(m);
    assert(buf);
    assert(len > 0);
    if (0 > sendto(m->fd, buf, len, 0, (struct sockaddr *) &m->addr,
                   sizeof(m->addr)))
    {
        return 0;
    }
    return 1;
}

/**
 * Receive message from sender @a m.
 * @param m listening mrecv_t
 * @param buf a buffer to store the received data in.
 * @param len buffer size
 * @param wait block or not.
 * @returns number of bytes received or EAGAIN if the recv was non blocking.
 */
int recv_send_msg(msend_t *m, void *buf, size_t len, char wait) {
    size_t addrlen = sizeof(m->addr);
    int n;
    if (wait)
        n  = recvfrom(m->fd, buf, len, 0,
                      (struct sockaddr *) &m->addr, (socklen_t *)&addrlen);
    else
        n  = recvfrom(m->fd, buf, len, MSG_DONTWAIT,
                      (struct sockaddr *) &m->addr, (socklen_t *)&addrlen);
    if (0 > n) {
        if (errno == EAGAIN) {
            return EAGAIN;
        }
        printf("%d\n", n);
        perror("recvfrom");
        exit(2);
    }
    return n;
}



