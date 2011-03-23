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
#include "tcpserver.h"

/**
 * Maximum packet size, normally much less.
 */
#define RECV_PORT  4343
#define SEND_PORT  4344

#define BUFFSIZE 1500
#define MAXBUFFSIZE2STORE (10*1024*1024)


#define MAX_ID   1000
#define OK_COOKIE 0x2938
#define MAX_USERS 10000

NameEntry users[MAX_USERS];
BufEntry session[MAX_ID];

static unsigned int next_id = 0;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER; 

int AddBuf(int id, long seq, char* buf, int len)
{
	pthread_mutex_lock(&session[id].mutex);
	if (session[id].head != session[id].tail) {
		printf("head=%d session[id].seq[session[id].head]=%d\n", session[id].head, session[id].seq[session[id].head]);
		if(session[id].seq[session[id].head] >= seq) {
			printf("dublicates!\n");
			pthread_mutex_unlock(&session[id].mutex);
			return 1;
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
	session[id].buf[next_head] = buf;
	session[id].len[next_head] = len;
	session[id].seq[next_head] = seq;
	time(&session[id].last_access);
	session[id].head = next_head;		
	pthread_mutex_unlock(&session[id].mutex);
}

int SendBuf(int id, long seq, int sock)
{
	pthread_mutex_lock(&session[id].mutex);
	if (session[id].head == session[id].tail) {
		Response r;
		pthread_mutex_unlock(&session[id].mutex);
		printf("session[id].head == session[id].tail\n");
		r.response = htons(E_R_NODATA);
		r.len = 0;
		return send(sock, &r, sizeof(r), 0) != sizeof(r);
	} else
	if (seq > session[id].seq[session[id].head]) {
		Response r;
		int val;
		val = htonl(session[id].seq[session[id].head]);
		pthread_mutex_unlock(&session[id].mutex);
		r.response = htons(E_R_HEAD);
		r.len = htons(4);
		printf("seq > session[id].seq[session[id].head]\n");
		if (send(sock, &r, sizeof(r), 0) != sizeof(r)) return -1;
		return send(sock, &val, 4, 0) != 4;
	} else
	if (seq < session[id].seq[session[id].tail]) {
		Response r;
		int val;
		val = htonl(session[id].seq[session[id].tail]);
		pthread_mutex_unlock(&session[id].mutex);
		r.response = htons(E_R_TAIL);
		r.len = htons(4);
		printf("seq < session[id].seq[session[id].tail]\n");
		if (send(sock, &r, sizeof(r), 0) != sizeof(r)) return -1;
		return send(sock, &val, 4, 0) != 4;
	} else {
		int next = session[id].tail;
        bool sent = false;
		do {
			if (session[id].seq[next] == seq) {
				Response r;
				pthread_mutex_unlock(&session[id].mutex);

				printf("session[id].seq[next] == seq\n");
				
				r.response = htons(E_R_BUF);
				r.len = htons(session[id].len[next]);
				if (send(sock, &r, sizeof(r), 0) != sizeof(r)) return -1;
				pthread_mutex_lock(&session[id].mutex);
				int sent_len = send(sock, session[id].buf[next], session[id].len[next], 0);
				pthread_mutex_unlock(&session[id].mutex);
				return (sent_len != session[id].len[next]);
				
			}
			if (session[id].seq[next] > seq) {
				Response r;
				int val;
				val = htonl(session[id].seq[session[id].tail]);
				pthread_mutex_unlock(&session[id].mutex);
				r.response = htons(E_R_TAIL);
				r.len = htons(4);
				printf("session[id].seq[next] > seq\n");
				if (send(sock, &r, sizeof(r), 0) != sizeof(r)) return -1;
				return send(sock, &val, 4, 0) != 4;
				
			}
			if (next == session[id].head) break;
			next = (next + 1) % MAX_BUFS;
		} while(1);
		Response r;
		int val;
		val = htonl(session[id].seq[session[id].head]);
		pthread_mutex_unlock(&session[id].mutex);
		r.response = htons(E_R_HEAD);
		r.len = htons(4);
		printf("!sent\n");
		if (send(sock, &r, sizeof(r), 0) != sizeof(r)) return -1;
		return send(sock, &val, 4, 0) != 4;
	}
}


void* handle_tcp_client(void * arg)
{
	printf("handling tcp client\n");
	
	char *buffer = NULL;
	Header hdr;
	int received = -1;
	long sock = reinterpret_cast<long> (arg);
	/* Receive message */
	do {
	
		if ((received = recv(sock, &hdr, sizeof(Header), 0)) < 0) {
			printf("handle_tcp_client: no header");
			break;
		}
		printf("received = %d\n", received);
		if (received == 0) break;
		while (received < sizeof(Header)) {
			int next_rec;
			if ((next_rec = recv(sock, (char*) &hdr + received, sizeof(Header) - received, 0)) <= 0) {
				break;
			} else {
				usleep(100);
			}

			
			received += next_rec;
			printf("received part = %d\n", next_rec);
		}
		if (received < sizeof(Header)) break;
		for (int i=0; i < received; i++) printf("%02x ", (reinterpret_cast<char*> (&hdr))[i]);
		printf("\n");
		
		hdr.cookie = ntohs(hdr.cookie);
		hdr.id = ntohs(hdr.id);
		hdr.cmd = ntohs(hdr.cmd);
		hdr.len = ntohs(hdr.len);
		hdr.seq = ntohl(hdr.seq);
		
		printf("cookie=%x id=%d cmd=%d len=%d seq=%ld\n", hdr.cookie, hdr.id, hdr.cmd, (hdr.len), (hdr.seq))	;
	
	
		if ((hdr.id == 0 && hdr.cookie == OK_COOKIE) || (hdr.id < MAX_ID && hdr.cookie == session[hdr.id].cookie)) {
			printf("OK\n");
			if (hdr.len > 0) {
				buffer = (char*) malloc(hdr.len);
				if (buffer == NULL) {
					printf("out of memory\n");
					break;
				}
				for (received = 0; received < hdr.len; ) {
					int next_rec = recv(sock, buffer + received, hdr.len - received, 0);
					if (next_rec < 0) {
						printf("couldn't get all buffer\n");
						break;
					}
					received += next_rec;
				}
				if (received < hdr.len) break;
				printf("got payload\n");
			}
			if ((Cmds) hdr.cmd == E_STORE) {
				printf("adding len=%d\n", hdr.len);
				if (AddBuf(hdr.id, hdr.seq, buffer, hdr.len)) {
					free(buffer);
				}
				buffer = NULL;
			} else 
			if ((Cmds) hdr.cmd == E_GET) {
				printf("E_GET\n");
				if (buffer) {
					free(buffer);
					buffer = NULL;
				}
				if (SendBuf(hdr.id, hdr.seq, sock)) break;
			} else 
			if ((Cmds) hdr.cmd == E_STATUS) {
				if (hdr.len != sizeof(BUF_Status)) {
					printf("hdr.len != sizeof(BUF_Status)\n");
				} else {
					printf("E_STATUS\n");
					BUF_Status* status = (BUF_Status*) buffer;
					session[hdr.id].alert = ntohs(status->alert);
					session[hdr.id].last_access = time(NULL);
 				}
				if (buffer != NULL) {
					free(buffer);
					buffer = NULL;
				}
			} else
			if ((Cmds) hdr.cmd == E_CHECK) {
				Response r;
				BUF_R_Check c;
				printf("E_CHECK\n");
				time_t now = time(NULL);
				r.response = htons(E_R_CHECK);
				r.len = htons(sizeof(BUF_R_Check));
				c.alert = htons(session[hdr.id].alert);
				c.last_access_sec = htons((int) difftime(now, session[hdr.id].last_access));
				
				if (send(sock, &r, sizeof(r), 0) != sizeof(r)) break;
				if (send(sock, &c, sizeof(c), 0) != sizeof(c)) break;				
			} else
			if ((Cmds) hdr.cmd == E_CREATE) {
				Response r;
				BUF_R_Create_Login c;
				time_t now = time(NULL);
				printf("E_CREATE\n");
				
				if (hdr.len != sizeof(BUF_Create_Login)) {
					printf("hdr.len != sizeof(BUF_Create_Login)\n");
				} else {
					BUF_Create_Login* status = (BUF_Create_Login*) buffer;
					char username[MAX_NAME_LEN];
					char password[MAX_NAME_LEN];
					strncpy(username, status->username, MAX_NAME_LEN);
					strncpy(password, status->password, MAX_NAME_LEN);
					username[MAX_NAME_LEN-1] = 0;
					password[MAX_NAME_LEN-1] = 0;
					bool done = false;
					for (int i = 0; i < MAX_ID; i++) {
						if (users[i].username[0] == 0 || !strcmp(users[i].username, username) || difftime(now, users[i].last_access) > 60*60*24) {
							strcpy(users[i].password, password);
							users[i].last_access = time(NULL);
							pthread_mutex_lock(&g_mutex);
							while (session[next_id].last_access != 0 && difftime(now, session[next_id].last_access) < 60) {
								next_id = (next_id + 1) & MAX_ID;
								usleep(1000);
								now = time(NULL);
							}
							int id = next_id;
							next_id	= (next_id + 1) % MAX_ID;
							pthread_mutex_unlock(&g_mutex);
							
							pthread_mutex_lock(&session[next_id].mutex);

							for (int i = 0; i < MAX_BUFS; i++) 
								if (session[id].buf[i] != NULL) {
									free(session[id].buf);
									session[id].buf[i] = NULL;
								}
							session[id].alert = 0;
							session[id].head = 0;
							session[id].tail = 0;
							session[id].last_access = time(NULL);
							session[id].cookie = (unsigned short) time(NULL);
							pthread_mutex_unlock(&session[id].mutex);
							Response r;
							BUF_R_Create_Login c;
							r.response = htons(E_R_CREATE);
							r.len = htons(sizeof(BUF_R_Create_Login));
							c.id = htons(id);
							c.cookie = htons(session[id].cookie);
							done = true;
							if (send(sock, &r, sizeof(r), 0) != sizeof(r)) break;
							if (send(sock, &c, sizeof(c), 0) != sizeof(c)) break;				
							break;
							
						}
					}
					if (!done) {
						Response r;
						BUF_R_Create_Login c;
						r.response = htons(E_R_CREATE);
						r.len = htons(sizeof(BUF_R_Create_Login));
						c.id = htons(0xFFFF);
						c.cookie = htons(0xFFFF);
						if (send(sock, &r, sizeof(r), 0) != sizeof(r)) break;
						if (send(sock, &c, sizeof(c), 0) != sizeof(c)) break;				
					}						
				}
			} else
			if ((Cmds) hdr.cmd == E_LOGIN) {
				Response r;
				BUF_R_Create_Login c;
				printf("E_LOGIN");
				if (hdr.len != sizeof(BUF_Create_Login)) {
					printf("hdr.len != sizeof(BUF_Create_Login)\n");
				} else {
					BUF_Create_Login* status = (BUF_Create_Login*) buffer;
					char username[MAX_NAME_LEN];
					char password[MAX_NAME_LEN];
					strncpy(username, status->username, MAX_NAME_LEN);
					strncpy(password, status->password, MAX_NAME_LEN);
					username[MAX_NAME_LEN-1] = 0;
					password[MAX_NAME_LEN-1] = 0;
					printf("username=%s, password=%d\n", username, password);
					bool done = false;
					Response r;
					BUF_R_Create_Login c;
					r.response = htons(E_R_LOGIN);
					r.len = htons(sizeof(BUF_R_Create_Login));
					c.id = htons(0xFFFF);
					c.cookie = htons(0xFFFF);
					
					for (int i = 0; i < MAX_ID; i++) {
						if (!strcmp(users[i].username, username)) {
							if (!strcmp(users[i].password, password)) {
								c.id = htons(users[i].id);
								c.cookie = htons(users[i].cookie);
								printf("found login\n");
								break;
							} else {
								printf("username ok password not=%s\n", users[i].password);
							}

						} else {
							printf("not username=%s\n", users[i].username);
						}

					}
					if (send(sock, &r, sizeof(r), 0) != sizeof(r)) break;
					if (send(sock, &c, sizeof(c), 0) != sizeof(c)) break;				
				}						
			} else {
				printf("Unknown command\n");
				if (buffer) {
					free(buffer);
					buffer = NULL;
				}
			}

				
		}
	
	} while (1);
	close(sock);

	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	
	printf("client closed\n");
	pthread_exit( NULL );
}

int main(int argc, const char* argv[]) {
	
    for (int i = 0; i < MAX_ID; i++) {
        session[i].head = 0;
        session[i].tail = 0;
		session[i].last_access = 0;
		pthread_mutex_init(&session[i].mutex, NULL);
        for (int j = 0; j < MAX_BUFS; j++) {
            session[i].buf[j] = NULL;
            session[i].len[j] = 0;
            session[i].seq[j] = 0;
        }
    }

	printf("sizeof(Header) = %d\n", sizeof(Header));
	
/*	
    pthread_t send_id;
    pthread_create(&send_id, NULL, send_thread, 0);

    
    pthread_t input_id;
    pthread_create(&input_id, NULL, input_thread, 0);
*/
	create_tcpserver(SEND_PORT, handle_tcp_client);

  return 0;
}