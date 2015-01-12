#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "msg.h"
#include "group_pool.h"
#include "pool.h"

#include "msg_group.h"

static int no_clip(msg_group_t* g, struct timeval tv, const void* data, const unsigned int len)
{
    msg_t* msg;
    if (len > this.max_length) return 0;
    msg = group_pool_room_alloc(&this.group_pool, sizeof(msg_t) + len);
    if (msg == NULL) return 0;
    g->elements = group_pool_room_alloc(&this.group_pool, sizeof(msg_t*));
    if (g->elements == NULL) return 0;
    g->count = 1;
    memcpy(msg->data, data, len);
    msg->syscontrol  = 0;
    msg->compress    = this.compress;
    msg->encrypt     = this.encrypt;
    msg->ident       = htonl(g->ident);
    msg->sec         = htonl(tv.tv_sec);
    msg->usec        = little32(tv.tv_usec);
    msg->len         = little16(floor(len / 16));
    msg->pfx         = little16(len % 16);
    msg->sys         = 0;
    msg->zone.unused = 0;
    msg->zone.clip   = 0;
    msg->zone.last   = 0;
    msg->zone.idx    = 0;
    msg->checksum    = 0;
    msg->checksum    = checksum(msg, sizeof(msg_t) + len);
    *g->elements     = msg;
    return 1;
}

void msg_group_free(msg_group_t* g)
{
    if (g)
    {
        if (g->elements)
        {
            size_t i;
            for (i = 0; i < g->count; ++i)
            {
                if (g->elements[i] == NULL) continue;
                group_pool_room_free(&this.group_pool, g->elements[i]);
            }
            group_pool_room_free(&this.group_pool, g->elements);
        }
        g->elements = NULL;
        g->count = 0;
        group_pool_room_free(&this.group_pool, g);
    }
}

msg_group_t* new_msg_group(const void* data, const unsigned short len)
{
    struct timeval tv;
    msg_group_t* ret = NULL;
    void* dst;
    unsigned int dst_len;
    int want_free = 0;
    size_t room_id;
    unsigned int left;
    size_t i = 0;

    if (!process_asc((void*)data, (unsigned int)len, &dst, &dst_len, &want_free, &room_id)) goto end;
    ret = group_pool_room_alloc(&this.group_pool, sizeof(msg_group_t));
    if (ret == NULL) goto end;
    ret->ident = ++this.msg_ident;
    if (dst_len <= this.max_length)
    {
        if (!no_clip(ret, tv, dst, dst_len)) goto end;
    }
    else
    {
        unsigned int sec;
        unsigned int usec;
        unsigned short len;
        unsigned char pfx;
        ret->count = ceil((double)dst_len / this.max_length);
        ret->elements = group_pool_room_alloc(&this.group_pool, sizeof(msg_t*) * ret->count);
        if (ret->elements == NULL) goto end;
        memset(ret->elements, 0, sizeof(msg_t*) * ret->count);

        gettimeofday(&tv, NULL);
        left = dst_len;
        sec = htonl(tv.tv_sec);
        usec = little32(tv.tv_usec);
        len = little16(floor(dst_len / 16));
        pfx = little16(dst_len % 16);
        while (left)
        {
            unsigned int length = left > this.max_length ? this.max_length : left;
            msg_t* msg = group_pool_room_alloc(&this.group_pool, sizeof(msg_t) + length);
            if (msg == NULL) goto end;
            ret->elements[i] = msg;
            memcpy(msg->data, dst, length);
            msg->syscontrol  = 0;
            msg->compress    = this.compress;
            msg->encrypt     = this.encrypt;
            msg->ident       = htonl(ret->ident);
            msg->sec         = sec;
            msg->usec        = usec;
            msg->len         = len;
            msg->pfx         = pfx;
            msg->sys         = 0;
            msg->zone.unused = 0;
            msg->zone.clip   = 1;
            msg->zone.last   = i == ret->count - 1 ? 1 : 0;
            msg->zone.idx    = little16((i * this.max_length) >> 3);
            msg->checksum    = 0;
            msg->checksum    = checksum(msg, sizeof(msg_t) + length);
            left -= length;
            dst  += length;
            ++i;
        }
    }
    if (want_free) pool_room_free(&this.pool, room_id);
    return ret;
end:
    if (want_free) pool_room_free(&this.pool, room_id);
    msg_group_free(ret);
    return NULL;
}

int parse_msg_group(unsigned short max_length, msg_group_t* g, void** output, unsigned short* output_len, size_t* room_id)
{
    unsigned short src_len = msg_data_length(g->elements[0]);
    link_iterator_t iter = link_rev_begin(&msg_process_handlers);
    const void* i;
    size_t j;

    *output_len = src_len;
    *room_id = TMP_ROOM_IDX;
    *output = pool_room_alloc(&this.pool, *room_id, src_len);
    if (*output == NULL) return 0;
    for (j = 0; j < g->count; ++j)
    {
        if (!g->elements[j]->zone.last) memcpy(*output + (g->elements[j]->zone.idx << 3), g->elements[j]->data, max_length);
        else memcpy(*output + (g->elements[j]->zone.idx << 3), g->elements[j]->data, src_len & max_length);
    }
    if (this.compress == 0 && this.encrypt == 0) return 1;

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

