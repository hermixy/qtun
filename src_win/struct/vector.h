#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdlib.h>

typedef struct vector_s vector_t;

typedef struct
{
    void*  data;
    size_t len;
} vector_node_t;

typedef struct
{
    void* (*dup)(const void*, const size_t);
    void  (*free)(void*, size_t);
} vector_functor_t;

struct vector_s
{
    vector_functor_t functor;
    vector_node_t*   elements;
    size_t           count;
    size_t           capacity;
};

extern int vector_init(vector_t* v, vector_functor_t functor);
extern void vector_free(vector_t* v);
extern int vector_push_back(vector_t* v, const void* data, const size_t len);
extern int vector_push_head(vector_t* v, const void* data, const size_t len);
extern int vector_pop_back(vector_t* v, void** data, size_t* len);
extern int vector_pop_head(vector_t* v, void** data, size_t* len);

extern void* vector_dummy_dup(const void* data, const size_t len);
extern void vector_dummy_free(void* data, size_t len);
extern void vector_normal_free(void* data, size_t len);

#define vector_count(v) (v->count)

#endif

