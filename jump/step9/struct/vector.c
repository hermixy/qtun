#include <string.h>

#include "vector.h"

#define BEGIN_CAPACITY      11
#define CAPACITY_LIMIT      (1024 * 1024)
#define CAPACITY_LIMIT_STEP 100

int vector_init(vector_t* v, vector_functor_t functor)
{
    v->elements = malloc(sizeof(vector_node_t) * BEGIN_CAPACITY);
    if (v->elements == NULL) return 0;
    v->functor = functor;
    v->count = 0;
    v->capacity = BEGIN_CAPACITY;
    return 1;
}

void vector_free(vector_t* v)
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

int vector_push_back(vector_t* v, const void* data, const size_t len)
{
    if (v->count >= v->capacity)
    {
        size_t capacity = (v->capacity < CAPACITY_LIMIT) ? (v->capacity << 1) : (v->capacity + CAPACITY_LIMIT_STEP);
        vector_node_t* elements = malloc(sizeof(vector_node_t) * capacity);
        if (elements == NULL) return 0;
        memcpy(elements, v->elements, sizeof(vector_node_t) * v->count);
        v->elements = elements;
        v->capacity = capacity;
    }
    v->elements[v->count].data = v->functor.dup(data, len);
    v->elements[v->count].len = len;
    ++v->count;
    return 1;
}

int vector_push_head(vector_t* v, const void* data, const size_t len)
{
    if (v->count >= v->capacity)
    {
        size_t capacity = (v->capacity < CAPACITY_LIMIT) ? (v->capacity << 1) : (v->capacity + CAPACITY_LIMIT_STEP);
        vector_node_t* elements = malloc(sizeof(vector_node_t) * capacity);
        if (elements == NULL) return 0;
        memcpy(elements + 1, v->elements, sizeof(vector_node_t) * v->count);
        v->elements = elements;
        v->capacity = capacity;
        elements[0].data = v->functor.dup(data, len);
        elements[0].len = len;
    }
    else
    {
        memmove(v->elements + 1, v->elements, sizeof(vector_node_t) * v->count);
        v->elements[0].data = v->functor.dup(data, len);
        v->elements[0].len = len;
    }
    ++v->count;
    return 1;
}

int vector_pop_back(vector_t* v, void** data, size_t* len)
{
    if (v->count == 0) return 0;
    *data = v->elements[v->count - 1].data;
    *len = v->elements[v->count - 1].len;
    --v->count;
    return 1;
}

int vector_pop_head(vector_t* v, void** data, size_t* len)
{
    if (v->count == 0) return 0;
    *data = v->elements[0].data;
    *len = v->elements[0].len;
    --v->count;
    if (v->count) memmove(v->elements, v->elements + 1, sizeof(vector_node_t) * v->count);
    return 1;
}

void* vector_dummy_dup(const void* data, const size_t len)
{
    return (void*)data;
}

void vector_dummy_free(void* data, size_t len)
{
}

void vector_normal_free(void* data, size_t len)
{
    free(data);
}

