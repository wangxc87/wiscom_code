#ifndef __KBUF_UTILS_H__
#define __KBUF_UTILS_H__

#include "kfifo_buf.h"

struct pcie_bufHndl {
    OSA_BufHndl buf_handle;
    /* sem_t buf_lock; */
    int cur_fullId;
    int cur_emptyId;
};

int pcie_buf_init(struct pcie_bufHndl *pBufHndl, struct kfifo_buf_create_arg *buf_create);

int pcie_buf_deInit(struct pcie_bufHndl *pBufHndl);

char *pcie_buf_getEmtpy(struct pcie_bufHndl *pBufHndl);

int pcie_buf_setEmpty(struct pcie_bufHndl *pBufHndl, char *buf_ptr);

int pcie_buf_getFull(struct pcie_bufHndl *pBufHndl, char **buf_ptr);

int pcie_buf_setFull(struct pcie_bufHndl *pBufHndl, char *buf_ptr, int buf_size);

int pcie_buf_isFull(struct pcie_bufHndl *pBufHndl);

int pcie_buf_isEmpty(struct pcie_bufHndl *pBufHndl);

#endif
