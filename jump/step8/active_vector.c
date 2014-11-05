#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "active_vector.h"

#define BEGIN_CAPACITY      11
#define CAPACITY_LIMIT      (1024 * 1024)
#define CAPACITY_LIMIT_STEP 100

int active_vector_init(active_vector_t* v, active_vector_functor_t functor)
{
    v->elements = malloc(sizeof(active_vector_node_t) * BEGIN_CAPACITY);
    if (v->elements == NULL) return 0;
    v->functor = functor;
    v->count = 0;
    v->capacity = BEGIN_CAPACITY;
    return 1;
}

void active_vector_free(active_vector_t* v)
{
    size_t i;
    if (v->functor.free)
    {
        for (i = 0; i < v->count; ++i)
        {
            v->functor.free(v->elements[i].data, v->elements[i].len);
        }
    }
    free(v->elements);
    v->elements = NULL;
    v->count = 0;
    v->capacity = 0;
}

int active_vector_append(active_vector_t* v, const void* data, const size_t len)
{
    if (v->count >= v->capacity)
    {
        size_t capacity = (v->capacity < CAPACITY_LIMIT) ? (v->capacity << 1) : (v->capacity + CAPACITY_LIMIT_STEP);
        active_vector_node_t* elements = malloc(sizeof(active_vector_node_t) * capacity);
        if (elements == NULL) return 0;
        memcpy(elements, v->elements, sizeof(active_vector_node_t) * v->count);
        v->elements = elements;
        v->capacity = capacity;
    }
    v->elements[v->count].data = v->functor.dup(data, len);
    v->elements[v->count].len = len;
    v->elements[v->count].idx = 0;
    ++v->count;
    return 1;
}

void active_vector_up(active_vector_t* v, size_t idx)
{
    size_t prev = idx - 1;
    if (idx >= v->count) return;
    ++v->elements[idx].idx;
    if (idx && v->elements[idx].idx > v->elements[prev].idx)
    {
        SWAP(v->elements[idx].data, v->elements[prev].data);
        SWAP(v->elements[idx].len, v->elements[prev].len);
        SWAP(v->elements[idx].idx, v->elements[prev].idx);
    }
}

ssize_t active_vector_lookup(
    active_vector_t* v,
    int (*compare)(const void*, const size_t, const void*, const size_t),
    const void* data,
    const size_t len
)
{
    size_t i;
    for (i = 0; i < v->count; ++i)
    {
        if (compare(v->elements[i].data, v->elements[i].len, data, len))
        {
            active_vector_up(v, i);
            return i;
        }
    }
    return -1;
}

