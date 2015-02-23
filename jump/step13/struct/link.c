#include "link.h"

int link_init(link_t* l, link_functor_t functor)
{
    link_node_t* node = malloc(sizeof(link_node_t));
    if (node == NULL) return 0;
    node->data = NULL;
    node->len = 0;
    node->prev = node->next = node;
    l->functor = functor;
    l->header = node;
    l->count = 0;
    return 1;
}

void link_free(link_t* l)
{
    link_node_t *node, *next;
    size_t i, m;

    node = l->header;
    for (i = 0, m = link_count(l) + 1; i < m; ++i)
    {
        next = node->next;
        if (l->functor.free) l->functor.free(node->data, node->len);
        free(node);
        node = next;
    }
    l->header = NULL;
    l->count = 0;
}

int link_insert_head(link_t* l, const void* data, const size_t len)
{
    link_node_t* node = malloc(sizeof(link_node_t));
    if (node == NULL) return 0;

    node->data = l->functor.dup(data, len);
    node->len = len;
    node->prev = l->header;
    node->next = l->header->next;
    l->header->next->prev = node;
    l->header->next = node;
    ++l->count;
    return 1;
}

int link_insert_tail(link_t* l, const void* data, const size_t len)
{
    link_node_t* node = malloc(sizeof(link_node_t));
    if (node == NULL) return 0;

    node->data = l->functor.dup(data, len);
    node->len = len;
    node->prev = l->header->prev;
    node->next = l->header;
    l->header->prev->next = node;
    l->header->prev = node;
    ++l->count;
    return 1;
}

int link_pop_head(link_t* l)
{
    link_node_t* node;
    if (link_count(l) == 0) return 0;

    node = l->header->next;
    l->header->next = node->next;
    node->next->prev = l->header;
    --l->count;

    if (l->functor.free) l->functor.free(node->data, node->len);
    free(node);

    return 1;
}

int link_pop_tail(link_t* l)
{
    link_node_t* node;
    if (link_count(l) == 0) return 0;

    node = l->header->prev;
    l->header->prev = node->prev;
    node->prev->next = l->header;
    --l->count;

    if (l->functor.free) l->functor.free(node->data, node->len);
    free(node);

    return 1;
}

void link_clear(link_t* l)
{
    link_node_t *node, *next;
    size_t i, m;

    node = l->header->next;
    for (i = 0, m = link_count(l); i < m; ++i)
    {
        next = node->next;
        if (l->functor.free) l->functor.free(node->data, node->len);
        free(node);
        node = next;
    }
    l->header->prev = l->header->next = l->header;
    l->count = 0;
}

int link_exists(link_t* l, const void* data, const size_t len)
{
    link_node_t* node;
    size_t i, m;

    node = l->header->next;
    for (i = 0, m = link_count(l); i < m; ++i)
    {
        if (l->functor.compare(data, len, node->data, node->len)) return 1;
        node = node->next;
    }
    return 0;
}

void* link_first(link_t* l)
{
    if (link_count(l) == 0) return NULL;
    return l->header->next->data;
}

void* link_last(link_t* l)
{
    if (link_count(l) == 0) return NULL;
    return l->header->prev->data;
}

link_iterator_t link_begin(link_t* l)
{
    link_iterator_t iter;
    iter.node = l->header->next;
    iter.data = iter.node->data;
    iter.len  = iter.node->len;
    iter.idx  = 0;
    iter.rev  = 0;
    return iter;
}

link_iterator_t link_rev_begin(link_t* l)
{
    link_iterator_t iter;
    iter.node = l->header->prev;
    iter.data = iter.node->data;
    iter.len  = iter.node->len;
    iter.idx  = 0;
    iter.rev  = 1;
    return iter;
}

static link_iterator_t _link_prev(link_t* l, link_iterator_t iter)
{
    link_iterator_t ret = iter;
    ret.node = iter.node->prev;
    ret.data = ret.node->data;
    ret.len  = ret.node->len;
    ret.idx += iter.rev ? 1 : -1;
    ret.rev  = iter.rev;
    return ret;
}

static link_iterator_t _link_next(link_t* l, link_iterator_t iter)
{
    link_iterator_t ret = iter;
    ret.node = iter.node->next;
    ret.data = ret.node->data;
    ret.len  = ret.node->len;
    ret.idx += iter.rev ? -1 : 1;
    ret.rev  = iter.rev;
    return ret;
}

link_iterator_t link_prev(link_t* l, link_iterator_t iter)
{
    if (iter.rev) return _link_next(l, iter);
    else return _link_prev(l, iter);
}

link_iterator_t link_next(link_t* l, link_iterator_t iter)
{
    if (iter.rev) return _link_prev(l, iter);
    else return _link_next(l, iter);
}

void* link_dummy_dup(const void* data, const size_t len)
{
    return (void*)data;
}

void link_dummy_free(void* key, size_t len)
{
}

void link_normal_free(void* key, size_t len)
{
    free(key);
}
