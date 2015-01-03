#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "common.h"
#include "library.h"
#include "pool.h"

#include "msg.h"

link_t msg_process_handlers;

#define IS_SYSCONTROL_CMD(ch, op) (((ch >> 2) & MSG_OP_MASK) == op)

static unsigned char syscontrol_cmds[] = {
    /* op, request_mask, reply_mask */
    SYSCONTROL_MASK(SYS_LOGIN, 1, 1),
    SYSCONTROL_MASK(SYS_PING,  0, 0),
};

inline static int find_cmd(unsigned char op, unsigned char masks[2])
{
    size_t i;
    for (i = 0; i < sizeof(syscontrol_cmds); ++i)
    {
        if (IS_SYSCONTROL_CMD(syscontrol_cmds[i], op))
        {
            masks[0] = (syscontrol_cmds[i] >> 1) & 1;
            masks[1] = (syscontrol_cmds[i] >> 0) & 1;
            return 1;
        }
    }
    return 0;
}

int gzip_compress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    size_t dlen;
    z_stream stream;
    unsigned char* ptr;

    dlen = compressBound(src_len) + sizeof(unsigned int);
    ptr = pool_room_alloc(&this.pool, GZIP_ROOM_IDX, dlen);
    if (ptr == NULL) return 0;
    *dst = ptr;
    *(unsigned int*)ptr = htonl(src_len);
    ptr += sizeof(unsigned int);
    stream.zalloc = NULL;
    stream.zfree  = NULL;
    stream.opaque = NULL;
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) return 0;
    stream.next_in   = (Bytef*)src;
    stream.avail_in  = src_len;
    stream.next_out  = ptr;
    stream.avail_out = dlen - sizeof(unsigned int);
    deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    *dst_len = dlen - sizeof(unsigned int) - stream.avail_out;

    return 1;
}

int gzip_decompress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    z_stream stream;

    *dst_len = ntohl(*(unsigned int*)src);
    src = (const unsigned char*)src + sizeof(unsigned int);
    *dst = pool_room_alloc(&this.pool, GZIP_ROOM_IDX, *dst_len);
    if (*dst == NULL) return 0;
    stream.zalloc = NULL;
    stream.zfree  = NULL;
    stream.opaque = NULL;
    if (inflateInit(&stream) != Z_OK) return 0;
    stream.next_in   = (Bytef*)src;
    stream.avail_in  = src_len;
    stream.next_out  = *dst;
    stream.avail_out = *dst_len;
    inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    return 1;
}

int aes_encrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    AES_KEY key;
    unsigned char iv[AES_BLOCK_SIZE];
    size_t left = src_len % AES_BLOCK_SIZE;
    unsigned char* ptr;

    *dst_len = src_len;
    if (left) *dst_len += AES_BLOCK_SIZE - left;
    ptr = pool_room_alloc(&this.pool, AES_ROOM_IDX, *dst_len + sizeof(unsigned int));
    if (ptr == NULL) return 0;
    *dst = ptr;
    *(unsigned int*)ptr = htonl(src_len);
    ptr += sizeof(unsigned int);

    memcpy(iv, this.aes_iv, sizeof(iv));
    AES_set_encrypt_key(this.aes_key, this.aes_key_len, &key);
    AES_cbc_encrypt(src, ptr, *dst_len, &key, iv, AES_ENCRYPT);
    *dst_len += sizeof(unsigned int);

    return 1;
}

int aes_decrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    AES_KEY key;
    unsigned char iv[AES_BLOCK_SIZE];
    const unsigned char* ptr = (const unsigned char*)src + sizeof(unsigned int);

    *dst_len = ntohl(*(unsigned int*)src);
    *dst = pool_room_alloc(&this.pool, AES_ROOM_IDX, src_len - sizeof(unsigned int));
    if (*dst == NULL) return 0;

    memcpy(iv, this.aes_iv, sizeof(iv));
    AES_set_decrypt_key(this.aes_key, this.aes_key_len, &key);
    AES_cbc_encrypt(ptr, *dst, src_len - sizeof(unsigned int), &key, iv, AES_DECRYPT);

    return 1;
}

int des_encrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    DES_key_schedule k1, k2, k3;
    DES_cblock iv;
    size_t left = src_len % DES_KEY_SZ;
    unsigned char* ptr;

    *dst_len = src_len;
    if (left) *dst_len += DES_KEY_SZ - left;
    ptr = pool_room_alloc(&this.pool, DES_ROOM_IDX, *dst_len + sizeof(unsigned int));
    if (ptr == NULL) return 0;
    *dst = ptr;
    *(unsigned int*)ptr = htonl(src_len);
    ptr += sizeof(unsigned int);

    memcpy(iv, this.des_iv, sizeof(iv));
    switch (this.des_key_len)
    {
    case DES_KEY_SZ:
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_cbc_encrypt(src, ptr, *dst_len, &k1, &iv, DES_ENCRYPT);
        break;
    case DES_KEY_SZ * 2:
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_set_key((C_Block*)this.des_key[1], &k2);
        DES_ede2_cbc_encrypt(src, ptr, *dst_len, &k1, &k2, &iv, DES_ENCRYPT);
        break;
    default: // DES_KEY_SZ * 3
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_set_key((C_Block*)this.des_key[1], &k2);
        DES_set_key((C_Block*)this.des_key[2], &k3);
        DES_ede3_cbc_encrypt(src, ptr, *dst_len, &k1, &k2, &k3, &iv, DES_ENCRYPT);
        break;
    }
    *dst_len += sizeof(unsigned int);

    return 1;
}

int des_decrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    DES_key_schedule k1, k2, k3;
    DES_cblock iv;
    const unsigned char* ptr = (const unsigned char*)src + sizeof(unsigned int);

    *dst_len = ntohl(*(unsigned int*)src);
    *dst = pool_room_alloc(&this.pool, DES_ROOM_IDX, src_len - sizeof(unsigned int));
    if (*dst == NULL) return 0;

    memcpy(iv, this.des_iv, sizeof(iv));
    switch (this.des_key_len)
    {
    case DES_KEY_SZ:
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_cbc_encrypt(ptr, *dst, src_len - sizeof(unsigned int), &k1, &iv, DES_DECRYPT);
        break;
    case DES_KEY_SZ * 2:
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_set_key((C_Block*)this.des_key[1], &k2);
        DES_ede2_cbc_encrypt(ptr, *dst, src_len - sizeof(unsigned int), &k1, &k2, &iv, DES_DECRYPT);
        break;
    default: // DES_KEY_SZ * 3
        DES_set_key((C_Block*)this.des_key[0], &k1);
        DES_set_key((C_Block*)this.des_key[1], &k2);
        DES_set_key((C_Block*)this.des_key[2], &k3);
        DES_ede3_cbc_encrypt(ptr, *dst, src_len - sizeof(unsigned int), &k1, &k2, &k3, &iv, DES_DECRYPT);
        break;
    }

    return 1;
}

static int msg_process_handlers_compare(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    const msg_process_handler_t *dst1 = (const msg_process_handler_t*)d1, *dst2 = (const msg_process_handler_t*)d2;
    return dst1->do_handler == dst2->do_handler && dst1->undo_handler == dst2->undo_handler;
}

void init_msg_process_handler()
{
    link_functor_t func = {
        msg_process_handlers_compare,
        link_dummy_dup,
        link_normal_free
    };
    link_init(&msg_process_handlers, func);
}

int append_msg_process_handler(
    int type,
    int id,
    size_t room_id,
    int (*do_handler)(const void*, const unsigned int, void**, unsigned int*),
    int (*undo_handler)(const void*, const unsigned int, void**, unsigned int*))
{
    msg_process_handler_t* h = malloc(sizeof(*h));
    int rc;
    if (h == NULL) return 0;
    h->do_handler = do_handler;
    h->undo_handler = undo_handler;
    h->room_id = room_id;
    rc = link_insert_tail(&msg_process_handlers, h, sizeof(*h));
    if (!rc) free(h);
    if (type == MSG_PROCESS_COMPRESS_HANDLER)
        this.compress |= id;
    else /* if (type == MSG_PROCESS_ENCRYPT_HANDLER) */
        this.encrypt |= id;
    return rc;
}

size_t msg_data_length(const msg_t* msg)
{
    return little2host16(msg->len) * 16 + little2host16(msg->pfx);
}

int process_asc(void* src, unsigned int src_len, void** dst, unsigned int* dst_len, int* want_free, size_t* room_id)
{
    size_t link_cnt = link_count(&msg_process_handlers);
    *want_free = 0;

    if (link_cnt == 0)
    {
        *dst = src;
        *dst_len = src_len;
    }
    else if (link_cnt == 1)
    {
        msg_process_handler_t* handler = (msg_process_handler_t*)link_first(&msg_process_handlers);
        if (!handler->do_handler(src, src_len, dst, dst_len)) return 0;
        *want_free = 1;
        *room_id = handler->room_id;
    }
    else
    {
        link_iterator_t iter;
        int free_src = 0;
        iter = link_begin(&msg_process_handlers);
        while (!link_is_end(&msg_process_handlers, iter))
        {
            msg_process_handler_t* handler = (msg_process_handler_t*)iter.data;
            if (!handler->do_handler(src, src_len, dst, dst_len)) return 0;
            if (free_src) pool_room_free(&this.pool, *room_id);
            src = *dst;
            src_len = *dst_len;
            free_src = 1;
            iter = link_next(&msg_process_handlers, iter);
            *want_free = 1;
            *room_id = handler->room_id;
        }
    }
    return 1;
}

msg_t* new_login_msg(unsigned int ip, unsigned char mask, unsigned char request)
{
    struct timeval tv;
    msg_t* ret = NULL;
    sys_login_msg_t msg;
    void* dst;
    unsigned int dst_len;
    int want_free = 0;
    size_t room_id;
    unsigned char cmd_mask[2];

    memcpy(msg.check, SYS_MSG_CHECK, sizeof(msg.check));
    msg.ip = ip;
    msg.mask = mask;

    if (!find_cmd(SYS_LOGIN, cmd_mask)) goto end;
    if (cmd_mask[request ? 0 : 1])
    {
        if (!process_asc((void*)&msg, (unsigned int)sizeof(msg), &dst, &dst_len, &want_free, &room_id)) goto end;
    }
    else
    {
        dst = &msg;
        dst_len = sizeof(msg);
    }

    gettimeofday(&tv, NULL);

    ret = pool_room_alloc(&this.pool, MSG_ROOM_IDX, sizeof(msg_t) + dst_len);
    ret->syscontrol = 1;
    ret->compress   = this.compress;
    ret->encrypt    = this.encrypt;
    ret->ident      = htonl(++this.msg_ident);
    ret->sec        = htonl(tv.tv_sec);
    ret->usec       = little32(tv.tv_usec);
    ret->len        = little16(floor(dst_len / 16));
    ret->pfx        = little16(dst_len % 16);
    ret->sys        = MAKE_SYS_OP(SYS_LOGIN, request ? 1 : 0);
    ret->checksum   = 0;
    memcpy(ret->data, dst, dst_len);
    ret->checksum   = checksum(ret, sizeof(msg_t) + dst_len);
end:
    if (want_free) pool_room_free(&this.pool, room_id);
    return ret;
}

msg_t* new_keepalive_msg(unsigned char request)
{
    struct timeval tv;
    msg_t* ret = pool_room_alloc(&this.pool, MSG_ROOM_IDX, sizeof(msg_t));
    if (ret == NULL) goto end;
    gettimeofday(&tv, NULL);

    ret->syscontrol = 1;
    ret->compress   = 0;
    ret->encrypt    = 0;
    ret->ident      = htonl(++this.msg_ident);
    ret->sec        = htonl(tv.tv_sec);
    ret->usec       = little32(tv.tv_usec);
    ret->len        = 0;
    ret->pfx        = 0;
    ret->sys        = MAKE_SYS_OP(SYS_PING, request ? 1 : 0);
    ret->checksum   = 0;
    ret->checksum   = checksum(ret, sizeof(msg_t));
end:
    return ret;
}

int parse_msg(const msg_t* input, int* sys, void** output, unsigned short* output_len, size_t* room_id)
{
    unsigned short src_len = msg_data_length(input);
    link_iterator_t iter = link_rev_begin(&msg_process_handlers);
    const void* i = input->data;

    *sys = input->syscontrol;
    *output_len = src_len;
    *room_id = TMP_ROOM_IDX;
    *output = pool_room_alloc(&this.pool, *room_id, src_len);
    if (*output == NULL) return 0;
    memcpy(*output, input->data, src_len);
    if (input->compress == 0 && input->encrypt == 0) return 1;

    while (!link_is_end(&msg_process_handlers, iter))
    {
        void* o;
        unsigned int ol;
        msg_process_handler_t* handler = (msg_process_handler_t*)iter.data;

        if (!handler->undo_handler(i, src_len, &o, &ol))
        {
            *output = NULL;
            *output_len = 0;
            pool_room_free(&this.pool, *room_id);
            return 0;
        }
        pool_room_free(&this.pool, *room_id);
        i = *output = o;
        src_len = *output_len = ol;
        *room_id = handler->room_id;
        iter = link_next(&msg_process_handlers, iter);
    }
    return 1;
}

int parse_login_reply_msg(const msg_t* input, unsigned int* ip, unsigned char* mask)
{
    int sys;
    void* data;
    unsigned short len;
    sys_login_msg_t* login;
    size_t room_id;

    if (!parse_msg(input, &sys, &data, &len, &room_id))
    {
        SYSLOG(LOG_ERR, "parse sys_login_reply failed");
        return 0;
    }
    login = (sys_login_msg_t*)data;
    if (!sys || !CHECK_SYS_OP(input->sys, SYS_LOGIN, 0) || memcmp(login->check, SYS_MSG_CHECK, sizeof(login->check)))
    {
        SYSLOG(LOG_ERR, "Invalid sys_login_reply message");
        return 0;
    }
    *ip = login->ip;
    *mask = login->mask;
    pool_room_free(&this.pool, room_id);
    return 1;
}

