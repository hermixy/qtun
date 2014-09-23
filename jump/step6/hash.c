#include "hash.h"

static unsigned int __prime_list[] =
{
  53u,         97u,         193u,       389u,       769u,
  1543u,       3079u,       6151u,      12289u,     24593u,
  49157u,      98317u,      196613u,    393241u,    786433u,
  1572869u,    3145739u,    6291469u,   12582917u,  25165843u,
  50331653u,   100663319u,  201326611u, 402653189u, 805306457u,
  1610612741u, 3221225473u, 4294967291u
};

int hash_init(hash_t* h, hash_functor_t functor, size_t max_length)
{
    h->max_length = max_length;
    h->count = 0;
    h->max_buckets_length = 0;
    h->buckets_idx = 0;
    h->buckets_count = __prime_list[h->buckets_idx];
    h->functor = functor;
    h->buckets = calloc(sizeof(hash_bucket_t*), h->buckets_count);
    return h->buckets != NULL;
}

void hash_free(hash_t* h)
{
    hash_bucket_t *node, *next;
    size_t i;
    for (i = 0; i < h->buckets_count; ++i)
    {
        node = h->buckets[i];
        while (node)
        {
            next = node->next;
            h->functor.free(node->data.key, node->data.key_len, node->data.val, node->data.val_len);
            node = next;
        }
    }
    free(h->buckets);
    h->buckets = NULL;
    h->max_buckets_length = 0;
    h->buckets_count = 0;
    h->buckets_idx = 0;
    h->count = 0;
}

int hash_set(hash_t* h, const void* key, const size_t key_len, const void* val, const size_t val_len)
{
    size_t idx = h->functor.hash(key, key_len) % h->buckets_count;
    hash_bucket_t* node = h->buckets[idx];
    size_t length = 0;

    while (node)
    {
        if (h->functor.compare(key, key_len, node->data.key, node->data.key_len))
        {
            h->functor.free_val(node->data.val, node->data.val_len);
            node->data.val = h->functor.dup_val(val, val_len);
            return 1;
        }
        ++length;
    }

    if (h->max_buckets_length >= h->max_length) // do nothing
    {
    }
    else
    {
        node = malloc(sizeof(hash_bucket_t));
        if (node == NULL) return 0;
    }
    node->data.key = h->functor.dup_key(key, key_len);
    node->data.key_len = key_len;
    node->data.val = h->functor.dup_val(val, val_len);
    node->data.val_len = val_len;
    node->next = h->buckets[idx];
    h->buckets[idx] = node;
    if (length >= h->max_buckets_length)
    return 1;
}

int hash_get(hash_t* h, const void* key, const size_t key_len, void** val, size_t* val_len)
{
    size_t idx = h->functor.hash(key, key_len) % h->buckets_count;
    hash_bucket_t* node = h->buckets[idx];

    while (node)
    {
        if (h->functor.compare(key, key_len, node->data.key, node->data.key_len))
        {
            *val = node->data.val;
            *val_len = node->data.val_len;
            return 1;
        }
        node = node->next;
    }
    return 0;
}
