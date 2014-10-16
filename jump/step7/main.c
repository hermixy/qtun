#include <openssl/aes.h>

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "library.h"
#include "network.h"
#include "main.h"

#define SYSTEM(cmd) \
do {\
    if (system(cmd) == -1) \
    { \
        perror("system"); \
        exit(1); \
    } \
} while (0)

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

int main(int argc, char* argv[])
{
    signal(SIGSEGV, crash_sig);
    signal(SIGABRT, crash_sig);

    char name[IFNAMSIZ];
    char cmd[1024];
    int localfd, remotefd;
    library_conf_t conf;
    int opt;
    unsigned char n = 0;
    char* ip = NULL;
    FILE* fp;

    this.msg_ident = 0;

    memset(&conf, 0, sizeof(conf));
    while ((opt = getopt(argc, argv, "a:d:gn:s:")) != -1)
    {
        switch (opt)
        {
        case 'a':
            {
                ssize_t len;
                conf.use_aes = 1;
                fp = fopen(optarg, "r");
                if (fp == NULL)
                {
                    fprintf(stderr, "can not open aes key file\n");
                    return 1;
                }
                len = fread(conf.aes_iv, sizeof(char), sizeof(conf.aes_iv), fp);
                if (len <= 0 || len != sizeof(conf.aes_iv))
                {
                    fprintf(stderr, "error aes iv\n");
                    return 1;
                }
                len = fread(conf.aes_key, sizeof(char), sizeof(conf.aes_key), fp);
                if (len <= 0 || (len != 16 && len != 24 && len != 32))
                {
                    fprintf(stderr, "error aes key file\n");
                    return 1;
                }
                conf.aes_key_len = len << 3;
                fclose(fp);
            }
            break;
        case 'd':
            break;
        case 'g':
            conf.use_gzip = 1;
            break;
        case 'n':
            n = atoi(optarg);
            break;
        case 's':
            ip = optarg;
            break;
        default:
            fprintf(stderr, "param error\n");
            return 1;
        }
    }

    if (n == 0)
    {
        fprintf(stderr, "-n0 is not support\n");
        return 1;
    }

    if (n > 1 && ip == NULL)
    {
        fprintf(stderr, "have no ip for connect\n");
        return 1;
    }

    memset(name, 0, IFNAMSIZ);
    localfd = tun_open(name);
    if (localfd == -1) return 1;
    fprintf(stdout, "%s opened\n", name);
    library_init(conf);

    if (n == 1)
    {
        sprintf(cmd, "ifconfig %s 10.0.1.%u mtu 1492 up", name, n);
        SYSTEM(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        SYSTEM(cmd);
        remotefd = bind_and_listen(6687);
        if (remotefd == -1) return 1;
        server_loop(remotefd, localfd);
    }
    else
    {
        sprintf(cmd, "ifconfig %s 10.0.1.%u mtu 1492 up", name, n);
        SYSTEM(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add 8.8.8.8 dev %s", name);
        SYSTEM(cmd);
        remotefd = connect_server(ip, 6687);
        if (remotefd == -1) return 1;
        client_loop(remotefd, localfd);
    }
    return 0;
}
