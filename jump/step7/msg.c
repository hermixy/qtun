#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "common.h"
#include "library.h"
#include "main.h"
#include "msg.h"

link_t msg_process_handlers;

int gzip_compress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    size_t dlen;
    z_stream stream;
    unsigned char* ptr;
    unsigned int i;

    dlen = compressBound(src_len) + sizeof(unsigned int);
    ptr = malloc(dlen);
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
    for (i = 0; i < *dst_len; ++i) printf("%02X ", ((unsigned char*)*dst)[i]);
    printf("\n");
    return 1;
}

int gzip_decompress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    z_stream stream;

    *dst_len = ntohl(*(unsigned int*)src);
    src = (const unsigned char*)src + sizeof(unsigned int);
    *dst = malloc(*dst_len);
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
    unsigned int i;

    for (i = 0; i < src_len; ++i) printf("%02X ", ((unsigned char*)src)[i]);
    printf("\n");

    *dst_len = src_len;
    if (left) *dst_len += AES_BLOCK_SIZE - left;
    ptr = malloc(*dst_len + sizeof(unsigned int));
    if (ptr == NULL) return 0;
    *dst = ptr;
    *(unsigned int*)ptr = htonl(src_len);
    ptr += sizeof(unsigned int);

    memcpy(iv, this.aes_iv, sizeof(this.aes_iv));
    AES_set_encrypt_key(this.aes_key, this.aes_key_len, &key);
    AES_cbc_encrypt(src, ptr, *dst_len, &key, iv, AES_ENCRYPT);
    *dst_len += sizeof(unsigned int);
    for (i = 0; i < *dst_len; ++i) printf("%02X ", ((unsigned char*)*dst)[i]);
    printf("\n");

    return 1;
}

int aes_decrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len)
{
    AES_KEY key;
    unsigned char iv[AES_BLOCK_SIZE];
    const unsigned char* ptr = (const unsigned char*)src + sizeof(unsigned int);

    *dst_len = ntohl(*(unsigned int*)src);
    *dst = malloc(src_len - sizeof(unsigned int));
    if (*dst == NULL) return 0;

    memcpy(iv, this.aes_iv, sizeof(this.aes_iv));
    AES_set_decrypt_key(this.aes_key, this.aes_key_len, &key);
    AES_cbc_encrypt(ptr, *dst, src_len - sizeof(unsigned int), &key, iv, AES_DECRYPT);

    return 1;
}

static int msg_process_handlers_compare(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    const msg_process_handler_t *dst1 = (const msg_process_handler_t*)d1, *dst2 = (const msg_process_handler_t*)d2;
    return dst1->type == dst2->type && dst1->id == dst2->id;
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
    int (*do_handler)(const void*, const unsigned int, void**, unsigned int*),
    int (*undo_handler)(const void*, const unsigned int, void**, unsigned int*))
{
    msg_process_handler_t* h = malloc(sizeof(*h));
    int rc;
    if (h == NULL) return 0;
    h->type = type;
    h->id = id;
    h->do_handler = do_handler;
    h->undo_handler = undo_handler;
    rc = link_insert_tail(&msg_process_handlers, h, sizeof(*h));
    if (!rc) free(h);
    return rc;
}

size_t msg_data_length(const msg_t* msg)
{
    return little2host16(msg->len) * 16 + little2host16(msg->pfx);
}

msg_t* new_sys_msg(const void* data, const unsigned short len)
{
    struct timeval tv;
    msg_t* ret = malloc(sizeof(msg_t) + len);
    if (ret == NULL) goto end;
    gettimeofday(&tv, NULL);

    ret->syscontrol = 1;
    ret->compress   = 0;
    ret->encrypt    = 0;
    ret->ident      = htonl(++this.msg_ident);
    ret->sec        = htonl(tv.tv_sec);
    ret->usec       = little32(tv.tv_usec);
    ret->len        = little16(floor(len / 16));
    ret->pfx        = little16(len % 16);
    ret->unused     = 0;
    ret->checksum   = 0;
    memcpy(ret->data, data, len);
    ret->checksum   = htons(checksum(ret, sizeof(msg_t) + len));
end:
    return ret;
}

msg_t* new_msg(const void* data, const unsigned short len)
{
    struct timeval tv;
    msg_t* ret = malloc(sizeof(msg_t) + len);
    link_iterator_t iter = link_begin(&msg_process_handlers);
    void *dst;
    unsigned int src_len = len, dst_len = len;

    if (ret == NULL) goto end;
    gettimeofday(&tv, NULL);

    ret->syscontrol = 0;
    memcpy(ret->data, data, len);
    while (!link_is_end(&msg_process_handlers, iter))
    {
        msg_process_handler_t* handler = (msg_process_handler_t*)iter.data;
        if (!handler->do_handler(ret->data, src_len, &dst, &dst_len)) goto failed;
        ret = realloc(ret, sizeof(msg_t) + dst_len);
        if (ret == NULL)
        {
            free(dst);
            goto failed;
        }
        memcpy(ret->data, dst, dst_len);
        free(dst);
        if (handler->type == MSG_PROCESS_COMPRESS_HANDLER)
        {
            ret->compress |= handler->id;
        }
        else /* if (handler->type == MSG_PROCESS_ENCRYPT_HANDLER) */
        {
            ret->encrypt |= handler->id;
        }
        iter = link_next(&msg_process_handlers, iter);
    }
    ret->ident    = htonl(++this.msg_ident);
    ret->sec      = htonl(tv.tv_sec);
    ret->usec     = little32(tv.tv_usec);
    ret->len      = little16(floor(dst_len / 16));
    ret->pfx      = little16(dst_len % 16);
    ret->unused   = 0;
    ret->checksum = 0;
    ret->checksum = htons(checksum(ret, sizeof(msg_t) + dst_len));
end:
    return ret;
failed:
    free(ret);
    return NULL;
}

int parse_msg(const msg_t* input, int* sys, void** output, unsigned short* output_len)
{
    unsigned short src_len = msg_data_length(input);
    link_iterator_t iter = link_rev_begin(&msg_process_handlers);
    const void* i = input->data;

    *sys = input->syscontrol;
    *output_len = src_len;
    *output = malloc(src_len);
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
            free(*output);
            return 0;
        }
        free(*output);
        i = *output = o;
        src_len = *output_len = ol;
        iter = link_next(&msg_process_handlers, iter);
    }
    return 1;
}
