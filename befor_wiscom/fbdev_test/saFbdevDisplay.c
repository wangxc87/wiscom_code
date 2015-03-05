/*
 * saFbdevDisplay.c
 *
 * This is a Fbdev sample application to show the display functionality
 * The app puts a swapping horizontal bar on the display device in various
 * shades of colors. This application runs RGB565 format with size VGA.
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Author: Vaibhav Hiremath <hvaibhav@ti.com>
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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <stdlib.h>
/*
 * Macros Definations
 */
#define MAX(x, y)               (x > y ? x : y)

#define MAXLOOPCOUNT		20
/*
 * Default fbdev device node
 */
static char display_dev_name[30] = {"/dev/fb1"};
/*
 * Color bars
 */
#define CONFIG_TI81XX
 #if defined (CONFIG_TI81XX)
/*this is RGB data instead*/
 static int rgb[2][8] = {
    {
        (0xFF << 16) | (0xFF << 8) | (0xFF),
        (0x00 << 16) | (0x00 << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0x00),
        (0x00 << 16) | (0xFF << 8) | (0x00),
        (0x00 << 16) | (0x00 << 8) | (0xFF),
        (0xFF << 16) | (0xFF << 8) | (0x00),
        (0xFF << 16) | (0x00 << 8) | (0xFF),
        (0x00 << 16) | (0xFF << 8) | (0xFF),
    },
    {
         (0x00 << 16) | (0xFF << 8) | (0xFF),
         (0xFF << 16) | (0x00 << 8) | (0xFF),
         (0xFF << 16) | (0xFF << 8) | (0x00),
         (0x00 << 16) | (0x00 << 8) | (0xFF),
         (0x00 << 16) | (0xFF << 8) | (0x00),
         (0xFF << 16) | (0x00 << 8) | (0x00),
         (0x00 << 16) | (0x00 << 8) | (0x00),
         (0xFF << 16) | (0xFF << 8) | (0xFF),
    },
 };
#define VERTICAL 1
#if VERTICAL
void fill_color_bar(unsigned char *addr,
                        int width,
                        int line_len,
                        int height,
                        int index)
{
	int i,j,k;
	unsigned int *start = (unsigned int*)addr;
	if (line_len & 0xF)
               line_len  += 16 - (line_len & 0xF);

    for(i = 0; i<height;i++){
    for(k = 0;k < 8; k++){
    for(j = 0; j < (line_len /32) ;j++)
        start[j] = rgb[index][k];
    start += line_len /32;
}     
}

}
#else
void fill_color_bar(unsigned char *addr,
		    int width,
		    int line_len,
		    int height,
		    int index)
{
	int i,j,k;
	unsigned int *start = (unsigned int*)addr;
	if (line_len & 0xF)
               line_len  += 16 - (line_len & 0xF);

	for(i = 0 ; i < 8 ; i++) {
		for(j = 0 ; j < (height / 8) ; j++) {
			for(k = 0 ; k < (line_len/4); k++)
				start[k] = rgb[index][i];
			start += (line_len/4);
		}
	}

}
#endif
 #else
static short ycbcr[2][8] = {
	{
		(0x1F << 11) | (0x3F << 5) | (0x1F),
		(0x00 << 11) | (0x00 << 5) | (0x00),
		(0x1F << 11) | (0x00 << 5) | (0x00),
		(0x00 << 11) | (0x3F << 5) | (0x00),
		(0x00 << 11) | (0x00 << 5) | (0x1F),
		(0x1F << 11) | (0x3F << 5) | (0x00),
		(0x1F << 11) | (0x00 << 5) | (0x1F),
		(0x00 << 11) | (0x3F << 5) | (0x1F),
	}, {
		(0x00 << 11) | (0x3F << 5) | (0x1F),
		(0x1F << 11) | (0x00 << 5) | (0x1F),
		(0x1F << 11) | (0x3F << 5) | (0x00),
		(0x00 << 11) | (0x00 << 5) | (0x1F),
		(0x00 << 11) | (0x3F << 5) | (0x00),
		(0x1F << 11) | (0x00 << 5) | (0x00),
		(0x00 << 11) | (0x00 << 5) | (0x00),
		(0x1F << 11) | (0x3F << 5) | (0x1F),
	}
};
/*
 * This function is used to fill up buffer with color bars.
 */
void fill_color_bar(unsigned char *addr, int width, int line_len, int height, int index)
{
	unsigned short *start = (unsigned short *)addr;
	int i, j, k;

	if (index) {
		for(i = 0 ; i < 8 ; i ++) {
			for(k=0; k < (height/8); k++) {
				for(j = 0 ; j < width ; j ++) {
					start[j] = ycbcr[1][i];
				}
				start = start + line_len;
			}
		}
	} else {
		for(i = 0 ; i < 8 ; i ++) {
			for(k=0; k < (height/8); k++) {
				for(j = 0 ; j < width ; j ++) {
					start[j] = ycbcr[0][i];
				}
				start = start + line_len;
			}
		}
	}
}
#endif

static int app_main(void)
{
	unsigned char *buffer_addr;
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	int buffersize, ret, display_fd, i;

    system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics1/nodes");//wxc@ 20140515

	/* Open the display device */
	display_fd = open(display_dev_name, O_RDWR);
	if (display_fd <= 0) {
		perror("Could not open device\n");
		return -1;
	}

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}
        printf("\nFix Screen Info:\n");
        printf("----------------\n");
        printf("Line Length - %d\n", fixinfo.line_length);
	printf("Physical Address = %lx\n",fixinfo.smem_start);
	printf("Buffer Length = %d\n",fixinfo.smem_len);

	/* Get variable screen information. Variable screen information
	 * gives informtion like size of the image, bites per pixel,
	 * virtual size of the image etc. */
	ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		goto exit1;
	}
        printf("\nVar Screen Info:\n");
        printf("----------------\n");
        printf("Xres - %d\n", varinfo.xres);
        printf("Yres - %d\n", varinfo.yres);
        printf("Xres Virtual - %d\n", varinfo.xres_virtual);
        printf("Yres Virtual - %d\n", varinfo.yres_virtual);
        printf("Bits Per Pixel - %d\n", varinfo.bits_per_pixel);
        printf("Pixel Clk - %d\n", varinfo.pixclock);
        printf("Rotation - %d\n", varinfo.rotate);

	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

	/*
	 * Set the resolution which read before again to prove the
	 * FBIOPUT_VSCREENINFO ioctl.
	 */

	ret = ioctl(display_fd, FBIOPUT_VSCREENINFO, &org_varinfo);
	if (ret < 0) {
		perror("Error writing variable information.\n");
		goto exit1;
	}

	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}

	/* Mmap the driver buffers in application space so that application
	 * can write on to them. Driver allocates contiguous memory for
	 * three buffers. These buffers can be displayed one by one. */
	buffersize = fixinfo.line_length * varinfo.yres;
	buffer_addr = (unsigned char *)mmap (0, buffersize,
			(PROT_READ|PROT_WRITE), MAP_SHARED, display_fd, 0);

	if (buffer_addr == MAP_FAILED) {
		printf("MMap failed\n");
		ret = -ENOMEM;
		goto exit1;
	}
    system("echo 1 > /sys/devices/platform/vpss/graphics1/enabled");//wxc@ 20140515
    printf("Enable %s ...\n",display_dev_name);//wxc@ 20140515
	/* Color bar display loop */
	for (i = 0 ; i < MAXLOOPCOUNT ; i) { //wxc@ 20140515
		/* Fill the buffers with the color bars */
 #if defined (CONFIG_TI81XX)
		fill_color_bar(buffer_addr, varinfo.xres, fixinfo.line_length,
				varinfo.yres, i%2);
#else
		fill_color_bar(buffer_addr, varinfo.xres, fixinfo.line_length/2,
				varinfo.yres, i%2);
#endif
		sleep(1);
	}
	ret = 0;
	munmap(buffer_addr, buffersize);

exit1:
	close(display_fd);
	return ret;
}

int main(void)
{
	return app_main();
}
