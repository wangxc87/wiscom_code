/*
 * saFbdevScalingDisplay.c
 *
 * Scaling Panning application on top of Fbdev
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 * Author: Yihe Hu <yihehu@ti.com>
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <linux/fb.h>
#include <linux/ti81xxfb.h>

/*
 * Macros Definations
 */
#define MAXLOOPCOUNT		1000
#define MAX_BUFFER		2

/*
 * Default fbdev device node
 */
static char display_dev_name[30] = {"/dev/fb0"};
/*
 * Color bar
 */
 static int rgb[8] = {
        (0xFF << 16) | (0xFF << 8) | (0xFF),
        (0x00 << 16) | (0x00 << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0x00),
        (0x00 << 16) | (0xFF << 8) | (0x00),
        (0x00 << 16) | (0x00 << 8) | (0xFF),
        (0xFF << 16) | (0xFF << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0xFF),
        (0x00 << 16) | (0xFF << 8) | (0xFF),
 };

void fill_color_bar(unsigned char *addr, int width, int height)
{
	int i,j,k;
	unsigned int *start = (unsigned int*)addr;
	if (width & 0xF)
               width  += 16 - (width & 0xF);

	for(i = 0 ; i < 8 ; i++) {
		for(j = 0 ; j < (height / 8) ; j++) {
			for(k = 0 ; k < (width/4); k++)
				start[k] = rgb[i];
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

int get_fixinfo(int display_fd, struct fb_fix_screeninfo *fixinfo)
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
int get_varinfo(int display_fd, struct fb_var_screeninfo *varinfo)
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
int get_regparams(int display_fd, struct ti81xxfb_region_params *regp)
{
	int ret;
	ret = ioctl(display_fd, TIFB_GET_PARAMS, regp);
	if (ret < 0) {
		perror("Can not get region params\n");
		return ret;
	}
	printf("\nReg Params Info:  \n");
	printf("----------------\n");
	printf("region %d postion %d x %d prioirty %d\n",
		regp->ridx,
		regp->pos_x,
		regp->pos_y,
		regp->priority);
	printf("first %d last %d\n",
		regp->firstregion,
		regp->lastregion);
	printf("sc en %d sten en %d\n",
		regp->scalaren,
		regp->stencilingen);
	printf("tran en %d, type %d, key %d\n",
		regp->transen,
		regp->transtype,
		regp->transcolor);
	printf("blend %d alpha %d\n"
		,regp->blendtype,
		regp->blendalpha);
	printf("bb en %d alpha %d\n",
		regp->bben,
		regp->bbalpha);
	return 0;
}


int get_sccoeff(int display_fd, struct ti81xxfb_scparams *scp)
{
	int ret;
	ret = ioctl(display_fd, TIFB_GET_SCINFO, scp);
	if (ret < 0) {
		perror("Can not get coeff.\n");
		return ret;
	}
	printf("\nCoeff Info:  \n");
	printf("----------------\n");
	printf("input %dx%d to %dx%d coeff %x\n",
		scp->inwidth,
		scp->inheight,
		scp->outwidth,
		scp->outheight,
		(__u32)scp->coeff);
	return 0;
}
static int app_main(void)
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	struct ti81xxfb_region_params  regp, oldregp;
	struct ti81xxfb_scparams  scp, scp1;
	int display_fd;
	struct timeval before, after, result;
	int ret = 0;
	int buffersize;
	int i;
	unsigned char *buffer_addr[MAX_BUFFER];
	int dummy;

	display_fd = open(display_dev_name, O_RDWR);
	if (display_fd <= 0) {
		perror("Could not open device\n");
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

	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

	if (get_regparams(display_fd, &regp))
		goto exit1;

	varinfo.xres = 800;
	varinfo.yres = 600;

	varinfo.xres_virtual = varinfo.xres;
	varinfo.yres_virtual = varinfo.yres * 2;

	/* set the ARGB8888  format*/
	varinfo.red.length = varinfo.green.length
		= varinfo.blue.length = varinfo.transp.length = 8;
	varinfo.red.msb_right = varinfo.blue.msb_right =
		varinfo.green.msb_right =
		varinfo.transp.msb_right = 0;

	varinfo.red.offset = 16;
	varinfo.green.offset = 8;
	varinfo.blue.offset = 0;
	varinfo.transp.offset = 24;
	varinfo.bits_per_pixel = 32;

	ret =ioctl(display_fd, FBIOPUT_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error setting variable information.\n");
		goto exit1;
	}


	if (get_varinfo(display_fd,&varinfo))
		goto exit2;


	if (get_fixinfo(display_fd, &fixinfo))
		goto exit2;


	if (get_regparams(display_fd, &regp))
		goto exit2;


	oldregp = regp;

	/*setup the scaling ratio*/
	scp.inwidth = varinfo.xres;
	scp.inheight = varinfo.yres ;

	scp.outwidth = 1000;
	scp.outheight = 700;
	scp.coeff = NULL;
	ret = ioctl(display_fd, TIFB_SET_SCINFO, &scp);
	if (ret < 0) {
		perror("Can not set coeff.\n");
	}
	get_sccoeff(display_fd, &scp1);
	/*enable the scaling*/
	regp.scalaren = TI81XXFB_FEATURE_ENABLE;
    	ret = ioctl(display_fd, TIFB_SET_PARAMS, &regp);
	if (ret < 0) {
		printf("failed to set reg params\n");
		goto exit3;
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
		goto exit3;
	}
	/* Store each buffer addresses in the local variable. These buffer
	 * addresses can be used to fill the image. */

	for(i = 1 ; i < MAX_BUFFER ; i ++)
		buffer_addr[i] = buffer_addr[i-1] + buffersize;

	/*fill buffer with color bar pattern*/
	for (i = 0; i < MAX_BUFFER; i++) {
		fill_color_bar(buffer_addr[i],
			       fixinfo.line_length,
			       varinfo.yres);
	}

	gettimeofday(&before, NULL);

	/*pan loop*/
	for (i = 0; i < MAXLOOPCOUNT; i++) {

		ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
		if (ret < 0) {
			perror("ger varinfo failed.\n");
			break;
		}

		varinfo.yoffset = i % varinfo.yres;
		/*runtime enable/disable scaling*/
		if ( i % 100 == 0) {
			regp.scalaren ^= TI81XXFB_FEATURE_ENABLE;
		    	ret = ioctl(display_fd, TIFB_SET_PARAMS, &regp);
			if (ret < 0) {
				perror ("can not set params.\n");
				break;
			}
		}
		ret = ioctl(display_fd, FBIOPAN_DISPLAY, &varinfo);
		if (ret < 0) {
			perror("Cannot pan display.\n");
			break;
		}

		ret = ioctl(display_fd, FBIO_WAITFORVSYNC, &dummy);
		if (ret < 0) {
			perror("wait vsync failed.\n");
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

	munmap(buffer_addr[0], buffersize*MAX_BUFFER);


exit3:
	ret = ioctl(display_fd, TIFB_SET_PARAMS, &oldregp);
	if (ret < 0) {
		perror("Error set reg params.\n");
	}

exit2:
	ret = ioctl(display_fd, FBIOPUT_VSCREENINFO, &org_varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
	}
exit1:
	close(display_fd);
	return ret;
}


int main(void)
{
	return app_main();
}

