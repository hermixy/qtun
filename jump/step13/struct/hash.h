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
    hash_bucket_t** buckets;
    size_t*         buckets_pre_length; // 每个吊桶的长度
    size_t          buckets_count;
    size_t          count;
}hash_dict_t;

typedef struct
{
    pair_t         data;
    int            dict;
    size_t         bidx;
    size_t         idx;
    hash_bucket_t* node;
    int            end;
} hash_iterator_t;

typedef struct
{
    hash_functor_t functor;
    hash_dict_t    dicts[2];
    size_t         max_length;         // 允许的最大吊桶长度
    int            rehashing;          // 正在rehash
    size_t         rehashing_idx;      // 当前正在rehash的指针
    size_t         buckets_idx;        // 当前使用素数的索引
} hash_t;

extern int hash_init(hash_t* h, hash_functor_t functor, size_t max_length);
extern void hash_free(hash_t* h);
extern int hash_set(hash_t* h, const void* key, const size_t key_len, const void* val, const size_t val_len);
extern int hash_get(hash_t* h, const void* key, const size_t key_len, void** val, size_t* val_len);
extern int hash_del(hash_t* h, const void* key, const size_t key_len);
extern void hash_clear(hash_t* h);

extern hash_iterator_t hash_begin(hash_t* h);
extern hash_iterator_t hash_next(hash_t* h, hash_iterator_t iter);

#define hash_count(h) ((h)->dicts[0].count + (h)->dicts[1].count)
#define hash_is_end(iter) iter.end

extern void* hash_dummy_dup(const void* data, const size_t len);
extern void hash_dummy_free(void* key, size_t key_len, void* val, size_t val_len);

#endif
