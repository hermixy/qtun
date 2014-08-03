#ifndef _NETWORK_H_
#define _NETWORK_H_

#define MAX_CLIENTS   100

typedef struct
{
    int  bindfd;
    int  connfd;
} server_network_t;

extern void* network_this;

extern int create_server(int port);
extern void network_loop();

extern int s_send(int fd, const void* data, size_t len);
extern int s_recv(int fd, void* data, size_t len, double timeout);

#endif

