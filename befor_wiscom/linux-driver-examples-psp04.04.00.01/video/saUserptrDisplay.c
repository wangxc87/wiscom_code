/*
 * saUserptrDisplay.c
 *
 * This is a V4L2 sample application to show the display functionality
 * The app puts a moving horizontal bar on the display device in various
 * shades of colors. It uses user pointer buffer exchange mechanism. It
 * takes buffers from FBDEV drive and provides virtual address of the
 * buffers to the V4L2. This appplication runs in RGB565 mode with VGA
 * display resolution.
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

 /******************************************************************************
  Header File Inclusion
 ******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
/******************************************************************************
 Macros
 MAX_BUFFER     : Changing the following will result different number of
		  instances of buf_info structure.
 MAXLOOPCOUNT	: Display loop count
 ******************************************************************************/
#define MAX_BUFFER	3
#define MAXLOOPCOUNT	1000

#define WIDTH 1920
#define HEIGHT 1080

static int display_fd = 0;
static int fbdev_fd = 0;
static char display_dev_name[20] = {"/dev/video1"};
static char fbdev_driver_name[20] = {"/dev/fb0"};

struct buf_info {
	int index;
	unsigned int length;
	char *start;
};

static struct buf_info display_buff_info[MAX_BUFFER];
static int numbuffers = MAX_BUFFER;

/******************************************************************************
                        Function Definitions
 ******************************************************************************/
static int releaseDisplay(int ret_flag);
static int startDisplay(void);
static int stopDisplay(void);
void color_bar(char *addr, int width, int line_len, int height);
int app_main(void);

static int calc_result_time(struct timeval *result, struct timeval *after,
		                                     struct timeval *before)
{
	/* Perform the carry for the later subtraction by updating "before" */
	if (after->tv_usec < before->tv_usec) {
		int nsec = (before->tv_usec - after->tv_usec) / 1000000 + 1;
		before->tv_usec -= 1000000 * nsec;
		before->tv_sec += nsec;
	}
	if (after->tv_usec - before->tv_usec > 1000000) {
		int nsec = (after->tv_usec - before->tv_usec) / 1000000;

		before->tv_usec += 1000000 * nsec;
		before->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait, tv_usec is certainly positive.
	 * */
	result->tv_sec = after->tv_sec - before->tv_sec;
	result->tv_usec = after->tv_usec - before->tv_usec;
	/* Return 1 if result is negative. */
	return after->tv_sec < before->tv_sec;
}

/*
        This routine unmaps all the buffers
        This is the final step.
*/
static int releaseDisplay(int ret_flag)
{
	int i;
	if(ret_flag > -6) {
		for (i = 0; i < numbuffers; i++) {
			munmap(display_buff_info[i].start,
					display_buff_info[i].length);
			display_buff_info[i].start = NULL;
		}
	}
	close(display_fd);
	close(fbdev_fd);
	display_fd = 0;
	fbdev_fd = 0;
	return 0;
}
/*
	Starts Streaming
*/
static int startDisplay(void)
{
	int a = V4L2_BUF_TYPE_VIDEO_OUTPUT, ret;
	ret = ioctl(display_fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		return -1;
	}
	return 0;
}

/*
 Stops Streaming
*/
static int stopDisplay(void)
{
	int ret, a = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(display_fd, VIDIOC_STREAMOFF, &a);
	return ret;
}

static unsigned long yuyv[8] = {
	0x80eb80eb,
	0x801e801e,
	0xf0515a51,
	0x22913691,
	0x6e29f029,
	0x92d210d2,
	0x10aaa6aa,
	0xde6aca6a,
};

void color_bar(char *addr, int width, int line_len, int height)
{
	unsigned long *ptr = (unsigned long *)addr;
	int i, j, k;

	for(i = 0 ; i < 8 ; i++) {
		for(j = 0 ; j < (height/8) ; j++) {
			for(k = 0 ; k < (width >> 1) ; k++) {
				ptr[k] = yuyv[i];
			}
			ptr = ptr + (width >> 1);
			if((unsigned int)ptr >=
					((unsigned int)addr + line_len * height))
				ptr = (unsigned long *)addr;
		}
	}
}

int sysfs_write(char *fileName, char *val)
{
	FILE *fp;
	char *valString;
	int ret;
	valString = malloc(strlen(val) + 1);

	if (valString == NULL) {
		printf("Failed to allocate memory for temporary string\n");
		return -1;
	}
	fp = fopen(fileName, "w");
	if (fp == NULL) {
		printf("Failed to open %s for writing\n", fileName);
		free(valString);
		return -1;
	}
	ret = fwrite(val, strlen(val) + 1, 1, fp);
	if ( ret != 1) {
		printf("Failed to write sysfs variable %s to %s\n", fileName, val);
		fclose(fp);
		free(valString);
		return -1;
	}
	fclose(fp);
	free(valString);
	return 0;
}


int app_main(void)
{
	int mode = O_RDWR;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_format fmt;
	int i = 0;
	void *displaybuffer;
	int counter = 0;
	int ret = 0;
	int dispheight, dispwidth, sizeimage, line_pitch;
	unsigned long buffer_addr[MAX_BUFFER];
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, oldvarinfo;
	int buffersize;
	struct timeval before, after, result;

	/* open display channel */
	display_fd = open((const char *)display_dev_name, mode);
	if(display_fd == -1) {
		printf("Failed to open display device\n");
		return 0;
	}

	fbdev_fd = open((const char *)fbdev_driver_name, O_RDWR);
	if(fbdev_fd <= 0) {
		perror("Cound not open device\n");
		close(display_fd);
		return 0;
	}

	sysfs_write("/sys/devices/platform/vpss/graphics0/enabled", "0");

	/* Set the image size to 640x480 and pixel format to RGB565 */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	ret = ioctl(display_fd, VIDIOC_S_FMT, &fmt);
	if(ret < 0) {
		perror("Set Format failed\n");
		return -1;
	}


	/* It is necessary for applications to know about the
	 * buffer chacteristics that are set by the driver for
	 * proper handling of buffers
	 * These are : width,height,pitch and image size */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(display_fd, VIDIOC_G_FMT, &fmt);
	if(ret < 0) {
		perror("Get Format failed\n");
		return -2;
	}
	dispheight = fmt.fmt.pix.height;
	dispwidth = fmt.fmt.pix.width;
	line_pitch = fmt.fmt.pix.bytesperline;
	sizeimage = fmt.fmt.pix.sizeimage;

	/* Now for the buffers.Request the number of buffers needed and
	 * the kind of buffers(User buffers or kernel buffers for memory
	 * mapping). Please note that the return value in the reqbuf.count
	 * might be lesser than numbuffers under some low memory
	 * circumstances */
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.count = numbuffers;
	reqbuf.memory = V4L2_MEMORY_USERPTR;

	ret = ioctl(display_fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		perror("Could not allocate the buffers\n");
		return -3;
	}


	/* Now allocate physically contiguous buffers. Application uses
	 * FBDEV driver to get the physically contiguous buffer. Any other
	 * mechanism can be used to get the buffer and provide user
	 * space virtual address to the V4L2 drier.
	 * Using physical contiguous buffer from FBDEV driver is not
	 * standard/recomded method. Application should use cmem or any
	 * other method to have physically contiguous buffer. We are FBDEV
	 * due to lack of having any other method for physically contiguous
	 * buffer. Make sure that size of the buffer in FBDEV is enough to
	 * store the complete frame of 640*480*2 size */

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 * Here fix screen info is used to get line length/pitch.
	 */
	ret = ioctl(fbdev_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return -18;
	}

	/* Get variable screen information. Variable screen information
	 * gives informtion like size of the image, bites per pixel,
	 * virtual size of the image etc. Here variable screen info is
	 * used to get the screen resolution */
	ret = ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		return -4;
	}

	oldvarinfo = varinfo;
	varinfo.bits_per_pixel = 16;
	ret =ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error setting variable information.\n");
		return -5;
	}

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 * Here fix screen info is used to get line length/pitch.
	 */
	ret = ioctl(fbdev_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return -13;
	}

	/* Mmap the driver buffers in application space so that application
	 * can write on to them. Driver allocates contiguous memory for
	 * three buffers. These buffers can be displayed one by one. */
	buffersize = fixinfo.line_length * varinfo.yres;
	printf("Buffer size = %d, X - %d, Y - %d\n",fixinfo.line_length *
		varinfo.yres * MAX_BUFFER, fixinfo.line_length, varinfo.yres);

	buffer_addr[0] = (unsigned long)mmap ((void*)0, buffersize*MAX_BUFFER,
			(PROT_READ|PROT_WRITE), MAP_SHARED, fbdev_fd, 0) ;
	if (buffer_addr[0] == (unsigned long)MAP_FAILED) {
		printf("MMap failed for %d x %d  of %d\n", fixinfo.line_length,
				varinfo.yres*MAX_BUFFER, fixinfo.smem_len);
		return -6;
	}
	memset((void*)buffer_addr[0], 0, buffersize*MAX_BUFFER);

	/* Store each buffer addresses in the local variable */
	for(i = 1 ; i < MAX_BUFFER ; i++) {
		buffer_addr[i] = buffer_addr[i-1] + buffersize;
	}
	for (i = 0; i  < reqbuf.count; i++) {
		color_bar((char *)buffer_addr[i], dispwidth, line_pitch, dispheight);
	}


	/* enqueue all the buffers in the driver's incoming queue. Driver
	 * will take buffers one by one from this incoming queue.
	 * buffer length is must parameter for user pointer buffer exchange
	 * mechanism. Using this parameters, driver validates user buffer
	 * size with the size required to store image of given resolution. */
	memset(&buf,  0, sizeof(buf));
	for(i = 0 ; i < reqbuf.count ; i ++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = buffer_addr[i];
		buf.length = buffersize;
		ret = ioctl(display_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -7;
		}
	}
	gettimeofday(&before, NULL);

	/* Start Displaying */
	ret = startDisplay();
	if(ret < 0) {
		perror("Error in starting display\n");
		return -8;
	}

	/*
	   This is a running loop where the buffer is
	   DEQUEUED  <-----|
	   PROCESSED	|
	   & QUEUED -------|
	 */
	counter = 0;
	while(counter < MAXLOOPCOUNT) {
		/* Get display buffer using DQBUF ioctl */
		ret = ioctl(display_fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_DQBUF\n");
			return -9;
		}
		displaybuffer = (void*)buffer_addr[buf.index];
		/* Now queue it back to display it */
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.m.userptr = buffer_addr[buf.index];
		buf.length = buffersize;
		ret = ioctl(display_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -10;
		}

		counter++;
	}
	gettimeofday(&after, NULL);
	calc_result_time(&result, &after, &before);
	printf("The V4L2 Display : Frame rate = %lu\n", MAXLOOPCOUNT/result.tv_sec);

	/* Once the streaming is done  stop the display hardware */
	ret = stopDisplay();
	if(ret < 0) {
		perror("Error in stopping display\n");
		return -11;
	}
	sysfs_write("/sys/devices/platform/vpss/graphics0/enabled", "1");
	/*store back*/
	ret =ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, &oldvarinfo);
	if (ret < 0) {
		perror("Error setting variable information.\n");
		return -12;
	}

	return 0;
}

int main(void)
{
	int ret;
	ret = app_main();
	releaseDisplay(ret);
	return ret;
}


