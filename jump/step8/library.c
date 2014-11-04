#include <string.h>

#include "common.h"
#include "msg.h"
#include "library.h"

this_t this;

int library_init(library_conf_t conf)
{
    FILE* fp;
    ssize_t len;
    hash_functor_t functor_ip = {
        ip_hash,
        ip_compare,
        ip_dup,
        hash_dummy_dup,
        hash_dummy_free,
        NULL
    };

    init_msg_process_handler();

    this.localip  = conf.localip;
    this.compress = 0;
    this.encrypt  = 0;
    this.netmask  = conf.netmask;

    hash_init(&this.hash_ip, functor_ip, 11);
    if (!hash_set(&this.hash_ip, (void*)(long)conf.localip, sizeof(conf.localip), (void*)(long)0, sizeof(int)))
    {
        fprintf(stderr, "Not enough memory\n");
        return 0;
    }

    if (conf.use_gzip)
        if (!append_msg_process_handler(MSG_PROCESS_COMPRESS_HANDLER, MSG_COMPRESS_GZIP_ID, gzip_compress, gzip_decompress))
            return 0;

    if (conf.use_aes)
    {
        if (!append_msg_process_handler(MSG_PROCESS_ENCRYPT_HANDLER, MSG_ENCRYPT_AES_ID, aes_encrypt, aes_decrypt))
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
        if (!append_msg_process_handler(MSG_PROCESS_ENCRYPT_HANDLER, MSG_ENCRYPT_DES_ID, des_encrypt, des_decrypt))
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
