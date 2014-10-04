#include <execinfo.h>
#include <signal.h>
#include <stdio.h>

#include "../common.h"
#include "../hash.h"

static void crash_sig(int signum)
{
    void* array[10];
    size_t size;
    char** strings;
    size_t i;

    signal(signum, SIG_DFL);

    size = backtrace(array, sizeof(array) / sizeof(void*));
    strings = (char**)backtrace_symbols(array, size);

    for (i = 0; i < size; ++i)
    {
        fprintf(stderr, "%s\n", strings[i]);
    }

    free(strings);
    exit(1);
}

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
    long* p = malloc(sizeof(*p));
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
    signal(SIGSEGV, crash_sig);
    signal(SIGABRT, crash_sig);

    hash_t h;
    hash_functor_t func = {
        hash,
        compare,
        dup,
        dup,
        f,
        fv
    };
    /*hash_functor_t func = {
        fd_hash,
        fd_compare,
        fd_dup,
        hash_dummy_dup,
        hash_dummy_free,
        NULL
    };*/
    long i;
    int fd;
    void* v;
    size_t v_len;
    hash_iterator_t iter;

    hash_init(&h, func, 11);
    for (i = 0; i < 1000000; ++i)
    {
        hash_set(&h, &i, sizeof(i), &i, sizeof(i));
        hash_get(&h, &i, sizeof(i), &v, &v_len);
        i = *(long*)v;
        printf("%ld\n", i);
    }
    hash_del(&h, &i, sizeof(i));
    i = 0;
    hash_del(&h, &i, sizeof(i));
    /*fd = 5;
    hash_set(&h, (void*)(long)fd, sizeof(fd), NULL, 0);
    fd = 6;
    hash_set(&h, (void*)(long)fd, sizeof(fd), NULL, 0);
    printf("count: %lu\n", hash_count(&h));
    iter = hash_begin(&h);
    while (!hash_is_end(iter))
    {
        printf("idx: %lu\n", iter.idx);
        fd = hash2fd(iter.data.key);
        printf("%d\n", fd);
        iter = hash_next(&h, iter);
    }*/
    hash_free(&h);
    return 0;
}
