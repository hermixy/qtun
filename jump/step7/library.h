#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include <openssl/aes.h>

typedef struct
{
    unsigned int  msg_ident;
    unsigned char aes_key[AES_BLOCK_SIZE];
    unsigned int  aes_key_len;
    unsigned char aes_iv[AES_BLOCK_SIZE];
} this_t;

extern this_t this;

typedef struct
{
    int           use_gzip;
    int           use_aes;
    unsigned char aes_key[32];
    unsigned int  aes_key_len;
    unsigned char aes_iv[AES_BLOCK_SIZE];
    int           use_des;
} library_conf_t;

extern int library_init(library_conf_t conf);

#endif
