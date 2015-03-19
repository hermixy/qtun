#ifndef _POOL_H_
#define _POOL_H_

#include <stdlib.h>

#define ROOM_COUNT 10

typedef struct
{
    void*         ptr;
    size_t        length;
    size_t        capacity;
    unsigned char used;
} pool_room_t;

typedef struct
{
    pool_room_t room[ROOM_COUNT];
    size_t      total;
    size_t      max_capacity;
} pool_t;

extern int pool_init(pool_t* p);
extern void pool_free(pool_t* p);
extern void pool_gc(pool_t* p);
extern void* pool_room_alloc(pool_t* p, size_t idx, size_t len);
extern void* pool_room_realloc(pool_t* p, size_t idx, size_t len);
extern void pool_room_free(pool_t* p, size_t idx);

#endif

