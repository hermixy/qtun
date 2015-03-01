#if !defined(_WIN_H_) && defined(WIN32)
#define _WIN_H_

#include <io.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>

extern int gettimeofday(struct timeval *tp, void *tzp);

#define write(a, b, c) send(a, b, c, 0)
#define read(a, b, c) recv(a, b, c, 0)
#define close(x) closesocket(x)

#define bswap_16(x) \
((unsigned short int) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define bswap_32(x) (\
(((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#pragma pack(1)
struct iphdr
{
    unsigned char ihl : 4;
    unsigned char version : 4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

struct udphdr
{
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
};

struct tcphdr
{
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t res1 : 4;
    uint8_t doff : 4;
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t rst : 1;
    uint8_t psh : 1;
    uint8_t ack : 1;
    uint8_t urg : 1;
    uint8_t res2 : 2;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};
#pragma pack(1)

#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */

#endif
