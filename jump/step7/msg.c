#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "msg.h"

link_t msg_process_handlers;

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
    ret->hold       = 0;
    ret->unused     = 0;
    ret->checksum   = 0;
    memcpy(ret->data, data, len);
    ret->checksum   = htons(ret, sizeof(msg_t) + len);
end:
    return ret;
}

msg_t* new_msg(const unsigned char flag, const void* data, const unsigned short len)
{
    struct timeval tv;
    msg_t* ret = malloc(sizeof(msg_t));
    link_iterator_t iter = link_begin(&msg_process_handlers);
    void *src = (void*)data, *dst;
    size_t src_len = len, dst_len = len;

    if (ret == NULL) goto end;
    gettimeofday(&tv, NULL);

    ret->syscontrol = 0;
    memcpy(ret->data, data, len);
    while (!link_is_end(&msg_process_handlers, iter))
    {
        msg_process_handler_t* handler = (msg_process_handler_t*)iter.data;
        if (!handler->do_handler(src, src_len, &dst, &dst_len)) goto failed;
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
    ret->hold     = 0;
    ret->unused   = 0;
    ret->checksum = 0;
    ret->checksum = htons(ret, sizeof(msg_t) + dst_len);
end:
    return ret;
failed:
    free(ret);
    return NULL;
}

int parse_msg(const msg_t* input, int* sys, void** output, unsigned short* output_len)
{
    unsigned short src_len = little2host16(input->len) * 16 + little2host16(input->pfx);
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
        size_t ol;
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

