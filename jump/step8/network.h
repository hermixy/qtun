#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/socket.h>
#include <linux/if.h>

#include "hash.h"
#include "link.h"

#define SERVER_AUTH_MSG "Who are you"
#define CLIENT_AUTH_MSG "I am 0000"

#define UNCOMPRESSED       0
#define COMPRESS_WITH_GZIP 1

#define UNENCRYPTED        0
#define ENCRYPT_WITH_AES   1
#define ENCRYPT_WITH_DES   2

extern int tun_open(char name[IFNAMSIZ]);
extern int bind_and_listen(unsigned short port);
extern int connect_server(char* ip, unsigned short port);
extern void server_loop(int remotefd, int localfd);
extern void client_loop(int remotefd, int localfd);
extern ssize_t read_n(int fd, void* buf, size_t count);
extern ssize_t write_n(int fd, const void* buf, size_t count);
extern ssize_t read_t(int fd, void* buf, size_t count, double timeout);

#endif
