#include <string.h>

#include "common.h"
#include "msg.h"
#include "library.h"

this_t this;

int library_init(library_conf_t conf)
{
    FILE* fp;
    ssize_t len;
    active_vector_functor_t functor_clients = {
        active_vector_dummy_dup,
        active_vector_normal_free
    };

    init_msg_process_handler();

    this.msg_ident = 0;
    this.localip  = conf.localip;
    this.compress = 0;
    this.encrypt  = 0;
    this.netmask  = conf.netmask;
    this.keepalive = 0;
    this.keepalive_replyed = 0;

    active_vector_init(&this.clients, functor_clients);
    pool_init(&this.pool);

    if (conf.use_gzip)
        if (!append_msg_process_handler(MSG_PROCESS_COMPRESS_HANDLER, MSG_COMPRESS_GZIP_ID, GZIP_ROOM_IDX, gzip_compress, gzip_decompress))
            return 0;

    if (conf.use_aes)
    {
        if (!append_msg_process_handler(MSG_PROCESS_ENCRYPT_HANDLER, MSG_ENCRYPT_AES_ID, AES_ROOM_IDX, aes_encrypt, aes_decrypt))
            return 0;
        fp = fopen(conf.aes_key_file, "r");
        if (fp == NULL)
        {
            fprintf(stderr, "can not open aes key file\n");
            return 0;
        }
        len = fread(this.aes_iv, sizeof(char), sizeof(this.aes_iv), fp);
        if (len != sizeof(this.aes_iv))
        {
            fprintf(stderr, "error aes iv\n");
            return 0;
        }
        len = fread(this.aes_key, sizeof(char), sizeof(this.aes_key), fp);
        if (len != 16 && len != 24 && len != 32)
        {
            fprintf(stderr, "error aes key file\n");
            return 0;
        }
        this.aes_key_len = len << 3;
        fclose(fp);
    }

    if (conf.use_des)
    {
        if (!append_msg_process_handler(MSG_PROCESS_ENCRYPT_HANDLER, MSG_ENCRYPT_DES_ID, DES_ROOM_IDX, des_encrypt, des_decrypt))
            return 0;
        fp = fopen(conf.des_key_file, "r");
        if (fp == NULL)
        {
            fprintf(stderr, "can not open des key file\n");
            return 0;
        }
        len = fread(this.des_iv, sizeof(char), sizeof(this.des_iv), fp);
        if (len != sizeof(this.des_iv))
        {
            fprintf(stderr, "error des iv\n");
            return 0;
        }
        len = fread(this.des_key, sizeof(char), sizeof(this.des_key), fp);
        if (len != DES_KEY_SZ && len != DES_KEY_SZ * 2 && len != DES_KEY_SZ * 3)
        {
            fprintf(stderr, "error des key file\n");
            return 1;
        }
        this.des_key_len = len;
        fclose(fp);
    }

    return 1;
}

inline int compare_clients_by_fd(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    client_t *c1 = (client_t*)d1;
    int fd = (long)d2;
    return c1->fd == fd;
}

inline int compare_clients_by_ip(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    client_t* client = (client_t*)d1;
    unsigned int ip = (unsigned long)d2;
    return IS_CLIENT_STATUS_NORMAL(client->status) && client->ip == ip;
}

inline unsigned char netmask()
{
    return this.netmask;
}

