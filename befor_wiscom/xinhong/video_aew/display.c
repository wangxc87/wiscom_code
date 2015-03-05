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
#include "display.h"

#define V4L2VID0_DEVICE    "/dev/video2"
#define V4L2VID1_DEVICE    "/dev/video3"

#define DISPLAY_INTERFACE_COMPOSITE	"COMPOSITE"
#define DISPLAY_INTERFACE_COMPONENT	"COMPONENT"
#define DISPLAY_MODE_PAL	"PAL"
#define DISPLAY_MODE_NTSC	"NTSC"
#define DISPLAY_MODE_720P	"720P-60"
#define DISPLAY_MODE_1080I	"1080I-30"
/* Standards and output information */
#define ATTRIB_MODE		"mode"
#define ATTRIB_OUTPUT		"output"
#define DEBUG_display
#ifdef DEBUG_display
#define  display_debug(fmt, args...) printf("%s: " fmt,__func__,## args)
#else
#define  display_debug(fmt, args...)
#endif


/*******************************************************************************
 *	Function will use the SysFS interface to change the output and mode
 */
static int change_sysfs_attrib(char *attribute, char *value)
{
	int sysfd = -1;
	char init_val[32];
	char attrib_tag[128];

	bzero(init_val, sizeof(init_val));
	strcpy(attrib_tag, "/sys/class/davinci_display/ch0/");
	strcat(attrib_tag, attribute);

	sysfd = open(attrib_tag, O_RDWR);
	if (!sysfd) {
		printf("Error: cannot open %d\n", sysfd);
		return -1;
	}
	printf("%s was opened successfully\n", attrib_tag);

	read(sysfd, init_val, 32);
	lseek(sysfd, 0, SEEK_SET);
	printf("Current %s value is %s\n", attribute, init_val);

	write(sysfd, value, 1 + strlen(value));
	lseek(sysfd, 0, SEEK_SET);

	memset(init_val, '\0', 32);
	read(sysfd, init_val, 32);
	lseek(sysfd, 0, SEEK_SET);
	printf("Changed %s to %s\n", attribute, init_val);

	close(sysfd);
	return 0;
}

/******************************************************************************/
 int start_display_streaming(int fdDisplay,struct buf_info *capture_buffers,int num_buf)
{
	int i = 0;
	enum v4l2_buf_type type;
    struct v4l2_requestbuffers req;
    int ret;
    
    req.count = num_buf;
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(fdDisplay, VIDIOC_REQBUFS, &req);
	if (ret) {
      perror("cannot allocate memory\n");
      return -1;
	}
    
	for (i = 0; i < num_buf; i++) {
		struct v4l2_buffer buf;
		
        memset(&buf,0,sizeof(struct v4l2_buffer));
        
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.length =capture_buffers[i].buf_size;
        /*		if (disp_second_output) {
			buf.m.userptr = (unsigned long)capture_buffers[i].user_addr +
			second_output_offset;
			buf.length = ALIGN((second_out_width * second_out_height * 2), 4096);
            } else*/
			buf.m.userptr = (unsigned long)capture_buffers[i].user_addr;
		printf("Queing buffer:%d\n", i);

		if (-1 == ioctl(fdDisplay, VIDIOC_QBUF, &buf)) {
			printf("start_display_streaming:ioctl:VIDIOC_QBUF:error\n");
			return -1;
		}
	}
	/* all done , get set go */
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (-1 == ioctl(fdDisplay, VIDIOC_STREAMON, &type)) {
		printf("start_display_streaming:ioctl:VIDIOC_STREAMON:error\n");
		return -1;
	}

	return 0;
}

int display_open(int device)
{
  int fdDisplay;
  
  if (device == 0) {
    if ((fdDisplay = open(V4L2VID0_DEVICE, O_RDWR)) < 0) {
      printf("Open failed for vid0\n");
      return -1;
    }
  }
  else if (device == 1)
  {
    if ((fdDisplay = open(V4L2VID1_DEVICE, O_RDWR)) < 0) {
      printf("Open failed for vid1\n");
      return -1;
    }
  } else {
    printf("Invalid device id\n");
    return -1;
  }
  return fdDisplay;
}


// assumes we use ntsc display
int init_display_device(int fdDisplay,struct display_param display_param )
{
	struct v4l2_requestbuffers req;
	int  ret;
	struct v4l2_format fmt;
	char output[40], mode[40];
    int width,  height;
    int output_channel = 0;
    
    width  = display_param.width;
    height = display_param.height;
    printf("Display:width %d,height %d\n",width,height);
    
	//CLEAR(fmt);
    memset(&fmt,0,sizeof(struct v4l2_format));
    
    if (output_channel) {// lCD output
      strcpy(output, "LCD");
      strcpy(mode, "480x272");
      fmt.fmt.pix.field = V4L2_FIELD_NONE;
    }else{ //TV output
      if (width <= 750 && height <= 480) {
		strcpy(output, DISPLAY_INTERFACE_COMPOSITE);
		strcpy(mode, DISPLAY_MODE_NTSC);
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
      } else if (width <= 750 && height <= 576) {
		strcpy(output, DISPLAY_INTERFACE_COMPOSITE);
		strcpy(mode, DISPLAY_MODE_PAL);
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
      } else if (width <= 1280 && height <= 720) {
		strcpy(output, DISPLAY_INTERFACE_COMPONENT);
		strcpy(mode, DISPLAY_MODE_720P);
		fmt.fmt.pix.field = V4L2_FIELD_NONE;
      } else if (width <= 1920 && height <= 1080) {
		strcpy(output, DISPLAY_INTERFACE_COMPONENT);
		strcpy(mode, DISPLAY_MODE_1080I);
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
      } else {
		printf("Cannot display input video resolution width = %d, height = %d\n",
               width, height);
		return -1;
      }
    }
	if (change_sysfs_attrib(ATTRIB_OUTPUT, output))
      return -1;
	if (change_sysfs_attrib(ATTRIB_MODE, mode))
      return -1;
    
    int output_format = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (output_format) {
      printf("Setting display pixel format to V4L2_PIX_FMT_NV12\n");
      //      fmt.fmt.pix.bytesperline = ((width + 31) & ~31);
      //      fmt.fmt.pix.sizeimage= ((fmt.fmt.pix.bytesperline * height) +
      //                            (fmt.fmt.pix.bytesperline * (height >> 1)));
      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
      fmt.fmt.pix.width = width;
      fmt.fmt.pix.height = height;
	} else {
      printf("Setting display pixel format to V4L2_PIX_FMT_UYVY\n");
      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY;
      //      fmt.fmt.pix.bytesperline = width * 2;
      fmt.fmt.pix.width = width;
      fmt.fmt.pix.height = height;
      //      fmt.fmt.pix.sizeimage= (fmt.fmt.pix.bytesperline * height);
	}

	ret = ioctl(fdDisplay, VIDIOC_S_FMT, &fmt);
	if (ret) {
      perror("VIDIOC_S_FMT failed\n");
      goto error;
	}

    memset(&fmt,0,sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ret = ioctl(fdDisplay, VIDIOC_G_FMT, &fmt);
	if (ret) {
      perror("VIDIOC_G_FMT failed\n");
      goto error;
	}
    display_debug("Modified FMT is:\n");
    display_debug("\tfmt.fmt.pix.width = %d\n",fmt.fmt.pix.width);
    display_debug("\tfmt.fmt.pix.height = %d\n",fmt.fmt.pix.height);
    display_debug("\tfmt.fmt.pix.bytesperline = %d\n",fmt.fmt.pix.bytesperline);
    display_debug("\tfmt.fmt.pix.sizeimage = %d\n",fmt.fmt.pix.sizeimage);
    if(V4L2_PIX_FMT_UYVY == fmt.fmt.pix.pixelformat)
      display_debug("\tfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY(%d)\n");
    else if(V4L2_PIX_FMT_NV12 == fmt.fmt.pix.pixelformat)
      display_debug("\tfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12(%d)\n");
    else display_debug("\tfmt.fmt.pix.pixelformat = UNKOWN\n");
    

    
    /*    struct v4l2_crop crop;
    int zoom = 1;
	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	crop.c.top  = 0;
	crop.c.left = 0;
	crop.c.width= width*zoom;
	crop.c.height= height*zoom;

	ret = ioctl(fdDisplay, VIDIOC_S_CROP, &crop);
	if (ret) {
      perror("VIDIOC_S_CROP\n");
      close(fdDisplay);
      exit(0);
	}
    printf("VIDIOC_S_CROP OK !\n");
    */
    /*
	req.count = APP_NUM_BUFS/2;
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(fdDisplay, VIDIOC_REQBUFS, &req);
	if (ret) {
      perror("cannot allocate memory\n");
      goto error;
	}

	start_display_streaming(fdDisplay);
	*/
    return 0;
 error:
//	close(fdDisplay);
	return -1;
}

int cleanup_display(int fd)
{
	int err = 0;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
		perror("cleanup_capture :ioctl:VIDIOC_STREAMOFF");
		err = -1;
	}

	if (close(fd) < 0) {
		perror("Error in closing device\n");
		err = -1;
	}
	return err;
}
