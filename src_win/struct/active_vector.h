#ifndef _ACTIVE_VECTOR_H_
#define _ACTIVE_VECTOR_H_

typedef struct active_vector_s active_vector_t;

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
    void*            data;
    size_t           len;
    ssize_t          idx;
    int              rev;
    active_vector_t* v;
} active_vector_iterator_t;

struct active_vector_s
{
    active_vector_functor_t functor;
    active_vector_node_t*   elements;
    size_t                  count;
    size_t                  capacity;
};

extern int active_vector_init(active_vector_t* v, active_vector_functor_t functor);
extern void active_vector_free(active_vector_t* v);
extern int active_vector_append(active_vector_t* v, const void* data, const size_t len);
extern ssize_t active_vector_lookup(
    active_vector_t* v,
    int (*compare)(const void*, const size_t, const void*, const size_t),
    const void* data,
    const size_t len
);
extern ssize_t active_vector_exists(
    active_vector_t* v,
    int (*compare)(const void*, const size_t, const void*, const size_t),
    const void* data,
    const size_t len
);
extern void active_vector_up(active_vector_t* v, size_t idx);
extern int active_vector_get(active_vector_t* v, size_t idx, void** data, size_t* len);
extern int active_vector_del(active_vector_t* v, size_t idx);

extern active_vector_iterator_t active_vector_begin(active_vector_t* v);
extern active_vector_iterator_t active_vector_rev_begin(active_vector_t* v);
extern active_vector_iterator_t active_vector_prev(active_vector_iterator_t i);
extern active_vector_iterator_t active_vector_next(active_vector_iterator_t i);

extern void* active_vector_dummy_dup(const void* data, const size_t len);
extern void active_vector_dummy_free(void* data, size_t len);
extern void active_vector_normal_free(void* data, size_t len);

#define active_vector_count(v) ((v)->count)
#define active_vector_is_begin(iter) (iter.rev ? (iter.idx == (iter.v->count - 1)) : (iter.idx == 0))
#define active_vector_is_end(iter) (iter.rev ? (iter.idx == -1) : (iter.idx == iter.v->count))
#define active_vector_iterator_idx(iter) (iter.idx)

#endif

