#ifndef _BITS_READ_H_
#define _BITS_READ_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#define DEFINE_FILE_DATA_NAME "1920x1080_00.h264"
#define DEFINE_FILE_HDR_NAME "1920x1080_00.h264.hdr"
#define ERR_FILE_END 0xffff0001
#define MAX_BITSBUF_SIZE (1024*1024)
struct BufInfo
{
    FILE *fpRdHdr;
    int fdRdData;
    char *pBuffer;
    int MaxBufsize;
    int filledBufSize;
};

int bitsRead_init(struct BufInfo *pBufInfo);

int bitsRead_process(struct BufInfo *pbufInfo);

int bitsRead_deinit(struct BufInfo *pbufinfo);

#endif
