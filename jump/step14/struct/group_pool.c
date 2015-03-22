#include <string.h>

#include "../library/common.h"
#include "group_pool.h"

#define MAX_HINT 200
#define CAPACITY_LIMIT (4 * 1024 * 1024)

int group_pool_init(group_pool_t* p)
{
    p->used = p->free = NULL;
    p->count = 0;
    return 1;
}

void group_pool_free(group_pool_t* p)
{
    group_pool_room_t* node = p->used;
    while (node)
    {
        group_pool_room_t* next = node->next;
        free(node->zone->ptr);
        free(node);
        node = next;
    }
    node = p->free;
    while (node)
    {
        group_pool_room_t* next = node->next;
        free(node->zone->ptr);
        free(node);
        node = next;
    }
    p->used = p->free = NULL;
    p->count = 0;
}

static group_pool_room_t* group_pool_remove_free(group_pool_t* p, group_pool_room_t* node)
{
    group_pool_room_t *prev, *next;
    if (node == NULL) return NULL;
    node->length = 0;
    prev = node->prev;
    next = node->next;
    free(node->zone->ptr);
    free(node);
    if (prev == NULL) p->free = next;
    else prev->next = next;
    if (next) next->prev = prev;
    return next;
}

void* group_pool_room_alloc(group_pool_t* p, size_t len)
{
    group_pool_room_t* node = p->free;
    while (node)
    {
        if (node->capacity >= len)
        {
            group_pool_room_t *prev = node->prev, *next = node->next;
            node->length = len;
            node->prev = NULL;
            node->next = p->used;
            if (p->used) p->used->prev = node;
            p->used = node;
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            if (node == p->free) p->free = next;
            return node->zone->ptr;
        }
        if (node->hint++ >= MAX_HINT)
        {
            node = group_pool_remove_free(p, node);
            continue;
        }
        node = node->next;
    }
    node = malloc(sizeof(group_pool_room_t));
    if (node == NULL) return NULL;
    node->capacity = len << 1;
    if (node->capacity > CAPACITY_LIMIT) node->capacity = MIN(len, node->capacity);
    node->zone = malloc(sizeof(group_pool_zone_t) + node->capacity);
    if (node->zone == NULL)
    {
        free(node);
        return NULL;
    }
    node->length = len;
    node->next = p->used;
    node->prev = NULL;
    node->zone->node = node;
    if (p->used) p->used->prev = node;
    p->used = node;
    return node->zone->ptr;
}

void group_pool_room_free(group_pool_t* p, void* ptr)
{
    group_pool_zone_t* zone = (group_pool_zone_t*)((char*)ptr - sizeof(group_pool_room_t*));
    group_pool_room_t* node = zone->node;
    group_pool_room_t *prev = node->prev, *next = node->next;
    node->length = 0;
    node->hint = 0;
    node->prev = NULL;
    node->next = p->free;
    if (p->free) p->free->prev = node;
    p->free = node;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    if (node == p->used) p->used = next;
}

void* group_pool_room_realloc(group_pool_t* p, void* ptr, size_t len)
{
    group_pool_zone_t* zone = (group_pool_zone_t*)((char*)ptr - sizeof(group_pool_room_t*));
    group_pool_room_t* node = zone->node;
    void* ret;
    if (node->capacity >= len)
    {
        node->length = len;
        return ptr;
    }
    ret = group_pool_room_alloc(p, len);
    if (ret == NULL) return NULL;
    memcpy(ret, ptr, node->length);
    group_pool_room_free(p, ptr);
    return ret;
}

