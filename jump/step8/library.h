#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include <sys/socket.h>
#include <linux/if.h>
#include <openssl/aes.h>
#include <openssl/des.h>

#include "active_vector.h"

typedef struct
{
    int          fd;
    unsigned int ip;
    unsigned int keepalive;
} client_t;

typedef struct
{
    unsigned int    msg_ident;
    unsigned int    localip;
    unsigned char   netmask;
    char            dev_name[IFNAMSIZ];
    active_vector_t clients;

    unsigned char   aes_key[32];
    unsigned int    aes_key_len;
    unsigned char   aes_iv[AES_BLOCK_SIZE];

    DES_cblock      des_key[3];
    unsigned int    des_key_len;
    unsigned char   des_iv[DES_KEY_SZ];

    unsigned char   compress;
    unsigned char   encrypt;

    unsigned int    keepalive;
    unsigned char   keepalive_replyed;
} this_t;

extern this_t this;

typedef struct
{
    unsigned int  localip;
    unsigned char netmask;

    int           use_gzip;

    int           use_aes;
    char*         aes_key_file;

    int           use_des;
    char*         des_key_file;
    unsigned char keepalive;
} library_conf_t;

extern int library_init(library_conf_t conf);
extern int compare_clients_by_fd(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern int compare_clients_by_ip(const void* d1, const size_t l1, const void* d2, const size_t l2);
extern unsigned char netmask();

#endif
