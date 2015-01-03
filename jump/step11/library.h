#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include <sys/socket.h>
#include <linux/if.h>
#include <openssl/aes.h>
#include <openssl/des.h>

#include "pool.h"
#include "group_pool.h"
#include "active_vector.h"

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

typedef struct
{
    int            fd;
    unsigned int   ip;
    unsigned char  status;
    unsigned char* buffer;
    unsigned char* read;
    size_t         want;
    unsigned int   keepalive;
} client_t;

typedef struct
{
    unsigned int    msg_ident;
    unsigned int    localip;
    unsigned char   netmask;
    unsigned char   log_level;
    char            dev_name[IFNAMSIZ];

    unsigned char   aes_key[32];
    unsigned int    aes_key_len;
    unsigned char   aes_iv[AES_BLOCK_SIZE];

    DES_cblock      des_key[3];
    unsigned int    des_key_len;
    unsigned char   des_iv[DES_KEY_SZ];
    unsigned short  mtu;
    unsigned short  max_length;

    unsigned char   compress;
    unsigned char   encrypt;

    pool_t          pool;
    group_pool_t    group_pool;

    // for server
    active_vector_t clients;

    // for client
    client_t        client;
    unsigned int    keepalive;
    unsigned char   keepalive_replyed;
} this_t;

extern this_t this;

typedef struct
{
    unsigned int   localip;
    unsigned char  netmask;
    unsigned char  log_level;
    unsigned short mtu;

    int           use_gzip;

    int           use_aes;
    char*         aes_key_file;

    int           use_des;
    char*         des_key_file;
} library_conf_t;

extern int library_init(library_conf_t conf);
extern int compare_clients_by_fd(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern int compare_clients_by_ip(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern unsigned char netmask();

#endif
