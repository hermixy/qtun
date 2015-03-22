#ifdef WIN32
#include <Windows.h>
#include <IPHlpApi.h>
#include <Mprapi.h>

#include <stdio.h>

#include "win.h"

int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}

size_t enum_devices(enum_device_t devices[MAX_DEVICE_COUNT])
{
    ULONG len = 0;
    IP_ADAPTER_INFO* adapters = NULL;
    IP_INTERFACE_INFO* interfaces = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;
    size_t idx = 0;

    if (GetAdaptersInfo(NULL, &len) == ERROR_NO_DATA) goto failed;

    adapters = malloc(sizeof(IP_ADAPTER_INFO) * len);
    if (adapters == NULL) goto failed;

    if (GetAdaptersInfo(adapters, &len) == ERROR_SUCCESS)
    {
        IP_ADAPTER_INFO* adp = adapters;
        while (adp)
        {
            if (adp->Type == MIB_IF_TYPE_ETHERNET)
            {
                if (strncmp(adp->Description, "QTun", sizeof("QTun") - 1) == 0)
                {
                    devices[idx].index = adp->Index;
                    strcpy(devices[idx].dev_path, "\\\\.\\");
                    strcat(devices[idx].dev_path, adp->AdapterName);
                    strcat(devices[idx].dev_path, ".tun");
                    ++idx;
                }
            }
            adp = adp->Next;
        }
    }

    MprConfigServerConnect(NULL, &handle);

    len = 0;
    if (GetInterfaceInfo(NULL, &len) == ERROR_NO_DATA) goto failed;

    interfaces = malloc(sizeof(IP_INTERFACE_INFO) * len);
    if (interfaces == NULL) goto failed;

    if (GetInterfaceInfo(interfaces, &len) == ERROR_SUCCESS)
    {
        LONG i;
        size_t j;
        for (i = 0; i < interfaces->NumAdapters; ++i)
        {
            for (j = 0; j < idx; ++j)
            {
                if (devices[j].index == interfaces->Adapter[i].Index)
                {
                    WCHAR w_name[128] = { 0 };
                    int name_len = 0;
                    MprConfigGetFriendlyName(handle, interfaces->Adapter[i].Name, w_name, sizeof(w_name));
                    name_len = WideCharToMultiByte(CP_ACP, 0, w_name, -1, NULL, 0, NULL, FALSE);
                    WideCharToMultiByte(CP_ACP, 0, w_name, -1, devices[j].dev_name, name_len, NULL, FALSE);
                    break;
                }
            }
        }
    }

    if (adapters) free(adapters);
    if (interfaces) free(interfaces);
    return idx;
failed:
    if (adapters) free(adapters);
    if (interfaces) free(interfaces);
    return 0;
}
#endif
