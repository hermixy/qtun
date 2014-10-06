#include <stdio.h>
#include "../link.h"

static int compare(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    return (int)(long)d1 == (int)(long)d2;
}

int main()
{
    link_t l;
    link_functor_t func = {
        compare,
        link_dummy_dup,
        link_dummy_free
    };
    int i;
    link_iterator_t iter;

    link_init(&l, func);
    for (i = 0; i < 20; ++i)
    {
        link_insert_head(&l, (void*)(long)i, sizeof(i));
        link_insert_tail(&l, (void*)(long)i, sizeof(i));
    }
    link_pop_head(&l);
    link_pop_tail(&l);
    printf("num 19 is %s\n", (link_exists(&l, (void*)(long)19, sizeof(i)) ? "exists" : "not exists"));
    printf("num 0 is %s\n", (link_exists(&l, (void*)(long)0, sizeof(i)) ? "exists" : "not exists"));
    printf("============================================\n");
    iter = link_begin(&l);
    while (!link_is_end(&l, iter))
    {
        printf("%d\n", (int)(long)iter.data);
        iter = link_next(&l, iter);
    }
    printf("============================================\n");
    iter = link_prev(&l, iter);
    while (!link_is_begin(&l, iter))
    {
        printf("%d\n", (int)(long)iter.data);
        iter = link_prev(&l, iter);
    }
    printf("%d\n", (int)(long)iter.data);
    printf("============================================\n");
    iter = link_rev_begin(&l);
    while (!link_is_end(&l, iter))
    {
        printf("%d\n", (int)(long)iter.data);
        iter = link_next(&l, iter);
    }
    printf("============================================\n");
    iter = link_prev(&l, iter);
    while (!link_is_begin(&l, iter))
    {
        printf("%d\n", (int)(long)iter.data);
        iter = link_prev(&l, iter);
    }
    printf("%d\n", (int)(long)iter.data);
    printf("============================================\n");
    link_free(&l);
    return 0;
}
