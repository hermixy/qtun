#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    ifr.ifr_flags = IFF_TUN;
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

int main()
{
    char name[IFNAMSIZ];
    char cmd[1024];
    int fd;
    
    memset(name, 0, IFNAMSIZ);
    fd = tun_open(name);
    if (fd == -1) return 1;
    fprintf(stdout, "%s opened\n", name);
    
    sprintf(cmd, "ifconfig %s 10.0.1.1 up", name);
    system(cmd);
    sprintf(cmd, "route add 10.0.1.2 dev %s", name);
    system(cmd);
    
    while (1) read(fd, cmd, sizeof(cmd));
    return 0;
}
