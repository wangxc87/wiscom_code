/*
  qt_start.c
 * 
 * 2013.08.30  修正启动黑屏bug
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

#define MAX(x, y)               (x > y ? x : y)
#define VERSION "V1.0 20130830"
#define DISPLAY_DEVICE0  "/dev/fb0"
#define DISPLAY_DEVICE1  "/dev/fb1"
#define DISPLAY_DEVICE2  "/dev/fb2"

#define GRPX_PLANE_WIDTH  640
#define GRPX_PLANE_HEIGHT 480

#define RGB_KEY_24BIT_GRAY   0x00FFFFFF
#define RGB_KEY_16BIT_GRAY   0xFFFF
#define GRPX_SC_MARGIN_OFFSET   (3)

#define GRPX_FB_SINGLE_BUFFER_TIED_GRPX
#define   VDIS_DEV_HDMI   0
/**< Display 0 */
#define   VDIS_DEV_HDCOMP  1
/**< Display 1 */
#define   VDIS_DEV_DVO2   2
/**< Display 2 */
#define   VDIS_DEV_SD     3
#define SD_NTSC_WIDTH 720
#define SD_NTSC_HEIGHT 480
#define SD_PAL_WIDTH 720
#define SD_PAL_HEIGHT 576
typedef unsigned int UInt32;
typedef int Int32;
//#define _QT_START_DEBUG_

typedef enum {
	GRPX_FORMAT_RGB565 = 0,
	GRPX_FORMAT_RGB888 = 1,
	GRPX_FORMAT_MAX

} grpx_plane_type;

Int32 grpx_fb_scale(int devId,
	UInt32 startX,
	UInt32 startY,
	UInt32 outWidth,
	UInt32 outHeight)
{

	struct ti81xxfb_scparams scparams;
	Int32 fd = 0, status = 0;
	int dummy;
	struct ti81xxfb_region_params regp;
	int r = -1;
	char buffer[10];
	unsigned char alpha = 0xff;


	fd = devId;

#if defined(GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX)
	if (devId == VDIS_DEV_SD) {
		scparams.inwidth = GRPX_PLANE_SD_WIDTH;
		scparams.inheight = GRPX_PLANE_SD_HEIGHT;
        scparams.outwidth = GRPX_PLANE_SD_WIDTH;
        scparams.outheight = GRPX_PLANE_SD_HEIGHT;

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
	if ((status = ioctl(fd, TIFB_SET_SCINFO, &scparams)) < 0) {
		printf("ERR:TIFB_SET_SCINFO !!!\n");
	}


	if (ioctl(fd, TIFB_GET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_GET_PARAMS !!!\n");
	}
	printf("regp.transcolor is 0x%x.\n", regp.transcolor);
	regp.blendtype = TI81XXFB_BLENDING_GLOBAL;
	regp.blendalpha = alpha;

	if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_SET_PARAMS Alpha !!!\n");
	}

	regp.pos_x = startX;
	regp.pos_y = startY;
	regp.transen = TI81XXFB_FEATURE_ENABLE;
	regp.transcolor = RGB_KEY_24BIT_GRAY;
	//regp.scalaren = TI81XXFB_FEATURE_ENABLE;
	//regp.blendalpha = 0xf0; //villion

	/*not call the IOCTL, ONLY if 100% sure that GRPX is off*/



    if (ioctl(fd, FBIO_WAITFORVSYNC, &dummy)) {
            printf("ERR:FBIO_WAITFORVSYNC !!!\n");
            return -1;
    }



	if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_SET_PARAMS Transen !!!\n");
	}

	return(status);

}

int init_fbdev(int devid, int inwidth, int inheight, int planeType)
{
	int ret;
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo;
	int width, height;
	int display_fd,i;
	char device_name[30];
	struct ti81xxfb_region_params regp; 
    
	switch (devid) {
	case VDIS_DEV_HDMI:
		strcpy(device_name, DISPLAY_DEVICE0);
		width = inwidth;
		height = inheight;
		break;
	case VDIS_DEV_DVO2:
		strcpy(device_name, DISPLAY_DEVICE1);
		width = inwidth;
		height = inheight;
		break;
	case VDIS_DEV_SD:
		strcpy(device_name, DISPLAY_DEVICE2);
		width = inwidth;
		height = inheight;
		break;
	default:
		strcpy(device_name, DISPLAY_DEVICE1);
		break;
	}

	display_fd = open(device_name, O_RDWR);
	if (display_fd <= 0) {
		printf("ERR:Could not open device:%s\n", device_name);
		return -1;
	}
	printf("Open device %s OK+++++\n", device_name);
/*

    if (ioctl(display_fd, TIFB_GET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_GET_PARAMS !!!\n");
	}
	regp.blendtype = TI81XXFB_BLENDING_GLOBAL;
	regp.blendalpha = alpha;
*/

	if (ioctl(display_fd, TIFB_SET_PARAMS, &regp) < 0) {
		printf("ERR:TIFB_SET_PARAMS Alpha !!!\n");
	}

#ifdef  _QT_START_DEBUG_
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}

	printf("\nFix Screen Info:\n");
	printf("\tLine Length - %d\n", fixinfo.line_length);
	printf("\tPhysical Address = %lx\n", fixinfo.smem_start);
	printf("\tBuffer Length = %d\n", fixinfo.smem_len);
#endif
	ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		goto exit1;
	}
#ifdef  _QT_START_DEBUG_
	printf("\nOriginal Var Screen Info:\n");
	printf("\tXres - %d\n", varinfo.xres);
	printf("\tYres - %d\n", varinfo.yres);
	printf("\tXres Virtual - %d\n", varinfo.xres_virtual);
	printf("\tYres Virtual - %d\n", varinfo.yres_virtual);
	printf("\tBits Per Pixel - %d\n", varinfo.bits_per_pixel);
	printf("\tRed:length %d,offset %d.\n",varinfo.red.length,varinfo.red.offset);
	printf("\tGreen:length %d,offset %d\n",varinfo.green.length,varinfo.green.offset);
	printf("\tBlue:length %d,offset %d\n",varinfo.blue.length,varinfo.blue.offset);
	printf("\tTransparnecy:length %d,offset %d.\n",varinfo.transp.length,varinfo.transp.offset);
	printf("\tPixel Clk - %d\n", varinfo.pixclock);
	printf("\tRotation - %d\n", varinfo.rotate);

//	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));
#endif
	varinfo.xres = width;
	varinfo.yres = height;
	varinfo.xres_virtual = width;
	varinfo.yres_virtual = height;
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
	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}

	ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		goto exit1;
	}
	int buffersize;
	unsigned char *buffer_addr,*buffer_tmp;
    buffersize = fixinfo.line_length * varinfo.yres;
    buffer_addr = (unsigned char *)mmap (0, buffersize,
                                         (PROT_READ|PROT_WRITE),
                                         MAP_SHARED, display_fd, 0);

    if (buffer_addr == MAP_FAILED) {
            printf("MMap failed\n");
            ret = -ENOMEM;
            goto exit1;
    }
    buffer_tmp = buffer_addr;
    for(i=buffersize/4;i>0;i--){
            *buffer_tmp = 0xff; //RGB_KEY_24BIT_GRAY >> 24;
            buffer_tmp++;
            *buffer_tmp = 0xff;//RGB_KEY_24BIT_GRAY >> 16;
            buffer_tmp++;
            *buffer_tmp = 0xff;//RGB_KEY_24BIT_GRAY >> 8;
            buffer_tmp++;
            *buffer_tmp = 0x00;//RGB_KEY_24BIT_GRAY >> 0;
            buffer_tmp++;
            
    }
    //memcpy(buffer_addr,RGB_KEY_24BIT_GRAY,buffersize/4);
           
#ifdef  _QT_START_DEBUG_
	printf("\nAfter Setting Var Screen Info:\n");
	printf("\tXres - %d\n", varinfo.xres);
	printf("\tYres - %d\n", varinfo.yres);
	printf("\tXres Virtual - %d\n", varinfo.xres_virtual);
	printf("\tYres Virtual - %d\n", varinfo.yres_virtual);
	printf("\tBits Per Pixel - %d\n", varinfo.bits_per_pixel);
	printf("\tPixel Clk - %d\n", varinfo.pixclock);
	printf("\tRotation - %d\n", varinfo.rotate);


#endif
    if (VDIS_DEV_SD == devid)
            grpx_fb_scale(display_fd, 0, 0, SD_PAL_WIDTH, SD_PAL_HEIGHT);
	grpx_fb_scale(display_fd, 0, 0, width, height);

	return display_fd;
exit1:
	return ret;
}

int main(int argc, char ** argv)
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	int buffersize, ret, display_fd1, display_fd2, i;
	int planeType = GRPX_FORMAT_RGB888;
	unsigned char *buffer_addr;
	struct timeval stop_time, start_time;
	/*
		if(argc >1)
		    if(argv[1][0]=='1'){
			    planeType = GRPX_FORMAT_RGB565;
			    printf("PlaneType is GRPX_FORMAT_RGB565.\n");
		    }
	 */
    printf("Current Version is %s.\n",VERSION);
    
	system("echo 1 > /sys/devices/platform/vpss/display0/enabled");
	system("echo 1 > /sys/devices/platform/vpss/display1/enabled");
	system("echo 0 > /sys/devices/platform/vpss/display2/enabled");
    system("echo pal > /sys/devices/platform/vpss/display2/mode");
    system("echo 1 > /sys/devices/platform/vpss/display2/enabled");
    
    display_fd1 = init_fbdev(VDIS_DEV_DVO2, GRPX_PLANE_WIDTH, GRPX_PLANE_HEIGHT, planeType);
	if (display_fd1 <= 0) {
		printf("ERR:Could not open device:%d\n", VDIS_DEV_DVO2);
		return -1;
	}

	display_fd2 = init_fbdev(VDIS_DEV_SD, GRPX_PLANE_WIDTH, GRPX_PLANE_HEIGHT, planeType);
	if (display_fd1 <= 0) {
		printf("ERR:Could not open device:%d\n", VDIS_DEV_DVO2);
		return -1;
	}

    printf("Run QT++++\n");
//	system("/opt/dvr_rdk/ti814x/bin/MyQt -qws -nomouse -nokeyboard -display \"LinuxFb:/dev/fb1\" &");
exit1:
	close(display_fd1);
	close(display_fd2);
	return ret;
}

