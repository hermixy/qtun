#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <openssl/aes.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include "getopt.h"
#else
#include <unistd.h>
#include <execinfo.h>
#include <getopt.h>
#endif

#include "common.h"
#include "library.h"
#include "network.h"
#include "main.h"

#ifndef WIN32
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
        syslog(LOG_ERR, "%s\n", strings[i]);
    }

    free(strings);
    exit(1);
}
#endif

static void longopt2shortopt(struct option* long_options, size_t count, char* short_options)
{
    size_t i;
    char* ptr = short_options;
    for (i = 0; i < count; ++i)
    {
        if (long_options[i].val)
        {
            *ptr++ = long_options[i].val;
            if (long_options[i].has_arg) *ptr++ = ':';
        }
    }
    *ptr = 0;
}

int main(int argc, char* argv[])
{
#ifndef WIN32
    signal(SIGSEGV, crash_sig);
    signal(SIGABRT, crash_sig);
    signal(SIGPIPE, SIG_IGN);
#endif

#ifndef WIN32
    char cmd[1024];
#endif
    int localfd, remotefd;
    library_conf_t conf;
    int opt;
    char* host = NULL;
    unsigned int port = 6687;
    struct in_addr a;

    struct option long_options[] = {
        {"aes",          1, NULL, 'a'},
        {"des",          1, NULL, 'd'},
        {"gzip",         0, NULL, 'g'},
        {"mask",         1, NULL, 'm'},
        {"localip",      1, NULL, 'l'},
        {"server",       1, NULL, 's'},
        {"port",         1, NULL, 'p'},
        {"log-level",    1, NULL, 'v'},
        {"internal-mtu", 1, NULL, 't'},
        {"udp",          0, NULL, 'u'},
        {NULL,           0, NULL,   0}
    };
    char short_options[512] = {0};
    longopt2shortopt(long_options, sizeof(long_options) / sizeof(struct option), short_options);
#ifndef WIN32
    openlog(argv[0], LOG_PERROR | LOG_CONS | LOG_PID, LOG_LOCAL0);
#endif

    localfd = remotefd = -1;

    conf.localip      = 0;
    conf.netmask      = 24;
    conf.log_level    = LOG_WARNING;
    conf.internal_mtu = 1492; // keep not to clip
    conf.use_gzip     = 0;
    conf.use_udp      = 0;
    conf.use_aes      = 0;
    conf.aes_key_file = NULL;
    conf.use_des      = 0;
    conf.des_key_file = NULL;

    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'a':
            conf.use_aes = 1;
            conf.aes_key_file = optarg;
            break;
        case 'd':
            conf.use_des = 1;
            conf.des_key_file = optarg;
            break;
        case 'g':
            conf.use_gzip = 1;
            break;
        case 'l':
            conf.localip = inet_addr(optarg);
            break;
        case 's':
            host = optarg;
            break;
        case 'm':
            conf.netmask = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'v':
            conf.log_level = atoi(optarg);
            break;
        case 't':
            conf.internal_mtu = atoi(optarg);
            break;
        case 'u':
            conf.use_udp = 1;
            break;
        default:
#ifndef WIN32
            syslog(LOG_ERR, "param error");
#endif
            return 1;
        }
    }

    if (conf.localip == 0)
    {
#ifndef WIN32
        syslog(LOG_ERR, "localip is zero\n");
#endif
        return 1;
    }
    if (port == 0)
    {
#ifndef WIN32
        syslog(LOG_ERR, "port is zero\n");
#endif
        return 1;
    }

#ifndef WIN32
    memset(this.dev_name, 0, IFNAMSIZ);
    localfd = tun_open(this.dev_name);
    if (localfd == -1) return 1;
    syslog(LOG_INFO, "%s opened\n", this.dev_name);
#endif
    a.s_addr = conf.localip;

    if (host == NULL)
    {
        if (conf.netmask == 0 || conf.netmask > 31)
        {
#ifndef WIN32
            syslog(LOG_ERR, "netmask must > 0 and <= 31\n");
#endif
            return 1;
        }
        library_init(conf);
        remotefd = bind_and_listen(port);
        if (remotefd == -1) return 1;
#ifndef WIN32
        sprintf(cmd, "ifconfig %s %s/%u up", this.dev_name, inet_ntoa(a), conf.netmask);
        SYSTEM_EXIT(cmd);
        a.s_addr = conf.localip & LEN2MASK(conf.netmask);
        sprintf(cmd, "route add -net %s/%u dev %s", inet_ntoa(a), conf.netmask, this.dev_name);
        SYSTEM_EXIT(cmd);
#endif
        server_loop(remotefd, localfd);
    }
    else
    {
#ifndef WIN32
        unsigned char mask;
#endif
        int inited = 0;
        library_init(conf);

        while (1)
        {
            remotefd = connect_server(host, port);
            if (remotefd == -1)
            {
#ifdef WIN32
                Sleep(5000);
#else
                sleep(5);
#endif
                continue;
            }
            if (!inited)
            {
#ifndef WIN32
                sprintf(cmd, "ifconfig %s %s up", this.dev_name, inet_ntoa(a));
                SYSTEM_EXIT(cmd);
                mask = netmask();
                a.s_addr = conf.localip & LEN2MASK(mask);
                sprintf(cmd, "route add -net %s/%u dev %s", inet_ntoa(a), mask, this.dev_name);
                SYSTEM_EXIT(cmd);
#endif
                inited = 1;
            }
            client_loop(remotefd, localfd);
            close(remotefd);
            SYSLOG(LOG_WARNING, "retry");
        }
    }
#ifndef WIN32
    closelog();
#endif
    return 0;
}
