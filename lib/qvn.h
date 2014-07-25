#include <linux/if_tun.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdlib.h>

typedef struct
{
    int  port;
} qvn_server_t;

typedef struct
{
} qvn_client_t;
