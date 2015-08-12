#ifndef __KFIFO_BUF_H__
#define __KFIFO_BUF_H__

#include <linux/kfifo.h>
#include <linux/wait.h>

#define KFIFO_BUF_NUM_MAX (64)

#define OSA_SOK      0  ///< Status : OK
#define OSA_EFAIL   -1  ///< Status : Generic error
#define OSA_BUF_ID_INVALID    (-1)

#define OSA_TIMEOUT_NONE        ((u32) 0)  // no timeout
#define OSA_TIMEOUT_FOREVER     ((u32)-1)  // wait forever


struct kfifo_buf_create_arg {
    void *physAddr[KFIFO_BUF_NUM_MAX];
    void *virtAddr[KFIFO_BUF_NUM_MAX];
    int numBuf;
};

struct kfifo_que_obj {
    struct kfifo kfifo_hndl;
    wait_queue_head_t kfifo_que_wq_h;    
};
    
struct kbuf_info {
    int size;
    void   *physAddr;
    void   *virtAddr;    
};

struct kfifo_buf_obj {
    struct kbuf_info bufInfo[KFIFO_BUF_NUM_MAX];
    struct kfifo_que_obj fullQue;
    struct kfifo_que_obj emptyQue;
    int numBuf;
};

typedef struct kbuf_info OSA_BufInfo;
typedef struct kfifo_buf_obj OSA_BufHndl;

int OSA_bufDelete(OSA_BufHndl *hndl);

int OSA_bufCreate(OSA_BufHndl *hndl,
                     struct kfifo_buf_create_arg *bufInit);

int OSA_bufGetEmpty(OSA_BufHndl *hndl, int *bufId, u32 timeout);

int OSA_bufPutFull (OSA_BufHndl *hndl, int bufId);

int OSA_bufGetFull(OSA_BufHndl *hndl, int *bufId, u32 timeout);

int OSA_bufPutEmpty(OSA_BufHndl *hndl, int bufId);

OSA_BufInfo *OSA_bufGetBufInfo(OSA_BufHndl *hndl, int bufId);


#endif
