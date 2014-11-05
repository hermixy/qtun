#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include <sys/socket.h>
#include <linux/if.h>
#include <openssl/aes.h>
#include <openssl/des.h>

#include "hash.h"

typedef struct
{
    unsigned int  msg_ident;
    unsigned int  localip;
    unsigned char netmask;
    char          dev_name[IFNAMSIZ];
    hash_t        hash_ip; // ip => fd

    unsigned char aes_key[32];
    unsigned int  aes_key_len;
    unsigned char aes_iv[AES_BLOCK_SIZE];

    DES_cblock    des_key[3];
    unsigned int  des_key_len;
    unsigned char des_iv[DES_KEY_SZ];

    unsigned char compress;
    unsigned char encrypt;
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
} library_conf_t;

extern int library_init(library_conf_t conf);

#endif
