#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#endif
#include <openssl/aes.h>
#include <openssl/des.h>
#include <lua.h>
#include <lauxlib.h>

#include "../network/msg_group.h"

#include "../struct/active_vector.h"
#include "../struct/group_pool.h"
#include "../struct/hash.h"
#include "../struct/pool.h"

#include "common.h"

#define CLIENT_STATUS_UNKNOWN        0
#define CLIENT_STATUS_CHECKLOGIN     1
#define CLIENT_STATUS_NORMAL         2
#define CLIENT_STATUS_WAITING_HEADER 64
#define CLIENT_STATUS_WAITING_BODY   128

#define IS_CLIENT_STATUS_CHECKLOGIN(status)     (status & CLIENT_STATUS_CHECKLOGIN)
#define IS_CLIENT_STATUS_NORMAL(status)         (status & CLIENT_STATUS_NORMAL)
#define IS_CLIENT_STATUS_WAITING_HEADER(status) (status & CLIENT_STATUS_WAITING_HEADER)
#define IS_CLIENT_STATUS_WAITING_BODY(status)   (status & CLIENT_STATUS_WAITING_BODY)

#define RECV_ROOM_IDX 0
#define GZIP_ROOM_IDX 1
#define AES_ROOM_IDX  2
#define DES_ROOM_IDX  3
#define MSG_ROOM_IDX  4
#define TMP_ROOM_IDX  (ROOM_COUNT - 1)

#ifdef WIN32
#define IOCTL_HAVE_DATA CTL_CODE(FILE_DEVICE_NETWORK, 1, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifdef WIN32
typedef SOCKET fd_type;
typedef HANDLE local_fd_type;
#define LOCAL_HAVE_DATA(set) (local_have_data())
#define SLEEP(n) Sleep(n * 1000)
#else
typedef int fd_type;
typedef int local_fd_type;
#define LOCAL_HAVE_DATA(set) (FD_ISSET(this.localfd, set))
#define SLEEP(n) sleep(n)
#endif

typedef struct
{
    unsigned int       local_ip;
    unsigned int       remote_ip;
    unsigned short     remote_port;
    struct sockaddr_in addr;
    unsigned int       keepalive;
    hash_t             recv_table;
    unsigned short     internal_mtu;
    unsigned short     max_length;

    // for tcp connection
    fd_type        fd;
    unsigned char  status;
    unsigned char* buffer;
    size_t         buffer_len;
    unsigned char* read;
    size_t         want;
} client_t;

typedef struct
{
    unsigned int    msg_ident;
    unsigned int    msg_ttl;
    unsigned int    localip;
    unsigned char   netmask;
    unsigned char   log_level;
#ifdef WIN32
    char            dev_symbol[MAX_PATH];
    char            dev_name[255];
    DWORD           dev_index;
#else
    char            dev_name[IFNAMSIZ];
#endif
    fd_type         remotefd;
    unsigned char   little_endian;
    local_fd_type   localfd;
    unsigned short  internal_mtu;
    unsigned short  max_length;
    unsigned char   use_udp;

    unsigned char   aes_key[32];
    unsigned int    aes_key_len;
    unsigned char   aes_iv[AES_BLOCK_SIZE];

    DES_cblock      des_key[3];
    unsigned int    des_key_len;
    unsigned char   des_iv[DES_KEY_SZ];

    unsigned char   compress;
    unsigned char   encrypt;

    pool_t          pool;
    group_pool_t    group_pool;

    lua_State*      lua;

    // for server
    active_vector_t clients;
    unsigned char*  recv_buffer;     // for udp connection
    size_t          recv_buffer_len; // for udp connection

    // for client
    client_t        client;
    unsigned int    keepalive;
    unsigned char   keepalive_replyed;
} this_t;

extern this_t this;

typedef struct
{
    char*          conf_file;
    unsigned int   localip;
    unsigned char  netmask;
    unsigned char  log_level;
    unsigned short internal_mtu;
#ifdef WIN32
    char           dev_symbol[MAX_PATH];
    char           dev_name[255];
    DWORD          dev_index;
#endif
    unsigned char  use_udp;

    int            use_gzip;

    int            use_aes;
    char*          aes_key_file;

    int            use_des;
    char*          des_key_file;
} library_conf_t;

extern int library_init(library_conf_t conf);
extern void library_free();
extern int compare_clients_by_fd(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern int compare_clients_by_local_ip(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern int compare_clients_by_remote_ip_and_port(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern unsigned char netmask();
extern void free_client(void* c, size_t l);

#endif
