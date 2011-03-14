typedef struct {
    short cookie;
    short id;
} Header;

#define MAX_BUFS 100

typedef struct {
    int head, tail;
    char* buf[MAX_BUFS];
} BufEntry;