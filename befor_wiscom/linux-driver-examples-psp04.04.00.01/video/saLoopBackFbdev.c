/*
 * saLoopBackFbdev.c
 *
 * Copyright (C) 2011 TI
 * Author: Hardik Shah <hardik.shah@ti.com>
 *
 * This is a sample for loopback application. This application captures through
 * V4L2 capture driver on TVP7002 input, converts it to RGB using
 * HDVPSS capture color space converter and displays the captured content
 * on FBDEV
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <linux/ti81xxfb.h>
#include <linux/ti81xxvin.h>
#include<linux/videodev2.h>
/* Number of buffers capture driver */
#define MAX_BUFFER	4
/* Number of times queue/dequeue happens before stopping applications */
#define MAXLOOPCOUNT	500

/* Application name */
#define APP_NAME		"saLoopBackFbdev"

/* device node to be used for capture */
#define CAPTURE_DEVICE		"/dev/video0"

/* device node to be used for fbdev */
#define FBDEV_DEVICE		"/dev/fb0"

/* Define below #def for debug information enable */
#undef SALOOPBACKFBDEV_DEBUG

/* Structure for storing application data like file pointers, format etc.
 * Same structure will be used for fbdev as well as V4L2 capture. Only/
 * required fields will be initialized for both of them*/
struct app_obj {
	int fd;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_dv_preset dv_preset;
	struct v4l2_requestbuffers reqbuf;
	unsigned int numbuffers;
	unsigned int buffersize;
	unsigned char *buffer_addr[MAX_BUFFER];
	struct v4l2_buffer buf;
	struct ti81xxvin_overflow_status over_flow;
	struct fb_var_screeninfo varinfo;
	struct fb_var_screeninfo org_varinfo;
	struct fb_fix_screeninfo fixinfo;
};


/* Buffer descriptor for caputre queue/dequeue */
struct buf_info {
	int index;
	unsigned int length;
	char *start;
};

/* Globals for capture and display application objects */
struct app_obj fbdev, capt;


/* Utility function to calculate FPS
 *
 */
static int timeval_subtract(struct timeval *result, struct timeval *x,
		struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 *	nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait, tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/* Utility function which gets the fixed screeninfo from driver and prints it
 */
static int get_fixinfo(int fbdev_fd, struct fb_fix_screeninfo *fixinfo)
{
	int ret;
	ret = ioctl(fbdev_fd, FBIOGET_FSCREENINFO, fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return ret;
	}

	printf("\nFix Screen Info:\n");
	printf("----------------\n");
	printf("Line Length - %d\n", fixinfo->line_length);
	printf("Physical Address = %lx\n", fixinfo->smem_start);
	printf("Buffer Length = %d\n", fixinfo->smem_len);
	return 0;
}
/* Utility function which gets the variable screen info from driver and prints\
 * it on console
 */
static int get_varinfo(int fbdev_fd, struct fb_var_screeninfo *varinfo)
{
	int ret;

	ret = ioctl(fbdev_fd, FBIOGET_VSCREENINFO, varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		return ret;
	}
	printf("\nVar Screen Info:\n");
	printf("----------------\n");
	printf("Xres - %d\n", varinfo->xres);
	printf("Yres - %d\n", varinfo->yres);
	printf("Xres Virtual - %d\n", varinfo->xres_virtual);
	printf("Yres Virtual - %d\n", varinfo->yres_virtual);
	printf("nonstd       - %d\n", varinfo->nonstd);
	printf("Bits Per Pixel - %d\n", varinfo->bits_per_pixel);
	printf("blue lenth %d msb %d offset %d\n",
			varinfo->blue.length,
			varinfo->blue.msb_right,
			varinfo->blue.offset);
	printf("red lenth %d msb %d offset %d\n",
			varinfo->red.length,
			varinfo->red.msb_right,
			varinfo->red.offset);
	printf("green lenth %d msb %d offset %d\n",
			varinfo->green.length,
			varinfo->green.msb_right,
			varinfo->green.offset);
	printf("trans lenth %d msb %d offset %d\n",
			varinfo->transp.length,
			varinfo->transp.msb_right,
			varinfo->transp.offset);
	return 0;

}
/* Utility function for printing format */
static void printFormat(char *string, struct v4l2_format *fmt)
{
	printf("=============================================================\n");
	printf("%s Format:\n", string);
	printf("=============================================================\n");
	printf("fmt.type\t\t = %d\n", fmt->type);
	printf("fmt.width\t\t = %d\n", fmt->fmt.pix.width);
	printf("fmt.height\t\t = %d\n", fmt->fmt.pix.height);
	printf("fmt.pixelformat\t = %d\n", fmt->fmt.pix.pixelformat);
	printf("fmt.bytesperline\t = %d\n", fmt->fmt.pix.bytesperline);
	printf("fmt.sizeimage\t = %d\n", fmt->fmt.pix.sizeimage);
	printf("=============================================================\n");
}

/* Open and query dv preset for capture driver*/
static int initCapture(void)
{
	int mode = O_RDWR;

	/* Open capture driver */
	capt.fd = open((const char *)CAPTURE_DEVICE, mode);
	if (capt.fd == -1) {
		printf("failed to open capture device\n");
		return -1;
	}
	/* Query for capabilities */
	if (ioctl(capt.fd, VIDIOC_QUERYCAP, &capt.cap)) {
		printf("Query capability failed\n");
		exit(2);
	} else {
		printf("Driver Name: %s\n", capt.cap.driver);
		printf("Driver bus info: %s\n", capt.cap.bus_info);
		if (capt.cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
			printf("Driver is capable of doing capture\n");
		if (capt.cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
			printf("Driver is capabled of scaling and cropping\n");

	}
	system("echo 0 > /sys/devices/platform/vpss/display0/enabled");
	/* Query the preset. Set it to invalid and query for it */
	capt.dv_preset.preset = 0x0;
	if (ioctl(capt.fd, VIDIOC_QUERY_DV_PRESET, &capt.dv_preset)) {
		printf("Querying DV Preset failed\n");
		exit(2);
	}
	switch (capt.dv_preset.preset) {
	case V4L2_DV_720P60:
		printf("%s:\n Mode set is 720P60\n", APP_NAME);
		system ("echo 720p-60 > /sys/devices/platform/vpss/display0/mode");
		break;
	case V4L2_DV_1080I60:
		printf("%s:\n Mode set is 1080I60\n", APP_NAME);
		printf("1080I is not supported for RGB capture\n");
		return -1;
		break;
	case V4L2_DV_1080P60:
		printf("%s:\n Mode set is 1080P60\n", APP_NAME);
		system ("echo 1080p-60 > /sys/devices/platform/vpss/display0/mode");
		break;
	case V4L2_DV_1080P30:
		printf("%s:\n Mode set is 1080P30\n", APP_NAME);
		system ("echo 1080p-30 > /sys/devices/platform/vpss/display0/mode");
		break;
	default:
		printf("%s:\n Mode set is %d\n", APP_NAME, capt.dv_preset.preset);
	}
	if (ioctl(capt.fd, VIDIOC_S_DV_PRESET, &capt.dv_preset)) {
		printf("Setting DV Preset failed\n");
		exit(2);
	}
	system("echo 1 > /sys/devices/platform/vpss/display0/enabled");
	return 0;
}

static int initFbdev(void)
{
	int mode = O_RDWR;

	/* Open display driver */
	fbdev.fd = open((const char *)FBDEV_DEVICE, mode);
	if (fbdev.fd == -1) {
		printf("failed to open fbdev device\n");
		return -1;
	}
	return 0;
}

static int setupCapture(void)
{
	int ret;

	/* Get current format */
	capt.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(capt.fd, VIDIOC_G_FMT, &capt.fmt);
	if (ret < 0) {
		printf("Set Format failed\n");
		return -1;
	}
	/* Set format according to mode detected */
	if (capt.dv_preset.preset == V4L2_DV_720P60) {
		capt.fmt.fmt.pix.width = 1280;
		capt.fmt.fmt.pix.height = 720;
	} else if (capt.dv_preset.preset == V4L2_DV_1080P60) {
		capt.fmt.fmt.pix.width = 1920;
		capt.fmt.fmt.pix.height = 1080;
	}  else {
		capt.fmt.fmt.pix.width = 1920;
		capt.fmt.fmt.pix.height = 1080;
	}
	capt.fmt.fmt.pix.bytesperline = capt.fmt.fmt.pix.width * 3;
	capt.fmt.fmt.pix.sizeimage = capt.fmt.fmt.pix.bytesperline *
		capt.fmt.fmt.pix.height;
	capt.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	ret = ioctl(capt.fd, VIDIOC_S_FMT, &capt.fmt);
	if (ret < 0) {
		printf("Set Format failed\n");
		return -1;
	}
	/* Get format again and print it on console */
	capt.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(capt.fd, VIDIOC_G_FMT, &capt.fmt);
	if (ret < 0) {
		printf("Set Format failed\n");
		return -1;
	}
	printFormat("Capture", &capt.fmt);

	/* Request buffers. We are operating in userPtr mode */
	capt.reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capt.reqbuf.count = MAX_BUFFER;
	capt.reqbuf.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(capt.fd, VIDIOC_REQBUFS, &capt.reqbuf);
	if (ret < 0) {
		printf("Could not allocate the buffers\n");
		return -1;
	}
	return 0;
}

/* Setup display driver */
static int setupFbdevAndBuffers(void)
{
	int ret, i;

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	if (get_fixinfo(fbdev.fd, &fbdev.fixinfo))
		return -1;

	/* Get variable screen information. Variable screen information
	 * gives informtion like size of the image, bites per pixel,
	 * virtual size of the image etc. */
	if (get_varinfo(fbdev.fd, &fbdev.varinfo))
		return -1;
	/*store the original information*/
	memcpy(&fbdev.org_varinfo, &fbdev.varinfo, sizeof(fbdev.varinfo));

	/*
	 * Set the resolution which read before again to prove the
	 * FBIOPUT_VSCREENINFO ioctl, except virtual part which is required for
	 * panning.
	 */
	fbdev.varinfo.xres = capt.fmt.fmt.pix.width;
	fbdev.varinfo.yres = capt.fmt.fmt.pix.height;
	fbdev.varinfo.xres_virtual = fbdev.varinfo.xres;
	fbdev.varinfo.yres_virtual = fbdev.varinfo.yres * MAX_BUFFER;
	fbdev.varinfo.bits_per_pixel = 24;
	fbdev.varinfo.transp.offset = 0;
	fbdev.varinfo.transp.length = 0;
	fbdev.varinfo.transp.msb_right = 0;

	ret = ioctl(fbdev.fd, FBIOPUT_VSCREENINFO, &fbdev.varinfo);
	if (ret < 0) {
		perror("Error writing variable information.\n");
		return -1;
	}
	/* Get variable info and print it again after setting it */
	if (get_varinfo(fbdev.fd, &fbdev.varinfo))
		return -1;
	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info.
	 * Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	if (get_fixinfo(fbdev.fd, &fbdev.fixinfo))
		return -1;
	/* Mmap the driver buffers in application space so that application
	 * can write on to them. Driver allocates contiguous memory for
	 * three buffers. These buffers can be displayed one by one. */
	fbdev.buffersize = fbdev.fixinfo.line_length * fbdev.varinfo.yres;
	fbdev.buffer_addr[0] = (unsigned char *)mmap(0,
			fbdev.buffersize *  MAX_BUFFER,
			(PROT_READ | PROT_WRITE),
			MAP_SHARED, fbdev.fd, 0);

	if ((int)fbdev.buffer_addr[0] == -1) {
		printf("MMap Failed.\n");
		return -1;
	}
	/* Store each buffer addresses in the local variable. These buffer
	 * addresses can be used to fill the image. */
	for (i = 1; i < MAX_BUFFER; i++)
		fbdev.buffer_addr[i] = fbdev.buffer_addr[i-1] + fbdev.buffersize;

	memset(fbdev.buffer_addr[0], 0x00, fbdev.buffersize * MAX_BUFFER);

	return 0;
}

static int queueCaptureBuffers(void)
{
	int ret, i;
	for (i = 0; i < MAX_BUFFER; i++) {
		capt.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capt.buf.memory = V4L2_MEMORY_USERPTR;
		capt.buf.index = i;
		capt.buf.m.userptr = (unsigned long)fbdev.buffer_addr[capt.buf.index];
		capt.buf.length = capt.fmt.fmt.pix.sizeimage;
		ret = ioctl(capt.fd, VIDIOC_QBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
	}
	return ret;
}

/*
	Starts Streaming of capture
*/
static int startCapture(void)
{
	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE, ret;
	ret = ioctl(capt.fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		return -1;
	}
	return 0;
}

/*
 * Stop streaming on capture
 */
static int stopCapture(void)
{
	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE, ret;
	ret = ioctl(capt.fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		return -1;
	}
	return 0;
}

/* Application main */
static int app_main(int argc , char *argv[])
{
	int ret;
	int i;
	struct timeval before, after, result;
	FILE *filePtr;

	filePtr = fopen("captdump.yuv", "wb");
	if (filePtr == NULL) {
		printf("Cannot Open Output file:captdump.yuv \n");
		return 0;
	}
	system ("echo 1:hdmi > /sys/devices/platform/vpss/graphics0/nodes");
	/* Open the capture driver. Query the resolution from the capture driver
	 */
	ret = initCapture();
	if (ret < 0) {
		printf("Error in opening capture device for channel 0\n");
		return ret;
	}
	/* Open the Display driver. */
	ret = initFbdev();
	if (ret < 0) {
		printf("Error in opening display device for channel 0\n");
		return ret;
	}
restart:
	/* Setup the capture driver Step includes
	 * Set the format according to the resolution queried.
	 * request for buffer descriptors for userpointer buffers
	 */
	ret = setupCapture();
	if (ret < 0) {
		printf("Error in setting up of capture\n");
		return ret;
	}

	/* Setup the display driver Step includes
	 * Set the format according to the resolution queried by capture.
	 * request for buffer descriptors for userpointer buffers
	 */
	ret = setupFbdevAndBuffers();
	if (ret < 0) {
		printf("Error in setting up of Display\n");
		return ret;
	}
	ret = queueCaptureBuffers();
	if (ret < 0) {
		printf("Error in queuing capture buffers\n");
		return ret;
	}
	/* Start Captureing */
	ret = startCapture();
	if (ret < 0) {
		printf("Error in starting capture\n");
		return ret;
	}
	/* Get time of day for calculating FPS */
	gettimeofday(&before, NULL);
	/* Start capture and display loop */
	for (i = 0; i < MAXLOOPCOUNT; i++) {
		/*
		   Get capture buffer using DQBUF ioctl
		 */
		ret = ioctl(capt.fd, VIDIOC_DQBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_DQBUF\n");
			return -1;
		}
		if ((i % 1000) == 0)
			printf("C: %d\n TS: %d index %d\n", i,
					(unsigned int)capt.buf.timestamp.tv_usec,
					capt.buf.index);
		/* Pan the display to the next buffer captured.
		 * Application should provide y-offset in terms of number of
		 * lines to have panning effect. To entirely change to next
		 * buffer, yoffset needs to be changed to yres field. */

		fbdev.varinfo.yoffset = (capt.buf.index * 1080) % MAX_BUFFER;
		ret = ioctl(fbdev.fd, FBIOPAN_DISPLAY, &fbdev.varinfo);
		if (ret < 0) {
			perror("Cannot pan display.\n");
		}
		/* Wait for buffer to complete display */
		ret = ioctl(fbdev.fd, FBIO_WAITFORVSYNC, 0);
		if (ret < 0) {
			perror("wait vsync failed.\n");
			break;
		}

		/* HDVPSS VIP capture is having bug that it locks up some time
		 * under heavy bandwidth conditions. DQBUF will return error in
		 * case hardware lock up. Applicatin needs to check for the
		 * lockup and in case of lockup, restart the driver */
		if (capt.buf.flags & V4L2_BUF_FLAG_ERROR) {
			ret = ioctl(capt.fd, TICAPT_CHECK_OVERFLOW, &capt.over_flow);
			if (ret < 0) {
				perror("TICAPT_CHECK_OVERFLOW\n");
				return -1;
			} else {
				if (capt.over_flow.porta_overflow) {
					printf("Port a overflowed\n\n\n\n\n\n\n");
					stopCapture();
					goto restart;

				}
				if (capt.over_flow.portb_overflow) {
					printf("Port b overflowed\n\n\n\n\n\n\n");
					stopCapture();
					goto restart;
				}
			}
		}

		/* Queue the buffer back to display */
		capt.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capt.buf.memory = V4L2_MEMORY_USERPTR;
		capt.buf.m.userptr = (unsigned long)fbdev.buffer_addr[capt.buf.index];
		capt.buf.length = fbdev.buffersize;
		ret = ioctl(capt.fd, VIDIOC_QBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
	}
	/* Get time of day after to do FPS calculation */
	fwrite(fbdev.buffer_addr[0], 1, capt.fmt.fmt.pix.sizeimage, filePtr);
	gettimeofday(&after, NULL);
	timeval_subtract(&result, &after, &before);
	printf("Result Time:\t%ld %ld\n", result.tv_sec, result.tv_usec);
	printf("Calculated Frame Rate:\t%ld Fps\n\n", MAXLOOPCOUNT/result.tv_sec);


	ret = ioctl(fbdev.fd, FBIOPUT_VSCREENINFO, &fbdev.org_varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
	}
	close(capt.fd);
	close(fbdev.fd);
	return ret;

}


int main(int argc , char *argv[])
{
	return app_main(argc , argv);
}
