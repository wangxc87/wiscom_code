/*
 *
 * Fbdev HDMI Test Applicaton for TI81XX EVM
 *
 * Copyright (C) 2010 TI
 * Author: Yihe Hu <yihehu@ti.com>
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
#include <linux/ti81xxhdmi.h>


#define MAX_BUFFER 2
#define MAXLOOPCOUNT	1000
#define X_RES		1280
#define Y_RES		720
/*
 * Default fbdev device node
 */
static char display_dev_name[30] = {"/dev/fb0"};

static unsigned int rgb888[8] = {
        (0xFF << 16) | (0xFF << 8) | (0xFF),
        (0x00 << 16) | (0x00 << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0x00),
        (0x00 << 16) | (0xFF << 8) | (0x00),
        (0x00 << 16) | (0x00 << 8) | (0xFF),
        (0xFF << 16) | (0xFF << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0xFF),
        (0x00 << 16) | (0xFF << 8) | (0xFF),
};

static void fill_color_bar(unsigned char *addr,
		 unsigned int width,
		 unsigned int height)
{

	int i,j,k;
	unsigned int *start = (unsigned int*)addr;

	if (width & 0xF)
                width+= 16 - (width & 0xF);

	for(i = 0 ; i < 8 ; i++) {
		for(j = 0 ; j < (height / 8) ; j++) {
			for(k = 0 ; k < (width/4); k++)
				start[k] = rgb888[i];
			start += (width/4);
		}
	}

}


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

static int get_fixinfo(int display_fd, struct fb_fix_screeninfo *fixinfo)
{
	int ret;
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return ret;
	}

	printf("\nFix Screen Info:\n");
	printf("----------------\n");
	printf("Line Length - %d\n", fixinfo->line_length);
	printf("Physical Address = %lx\n",fixinfo->smem_start);
	printf("Buffer Length = %d\n",fixinfo->smem_len);
	return 0;
}
static int get_varinfo(int display_fd, struct fb_var_screeninfo *varinfo)
{
	int ret;

	ret = ioctl(display_fd, FBIOGET_VSCREENINFO, varinfo);
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
	printf("nonstd       - %d\n",varinfo->nonstd);
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

static int hdmi_change_mode(void)
{
	/* Disable Venc and HDMI to change mode */
	if(system("echo 0 > /sys/devices/platform/vpss/display0/enabled")) {
		perror("Failed to stop Venc and HDMI\n");
		return -1;
	}
	/* Change the mode */
	if(system("echo 720p-60 > /sys/devices/platform/vpss/display0/mode")) {
		perror("Failed to change mode on Venc and HDMI\n");
		return -1;
	}
	/* Enable Venc and HDMI */
	if(system("echo 1 > /sys/devices/platform/vpss/display0/enabled")) {
		perror("Failed to start Venc and HDMI\n");
		return -1;
	}
	printf("Mode Changed to 720P60 from default mode of 1080P60\n");
	printf("press any key and hit return to continue...\n");
	getchar();
	return 0;
}

int app_main(int argc , char *argv[])
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	struct timeval before, after, result;
	int display_fd;
	int ret;
	int buffersize;
	int i;
	unsigned char *buffer_addr[MAX_BUFFER];

	ret = hdmi_change_mode();
	if (ret) {
		perror("failed to set HDMI mode\n");
		return -1;
	}
	/*Open FB device*/
	display_fd = open(display_dev_name, O_RDWR);
	if (display_fd <= 0) {
		perror("Could not open fb device\n");
		return -1;
	}
	/* Get fix screen information. Fix screen information gives
	* fix information like panning step for horizontal and vertical
	* direction, line length, memory mapped start address and length etc.
	*/
	if (get_fixinfo(display_fd, &fixinfo))
		goto exit1;

	/* Get variable screen information. Variable screen information
	* gives informtion like size of the image, bites per pixel,
	* virtual size of the image etc. */
	if (get_varinfo(display_fd, &varinfo))
		goto exit1;
	/*store the original information*/
	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));


	/*
	 * Set the resolution which read before again to prove the
	 * FBIOPUT_VSCREENINFO ioctl, except virtual part which is required for
	 * panning.
	 */
	varinfo.xres = X_RES;
	varinfo.yres = Y_RES;
	varinfo.xres_virtual = varinfo.xres;
	varinfo.yres_virtual = varinfo.yres * MAX_BUFFER;

	ret = ioctl(display_fd, FBIOPUT_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error writing variable information.\n");
		goto exit1;
	}

	if (get_varinfo(display_fd, &varinfo))
		goto exit1;
	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit2;
	}

	/* Mmap the driver buffers in application space so that application
	 * can write on to them. Driver allocates contiguous memory for
	 * three buffers. These buffers can be displayed one by one. */
	buffersize = fixinfo.line_length * varinfo.yres;
	buffer_addr[0] = (unsigned char *)mmap(0,
					   buffersize *  MAX_BUFFER,
					   (PROT_READ | PROT_WRITE),
					   MAP_SHARED, display_fd, 0);

	if ((int)buffer_addr[0] == -1) {
		printf("MMap Failed.\n");
		goto exit2;
	}
	/* Store each buffer addresses in the local variable. These buffer
	 * addresses can be used to fill the image. */
	for (i = 1; i < MAX_BUFFER; i++)
		buffer_addr[i] = buffer_addr[i-1] + buffersize;
	/*fill the buffer with color bar data*/
	for (i = 0; i < MAX_BUFFER; i++)
		fill_color_bar(buffer_addr[i],
			    fixinfo.line_length,
			    varinfo.yres);
	gettimeofday(&before, NULL);

	/*PAN the display*/
	for (i = 0; i < MAXLOOPCOUNT; i++) {
		ret = ioctl(display_fd, FBIO_WAITFORVSYNC, 0);
		if (ret < 0) {
			perror("wait vsync failed.\n");
			break;
		}

		ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
		if (ret < 0) {
			perror("ger varinfo failed.\n");
			break;
		}

		/* Pan the display to the next line. As all the buffers are
		 * filled with the same color bar, moving to next line gives
		 * effect of moving color color bar.
		 * Application should provide y-offset in terms of number of
		 * lines to have panning effect. To entirely change to next
		 * buffer, yoffset needs to be changed to yres field. */
		varinfo.yoffset = i % varinfo.yres;

		/* Change the buffer address */
		ret = ioctl(display_fd, FBIOPAN_DISPLAY, &varinfo);
		if (ret < 0) {
			perror("Cannot pan display.\n");
			break;
		}

	}
	gettimeofday(&after, NULL);
	printf("\nThis time for displaying %d frames\n", MAXLOOPCOUNT);
	printf("Before Time %lu %lu\n",before.tv_sec, before.tv_usec);
	printf("After Time %lu %lu\n",after.tv_sec, after.tv_usec);

	timeval_subtract(&result, &after, &before);
	printf("Result Time:\t%ld %ld\n",result.tv_sec, result.tv_usec);
	printf("Calculated Frame Rate:\t%ld Fps\n\n", MAXLOOPCOUNT/result.tv_sec);

	/*unmap the buffer*/
	munmap(buffer_addr[0], buffersize*MAX_BUFFER);


exit2:
	ret = ioctl(display_fd, FBIOPUT_VSCREENINFO, &org_varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
	}
exit1:
	close(display_fd);
	return ret;
}


int main(int argc , char *argv[])
{
	return app_main(argc , argv);
}
