#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include "cmem.h"
#include "capture.h"

struct display_param 
{
  int width;
  int height;
  unsigned int pixelformat;
  struct v4l2_rect c;
};

  
int start_display_streaming(int fdDisplay,struct buf_info *capture_buffers,int num_buf);

int display_open(int device);

int init_display_device(int fdDisplay, struct display_param display_param);
int cleanup_capture(int fd);

int cleanup_display(int fd);

#endif
