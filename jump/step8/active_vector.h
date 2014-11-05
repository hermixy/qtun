#ifndef _ACTIVE_VECTOR_H_
#define _ACTIVE_VECTOR_H_

typedef struct
{
    void*  data;
    size_t len;
    size_t idx;
} active_vector_node_t;

typedef struct
{
    void* (*dup)(const void*, const size_t);
    void  (*free)(void*, size_t);
} active_vector_functor_t;

typedef struct
{
    active_vector_functor_t functor;
    active_vector_node_t*   elements;
    size_t                  count;
    size_t                  capacity;
} active_vector_t;

extern int active_vector_init(active_vector_t* v, active_vector_functor_t functor);
extern void active_vector_free(active_vector_t* v);
extern int active_vector_append(active_vector_t* v, const void* data, const size_t len);
extern ssize_t active_vector_lookup(
    active_vector_t* v,
    int (*compare)(const void*, const size_t, const void*, const size_t),
    const void* data,
    const size_t len
);
extern void active_vector_up(active_vector_t* v, size_t idx);

#endif

