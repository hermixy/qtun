#ifndef _GROUP_POOL_H_
#define _GROUP_POOL_H_

#include <stdlib.h>

typedef struct group_pool_room_s group_pool_room_t;

typedef struct
{
    group_pool_room_t* node;
    unsigned char      ptr[];
} group_pool_zone_t;

struct group_pool_room_s
{
    group_pool_zone_t* zone;
    size_t             length;
    size_t             capacity;
    unsigned short     hint;
    group_pool_room_t* prev;
    group_pool_room_t* next;
};

typedef struct
{
    group_pool_room_t* used;
    group_pool_room_t* free;
    size_t             count;
} group_pool_t;

extern int group_pool_init(group_pool_t* p);
extern void group_pool_free(group_pool_t* p);
extern void* group_pool_room_alloc(group_pool_t* p, size_t len);
extern void group_pool_room_free(group_pool_t* p, void* ptr);
extern void* group_pool_room_realloc(group_pool_t* p, void* ptr, size_t len);

#endif

