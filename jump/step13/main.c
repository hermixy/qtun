#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if defined(unix) && defined(HAVE_LINUX_TCP_H)
#include <linux/tcp.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <openssl/aes.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "library.h"
#include "network.h"
#include "main.h"

#ifdef HAVE_EXECINFO_H
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
#ifndef HAVE_EXECINFO_H
    signal(SIGSEGV, crash_sig);
    signal(SIGABRT, crash_sig);
    signal(SIGPIPE, SIG_IGN);
#endif

#ifdef WIN32
    HANDLE localfd;
    WSADATA wsa;
#else
    char cmd[1024];
    int localfd;
#endif
    int remotefd;
    library_conf_t conf;
    int opt;
    char* host = NULL;
    unsigned int port = 6687;
    struct in_addr a;

    struct option long_options[] = {
        { "aes",          1, NULL, 'a' },
        { "des",          1, NULL, 'd' },
        { "gzip",         0, NULL, 'g' },
        { "mask",         1, NULL, 'm' },
        { "localip",      1, NULL, 'l' },
        { "server",       1, NULL, 's' },
        { "port",         1, NULL, 'p' },
        { "log-level",    1, NULL, 'v' },
        { "internal-mtu", 1, NULL, 't' },
        { "udp",          0, NULL, 'u' },
#ifdef WIN32
        { "device",       1, NULL, 'e' },
#endif
        { NULL,           0, NULL,  0  }
    };
    char short_options[512] = {0};
    longopt2shortopt(long_options, sizeof(long_options) / sizeof(struct option), short_options);
#ifdef HAVE_SYSLOG_H
    openlog(argv[0], LOG_PERROR | LOG_CONS | LOG_PID, LOG_LOCAL0);
#endif

#ifdef WIN32
    remotefd = -1;
    localfd = INVALID_HANDLE_VALUE;
#else
    localfd = remotefd = -1;
#endif

    memset(&this, 0, sizeof(this));

    conf.localip      = 0;
    conf.netmask      = 24;
    conf.log_level    = LOG_WARNING;
    conf.internal_mtu = 1492; // keep not to clip
#ifdef WIN32
    memset(conf.device, 0, sizeof(conf.device));
#endif
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
#ifdef WIN32
        case 'e':
            strcpy(conf.device, optarg);
            break;
#endif
        default:
#ifdef HAVE_SYSLOG_H
            syslog(LOG_ERR, "param error");
#endif
            return 1;
        }
    }

    if (conf.localip == 0)
    {
#ifdef HAVE_SYSLOG_H
        syslog(LOG_ERR, "localip is zero\n");
#endif
        return 1;
    }
    if (port == 0)
    {
#ifdef HAVE_SYSLOG_H
        syslog(LOG_ERR, "port is zero\n");
#endif
        return 1;
    }
#ifdef WIN32
    if (strlen(conf.device) == 0)
    {
        fprintf(stderr, "Missing param [-e] or [--device]\n");
        return 1;
    }
#endif

#ifdef WIN32
    localfd = tun_open(conf.device);
    if (localfd == INVALID_HANDLE_VALUE) return 1;
    fprintf(stdout, "%s opened\n", conf.device);
#else
    memset(this.dev_name, 0, IFNAMSIZ);
    localfd = tun_open(this.dev_name);
    if (localfd == -1) return 1;
    syslog(LOG_INFO, "%s opened\n", this.dev_name);
#endif
    a.s_addr = conf.localip;

#ifdef WIN32
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    if (host == NULL)
    {
        if (conf.netmask == 0 || conf.netmask > 31)
        {
#ifdef WIN32
            WSACleanup();
#endif

#ifdef HAVE_SYSLOG_H
            syslog(LOG_ERR, "netmask must > 0 and <= 31\n");
#endif
            return 1;
        }
        library_init(conf);
        remotefd = bind_and_listen(port);
        if (remotefd == -1)
        {
#ifdef WIN32
            WSACleanup();
#endif
            return 1;
        }
#ifdef unix
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
#ifdef unix
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
#ifdef unix
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
#ifdef WIN32
    WSACleanup();
#endif

#ifdef HAVE_SYSLOG_H
    closelog();
#endif
    return 0;
}
