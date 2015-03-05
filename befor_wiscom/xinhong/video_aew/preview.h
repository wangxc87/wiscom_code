#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>

int preview_open(void);

int preview_config(int preview_fd,unsigned long user_mode,void *config,int chain);

int preview_querybuf(int preview_fd,struct imp_reqbufs req_buf,struct imp_buffer *buf_in);


int wb_set(int preview_fd,struct prev_wb *wb,int flag);

#endif
