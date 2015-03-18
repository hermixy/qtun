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
    h->rehashing = 0;
    h->rehashing_idx = 0;
    h->max_length = max_length;
    h->buckets_idx = 0;
    h->functor = functor;

    h->dicts[0].buckets_count = __prime_list[h->buckets_idx];
    h->dicts[0].buckets = calloc(sizeof(hash_bucket_t*), h->dicts[0].buckets_count);
    h->dicts[0].buckets_pre_length = calloc(sizeof(size_t), h->dicts[0].buckets_count);
    h->dicts[0].count = 0;
    h->dicts[1].buckets = NULL;
    h->dicts[1].buckets_pre_length = NULL;
    h->dicts[1].buckets_count = 0;
    h->dicts[1].count = 0;

    return h->dicts[0].buckets != NULL && h->dicts[0].buckets_pre_length != NULL;
}

void hash_free(hash_t* h)
{
    hash_bucket_t *node, *next;
    size_t i, j;
    for (j = 0; j < (h->rehashing ? 2UL : 1UL); ++j)
    {
        for (i = 0; i < h->dicts[j].buckets_count; ++i)
        {
            node = h->dicts[j].buckets[i];
            while (node)
            {
                next = node->next;
                h->functor.free(node->data.key, node->data.key_len, node->data.val, node->data.val_len);
                free(node);
                node = next;
            }
        }
        free(h->dicts[j].buckets);
        free(h->dicts[j].buckets_pre_length);
        h->dicts[j].buckets = NULL;
        h->dicts[j].buckets_pre_length = NULL;
        h->dicts[j].buckets_count = 0;
        h->dicts[j].count = 0;
    }
    h->buckets_idx = 0;
    h->rehashing = 0;
    h->rehashing_idx = 0;
}

static void rehash_start(hash_t* h)
{
    if (++h->buckets_idx >= sizeof(__prime_list)) return; // 数据太多
    h->dicts[1].buckets_count = __prime_list[h->buckets_idx];
    h->dicts[1].buckets = calloc(sizeof(hash_bucket_t*), h->dicts[1].buckets_count);
    if (h->dicts[1].buckets == NULL)
    {
        h->dicts[1].buckets_count = 0;
        return;
    }
    h->dicts[1].buckets_pre_length = calloc(sizeof(size_t), h->dicts[1].buckets_count);
    if (h->dicts[1].buckets_pre_length == NULL)
    {
        h->dicts[1].buckets_count = 0;
        free(h->dicts[1].buckets);
        h->dicts[1].buckets = NULL;
        return;
    }
    h->dicts[1].count = 0;
    h->rehashing = 1;
    h->rehashing_idx = 0;
}

static void rehash_end(hash_t* h)
{
    free(h->dicts[0].buckets);
    free(h->dicts[0].buckets_pre_length);

    h->dicts[0].buckets = h->dicts[1].buckets;
    h->dicts[0].buckets_pre_length = h->dicts[1].buckets_pre_length;
    h->dicts[0].buckets_count = h->dicts[1].buckets_count;
    h->dicts[0].count = h->dicts[1].count;

    h->dicts[1].buckets = NULL;
    h->dicts[1].buckets_pre_length = NULL;
    h->dicts[1].buckets_count = 0;
    h->dicts[1].count = 0;

    h->rehashing = 0;
    h->rehashing_idx = 0;
}

static void rehash_step(hash_t* h)
{
    size_t idx;
    hash_bucket_t* node;
    while (h->dicts[0].count)
    {
        node = h->dicts[0].buckets[h->rehashing_idx];
        if (node == NULL)
        {
            ++h->rehashing_idx;
            continue;
        }
        h->dicts[0].buckets[h->rehashing_idx] = node->next;
        idx = h->functor.hash(node->data.key, node->data.key_len) % h->dicts[1].buckets_count;
        node->next = h->dicts[1].buckets[idx];
        h->dicts[1].buckets[idx] = node;
        ++h->dicts[1].count;
        ++h->dicts[1].buckets_pre_length[idx];

        --h->dicts[0].buckets_pre_length[h->rehashing_idx];
        if (--h->dicts[0].count <= 0) rehash_end(h);
        break;
    }
}

static int hash_set_exists(hash_t* h, int dst, const void* key, const size_t key_len, const void* val, const size_t val_len, size_t* idx)
{
    hash_bucket_t* node;

    *idx = h->functor.hash(key, key_len) % h->dicts[dst].buckets_count;
    node = h->dicts[dst].buckets[*idx];
    if (h->rehashing) rehash_step(h);
    while (node)
    {
        if (h->functor.compare(key, key_len, node->data.key, node->data.key_len))
        {
            if (h->functor.free_val) h->functor.free_val(node->data.val, node->data.val_len);
            node->data.val = h->functor.dup_val(val, val_len);
            return 1;
        }
        node = node->next;
    }
    return 0;
}

int hash_set(hash_t* h, const void* key, const size_t key_len, const void* val, const size_t val_len)
{
    size_t idx;
    hash_bucket_t* node;
    int dst;

    dst = 0;

    if (hash_set_exists(h, 0, key, key_len, val, val_len, &idx)) return 1;
    if (h->rehashing)
    {
        if (hash_set_exists(h, 1, key, key_len, val, val_len, &idx)) return 1;
        if (!h->rehashing) idx = h->functor.hash(key, key_len) % h->dicts[0].buckets_count;
        else dst = 1;
    }

    node = malloc(sizeof(hash_bucket_t));
    if (node == NULL) return 0;
    node->data.key = h->functor.dup_key(key, key_len);
    node->data.key_len = key_len;
    node->data.val = h->functor.dup_val(val, val_len);
    node->data.val_len = val_len;
    node->next = h->dicts[dst].buckets[idx];
    h->dicts[dst].buckets[idx] = node;
    ++h->dicts[dst].count;
    ++h->dicts[dst].buckets_pre_length[idx];

    if (!h->rehashing && h->dicts[dst].buckets_pre_length[idx] >= h->max_length) rehash_start(h);
    return 1;
}

int hash_get(hash_t* h, const void* key, const size_t key_len, void** val, size_t* val_len)
{
    size_t idx;
    hash_bucket_t* node;
    size_t i;

    if (h->rehashing) rehash_step(h);
    for (i = 0; i < (h->rehashing ? 2UL : 1UL); ++i)
    {
        idx = h->functor.hash(key, key_len) % h->dicts[i].buckets_count;
        node = h->dicts[i].buckets[idx];
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
    }
    return 0;
}

static int _hash_del(hash_t* h, int dst, const void* key, const size_t key_len)
{
    size_t idx;
    hash_bucket_t *prev, *node, *next;
    int find;

    idx = h->functor.hash(key, key_len) % h->dicts[dst].buckets_count;
    node = h->dicts[dst].buckets[idx];
    find = 0;
    prev = NULL;
    while (node)
    {
        if (h->functor.compare(key, key_len, node->data.key, node->data.key_len))
        {
            find = 1;
            break;
        }
        prev = node;
        node = node->next;
    }
    if (find)
    {
        next = node->next;
        if (prev) prev->next = next;
        else h->dicts[dst].buckets[idx] = next;
        h->functor.free(node->data.key, node->data.key_len, node->data.val, node->data.val_len);
        free(node);
        --h->dicts[dst].buckets_pre_length[idx];
        --h->dicts[dst].count;
        return 1;
    }
    return 0;
}

int hash_del(hash_t* h, const void* key, const size_t key_len)
{
    if (h->rehashing) rehash_step(h);

    if (_hash_del(h, 0, key, key_len)) return 1;
    if (h->rehashing) return _hash_del(h, 1, key, key_len);
    return 0;
}

void hash_clear(hash_t* h)
{
    hash_bucket_t *node, *next;
    size_t i, j;
    for (j = 0; j < (h->rehashing ? 2UL : 1UL); ++j)
    {
        for (i = 0; i < h->dicts[j].buckets_count; ++i)
        {
            node = h->dicts[j].buckets[i];
            while (node)
            {
                next = node->next;
                h->functor.free(node->data.key, node->data.key_len, node->data.val, node->data.val_len);
                free(node);
                node = next;
            }
            h->dicts[j].buckets[i] = NULL;
            h->dicts[j].buckets_pre_length[i] = 0;
        }
        h->dicts[j].count = 0;
    }
    if (h->rehashing)
    {
        free(h->dicts[1].buckets);
        free(h->dicts[1].buckets_pre_length);
        h->dicts[1].buckets = NULL;
        h->dicts[1].buckets_pre_length = NULL;
        h->dicts[1].buckets_count = 0;
    }
    h->rehashing = 0;
    h->rehashing_idx = 0;
}

static int _hash_enum(hash_t* h, int dict_start, size_t buckets_start, hash_iterator_t* iter)
{
    int i;
    for (i = dict_start; i < (h->rehashing ? 2 : 1); ++i)
    {
        hash_bucket_t* node;
        size_t j;
        for (j = buckets_start; j < h->dicts[i].buckets_count; ++j)
        {
            node = h->dicts[i].buckets[j];
            if (node == NULL) continue;
            iter->data = node->data;
            iter->dict = i;
            iter->bidx = j;
            iter->node = node;
            return 1;
        }
    }
    return 0;
}

hash_iterator_t hash_begin(hash_t* h)
{
    hash_iterator_t iter;
    if (hash_count(h) == 0)
    {
        iter.end = 1;
        return iter;
    }
    iter.end = 0;
    if (_hash_enum(h, 0, 0, &iter))
    {
        iter.idx = 0;
        return iter;
    }
    iter.end = 1;
    return iter;
}

hash_iterator_t hash_next(hash_t* h, hash_iterator_t iter)
{
    hash_iterator_t next;
    if (iter.idx >= hash_count(h) - 1)
    {
        next.end = 1;
        return next;
    }
    next.end = 0;
    if (iter.node->next)
    {
        next.data = iter.node->next->data;
        next.dict = iter.dict;
        next.bidx = iter.bidx;
        next.idx  = iter.idx + 1;
        next.node = iter.node->next;
        return next;
    }
    if (_hash_enum(h, iter.dict, iter.bidx + 1, &next))
    {
        next.idx = iter.idx + 1;
        return next;
    }
    next.end = 1;
    return next;
}

void* hash_dummy_dup(const void* data, const size_t len)
{
    return (void*)data;
}

void hash_dummy_free(void* key, size_t key_len, void* val, size_t val_len)
{
}
