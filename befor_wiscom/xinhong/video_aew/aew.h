#ifndef _AEW_H_
#define _AEW_H_

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/* Kernel header file, prefix path comes from makefile */
#include <media/davinci/dm365_aew.h>
#include <media/davinci/dm365_a3_hw.h>
#include <linux/videodev2.h>
#include <media/davinci/vpfe_types.h>
#include <media/davinci/dm365_ccdc.h>
#include <media/davinci/dm365_ipipe.h>

int aew_config(int fd,int *buf_size);

//int aew_read(int fd,void *buffer,int buff_size);

int aew_save(int fd,int buff_size);

int aew_open(void);

int aew_ctrl(int fd,struct prev_wb *wb);

#endif
