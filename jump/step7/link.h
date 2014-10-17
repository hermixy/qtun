#ifndef _LINK_H_
#define _LINK_H_

#include <stdlib.h>

typedef struct link_node_s link_node_t;

struct link_node_s
{
    void*        data;
    size_t       len;
    link_node_t* prev;
    link_node_t* next;
};

typedef struct
{
    int   (*compare)(const void*, const size_t, const void*, const size_t);
    void* (*dup)(const void*, const size_t);
    void  (*free)(void*, size_t);
}link_functor_t;

typedef struct
{
    void*        data;
    size_t       len;
    link_node_t* node;
    size_t       idx;
    int          rev;
}link_iterator_t;

typedef struct
{
    link_functor_t functor;
    link_node_t*   header;
    size_t         count;
}link_t;

extern int link_init(link_t* l, link_functor_t functor);
extern void link_free(link_t* l);
extern int link_insert_head(link_t* l, const void* data, const size_t len);
extern int link_insert_tail(link_t* l, const void* data, const size_t len);
extern int link_pop_head(link_t* l);
extern int link_pop_tail(link_t* l);
extern void link_clear(link_t* l);
extern int link_exists(link_t* l, const void* data, const size_t len);
extern void* link_first(link_t* l);
extern void* link_last(link_t* l);

extern link_iterator_t link_begin(link_t* l);
extern link_iterator_t link_rev_begin(link_t* l);
extern link_iterator_t link_prev(link_t* l, link_iterator_t iter);
extern link_iterator_t link_next(link_t* l, link_iterator_t iter);

#define link_count(l) ((l)->count)
#define link_is_begin(l, iter) (iter.rev ? (iter.node == (l)->header->prev) : (iter.node == (l)->header->next))
#define link_is_end(l, iter) (iter.node == (l)->header)

extern void* link_dummy_dup(const void* data, const size_t len);
extern void link_dummy_free(void* data, size_t len);
extern void link_normal_free(void* data, size_t len);

#endif
