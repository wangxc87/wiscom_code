/*
 * ===========================================================================
 *
 *       Filename:  test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/26/2012 01:55:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Johann (smith), hcywcx@gmail.com
 *        Company:  
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <jpeglib.h>
#include <jerror.h>
#include <string.h>
#include <assert.h>

#define FB_DEV "/dev/fb0"

void RGB888toRGB32(unsigned int width, unsigned char *inbuffer, unsigned char * outbuffer);
unsigned short  RGB888toRGB565(unsigned char red,unsigned char green, unsigned char blue);
int fb_pixel(void *fbmem, int width, int height, int x, int y, unsigned short color);

void RGB888toRGB32(unsigned int width, unsigned char *inbuffer, unsigned char *outbuffer)
{
     unsigned int i,y;
 
     for (i=0; i <  width; i++)
     {
	 /* RED */
         outbuffer[(i<<2)+0] = inbuffer[i * 3 + 2];
	 /* GREEN */
         outbuffer[(i<<2)+1] = inbuffer[i * 3 + 1];
	 /* BLUE */
         outbuffer[(i<<2)+2] = inbuffer[i * 3];
         /* ALPHA */
         outbuffer[(i<<2)+3] = '\0';
         y = 0.299*outbuffer[(i<<2)+0]+0.587*outbuffer[(i<<2)+1]+0.114*outbuffer[(i<<2)+2];
	if(y > 240)
	{
		outbuffer[(i<<2)+0] = 220 ;
		outbuffer[(i<<2)+1] = 220;
		outbuffer[(i<<2)+2] = 220;
	}
     }
}

unsigned short RGB888toRGB565(unsigned char red, unsigned char green, unsigned char blue)
{
	unsigned short  B = (blue >> 3) & 0x001F;
	unsigned short  G = ((green >> 2) << 5) & 0x07E0;
	unsigned short  R = ((red >> 3) << 11) & 0xF800;
	return (unsigned short) (R | G | B);
}

int fb_pixel(void *fbmem, int width, int height, int x, int y, unsigned short color)
{
	if ((x > width) || (y > height))
            return (-1);
	unsigned short *dst = ((unsigned short *) fbmem + y * width + x);
	*dst = color;
	return (0);
}

unsigned char * simple_resize(unsigned char * orgin,int ox,int oy,int dx,int dy) //Ëõ·Å
{
    unsigned char *cr,*p,*l;
    int i,j,k,ip;
    assert(cr = (unsigned char*) malloc(dx*dy*4));
    l=cr;
    
    for(j=0;j<dy;j++,l+=dx*4)
    {
	p=orgin+(j*oy/dy*ox*4);
	for(i=0,k=0;i<dx;i++,k+=4)
	{
	    ip=i*ox/dx*4;
	    l[k]=p[ip];
	    l[k+1]=p[ip+1];
	    l[k+2]=p[ip+2];
	    l[k+3]=p[ip+3];
	}
    }
    return(cr);
}

int main(int argc, char *argv[])
{
	FILE *jpgfile;
	int fbdev;
	char *fb_device;
	unsigned char *fbmem;
	unsigned short color;
	unsigned char *buffer, *outbuffer,*tempbuffer,*temp1buffer;
	unsigned int screensize;
	unsigned int fb_width, fb_height, fb_depth, x, y;
	struct fb_fix_screeninfo fb_finfo;
	struct fb_var_screeninfo fb_vinfo;
	
	fbdev = open("/dev/fb1", O_RDWR);
	if(fbdev < 0)
	{
        	printf("fbdev_error = %d\n",fbdev);
	}
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &fb_finfo)) {
		close(fbdev);
         printf("fb_finfo_eroor\n");
		return (-1);
	}
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		close(fbdev);
         printf("fb_vinfo_error\n");
		return (-1);
	} 
	fb_width = fb_vinfo.xres;
	fb_height = fb_vinfo.yres;
	fb_depth = fb_vinfo.bits_per_pixel;
	screensize = fb_width * fb_height * fb_depth / 8;
	
	if ((fbmem =(unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0)) == MAP_FAILED) {
		printf("MAP_FAILED\n");
		perror("mmap()");
		exit(-1);
	}

	if (( jpgfile = fopen(argv[1], "rb")) == NULL) {
		exit(-1);
	}
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, jpgfile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	/*
	if ((cinfo.output_width > fb_width) || (cinfo.output_height > fb_height)) {
		printf("your image too large to display\n");
		exit(-1);
	}
	*/
	buffer = (unsigned char *)malloc(cinfo.output_width * cinfo.output_components);
	printf("cinfo.output_width = %d,output_height = %d,\ncinfo.output_components = %d \n",cinfo.output_width,cinfo.output_height,cinfo.output_components);
	outbuffer = (unsigned char *)malloc(cinfo.output_width * 4);
	tempbuffer = (unsigned char *)malloc(cinfo.output_height*cinfo.output_width * 4);
	temp1buffer = (unsigned char *)malloc(fb_width*fb_height * 4);
	x = 0, y = 0;
	printf("fb_width =%d  fb_height = %d fb_depth =%d \n",fb_width,fb_height,fb_depth);
	while (cinfo.output_scanline < cinfo.output_height) {
		//printf("cinfo.output_scanline = %d cinfo.output_height = %d \n",cinfo.output_scanline,cinfo.output_height);
		jpeg_read_scanlines(&cinfo, &buffer, 1);
		if (fb_depth == 32) {
			//printf("32\n");
			RGB888toRGB32(fb_width, buffer, outbuffer);	
			//RGB888toRGB32(cinfo.output_width, buffer, outbuffer);
			memcpy(fbmem + y *fb_width * 4, outbuffer, cinfo.output_width * 4);
			//memcpy(tempbuffer + y*cinfo.output_width * 4, outbuffer, cinfo.output_width * 4);		
		} else if (fb_depth == 16) {
			printf("16\n");
			unsigned short  color;
			for (x = 0; x < cinfo.output_width; x++) {
				color = RGB888toRGB565(buffer[x * 3], buffer[x * 3 + 1], buffer[x * 3 + 2]);
                fb_pixel(fbmem, fb_width, fb_height, x, y, color);
			}
		} else {
			printf("other\n");
			memcpy(fbmem + y * fb_width * 3,
				buffer, cinfo.output_width * cinfo.output_components);
		}
		y++;
	}

	printf("end_while\n");
	//temp1buffer= simple_resize(tempbuffer,cinfo.output_width,cinfo.output_height,fb_width,fb_height);
	//fbmem = simple_resize(tempbuffer,cinfo.output_width,cinfo.output_height,fb_width,fb_height);
	printf("fbmem\n");
	//memcpy(fbmem,temp1buffer,fb_width*fb_height * 4);
	printf("finish\n");
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	munmap(fbmem, screensize);
	free(buffer);
	free(outbuffer);
	fclose(jpgfile);
	close(fbdev);
	return 0;
}
