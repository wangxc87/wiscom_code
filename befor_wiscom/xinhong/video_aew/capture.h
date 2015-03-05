#ifndef _CAPTURE_H_
#define _CAPTURE_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/videohd.h>
#include <media/davinci/dm365_ccdc.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>
#include <termios.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>
#include "cmem.h"

#define ALIGN(x, y)	(((x + (y-1))/y)*y)
struct buf_info {
  void *user_addr;
  unsigned long phy_addr;
  unsigned long buf_size;
 };

 struct capt_std_params {
	v4l2_std_id std;
	/* input image params */
	unsigned int scan_width;
	unsigned int scan_height;
	/* crop params */
	struct v4l2_rect crop;
	/* output image params */
	unsigned int image_width;
	unsigned int image_height;
	char *name;
};
//#define MAX_STDS 
extern struct capt_std_params mt9p031_std_params[13];


int set_data_format(int fdCapture, struct capt_std_params input_std_params);

int start_capture_streaming(int fdCapture,struct buf_info *capture_buffers,int num_buf);

int init_camera_capture(int capt_fd,struct capt_std_params capt_params);

int allocate_user_buffers(struct buf_info *capture_buffers,int num_buf,unsigned long buf_size);

int capture_open(void);


#endif
