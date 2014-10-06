#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <linux/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "main.h"

static int tun_open(char name[IFNAMSIZ])
{
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1)
    {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) == -1)
    {
        perror("ioctl");
        close(fd);
        return -1;
    }

    strncpy(name, ifr.ifr_name, IFNAMSIZ);
    return fd;
}


int main(int argc, char* argv[])
{
    char name[IFNAMSIZ];
    unsigned char cmd[1024];
    int localfd, remotefd;

    if (argc < 2)
    {
        fprintf(stderr, "usage: ./step7 <0|1>\n");
        return 1;
    }

    memset(name, 0, IFNAMSIZ);
    localfd = tun_open(name);
    if (localfd == -1) return 1;
    fprintf(stdout, "%s opened\n", name);

    switch (atoi(argv[1]))
    {
    case 1:
        if (argc < 3)
        {
            fprintf(stderr, "usage: ./step7 1 ip\n");
            return 1;
        }
        sprintf(cmd, "ifconfig %s 10.0.1.2 mtu 1000 up", name);
        system(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        system(cmd);
        sprintf(cmd, "route add 8.8.8.8 dev %s", name);
        system(cmd);
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
        sprintf(cmd, "ifconfig %s 10.0.1.3 mtu 1000 up", name);
        system(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        system(cmd);
        sprintf(cmd, "route add 8.8.8.8 dev %s", name);
        system(cmd);
        remotefd = connect_server(argv[2], 6687);
        if (remotefd == -1) return 1;
        client_loop(remotefd, localfd);
        break;
    default:
        sprintf(cmd, "ifconfig %s 10.0.1.1 mtu 1000 up", name);
        system(cmd);
        sprintf(cmd, "route add -net 10.0.1.0/24 dev %s", name);
        system(cmd);
        remotefd = bind_and_listen(6687);
        if (remotefd == -1) return 1;
        server_loop(remotefd, localfd);
        break;
    }
    return 0;
}
