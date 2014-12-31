#ifndef _MSG_GROUP_H_
#define _MSG_GROUP_H_

#include "msg.h"

typedef struct
{
    msg_t** elements;
    size_t  count;
} msg_group_t;

extern void msg_group_free(msg_group_t* g);
extern msg_group_t* new_msg_group(const void* data, const unsigned short len);

#endif

