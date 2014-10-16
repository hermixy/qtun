#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    library_conf_t conf = {
        1,
        0,
        0
    };

    if (argc < 2)
    {
        fprintf(stderr, "usage: ./step7 <0|1>\n");
        return 1;
    }

    this.msg_ident = 0;

    memset(name, 0, IFNAMSIZ);
    localfd = tun_open(name);
    if (localfd == -1) return 1;
    fprintf(stdout, "%s opened\n", name);

    library_init(conf);
    switch (atoi(argv[1]))
    {
    case 1:
        if (argc < 3)
        {
            fprintf(stderr, "usage: ./step7 1 ip\n");
            return 1;
        }
        sprintf(cmd, "ifconfig %s 10.0.1.2 mtu 1492 up", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add 8.8.8.8 dev %s", name);
        SYSTEM(cmd);
        remotefd = connect_server(argv[2], 6687);
        if (remotefd == -1) return 1;
        client_loop(remotefd, localfd);
        break;
    case 2:
        if (argc < 3)
        {
            fprintf(stderr, "usage: ./step7 2 ip\n");
            return 1;
        }
        sprintf(cmd, "ifconfig %s 10.0.1.3 mtu 1492 up", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add 8.8.8.8 dev %s", name);
        SYSTEM(cmd);
        remotefd = connect_server(argv[2], 6687);
        if (remotefd == -1) return 1;
        client_loop(remotefd, localfd);
        break;
    default:
        sprintf(cmd, "ifconfig %s 10.0.1.1 mtu 1492 up", name);
        SYSTEM(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        SYSTEM(cmd);
        remotefd = bind_and_listen(6687);
        if (remotefd == -1) return 1;
        server_loop(remotefd, localfd);
        break;
    }
    return 0;
}
