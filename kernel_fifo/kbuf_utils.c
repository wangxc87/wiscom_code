#include "kfifo_buf.h"
#include "kbuf_utils.h"

//#define CONFIG_BUF_LOCK
static int buf_lock_init(struct pcie_bufHndl *pBufHndl)
{
#ifdef CONFIG_BUF_LOCK
    sem_init(&pBufHndl->buf_lock,0,1);    
#endif
    return 0;
}
        
static int buf_lock(struct pcie_bufHndl *pBufHndl)
{
#ifdef CONFIG_BUF_LOCK    
    sem_wait(&pBufHndl->buf_lock);
#endif
    return 0;
}
static int buf_unlock(struct pcie_bufHndl *pBufHndl)
{
#ifdef CONFIG_BUF_LOCK
    sem_post(&pBufHndl->buf_lock);
#endif
    return 0;
}
int pcie_buf_init(struct pcie_bufHndl *pBufHndl, struct kfifo_buf_create_arg *buf_create)
{
    int ret;    
    ret = OSA_bufCreate(&pBufHndl->buf_handle, buf_create);
    if (ret < 0) {
        fprintf(stderr, "%s: osa_bufCreate failed.\n", __FUNCTION__);
        goto err_exit1;
    }

    buf_lock_init(pBufHndl);
        
    return 0;

err_exit1:
    return -1;
}

int pcie_buf_deInit(struct pcie_bufHndl *pBufHndl)
{
    int i;
    OSA_BufInfo *pBufInfo = NULL;
    
    for (i = 0; i < pBufHndl->buf_handle.numBuf; i++) {
        pBufInfo = &pBufHndl->buf_handle.bufInfo[i];
        pBufinfo->virtual = NULL;
        /* kfree(pBufInfo ->virtAddr); */
    }

    OSA_bufDelete(&pBufHndl->buf_handle);
    return 0;
}


char *pcie_buf_getEmtpy(struct pcie_bufHndl *pBufHndl)
{
    int ret = 0;
    int bufId;
    OSA_BufInfo *pBufinfo;
    
    buf_lock(pBufHndl);
    ret = OSA_bufGetEmpty(&pBufHndl->buf_handle, &bufId, OSA_TIMEOUT_FOREVER);
    if (ret < 0) {
        fprintf(stderr, "%s: OSA_bufGetEmpty [chnID-%d] failed.\n", __FUNCTION__, bufId);
        buf_unlock(pBufHndl);
        return NULL;
    }

    pBufinfo = OSA_bufGetBufInfo(&pBufHndl->buf_handle, bufId);
    if (pBufinfo == NULL) {
        fprintf(stderr, "%s: OSA_bufGetBufInfo [bufId: %d] failed.\n", __FUNCTION__, bufId);
        buf_unlock(pBufHndl);
        return NULL;
    }
    pBufHndl->cur_emptyId = bufId;

    buf_unlock(pBufHndl);
    
    return pBufinfo->virtAddr;;
}

int pcie_buf_setEmpty(struct pcie_bufHndl *pBufHndl, char *buf_ptr)
{
    int ret = 0;
    int i;
    for(i = 0;i < pBufHndl->buf_handle.numBuf; i ++){
        if(buf_ptr == pBufHndl->buf_handle.bufInfo[i].virtAddr)
            break;
    }
    if(i == pBufHndl->buf_handle.numBuf){
        return -1;
    }
    buf_lock(pBufHndl);
    ret = OSA_bufPutEmpty(&pBufHndl->buf_handle, i);
    if (ret < 0) {
        fprintf(stderr, "%s: OSA_bufPutEmpty [bufId: %d] failed.\n", __FUNCTION__, i);
        buf_unlock(pBufHndl);
        return -1;
    }            
    buf_unlock(pBufHndl);
    return 0;
}

int pcie_buf_getFull(struct pcie_bufHndl *pBufHndl, char **buf_ptr)
{
    int ret;
    OSA_BufInfo *pBufinfo;
    int bufId = 0;
    
    buf_lock(pBufHndl);
    
    ret = OSA_bufGetFull(&pBufHndl->buf_handle, &bufId, OSA_TIMEOUT_FOREVER);
    if (ret < 0) {
        fprintf(stderr, "%s: OSA_bufGetEmpty failed.\n", __FUNCTION__);
        buf_unlock(pBufHndl);
        return -1;
    }

    pBufinfo = OSA_bufGetBufInfo(&pBufHndl->buf_handle, bufId);
    if (pBufinfo == NULL) {
        fprintf(stderr, "%s: OSA_bufGetBufInfo [bufId: %d] failed.\n", __FUNCTION__, bufId);
        buf_unlock(pBufHndl);
        return -1;
    }

    *buf_ptr = pBufinfo->virtAddr;
    buf_unlock(pBufHndl);
    
    return pBufinfo->size;
}


int pcie_buf_setFull(struct pcie_bufHndl *pBufHndl, char *buf_ptr, int buf_size)
{
   int ret = 0;
   int i;
   OSA_BufInfo *pBufinfo = NULL;
   
   for(i = 0;i < pBufHndl->buf_handle.numBuf; i ++){
        if(buf_ptr == pBufHndl->buf_handle.bufInfo[i].virtAddr)
            break;
    }

    if(i == pBufHndl->buf_handle.numBuf){
        return -1;
    }

   buf_lock(pBufHndl);
   
   pBufinfo = OSA_bufGetBufInfo(&pBufHndl->buf_handle, i);
    if (pBufinfo == NULL) {
        fprintf(stderr, "%s: OSA_bufGetBufInfo [bufId: %d] failed.\n", __FUNCTION__, i);
        buf_unlock(pBufHndl);
        return -1;
    }

    pBufinfo->size = buf_size;

    ret = OSA_bufPutFull(&pBufHndl->buf_handle, i);
    if (ret < 0) {
        fprintf(stderr, "%s: OSA_bufPutEmpty [bufId: %d] failed.\n", __FUNCTION__, i);
        buf_unlock(pBufHndl);
        return -1;
    }

    buf_unlock(pBufHndl);
    
    return 0;    
}

//return : empty:TRUE  notEmpty:FALSE
int pcie_buf_isEmpty(struct pcie_bufHndl *pBufHndl)
{
    int ret;

    buf_lock(pBufHndl);
    
    if (OSA_queIsEmpty(&pBufHndl->buf_handle.fullQue) == TRUE)
        ret = TRUE;
    else
        ret = FALSE;
    
    buf_unlock(pBufHndl);
    
    return ret;
}

//return : full:TRUE  notFull:FALSE
int pcie_buf_isFull(struct pcie_bufHndl *pBufHndl)
{
    int ret;

    buf_lock(pBufHndl);
    
    if (OSA_queIsEmpty(&pBufHndl->buf_handle.emptyQue) == TRUE)
        ret = TRUE;
    else
        ret = FALSE;
    
    buf_unlock(pBufHndl);
    
    return ret;
}
