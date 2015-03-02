#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if defined(unix) && defined(HAVE_LINUX_IF_H)
#include <linux/if.h>
#endif

#include "hash.h"
#include "link.h"
#include "msg_group.h"
#include "msg.h"

#define UNCOMPRESSED       0
#define COMPRESS_WITH_GZIP 1

#define UNENCRYPTED        0
#define ENCRYPT_WITH_AES   1
#define ENCRYPT_WITH_DES   2

extern
#ifdef WIN32
HANDLE
#else
int
#endif
tun_open(char name[IFNAMSIZ]);

extern int bind_and_listen(unsigned short port);
extern int connect_server(char* host, unsigned short port);

extern void server_loop(
    fd_type remotefd,
#ifdef WIN32
    HANDLE localfd
#else
    int localfd
#endif
);
extern void client_loop(
    fd_type remotefd,
#ifdef WIN32
    HANDLE localfd
#else
    int localfd
#endif
);
extern ssize_t read_msg_t(client_t* client, msg_t** msg, double timeout);
extern ssize_t write_c(client_t* client, const void* buf, size_t count);
extern ssize_t read_t(client_t* client, void* buf, size_t count, double timeout);
extern ssize_t read_pre(fd_type fd, void* buf, size_t count);
extern ssize_t udp_read(fd_type fd, void* buf, size_t count, struct sockaddr_in* srcaddr, socklen_t* addrlen);
extern ssize_t send_msg_group(client_t* client, msg_group_t* g);

extern size_t msg_ident_hash(const void* data, const size_t len);
extern int msg_ident_compare(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern void msg_group_free_hash(void* key, size_t key_len, void* val, size_t val_len);
extern void msg_group_free_hash_val(void* val, size_t len);
extern msg_group_t* msg_group_lookup(hash_t* h, size_t ident);

extern int process_clip_msg(
#ifdef WIN32
    HANDLE fd,
#else
    int fd,
#endif
    client_t* client,
    msg_t* msg,
    size_t* room_id
);
extern int check_msg(client_t* client, msg_t* msg);

#ifdef WIN32
extern int local_have_data();
#endif

#define LOGIN_TIMEOUT      5
#define KEEPALIVE_INTERVAL 30
#define KEEPALIVE_TIMEOUT  25
#define KEEPALIVE_LIMIT    60

#endif
