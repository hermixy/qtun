#include <stdio.h>

#include "hash.h"

static size_t hash(const void* key, const size_t key_len)
{
    return *(long*)key;
}

static int compare(const void* k1, const size_t k1_len, const void* k2, const size_t k2_len)
{
    return *(long*)k1 == *(long*)k2;
}

static void* dup(const void* v, const size_t v_len)
{
    long* p = malloc(sizeof(p));
    *p = *(const long*)v;
    return p;
}

static void f(void* k, size_t k_len, void* v, size_t v_len)
{
    free(k);
    free(v);
}

static void fv(void* v, size_t v_len)
{
    free(v);
}

int main()
{
    hash_t h;
    hash_functor_t func = {
        hash,
        compare,
        dup,
        dup,
        f,
        fv
    };
    long i;
    void* v;
    size_t v_len;

    hash_init(&h, func, 11);
    for (i = 0; i < 100; ++i)
    {
        hash_set(&h, &i, sizeof(i), &i, sizeof(i));
        hash_get(&h, &i, sizeof(i), &v, &v_len);
        i = *(long*)v;
        printf("%ld\n", i);
    }
    hash_del(&h, &i, sizeof(i));
    i = 0;
    hash_del(&h, &i, sizeof(i));
    hash_free(&h);
    return 0;
}
