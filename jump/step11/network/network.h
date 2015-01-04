#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/socket.h>
#include <linux/if.h>

#include "hash.h"
#include "link.h"
#include "msg_group.h"
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
extern ssize_t read_msg_t(int fd, msg_t** msg, double timeout);
extern ssize_t write_n(int fd, const void* buf, size_t count);
extern ssize_t read_t(int fd, void* buf, size_t count, double timeout);
extern ssize_t read_pre(int fd, void* buf, size_t count);
extern ssize_t send_msg_group(int fd, msg_group_t* g);

extern size_t msg_ident_hash(const void* data, const size_t len);
extern int msg_ident_compare(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern void msg_group_free_hash(void* key, size_t key_len, void* val, size_t val_len);
extern void msg_group_free_hash_val(void* val, size_t len);
extern msg_group_t* msg_group_lookup(hash_t* h, size_t ident);

#define LOGIN_TIMEOUT      5
#define KEEPALIVE_INTERVAL 30
#define KEEPALIVE_TIMEOUT  25
#define KEEPALIVE_LIMIT    60

#endif
