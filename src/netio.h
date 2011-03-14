#ifndef _NETIO_H
#define _NETIO_H

#include <netinet/in.h>

#define PACKET_SIZE 1500

/**
 * Sender structure, used by send_* functions.
 */
typedef struct {
  /** File descriptor */
  int fd;
  /** Port */
  int port;
  /** Socket address */
  struct sockaddr_in addr;
} msend_t;

/**
 * Receiver structure, used by revc_* functions.
 */
typedef struct {
  /** File descriptor */
  int fd;
  /** Port */
  int port;
  /** Socket address */
  struct sockaddr_in addr;
} mrecv_t;

msend_t* send_open(int port, const char *dest);
int send_msg(msend_t *t, const void *buf, size_t len);
int send_msg_to(msend_t *m, const void *buf, size_t len, struct sockaddr_in *dest);
mrecv_t* recv_open(int port);
int recv_join_group(mrecv_t *mt, const char *group);
int recv_msg(mrecv_t *m,       void *buf, size_t len, char wait);
int recv_get_addr(mrecv_t *m);
struct sockaddr_in getownaddr(const char *ifname);
void getownip(unsigned char *buf);
unsigned int addr_for(const char *cp);

#endif /* _NETIO_H */