#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <linux/ip.h>

#include "hash.h"

#define SERVER_AUTH_MSG "Who are you"
#define CLIENT_AUTH_MSG "I am 0000"

typedef struct
{
    unsigned int id;
    int          fd;
}client_t;

typedef struct
{
    union
    {
        client_t client;
        hash_t   hash;
    };
}network_t;

extern network_t network;

extern int bind_and_listen(unsigned short port);
extern int connect_server(char* ip, unsigned short port);
extern void reply_echo(int fd, struct iphdr* ipHdr);
extern void server_loop(int remotefd, int localfd);
extern ssize_t read_n(int fd, void* buf, size_t count);
extern ssize_t write_n(int fd, const void* buf, size_t count);
extern ssize_t read_t(int fd, void* buf, size_t count, double timeout);

#endif
