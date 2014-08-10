#include <stdio.h>
#include <stdlib.h>

#include "rand.h"
#include "qvn.h"

typedef long type_t;

int comp(const void* a, const void* b)
{
    type_t n1 = *(type_t*)a;
    type_t n2 = *(type_t*)b;
    if (n1 < 0)
    {
        if (n2 < n1) return 1;
        else if (n2 == n1) return 0;
        else return -1;
    }
    else if (n1 > 0)
    {
        if (n2 < n1) return 1;
        else if (n2 == n1) return 0;
        else return -1;
    }
    else return -n2;
}

int main(int argc, char* argv[])
{
    //init_with_server(6687);
    //network_loop();
    
    {
#define CNT 1000000
        int i;
        static type_t tmp[CNT] = {0};
        int same;
        FILE* fp;
        struct timeval tv;
        
        do
        {
            same = 0;
            gettimeofday(&tv, NULL);
            srand64(tv.tv_sec * tv.tv_usec);
            for (i = 0; i < CNT; ++i)
            {
                tmp[i] = rand64();
            }
            qsort(tmp, CNT, sizeof(tmp[0]), comp);
            for (i = 1; i < CNT; ++i)
            {
                if (tmp[i] == tmp[i - 1]) ++same;
            }
            printf("same: %d %f\%\n", same, (double)same / CNT * 100);
            
            if (same > 1000) return 0;
            return 0;
            sleep(1);
        } while (1);
        
        same = 0;
        fp = fopen("/dev/urandom", "r");
        for (i = 0; i < CNT; ++i)
        {
            fread(&tmp[i], sizeof(tmp[i]), 1, fp);
        }
        fclose(fp);
        qsort(tmp, CNT, sizeof(tmp[0]), comp);
        for (i = 1; i < CNT; ++i)
        {
            if (tmp[i] == tmp[i - 1]) ++same;
        }
        printf("same: %d %f\%\n", same, (double)same / CNT * 100);
    }
    return 0;
}
