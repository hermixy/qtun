#if !defined(_WIN_H_) && defined(WIN32)
#define _WIN_H_

#include <io.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>

#define MAX_DEVICE_COUNT 20

typedef struct
{
    DWORD index;
    char dev_path[MAX_PATH];
    char dev_name[255];
} enum_device_t;

extern int gettimeofday(struct timeval *tp, void *tzp);
extern size_t enum_devices(enum_device_t devices[MAX_DEVICE_COUNT]);

#define write(a, b, c) send(a, b, c, 0)
#define read(a, b, c) recv(a, b, c, 0)
#define close(x) closesocket(x)

#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */

#endif
