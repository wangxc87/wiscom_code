/*
 * saMmapDisplay.c
 *
 * This is a V4L2 sample application to show the display functionality
 * The app puts a moving horizontal bar on the display device in various
 * shades of colors. This appplication runs in RGB565 mode with VGA
 * display resolution. It shows output on lcd.
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

/*
 * Header File Inclusion
 */
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
/*
 * Macros Defination
 */
#define MAX_BUFFER	3
#define MAXLOOPCOUNT	500
#define WIDTH 1920
#define HEIGHT 1080
static int display_fd = 0;
static char display_dev_name[20] = {"/dev/video1"};

struct buf_info {
	int index;
	unsigned int length;
	char *start;
};

static struct buf_info display_buff_info[MAX_BUFFER];
static int numbuffers = MAX_BUFFER;

/*
 * Function Definitions
 */
static int releaseDisplay(void);
static int startDisplay(void);
static int stopDisplay(void);
void color_bar(char *addr, int w, int line_len, int h, int order);
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
 * This routine unmaps all the buffers
 * This is the final step.
 */
static int releaseDisplay()
{
	int i;
	for (i = 0; i < numbuffers; i++) {
		munmap(display_buff_info[i].start,
				display_buff_info[i].length);
		display_buff_info[i].start = NULL;
	}
	close(display_fd);
	display_fd = 0;
	return 0;
}
/*
 *	Starts Streaming
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
 * Stops Streaming
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

void color_bar(char *addr, int width, int line_len, int height, int order)
{
	unsigned long *ptr = (unsigned long *)addr + order*width;
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

int app_main(void)
{
	struct v4l2_buffer buf;
	struct v4l2_format fmt;
	int i, ret = 0, counter = 0;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_capability capability;
	int dispheight, dispwidth, line_len;
	struct timeval before, after, result;

	/* open display channel */
	display_fd = open((const char *)display_dev_name, O_RDWR);
	if(display_fd == -1) {
		perror("failed to open display device\n");
		return -1;
	}

	/* Check if the device is capable of streaming */
	if (ioctl(display_fd, VIDIOC_QUERYCAP, &capability) < 0) {
		perror("VIDIOC_QUERYCAP");
		return -1;
	}

	/* Get the parameters before setting and
	 * set only required parameters */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(display_fd, VIDIOC_G_FMT, &fmt);
	if(ret<0) {
		perror("Get Format failed\n");
		return -1;
	}

	/* Set the image size to VGA and pixel format to RGB565 */
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	ret = ioctl(display_fd, VIDIOC_S_FMT, &fmt);
	if(ret<0) {
		perror("Set Format failed\n");
		return -1;
	}

	/* It is necessary for applications to know about the
	 * buffer chacteristics that are set by the driver for
	 * proper handling of buffers These are : width,height,
	 * pitch and image size
	 * */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(display_fd, VIDIOC_G_FMT, &fmt);
	if(ret<0){
		perror("Get Format failed\n");
		return -1;
	}
	dispheight = fmt.fmt.pix.height;
	dispwidth = fmt.fmt.pix.width;
	line_len = fmt.fmt.pix.bytesperline;

	/* Now for the buffers.Request the number of buffers needed
	 * and the kind of buffers(User buffers or kernel buffers
	 * for memory mapping). Please note that the return value
	 * in the reqbuf.count might be lesser than numbuffers under
	 * some low memory circumstances */
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.count = numbuffers;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(display_fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		perror("Could not allocate the buffers\n");
		return -1;
	}

	/*
	   Now map the buffers to the user space so the app can write
	   on to them( This is for driver buffers and not for User
	   pointers).This is done in two stages:
	   1. Query for the buffer info like the phys address
	   2. mmap the buffer to user space.
	   This information anout the buffers is currently stored in
	   a user level data structue
	 */

	numbuffers = reqbuf.count;
	for(i = 0 ; i < reqbuf.count ; i ++) {
		/* query */
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(display_fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			perror("quering for buffer info failed\n");
			return -1;
		}
		/* mmap */
		display_buff_info[i].length = buf.length;
		display_buff_info[i].index = i;
		display_buff_info[i].start =
			mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
					MAP_SHARED, display_fd, buf.m.offset);

		if ((unsigned int) display_buff_info[i].
				start == MAP_SHARED) {
			printf("Cannot mmap = %d buffer\n", i);
			return -1;
		}
		/*
		   After mapping each buffer, it is a good
		   idea to first "zero" them out.
		   Here it is being set to a mid grey-scale
		   Y=0x80, Cb=0x80, Cr=0x80
		 */
		memset(display_buff_info[i].start, 0x80, buf.length);

		/* Fill up the buffers with the values.*/
		color_bar(display_buff_info[i].start, dispwidth, line_len, dispheight, 0);
	}

	for(i = 0 ; i < reqbuf.count ; i ++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		ret = ioctl(display_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
	}
	gettimeofday(&before, NULL);

	/* Start Displaying */
	ret = startDisplay();
	if(ret < 0) {
		perror("Error in starting display\n");
		return ret;
	}

	/*
	   This is a running loop where the buffer is
	   DEQUEUED  <-----|
	   PROCESSED	|
	   & QUEUED -------|
	 */
	while(counter < MAXLOOPCOUNT) {
		/*
		   Get display buffer using DQBUF ioctl
		 */
		ret = ioctl(display_fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_DQBUF\n");
			return -1;
		}

		/* Now queue it back to display it */
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = buf.index;
		ret = ioctl(display_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}

		counter++;
	}

	gettimeofday(&after, NULL);
	calc_result_time(&result, &after, &before);
	printf("The V4L2 Display : Frame rate = %lu\n", MAXLOOPCOUNT/result.tv_sec);

	/*
	   Once the streaming is done  stop the display
	   hardware
	 */
	ret = stopDisplay();
	if(ret < 0) {
		perror("Error in stopping display\n");
		return ret;
	}
	/* open display channel */
	releaseDisplay();

	return ret;
}

int main(void)
{
	if(app_main())
		releaseDisplay();

	return 0;
}


