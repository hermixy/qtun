#ifndef WIN32
#include <netinet/tcp.h>
#endif
#include <string.h>

#include "../network/msg.h"

#include "common.h"
#include "proto.h"
#include "script.h"
#include "library.h"

this_t this;

int library_init(library_conf_t conf)
{
    FILE* fp;
    ssize_t len;
    active_vector_functor_t functor_clients = {
        active_vector_dummy_dup,
        free_client
    };
    unsigned short en = 0x1234;

    init_msg_process_handler();

    if (*(unsigned char*)&en == 0x12) this.little_endian = 0;
    else this.little_endian = 1;
    this.msg_ident    = 0;
    this.msg_ttl      = 0;
    this.localip      = conf.localip;
    this.log_level    = conf.log_level;
    this.internal_mtu = conf.internal_mtu;
#ifdef WIN32
    strcpy(this.dev_symbol, conf.dev_symbol);
#endif
    this.max_length   = ROUND_UP(conf.internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - (conf.use_udp ? sizeof(struct udphdr) : sizeof(struct tcphdr)), 8);
    this.use_udp      = conf.use_udp;
    this.compress     = 0;
    this.encrypt      = 0;
    this.netmask      = conf.netmask;
    this.keepalive    = 0;
    this.keepalive_replyed = 0;

    active_vector_init(&this.clients, functor_clients);
    pool_init(&this.pool);
    group_pool_init(&this.group_pool);

    if (conf.use_udp)
    {
        this.recv_buffer_len = (sizeof(msg_t) + sizeof(sys_login_msg_t)) << 1; // enough for sys login msg
        this.recv_buffer = pool_room_alloc(&this.pool, RECV_ROOM_IDX, this.recv_buffer_len);
        if (this.recv_buffer == NULL)
        {
            SYSLOG(LOG_ERR, "Not enough memory");
            exit(1);
        }
    }

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
            SYSLOG(LOG_ERR, "can not open aes key file");
            return 0;
        }
        len = (ssize_t)fread(this.aes_iv, sizeof(char), sizeof(this.aes_iv), fp);
        if (len != sizeof(this.aes_iv))
        {
            SYSLOG(LOG_ERR, "error aes iv");
            return 0;
        }
        len = (ssize_t)fread(this.aes_key, sizeof(char), sizeof(this.aes_key), fp);
        if (len != 16 && len != 24 && len != 32)
        {
            SYSLOG(LOG_ERR, "error aes key file");
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
            SYSLOG(LOG_ERR, "can not open des key file");
            return 0;
        }
        len = (ssize_t)fread(this.des_iv, sizeof(char), sizeof(this.des_iv), fp);
        if (len != sizeof(this.des_iv))
        {
            SYSLOG(LOG_ERR, "error des iv");
            return 0;
        }
        len = (ssize_t)fread(this.des_key, sizeof(char), sizeof(this.des_key), fp);
        if (len != DES_KEY_SZ && len != DES_KEY_SZ * 2 && len != DES_KEY_SZ * 3)
        {
            SYSLOG(LOG_ERR, "error des key file");
            return 1;
        }
        this.des_key_len = len;
        fclose(fp);
    }

    return 1;
}

int init_lua()
{
    this.lua = luaL_newstate();
    if (luaL_dofile(this.lua, "../scripts/qtun.lua") != 0)
    {
        fprintf(stderr, "%s\n", lua_tostring(this.lua, -1));
        lua_close(this.lua);
        exit(1);
    }
    script_global_init(this.lua);
    return 1;
}

void library_free()
{
    lua_close(this.lua);
}

void conf_init(library_conf_t* conf)
{
    memset(conf->server, 0, sizeof(conf->server));
    conf->server_port = 6687;
    memset(conf->conf_file, 0, sizeof(conf->conf_file));
    conf->localip      = 0;
    conf->netmask      = 24;
    conf->log_level    = LOG_WARNING;
    conf->internal_mtu = 1492; // keep not to clip
#ifdef WIN32
    memset(conf->dev_symbol, 0, sizeof(conf.dev_symbol));
    memset(conf->dev_name, 0, sizeof(conf.dev_name));
#endif
    conf->use_gzip     = 0;
    conf->use_udp      = 0;
    conf->use_aes      = 0;
    memset(conf->aes_key_file, 0, sizeof(conf->aes_key_file));
    conf->use_des      = 0;
    memset(conf->des_key_file, 0, sizeof(conf->des_key_file));
}

int compare_clients_by_fd(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    client_t *c1 = (client_t*)d1;
    int fd = (long)d2;
    return c1->fd == fd;
}

int compare_clients_by_local_ip(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    client_t* client = (client_t*)d1;
    unsigned int ip = (unsigned long)d2;
    return IS_CLIENT_STATUS_NORMAL(client->status) && client->local_ip == ip;
}

int compare_clients_by_remote_ip_and_port(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    client_t* client = (client_t*)d1;
    pair_t* p = (pair_t*)d2;
    return client->remote_ip == (unsigned long)p->key && client->remote_port == (unsigned long)p->val;
}

unsigned char netmask()
{
    return this.netmask;
}

void free_client(void* c, size_t l)
{
    client_t* client = c;
    hash_free(&client->recv_table);
    free(c);
}

