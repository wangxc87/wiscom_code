/*
 * saLoopBack.c
 *
 * Copyright (C) 2011 TI
 * Author: Hardik Shah <hardik.shah@ti.com>
 *
 * This is a sample for loopback application. This application captures through
 * V4L2 capture driver on TVP7002 input and displays the captured content
 * on the V4L2 display driver. Application uses userpointer mechanism for
 * both capture and display drivers. Buffers for the userpointer are taken from
 * framebuffer driver (fbdev).
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
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
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/ti81xxfb.h>
#include <linux/ti81xxhdmi.h>
#include <linux/ti81xxvin.h>

/* Number of buffers required for application. Less that this may cause
 * considerable frame drops
 */
#define MAX_BUFFER			8


/* device node to be used for capture */
#define CAPTURE_DEVICE		"/dev/video0"
#define CAPTURE_NAME		"Capture"
/* device node to be used for display */
#define DISPLAY_DEVICE		"/dev/video1"
#define DISPLAY_NAME		"Display"
/* number of frames to be captured and displayed
 * Increase this for long runs
 */
#define MAXLOOPCOUNT		500

/* Pixel format for capture and display. Capture supports
 * V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV16, V4L2_PIX_FMT_RGB24 but display
 * supports only V4L2_PIX_FMT_YUYV. So this has to be V4L2_PIX_FMT_YUYV
 */
#define DEF_PIX_FMT		V4L2_PIX_FMT_YUYV

/* Application name */
#define APP_NAME		"saLoopBackScale"

/* Define below #def for debug information enable */
#undef SALOOPBACKSCALE_DEBUG

#define SCALED_HEIGHT		480
#define SCALED_WIDTH		720

/* Structure for storing buffer information */
struct buf_info {
	int index;
	unsigned int length;
	char *start;
};

/* Structure for storing application data like file pointers, format etc. */
struct app_obj {
	int fd;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_dv_preset dv_preset;
	struct v4l2_requestbuffers reqbuf;
	int numbuffers;
	struct v4l2_buffer buf;
	struct ti81xxvin_overflow_status over_flow;
	 struct v4l2_crop crop;
	 struct v4l2_format fmt_win;
};

/* Globals for capture and display application objects */
struct app_obj disp, capt;

/* File pointer for fbdev. fbdev is only used to get buffers
 */
static int fbdev_fd;

/* Pointers for storing buffer addresses */
unsigned char *buffer_addr[MAX_BUFFER];

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
		system ("echo 1080p-30 > /sys/devices/platform/vpss/display0/mode");
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
/* Open display driver and set format according to resolution on capture */
static int initDisplay(void)
{
	int mode = O_RDWR;
	struct v4l2_capability cap;

	/* Open display driver */
	disp.fd = open((const char *)DISPLAY_DEVICE, mode);
	if (disp.fd == -1) {
		printf("failed to open display device\n");
		return -1;
	}
	/* Query driver capability */
	if (ioctl(disp.fd, VIDIOC_QUERYCAP, &disp.cap)) {
		printf("Query capability failed for display\n");
		exit(2);
	}  else {
		printf("Driver Name: %s\n", cap.driver);
		printf("Driver bus info: %s\n", cap.bus_info);
		if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
			printf("Driver is capable of doing capture\n");
		if (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
			printf("Driver is capabled of scaling and cropping\n");

	}
	return 0;
}
/* Setup capture driver */
int setupCapture(void)
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

	capt.crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capt.crop.c.left = 0;
	capt.crop.c.top = 0;
	capt.crop.c.width = capt.fmt.fmt.pix.width;
	capt.crop.c.height = capt.fmt.fmt.pix.height;

	if (ioctl(capt.fd, VIDIOC_S_CROP, &capt.crop)) {
		printf("Setting format failed\n");
		exit(2);
	}

	capt.fmt_win.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	capt.fmt_win.fmt.win.w.left = 0;
	capt.fmt_win.fmt.win.w.top = 0;
	capt.fmt_win.fmt.win.w.width = 720;
	capt.fmt_win.fmt.win.w.height = 480;

	if (ioctl(capt.fd, VIDIOC_S_FMT, &capt.fmt_win)) {
		printf("Setting window failed\n");
		exit(2);
	}

	capt.fmt.fmt.pix.bytesperline = capt.fmt.fmt.pix.width * 2;
	capt.fmt.fmt.pix.sizeimage = capt.fmt.fmt.pix.bytesperline *
		capt.fmt.fmt.pix.height;
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
int setupDisplay(void)
{
	int ret;

	/* Get format */
	disp.fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(disp.fd, VIDIOC_G_FMT, &disp.fmt);
	if (ret < 0) {
		printf("Get Format failed\n");
		return -1;
	}
	/* Set format according to display mode */
	if (capt.dv_preset.preset == V4L2_DV_720P60) {
		disp.fmt.fmt.pix.width = 1280;
		disp.fmt.fmt.pix.height = 720;
	} else if (capt.dv_preset.preset == V4L2_DV_1080P60) {
		disp.fmt.fmt.pix.width = 1920;
		disp.fmt.fmt.pix.height = 1080;
	}  else {
		disp.fmt.fmt.pix.width = 1920;
		disp.fmt.fmt.pix.height = 1080;
	}
	disp.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	disp.fmt.fmt.pix.bytesperline = disp.fmt.fmt.pix.width * 2;
	disp.fmt.fmt.pix.sizeimage = disp.fmt.fmt.pix.bytesperline *
		disp.fmt.fmt.pix.height;
	ret = ioctl(disp.fd, VIDIOC_S_FMT, &disp.fmt);
	if (ret < 0) {
		printf("Set Format failed\n");
		return -1;
	}
	/* Get format again and display it on console */
	disp.fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(disp.fd, VIDIOC_G_FMT, &disp.fmt);
	if (ret < 0) {
		printf("Get Format failed for display\n");
		return -1;
	}
	printFormat("Display", &disp.fmt);
	/* Requests buffers, we are operating in userPtr mode */
	disp.reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	disp.reqbuf.count = MAX_BUFFER;
	disp.reqbuf.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(disp.fd, VIDIOC_REQBUFS, &disp.reqbuf);
	if (ret < 0) {
		perror("Could not allocate the buffers\n");
		return -1;
	}
	return 0;
}
/* Stop capture */
static int stopCapture(void)
{

	/* Stop capture */
	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE, ret;
	ret = ioctl(capt.fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMOFF\n");
		return -1;
	}
	return 0;
}
/* Stop display */
static int stopDisplay(void)
{
	int a = V4L2_BUF_TYPE_VIDEO_OUTPUT, ret;
	ret = ioctl(disp.fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMOFF\n");
		return -1;
	}
	return 0;
}
/* Start display */
static int startDisplay(void)
{
	int a = V4L2_BUF_TYPE_VIDEO_OUTPUT, ret;
	ret = ioctl(disp.fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		return -1;
	}
	return 0;
}
/* Start capture */
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
/* Start display */
static int deInitCapture(void)
{
	if (close(capt.fd))
		return -1;
	return 0;
}

/* Close display */
static int deInitDisplay(void)
{
	if (close(disp.fd))
		return -1;
	return 0;
}

/* Get fix screeninfo on fbdev */
static int get_fixinfo(struct fb_fix_screeninfo *fixinfo)
{
	int ret;
	ret = ioctl(fbdev_fd, FBIOGET_FSCREENINFO, fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return ret;
	}
#ifdef SALOOPBACK_DEBUG
	printf("\nFix Screen Info:\n");
	printf("----------------\n");
	printf("Line Length - %d\n", fixinfo->line_length);
	printf("Physical Address = %lx\n", fixinfo->smem_start);
	printf("Buffer Length = %d\n", fixinfo->smem_len);
#endif
	return 0;
}
/* Get variable screeninfo on FBDEV */
static int get_varinfo(struct fb_var_screeninfo *varinfo)
{
	int ret;

	ret = ioctl(fbdev_fd, FBIOGET_VSCREENINFO, varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		return ret;
	}
#ifdef SALOOPBACK_DEBUG
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
#endif
	return 0;

}
/* We derive buffers from fbdev for userpointer operation. We have to setupBuffers
 * fbdev to get enough number of buffers and with enough size
 *
 */
static int setupBuffers(void)
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	int ret;
	int buffersize;
	int i;

	/*Open FB device*/
	fbdev_fd = open("/dev/fb0", O_RDWR);
	if (fbdev_fd <= 0) {
		perror("Could not open fb device\n");
		return -1;
	}
	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	if (get_fixinfo(&fixinfo)) {
		perror("Could not get fixed screen info\n");
		return -1;
	}
	/* Get variable screen information. Variable screen information
	 * gives informtion like size of the image, bites per pixel,
	 * virtual size of the image etc. */
	if (get_varinfo(&varinfo)) {
		perror("getting variable screen info failed\n");
		return -1;
	}
	/*store the original information*/
	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

	/*
	 * Set the resolution which read before again to prove the
	 * FBIOPUT_VSCREENINFO ioctl, except virtual part which is required for
	 * panning.
	 */
	varinfo.xres = disp.fmt.fmt.pix.width;
	varinfo.yres = disp.fmt.fmt.pix.height;
	varinfo.xres_virtual = varinfo.xres;
	varinfo.yres_virtual = varinfo.yres * MAX_BUFFER;
	varinfo.bits_per_pixel = 16;
	varinfo.transp.offset = 0;
	varinfo.transp.length = 0;
	varinfo.transp.msb_right = 0;
	varinfo.red.offset = 16;
	varinfo.red.length = 5;
	varinfo.blue.offset = 0;
	varinfo.blue.length = 5;
	varinfo.green.offset = 8;
	varinfo.green.length = 6;

	ret = ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error writing variable information.\n");
		return -1;
	}
	if (get_varinfo(&varinfo)) {
		perror("Error getting variable screen information\n");
		return -1;
	}
	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(fbdev_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return -1;
	}

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	if (get_fixinfo(&fixinfo)) {
		perror("Getting fixed screen info failed\n");
		return -1;
	}
	/* Mmap the driver buffers in application space so that application
	 * can write on to them. Driver allocates contiguous memory for
	 * three buffers. These buffers can be displayed one by one. */
	buffersize = fixinfo.line_length * varinfo.yres;
	buffer_addr[0] = (unsigned char *)mmap(0,
			buffersize *  MAX_BUFFER,
			(PROT_READ | PROT_WRITE),
			MAP_SHARED, fbdev_fd, 0);
	for (i = 1; i < MAX_BUFFER; i++)
		buffer_addr[i] = buffer_addr[i-1] + buffersize;

	memset(buffer_addr[0], 0x80, (buffersize * MAX_BUFFER));
	return 0;
}
/* Prime capture buffers */
static int queueCaptureBuffers(void)
{
	int ret, i;
	for (i = 0; i < (MAX_BUFFER / 2); i++) {
		capt.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capt.buf.memory = V4L2_MEMORY_USERPTR;
		capt.buf.index = i;
		capt.buf.m.userptr = (unsigned long)buffer_addr[capt.buf.index];
		capt.buf.length = capt.fmt.fmt.pix.sizeimage;
		ret = ioctl(capt.fd, VIDIOC_QBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
	}
	return ret;
}
/* Prime display buffers */
static int queueDisplayBuffers(void)
{
	int ret, i;
	for (i = (MAX_BUFFER / 2); i < MAX_BUFFER; i++) {
		disp.buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		disp.buf.memory = V4L2_MEMORY_USERPTR;
		disp.buf.index = i - (MAX_BUFFER / 2);
		disp.buf.m.userptr = (unsigned long)buffer_addr[i];
		disp.buf.length = capt.fmt.fmt.pix.sizeimage;
		ret = ioctl(disp.fd, VIDIOC_QBUF, &disp.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
	}
	return 0;
}
/* This is a utility function to calculate time  between start and stop. This
 * is used to calculate FPS
 */
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
/* Main function */
int main(int argc, char *argv[])
{
	int i = 0, ret = 0;
	struct v4l2_buffer temp_buf;
	struct timeval before, after, result;

	/* Divert fbdev to dvo2 so that it does  not do blending with display*/
	system ("echo 1:dvo2 > /sys/devices/platform/vpss/graphics0/nodes");
	/* Open the capture driver. Query the resolution from the capture driver
	 */
	ret = initCapture();
	if (ret < 0) {
		printf("Error in opening capture device for channel 0\n");
		return ret;
	}
	/* Open the Display driver. */
	ret = initDisplay();
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
	ret = setupDisplay();
	if (ret < 0) {
		printf("Error in setting up of Display\n");
		return ret;
	}
	/* As application works on userPointer, Buffers for both capture and
	 * display driver are allocated using the fbdev, buffers. Below functionality
	 * setups the fbdev to mmap the buffers and later capture and display
	 * will use those buffers
	 */
	ret = setupBuffers();
	if (ret < 0) {
		printf("Error in setting up of Buffers\n");
		return ret;
	}
	/* Total 8 buffers are allocated using fbdev, 4 buffers are primed to
	 * capture driver.
	 */
	ret = queueCaptureBuffers();
	if (ret < 0) {
		printf("Error in queuing capture buffers\n");
		return ret;
	}
	/* Total 8 buffers are allocated using fbdev, 4 buffers are primed to
	 * display driver.
	 */
	ret = queueDisplayBuffers();
	if (ret < 0) {
		printf("Error in queuing display buffers\n");
		return ret;
	}
	/* Start display driver always first. This is because display driver
	 * takes 2 frames to come out of start. If capture is started first
	 * frame drops will be seen, since capture will already complete two
	 * frame by time display starts
	 */
	ret = startDisplay();
	if (ret < 0) {
		printf("Error starring capture \n");
		return ret;
	}
	/* Start capture driver after display driver */
	ret = startCapture();
	if (ret < 0) {
		printf("Error starring capture \n");
		return ret;
	}
	/* Get time of day to calculate FPS */
	gettimeofday(&before, NULL);
	/* Start the steady state loop. Following steps are followed
	 * 1 dequeue buffer from capture
	 * 2 dequeue buffer from display
	 * 3 exchange capture and display buffer pointers
	 * 4 queue dislay buffer pointer to capture
	 * 5 queue capture buffer pointer to display
	 */
	for (i = 0; i < MAXLOOPCOUNT; i++) {
		/* Dq capture buffer */
		ret = ioctl(capt.fd, VIDIOC_DQBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_DQBUF\n");
			return -1;
		}
		/* Because of IP bugs, capture hardware gets locked up once in
		 * a while. In that case DQbuf will return  V4L2_BUF_FLAG_ERROR
		 * in flags.
		 */
		if (capt.buf.flags & V4L2_BUF_FLAG_ERROR) {
			/* If DQbuf returned error check for the hardware lockup
			 */
			ret = ioctl(capt.fd, TICAPT_CHECK_OVERFLOW, &capt.over_flow);
			if (ret < 0) {
				perror("TICAPT_CHECK_OVERFLOW\n");
				return -1;
			} else {
				/* If hardware locked up, restart display and
				 * capture driver
				 */
				if (capt.over_flow.porta_overflow) {
					printf("Port a overflowed\n\n\n\n\n\n\n");
					stopCapture();
					stopDisplay();
					goto restart;

				}
				if (capt.over_flow.portb_overflow) {
					printf("Port b overflowed\n\n\n\n\n\n\n");
					stopCapture();
					stopDisplay();
					goto restart;
				}
			}
		}
		/* DQ display buffer */
		ret = ioctl(disp.fd, VIDIOC_DQBUF, &disp.buf);
		if (ret < 0) {
			perror("VIDIOC_DQBUF Display\n");
			return -1;
		}
		/* Exchange display and capture buffer pointers */
		temp_buf.m.userptr = capt.buf.m.userptr;
		capt.buf.m.userptr = disp.buf.m.userptr;
		disp.buf.m.userptr = temp_buf.m.userptr;
		/* Queue the capture buffer with updated address */
		ret = ioctl(capt.fd, VIDIOC_QBUF, &capt.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			return -1;
		}
		disp.buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		disp.buf.memory = V4L2_MEMORY_USERPTR;
		disp.buf.length = capt.fmt.fmt.pix.sizeimage;
		/* Queue the display buffer back with updated address */
		ret = ioctl(disp.fd, VIDIOC_QBUF, &disp.buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF Display\n");
			return -1;
		}
		if ((i % 1000) == 0)
			printf("Count=%d\n", i);
	}
	/* Get end time to calculate FPS */
	gettimeofday(&after, NULL);
	/* Calculate FPS */
	calc_result_time(&result, &after, &before);
	printf("Frame rate = %lu\n", MAXLOOPCOUNT/result.tv_sec);
	/* Stop capture driver */
	ret = stopCapture();
	if (ret < 0) {
		printf("Error in stopping capture\n");
		return ret;
	}
	/* Stop display driver */
	ret = stopDisplay();
	if (ret < 0) {
		printf("Error in stopping display\n");
		return ret;
	}
	/* Deinit capture driver */
	ret = deInitCapture();
	if (ret < 0) {
		printf("Error in capture deInit\n");
		return ret;
	}
	/* Deinit display driver */
	ret = deInitDisplay();
	if (ret < 0) {
		printf("Error in display deInit\n");
		return ret;
	}
	close(fbdev_fd);
	system("echo 0 > /sys/devices/platform/vpss/display0/enabled");
	system ("echo 1080p-60 > /sys/devices/platform/vpss/display0/mode");
	system("echo 0 > /sys/devices/platform/vpss/graphics0/enabled");
	system("echo 1 > /sys/devices/platform/vpss/display0/enabled");
	system("echo 1:hdmi > /sys/devices/platform/vpss/graphics0/nodes");
	return 0;

}
