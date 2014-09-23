#ifndef _HASH_H_
#define _HASH_H_

#include "typedef.h"

typedef struct hash_bucket_s hash_bucket_t;

struct hash_bucket_s
{
    pair_t         data;
    hash_bucket_t* next;
};

typedef struct
{
    size_t (*hash)(const void*, const size_t);
    int    (*compare)(const void*, const size_t, const void*, const size_t);
    void*  (*dup_key)(const void*, const size_t);
    void*  (*dup_val)(const void*, const size_t);
    void   (*free)(void*, size_t, void*, size_t);
    void   (*free_val)(void*, size_t);
}hash_functor_t;

typedef struct
{
    hash_functor_t  functor;
    hash_bucket_t** buckets;
    size_t          max_buckets_length; // 当前吊桶最大长度
    size_t          buckets_count;      // 吊桶数
    size_t          buckets_idx;        // 当前使用素数的索引
    size_t          count;              // 元素总数
    size_t          max_length;         // 允许的最大吊桶长度
}hash_t;

extern int hash_init(hash_t* h, hash_functor_t functor, size_t max_length);
extern void hash_free(hash_t* h);
extern int hash_set(hash_t* h, const void* key, const size_t key_len, const void* val, const size_t val_len);
extern int hash_get(hash_t* h, const void* key, const size_t key_len, void** val, size_t* val_len);

#endif
