#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int tun_create(char* dev)
{
    struct ifreq ifr;
    int fd;
    static char* device = "/dev/net/tun";
    if ((fd = open(device, O_RDWR)) < 0)
    {
        perror("open");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if (ioctl(fd, _IOW('T', 202, int), (void *) &ifr) < 0)
    {
        perror("ioctl");
        close(fd);
        return -1;
    }
    return fd;
}

int main()
{
    int tun;
    static char name[IFNAMSIZ] = "tun001";

    tun = tun_create(name);
    if (tun < 0) return 1;

    while (1)
    {
        unsigned char buf[4096];
        unsigned int ip;
        int ret = read(tun, buf, sizeof(buf));

        if (ret < 0) break;

        memcpy(&ip, &buf[12], sizeof(ip));
        memcpy(&buf[12], &buf[16], sizeof(ip));
        memcpy(&buf[16], &ip, sizeof(ip));
        buf[20] = 0;
        *((unsigned short*)&buf[22]) += 8;
        printf("read %d bytes\n", ret);
        ret = write(tun, buf, ret);
        printf("write %d bytes\n", ret);
    }
    return 0;
}
