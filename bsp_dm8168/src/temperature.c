#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "bsp.h"

#define TEMP_SENSOR_FILE    "/sys/bus/i2c/drivers/tmp421/2-004c/temp1_input"
#define TEMP_STR_LEN        5


int BSP_GetCPUTemperature(float * const pf32Temp)
{
    int s32Fd;
    ssize_t s32Len;
    char szTempStr[TEMP_STR_LEN+1];

    s32Fd = open(TEMP_SENSOR_FILE, O_RDONLY);
    if (s32Fd < 0)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "open %s fail\n", TEMP_SENSOR_FILE);
        return BSP_ERR_DEV_OPEN;
    }

    memset(szTempStr, 0, sizeof(szTempStr));
    s32Len= read(s32Fd, szTempStr, TEMP_STR_LEN);
    if (s32Len != TEMP_STR_LEN)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG, "read temperature sensor, len=%d!\n", s32Len);
        close(s32Fd);
        return BSP_ERR_READ;
    }

    *pf32Temp = atof(szTempStr)/1000;

    close(s32Fd);

    return BSP_OK;
}


