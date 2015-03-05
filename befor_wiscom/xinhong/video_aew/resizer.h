#ifndef _RESIZER_h_
#define _RESIZER_h_

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

int resizer_open(void);

int resizer_config(int resizer_fd,unsigned long user_mode,void *config,int chain);

int resizer_convert(int resizer_fd,struct imp_convert convert);


#endif
