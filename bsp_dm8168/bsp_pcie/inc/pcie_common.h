#ifndef __PCIE_COMMON_H_U_
#define __PCIE_COMMON_H_U_

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <linux/pcie_common.h>
#include "pcie_std.h"

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define DEFAULT_INIT_TIMEOUT (150)
#define DEFAULT_SEND_TIMEOUT (5)
#define DEFAULT_RECV_TIMEOUT (5)
    

struct pciedev_init_config {
    int en_clearBuf;
    struct timeval tv;
};

/*****get env***/
static int gDebug_enable = 0;
inline static int set_printLevel(void)
{
    int ret = 0;
    char *env;
    env=getenv("PCIE_DEBUG");
    if(!env){
        return -1;
    }
    gDebug_enable = atoi(env);
    printf("get debug enable is %d.\n", gDebug_enable);    
    return ret;
}

/*******get local id*/
inline static int get_selfId(void)
{
    int id = 0;
    char *env;
    env=getenv("DEV_ID");
    if(!env){
        printf("%s failed.\n",__func__);
        return -1;
    }
    id = atoi(env);
    return id;
}

extern void *memcpy_neon(void *dest, void *src, int size);


#define debug_print(fmt, ...)                                           \
	do { if (DEBUG_TEST || gDebug_enable) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
				__LINE__, __func__, ##__VA_ARGS__); } while (0)


/* in debug mode every thing will be printed
 * error will always be printed
 */
#define err_print(fmt, ...) \
	do { fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
				__LINE__, __func__, ##__VA_ARGS__); } while (0)
    
inline static Int32 pcieDev_lockInit(struct pciedev_info *pciedev)
{
//    sem_init(&pciedev->dev_lock, 0, 1);
    return 0;
}
inline static Int32 pcieDev_lockDeInit(struct pciedev_info *pciedev)
{
    return 0;
}
inline static Int32 pcieDev_lock(struct pciedev_info *pciedev)
{
//    sem_wait(&pciedev->dev_lock);
    return 0;
}

inline static Int32 pcieDev_unlock(struct pciedev_info *pciedev)
{
//    sem_post(&pciedev->dev_lock);
    return 0;
}

#ifdef	__cplusplus
}
#endif

#endif
