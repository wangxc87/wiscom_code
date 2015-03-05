#include "bitsRead.h"

int bitsRead_init(struct BufInfo *pBufInfo)
{
    int ret;
    FILE *pFile;
    int fd;
    pFile = fopen(DEFINE_FILE_HDR_NAME,"r");
    if(pFile == NULL){
        fprintf(stdout,"Open file %s Error.\n",DEFINE_FILE_HDR_NAME);
        return -1;
    }
    fd = open(DEFINE_FILE_DATA_NAME,O_RDONLY);
    if(fd < 0){
        fprintf(stdout,"Open file %s Error.\n",DEFINE_FILE_DATA_NAME);
        return -1;
    }
    pBufInfo->fpRdHdr = pFile;
    pBufInfo->fdRdData = fd;
    pBufInfo->pBuffer = malloc(MAX_BITSBUF_SIZE);
    if(pBufInfo->pBuffer == NULL){
        fprintf(stdout,"memery malloc Error.\n");
        return -1;
    }
    pBufInfo->MaxBufsize = MAX_BITSBUF_SIZE;
    fprintf(stdout,"open files successful.\n");
    return 0;    
}
/*
  return: sucess 0: error:-1
  
 */
int bitsRead_process(struct BufInfo *pbufInfo)
{
    int ret;
    if((pbufInfo->fpRdHdr == NULL) || (pbufInfo->fdRdData == 0)){
        fprintf(stderr,"file handle is NULL..\n");
        return -1;
    }
    if(pbufInfo->pBuffer == NULL)
        return -1;
    ret  = fscanf(pbufInfo->fpRdHdr,"%d",&(pbufInfo->filledBufSize));
    if(pbufInfo->filledBufSize > pbufInfo->MaxBufsize)
        fprintf(stderr,"buf size error .\n");
    /*    else
        fprintf(stdout,"read from file hdr %d,\n",pbufInfo->filledBufSize);
    */
    ret = read(pbufInfo->fdRdData, pbufInfo->pBuffer, pbufInfo->filledBufSize);

    if( feof(pbufInfo->fpRdHdr) || ret != pbufInfo->filledBufSize)
    {
        fprintf(stderr," Reached the end of file!!!");
        return -1;
#ifdef REWIND_FILE
        clearerr(pbufInfo->fpRdHdr);
        
        rewind(pbufInfo->fpRdHdr);
        lseek(pbufInfo->fdRdData, 0, SEEK_SET);
        ret = fscanf(pbufInfo->fpRdHdr,"%d",&(pbufInfo->filledBufSize));
        
        ret = read(pbufInfo->fdRdData, pbufInfo->pBuffer,pbufInfo->filledBufSize);
#endif
    }
    return 0;   
}

int bitsRead_deinit(struct BufInfo  *pbufInfo)
{
    int ret;
    fclose(pbufInfo->fpRdHdr);
    close(pbufInfo->fdRdData);
    free(pbufInfo->pBuffer);
    return 0;
}


