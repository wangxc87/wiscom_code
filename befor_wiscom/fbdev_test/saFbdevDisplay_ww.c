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
 n *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *
 */

/*
 * Header File Inclusion
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/ti81xxfb.h>
/*

 * Macros Definations
 */
#define MAX(x, y)               (x > y ? x : y)

#define MAXLOOPCOUNT		5
#define CONFIG_TI81XX
/*
 * Default fbdev device node
 */
//static char display_dev_name[30] = {"/dev/fb1"};

#define DISPLAY_DEVICE0  "/dev/fb0"
#define DISPLAY_DEVICE1  "/dev/fb1"
#define DISPLAY_DEVICE2  "/dev/fb2"

#define GRPX_PLANE_WIDTH  640
#define GRPX_PLANE_HEIGHT 480

#define RGB_KEY_24BIT_GRAY   0x00FFFFFF
#define RGB_KEY_16BIT_GRAY   0xFFFF
#define GRPX_SC_MARGIN_OFFSET   (3)

#define GRPX_FB_SINGLE_BUFFER_TIED_GRPX
typedef unsigned int      UInt32;
typedef int               Int32;

typedef enum {
    GRPX_FORMAT_RGB565 = 0,
    GRPX_FORMAT_RGB888 = 1,
    GRPX_FORMAT_MAX

}grpx_plane_type;

/*
 * Color bars
 */

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

Int32 grpx_fb_scale(int devId,
	UInt32 startX,
	UInt32 startY,
	UInt32 outWidth,
	UInt32 outHeight)
{

	struct ti81xxfb_scparams scparams;
	Int32 fd = 0, status = 0;
//	app_grpx_t *grpx = &grpx_obj;
	int dummy;
	struct ti81xxfb_region_params regp;
	char filename[100], buffer[10];
	int r = -1;
    fd = devId;
    
/*
	if (devId == VDIS_DEV_HDMI) {
		fd = grpx->fd;
		VDIS_CMD_IS_GRPX_ON(filename, buffer, VDIS_GET_GRPX, 0, 1, r);
	}
	if (devId == VDIS_DEV_SD) {
		fd = grpx->fd2;
		VDIS_CMD_IS_GRPX_ON(filename, buffer, VDIS_GET_GRPX, 2, 1, r);
	}
*/

	/* Set Scalar Params for resolution conversion 
	 * inHeight and inWidth should remain same based on grpx buffer type 
	 */

#if defined(GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX)
	//if (devId == VDIS_DEV_HDMI){   //villion
	//	scparams.inwidth  = GRPX_PLANE_HD_WIDTH;
	//	scparams.inheight = GRPX_PLANE_HD_HEIGHT;
	//}
	if (devId == VDIS_DEV_SD) {
		scparams.inwidth = GRPX_PLANE_SD_WIDTH;
		scparams.inheight = GRPX_PLANE_SD_HEIGHT;
	}

	if (devId == VDIS_DEV_HDMI) {
		scparams.inwidth = GRPX_PLANE_WIDTH;
		scparams.inheight = GRPX_PLANE_HEIGHT;
	}
#endif 

#if defined(GRPX_FB_SINGLE_BUFFER_TIED_GRPX) || defined (GRPX_FB_SINGLE_BUFFER_NON_TIED_GRPX)
	scparams.inwidth = GRPX_PLANE_WIDTH;
	scparams.inheight = GRPX_PLANE_HEIGHT;
#endif 



	// this "-GRPX_SC_MARGIN_OFFSET" is needed since scaling can result in +2 extra pixels, so we compensate by doing -2 here
	scparams.outwidth = outWidth - GRPX_SC_MARGIN_OFFSET;
	scparams.outheight = outHeight - GRPX_SC_MARGIN_OFFSET;
	scparams.coeff = NULL;

	if (ioctl(fd, TIFB_GET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_GET_PARAMS !!!\n");
	}

	regp.pos_x = startX;
	regp.pos_y = startY;
	regp.transen = TI81XXFB_FEATURE_ENABLE;
	regp.transcolor = RGB_KEY_24BIT_GRAY;
	regp.scalaren = TI81XXFB_FEATURE_ENABLE;
	//regp.blendalpha = 0xf0; //villion
 
	/*not call the IOCTL, ONLY if 100% sure that GRPX is off*/
	if (!((r == 0) && (atoi(buffer) == 0))) {
		if (ioctl(fd, FBIO_WAITFORVSYNC, &dummy)) {
			printf("ERR:FBIO_WAITFORVSYNC !!!\n");
			return -1;
		}
	}
	if ((status = ioctl(fd, TIFB_SET_SCINFO, &scparams)) < 0) {
		printf("ERR:TIFB_SET_SCINFO !!!\n");
	}


	if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_SET_PARAMS !!!\n");
	}

	return(status);

}

static int app_main(void)
{
        unsigned char *buffer_addr;
        struct fb_fix_screeninfo fixinfo;
        struct fb_var_screeninfo varinfo, org_varinfo;
        int buffersize, ret, display_fd,display_fd1, i;
        int planeType = GRPX_FORMAT_RGB565;

      //  system("echo 0 > /sys/devices/platform/vpss/graphics0/enabled");    
        system("echo 1 > /sys/devices/platform/vpss/display1/enabled");

        /* Open the display device */
        display_fd = open(DISPLAY_DEVICE1, O_RDWR);
        if (display_fd <= 0) {
                printf("ERR:Could not open device:%s\n",DISPLAY_DEVICE0);
                return -1;
        }
        printf("Open device %s OK+++++\n",DISPLAY_DEVICE0);

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
        printf("\tLine Length - %d\n", fixinfo.line_length);
        printf("\tPhysical Address = %lx\n",fixinfo.smem_start);
        printf("\tBuffer Length = %d\n",fixinfo.smem_len);

        /* Get variable screen information. Variable screen information
         * gives informtion like size of the image, bites per pixel,
         * virtual size of the image etc. */
        ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
        if (ret < 0) {
                perror("Error reading variable information.\n");
                goto exit1;
        }
        printf("\nOriginal Var Screen Info:\n");
        printf("\tXres - %d\n", varinfo.xres);
        printf("\tYres - %d\n", varinfo.yres);
        printf("\tXres Virtual - %d\n", varinfo.xres_virtual);
        printf("\tYres Virtual - %d\n", varinfo.yres_virtual);
        printf("\tBits Per Pixel - %d\n", varinfo.bits_per_pixel);
        printf("\tPixel Clk - %d\n", varinfo.pixclock);
        printf("\tRotation - %d\n", varinfo.rotate);

        memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

        /*
         * Set the resolution which read before again to prove the
         * FBIOPUT_VSCREENINFO ioctl.
         */

        varinfo.xres = GRPX_PLANE_WIDTH;
        varinfo.yres = GRPX_PLANE_HEIGHT;
        varinfo.xres_virtual = GRPX_PLANE_WIDTH;
        varinfo.yres_virtual = GRPX_PLANE_HEIGHT;
        if (planeType == GRPX_FORMAT_RGB565) {
                varinfo.bits_per_pixel = 16;
                varinfo.red.length = 5;
                varinfo.green.length = 6;
                varinfo.blue.length = 5;

                varinfo.red.offset = 11;
                varinfo.green.offset = 5;
                varinfo.blue.offset = 0;
        }

        ret = ioctl(display_fd, FBIOPUT_VSCREENINFO, &varinfo);
        if (ret < 0) {
                perror("Error writing variable information.\n");
                goto exit1;
        }

        printf("\nAfter Setting Var Screen Info:\n");
        printf("\tXres - %d\n", varinfo.xres);
        printf("\tYres - %d\n", varinfo.yres);
        printf("\tXres Virtual - %d\n", varinfo.xres_virtual);
        printf("\tYres Virtual - %d\n", varinfo.yres_virtual);
        printf("\tBits Per Pixel - %d\n", varinfo.bits_per_pixel);
        printf("\tPixel Clk - %d\n", varinfo.pixclock);
        printf("\tRotation - %d\n", varinfo.rotate);

        grpx_fb_scale(display_fd,0,0,GRPX_PLANE_WIDTH,GRPX_PLANE_HEIGHT);


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
                                             (PROT_READ|PROT_WRITE),
                                             MAP_SHARED, display_fd, 0);

        if (buffer_addr == MAP_FAILED) {
                printf("MMap failed\n");
                ret = -ENOMEM;
                goto exit1;
        }
        i = MAXLOOPCOUNT;
        /* Color bar display loop */
        while(i){
                /* Fill the buffers with the color bars */

#if defined (CONFIG_TI81XX)
                fill_color_bar(buffer_addr, varinfo.xres, fixinfo.line_length,
                               varinfo.yres,1);// i%2);
#else
                fill_color_bar(buffer_addr, varinfo.xres, fixinfo.line_length/2,
                               varinfo.yres, i%2);
#endif
                sleep(1);
        }
        ret = 0;
        munmap(buffer_addr, buffersize);
 //       printf("Run QT++++\n");
 //       system("/opt/dvr_rdk/ti814x/runqt.sh");
exit1:
        close(display_fd);
        return ret;
}
int main(void)
{
        return app_main();
}
