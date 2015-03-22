#ifndef _MSG_GROUP_H_
#define _MSG_GROUP_H_

#include "../struct/hash.h"

#include "msg.h"

#define MSG_MAX_TTL 200

typedef struct
{
    msg_t**        elements;
    unsigned short count;
    unsigned int   ident;

    /* for recv */
    size_t         ttl_start;
} msg_group_t;

extern void msg_group_free(msg_group_t* g);
extern msg_group_t* new_msg_group(const void* data, const unsigned short len);
extern int parse_msg_group(unsigned short max_length, msg_group_t* g, void** output, unsigned short* output_len, size_t* room_id);
extern void checkout_ttl(hash_t* h);
extern unsigned short msg_idx(msg_t* m);

#endif

