#include <stdio.h>

#include "../library/common.h"
#include "pool.h"

#define CAPACITY_LIMIT (4 * 1024 * 1024)

int pool_init(pool_t* p)
{
    size_t i;
    for (i = 0; i < ROOM_COUNT; ++i)
    {
        p->room[i].ptr = NULL;
        p->room[i].length = p->room[i].capacity = 0;
        p->room[i].used = 0;
    }
    p->total = p->max_capacity = 0;
    return 1;
}

void pool_free(pool_t* p)
{
    size_t i;
    for (i = 0; i < ROOM_COUNT; ++i)
    {
        if (p->room[i].ptr)
        {
            free(p->room[i].ptr);
            p->room[i].ptr = NULL;
            p->room[i].length = p->room[i].capacity = 0;
            p->room[i].used = 0;
        }
    }
    p->total = p->max_capacity = 0;
}

void pool_gc(pool_t* p)
{
    size_t i;
    for (i = 0; i < ROOM_COUNT; ++i)
    {
        if (p->room[i].ptr && !p->room[i].used)
        {
            free(p->room[i].ptr);
            p->room[i].ptr = NULL;
            p->room[i].length = p->room[i].capacity = 0;
        }
    }
}

static void* _pool_room_alloc(pool_t* p, size_t idx, size_t len)
{
    size_t capacity;
    if (p->room[idx].capacity >= len) return p->room[idx].ptr;
    capacity = MAX(p->room[idx].capacity << 1, len);
    if (capacity > CAPACITY_LIMIT) capacity = len;
    p->room[idx].ptr = realloc(p->room[idx].ptr, capacity);
    if (p->room[idx].ptr == NULL)
    {
        capacity = MIN(capacity, len);
        pool_gc(p);
        p->room[idx].ptr = malloc(capacity);
        if (p->room[idx].ptr == NULL)
        {
            p->room[idx].length = p->room[idx].capacity = 0;
            p->room[idx].used = 0;
            return NULL;
        }
    }
    p->room[idx].length = len;
    p->room[idx].capacity = capacity;
    p->room[idx].used = 1;
    return p->room[idx].ptr;
}

void* pool_room_alloc(pool_t* p, size_t idx, size_t len)
{
    if (idx >= ROOM_COUNT) return NULL;
    if (p->room[idx].used)
    {
        SYSLOG(LOG_ERR, "using used room: %lu", idx);
        return NULL;
    }
    return _pool_room_alloc(p, idx, len);
}

void* pool_room_realloc(pool_t* p, size_t idx, size_t len)
{
    if (idx >= ROOM_COUNT) return NULL;
    return _pool_room_alloc(p, idx, len);
}

void pool_room_free(pool_t* p, size_t idx)
{
    if (idx > ROOM_COUNT) return;
    p->room[idx].used = 0;
}

