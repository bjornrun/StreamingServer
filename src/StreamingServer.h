#include <time.h>

#pragma pack(1)
typedef struct {
    unsigned short cookie;
    unsigned short id;
	unsigned short cmd;
	unsigned short len;
    unsigned long seq:32;
} Header;
#pragma pack(pop)

#define MAX_BUFS 100

typedef struct {
    int head, tail;
    long seq[MAX_BUFS];
    char* buf[MAX_BUFS];
    int len[MAX_BUFS];
    time_t last_access;
	int alert;
	unsigned short cookie;
	pthread_mutex_t mutex; 
} BufEntry;


typedef enum {
	E_STORE=1,
	E_GET,
	E_STATUS,
	E_CHECK,
	E_CREATE,
	E_LOGIN,
} Cmds;

#pragma pack(1)
typedef struct {
	unsigned short response;
	unsigned short len;	
} Response;
#pragma pack(pop)

typedef enum {
	E_R_BUF=1,
	E_R_NODATA,
	E_R_TAIL,
	E_R_HEAD,
	E_R_CHECK,
	E_R_CREATE,
	E_R_LOGIN,
} ResponseCmds;

#define MAX_NAME_LEN	50

typedef struct {
	char username[MAX_NAME_LEN];
	char password[MAX_NAME_LEN];
	int id;
	time_t last_access;
	unsigned short cookie;
} NameEntry;

#pragma pack (1)
typedef struct {
	unsigned short alert;
} BUF_Status;
#pragma pack(pop)

#pragma pack(1)
typedef struct {
	unsigned short last_access_sec;
	unsigned short alert;
} BUF_R_Check;
#pragma pack(pop)

#pragma pack(1)
typedef struct {
	char username[MAX_NAME_LEN];
	char password[MAX_NAME_LEN];
} BUF_Create_Login;
#pragma pack(pop)

#pragma pack(1)
typedef struct {
	unsigned short id;
	unsigned short cookie;
} BUF_R_Create_Login;
#pragma pack(pop)
	

