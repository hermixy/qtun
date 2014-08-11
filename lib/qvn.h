#ifndef _QVN_H_
#define _QVN_H_

#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>

#define QVN_STATUS_OK    0
#define QVN_STATUS_ERR   1

#define QVN_RESULT_WRONG_TYPE   QVN_STATUS_ERR

#define QVN_CONF_TYPE_SERVER   0
#define QVN_CONF_TYPE_CLIENT   1

typedef struct
{
    int  port;
} server_conf_t;

typedef struct
{
    in_addr_t  server;
    int        server_port;
} client_conf_t;

typedef struct
{
    int               type; // server or client
    server_conf_t     server;
    client_conf_t     client;
    int               running;
    int               tun_fd;
    char              tun_name[IFNAMSIZ];
    int               rmt_fd;
} qvn_conf_t;

extern qvn_conf_t conf;

extern int init_with_server(int port);
extern int init_with_client(in_addr_t addr, int port);
extern void do_network(int count, fd_set* set);

#endif

