#ifndef __PCIE_COMMON_H_U_
#define __PCIE_COMMON_H_U_
#include <stdlib.h>
#include <sys/time.h>
#include <drivers/char/pcie_common.h>

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
    //printf("get dev_id is %d.\n", id);    
    return id;
}

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
#endif
