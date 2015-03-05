#include <stdio.h>
#include <stdlib.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/watchdog.h>
#include <sys/stat.h>
#include <signal.h>
#include "bsp.h"

static int s32Fd = -1;

int BSP_EnableWatchdog(void)
{
    if (s32Fd >= 0)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "watchdog had been already opened!\n");
        return BSP_ERR_DEV_OPEN;
    }

    s32Fd = open("/dev/watchdog", O_WRONLY);
    if (-1 == s32Fd)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "open watchdog fail!\n");
        return BSP_ERR_DEV_OPEN;
    }

    return BSP_OK;
}

int BSP_DisableWatchdog(void)
{
    int s32Ret;

    if ( s32Fd >= 0 )
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "watchdog had been already closed!\n");
        return BSP_ERR_DEV_CLOSE;
    }

    s32Ret = close(s32Fd);
    if (-1 == s32Ret)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "close watchdog fail!\n");
        return BSP_ERR_DEV_CLOSE;
    }

    s32Fd = -1;

    return BSP_OK;
}

int BSP_GetWatchdogTimeout(unsigned int * const pu32Time)
{
    int s32Ret;

    if (NULL == pu32Time)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "pu32Time is NULL!\n");
        return BSP_ERR_ARG;
    }

    if ( s32Fd < 0 )
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "watchdog is not opened!\n");
        return BSP_ERR_DEV_CLOSE;
    }

    s32Ret = ioctl(s32Fd, WDIOC_GETTIMEOUT, pu32Time);
    if (s32Ret < 0)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "get watchdog timout fail!\n");
        return BSP_ERR_DEV_CTRL;
    }

    return BSP_OK;
}

int BSP_SetWatchdogTimeout(const unsigned int u32Timeout)
{
    int s32Ret;

    if ( s32Fd < 0 )
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "watchdog is not opened!\n");
        return BSP_ERR_DEV_CLOSE;
    }

    s32Ret = ioctl(s32Fd, WDIOC_SETTIMEOUT, &u32Timeout);
    if (s32Ret < 0)
    {
        printf ("set watchdog timeout fail!\n");
        return BSP_ERR_DEV_CTRL;
    }

    return BSP_OK;
}

int BSP_FeedWatchdog(void)
{
    ssize_t s32Len;

    if ( s32Fd < 0 )
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "watchdog is not opened!\n");
        return BSP_ERR_DEV_CLOSE;
    }

    s32Len = write(s32Fd, "\0", 1);
    if (s32Len != 1)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "feed watchdog fail!\n");
        return BSP_ERR_WRITE;
    }

    return BSP_OK;
}

