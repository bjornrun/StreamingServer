#include <time.h>

typedef struct {
    short cookie;
    short id;
    long seq;
} Header;

#define MAX_BUFS 100

typedef struct {
    int head, tail;
    long seq[MAX_BUFS];
    char* buf[MAX_BUFS];
    int len[MAX_BUFS];
    time_t last_access;
} BufEntry;