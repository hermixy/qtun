#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <netinet/in.h>

#define MAX_CLIENTS   100

typedef struct
{
    int  bindfd;
    int  connfd;
} server_network_t;

typedef struct
{
    int  serverfd;
} client_network_t;

extern void* network_this;

extern int create_server(int port);
extern int create_client(in_addr_t addr, int port);
extern void network_loop();

extern ssize_t read_n(int fd, void* data, size_t len);
extern ssize_t write_n(int fd, const void* data, size_t len);

extern ssize_t quick_read(int fd, void* data, size_t len);
extern ssize_t quick_write(int fd, const void* data, size_t len);

#endif
