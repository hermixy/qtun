#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/socket.h>
#include <linux/if.h>

#include "hash.h"
#include "link.h"
#include "msg.h"

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
extern ssize_t read_msg(int fd, msg_t** msg);
extern ssize_t read_msg_t(int fd, msg_t** msg, double timeout);
extern ssize_t read_n(int fd, void* buf, size_t count);
extern ssize_t write_n(int fd, const void* buf, size_t count);
extern ssize_t read_t(int fd, void* buf, size_t count, double timeout);

#define LOGIN_TIMEOUT      5
#define KEEPALIVE_INTERVAL 30
#define KEEPALIVE_TIMEOUT  10
#define KEEPALIVE_LIMIT    40 // 10 * 4 / 3

#endif
