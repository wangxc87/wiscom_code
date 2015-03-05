/*
  qt_start.c
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

struct display_dev 
{
        int fd;
        int width;
        int height;
        int plane_type;
        char *display_buffer;
        int buffer_size;
};

        
int display_init(int devid,struct display_dev *display_dev )
{
	int ret;
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo;
	int width,height;
	int display_fd;
	char device_name[30];
    int inwidth,  inheight, planeType,buffersize;
    char *buffer_addr;
    

    inwidth = display_dev->width;
    inheight = display_dev->height;
    planeType = display_dev->plane_type;
    
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

	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}
#ifdef  _QT_START_DEBUG_
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
	printf("\tPixel Clk - %d\n", varinfo.pixclock);
	printf("\tRotation - %d\n", varinfo.rotate);

	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));
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
//	if(VDIS_DEV_SD == devid)
//			grpx_fb_scale(display_fd, 0, 0, SD_PAL_WIDTH, SD_PAL_HEIGHT);

	ret = ioctl(display_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		goto exit1;
	}
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
	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(display_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		goto exit1;
	}

    buffersize = fixinfo.line_length * varinfo.yres;
    buffer_addr = (unsigned char *)mmap (0, buffersize,
                                             (PROT_READ|PROT_WRITE),
                                             MAP_SHARED, display_fd, 0);
        
    if (buffer_addr == MAP_FAILED) {
            printf("MMap failed\n");
            ret = -ENOMEM;
            goto exit1;
    }
    display_dev->fd = display_fd;
    display_dev->display_buffer = buffer_addr;
    display_dev->buffer_size = buffersize;
    
	return display_fd;
exit1:
	return ret;
}
int display_deinit(struct display_dev *display_dev)
{
        munmap(display_dev->display_buffer,display_dev->buffer_size);
        close(display_dev->fd);
        return 0;
}

int main(int argc, char ** argv)
{
	int buffer_size, ret, display_fd1, display_fd2, i;
	int planeType = GRPX_FORMAT_RGB565;
    struct display_dev  display_dev;
    char *display_buffer;
    
	system("echo 1 > /sys/devices/platform/vpss/display0/enabled");
    system("echo 1 > /sys/devices/platform/vpss/display1/enabled");
    system("echo 1 > /sys/devices/platform/vpss/display2/enabled");
	
	display_dev.width = GRPX_PLANE_WIDTH;
    display_dev.height = GRPX_PLANE_HEIGHT;
    display_dev.plane_type = planeType;
    	
	display_fd1 = display_init(VDIS_DEV_DVO2,&display_dev);
	if (display_fd1 <= 0) {
		printf("ERR:Could not open device:%d\n", VDIS_DEV_DVO2);
		return -1;
	}
    display_buffer = display_dev.display_buffer;
    buffer_size = display_dev.buffer_size;
    
	memset(display_buffer,0,buffer_size);
    display_deinit(&display_dev);
    	
exit1:
	
	return ret;
}

