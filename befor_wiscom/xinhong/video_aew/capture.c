#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/videohd.h>
#include <media/davinci/dm365_ccdc.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>
#include <termios.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>
#include "cmem.h"

#include "capture.h"
#define CAPTURE_DEVICE  "/dev/video0"
#define ALIGN(x, y)	(((x + (y-1))/y)*y)

struct capt_std_params mt9p031_std_params[] = {
	{
      .std = V4L2_STD_525_60,// 0
		.scan_width = 720,
		.scan_height = 480,
		.crop = { 0 , 0, 640, 480},
		.image_width = 720,
		.image_height = 480,
		.name = "V4L2_STD_525_60",
	},
	{
		.std = V4L2_STD_625_50,// 1
		.scan_width = 720,
		.scan_height = 576,
		.crop = { 0 , 0, 720, 480},
		.image_width = 720,
		.image_height = 576,
		.name = "V4L2_STD_625_50",
	},
	{
		.std = V4L2_STD_525P_60,// 2
		.scan_width = 720,
		.scan_height = 480,
		.crop = { 0 , 0, 720, 576},
		.image_width = 720,
		.image_height = 480,
		.name = "V4L2_STD_525P_60",
	},
	{
		.std = V4L2_STD_625P_50,// 3
		.scan_width = 720,
		.scan_height = 576,
		.crop = { 0 , 0, 720, 576},
		.image_width = 720,
		.image_height = 576,
		.name = "V4L2_STD_625P_50",
	},
	{
		.std = V4L2_STD_720P_30,// 4
		.scan_width = 1280,
		.scan_height = 720,
		.crop = { 0 , 0, 1280, 720},
		.image_width = 1280,
		.image_height = 720,
		.name = "V4L2_STD_720P_30",
	},
	{
		.std = V4L2_STD_720P_50,// 5
		.scan_width = 1280,
		.scan_height = 720,
		.crop = { 0 , 0, 1280, 720},
		.image_width = 1280,
		.image_height = 720,
		.name = "V4L2_STD_720P_50",
	},
	{
		.std = V4L2_STD_720P_60,// 6
		.scan_width = 1280,
		.scan_height = 720,
		.crop = { 0 , 0, 1280, 720},
		.image_width = 1280,
		.image_height = 720,
		.name = "V4L2_STD_720P_60",
	},
       { 
		.std = V4L2_STD_1080I_30,// 7
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080I_30",
	},
	{
		.std = V4L2_STD_1080I_50,// 8
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080I_50",
	},
	{
		.std = V4L2_STD_1080I_60,// 9
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080I_60",
	},
       {
		.std = V4L2_STD_1080P_30,// 10
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080P_30",
	},
	{
		.std = V4L2_STD_1080P_50,// 11
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080P_50",
	},
	{
		.std = V4L2_STD_1080P_60,// 12
		.scan_width = 1920,
		.scan_height = 1080,
		.crop = { 0 , 0, 1920, 1080},
		.image_width = 1920,
		.image_height = 1080,
		.name = "V4L2_STD_1080P_60",
	},
};


static int configCCDCraw(int capt_fd)
{
	struct ccdc_config_params_raw raw_params;
	/* Change these values for testing Gain - Offsets */
	struct ccdc_float_16 r = {0, 511};
	struct ccdc_float_16 gr = {0, 511};
	struct ccdc_float_16 gb = {0, 511};
	struct ccdc_float_16 b = {0, 511};
	struct ccdc_float_8 csc_coef_val = { 1, 0 };
	int i;

	bzero(&raw_params, sizeof(raw_params));

	/* First get the parameters */
	if (-1 == ioctl(capt_fd, VPFE_CMD_G_CCDC_RAW_PARAMS, &raw_params)) {
		printf("InitDevice:ioctl:VPFE_CMD_G_CCDC_PARAMS, %p", &raw_params);
		return -1;
	}

	raw_params.compress.alg = CCDC_NO_COMPRESSION;
	raw_params.gain_offset.gain.r_ye = r;
	raw_params.gain_offset.gain.gr_cy = gr;
	raw_params.gain_offset.gain.gb_g = gb;
	raw_params.gain_offset.gain.b_mg = b;
	raw_params.gain_offset.gain_sdram_en = 1;
	raw_params.gain_offset.gain_ipipe_en = 1;
	raw_params.gain_offset.offset = 0;
	raw_params.gain_offset.offset_sdram_en = 1;

    // flag to enable linearization
    static int linearization_en;
    // flag to enable color space conversion
    // input sensor should output CMYG pattern instead of RGGB
    // to test this. This is currently just used for unit
    // test verification
    static int csc_en;

    // vertical line defect correction enable
    static int vldfc_en;

    // enable culling
    // static int en_culling;
    static int en_culling;

	/* To test linearization, set this to 1, and update the
	 * linearization table with correct data
	 */
	if (linearization_en) {
      raw_params.linearize.en = 1;
      raw_params.linearize.corr_shft = CCDC_1BIT_SHIFT;
      raw_params.linearize.scale_fact.integer = 0;
      raw_params.linearize.scale_fact.decimal = 10;

      for (i = 0; i < CCDC_LINEAR_TAB_SIZE; i++)
        raw_params.linearize.table[i] = i;
	} else {
      raw_params.linearize.en = 0;
	}

	/* CSC */
	if (csc_en) {
      raw_params.df_csc.df_or_csc = 0;
      raw_params.df_csc.csc.en = 1;
      /* I am hardcoding this here. But this should
       * really match with that of the capture standard
       */
      raw_params.df_csc.start_pix = 1;
      raw_params.df_csc.num_pixels = 720;
      raw_params.df_csc.start_line = 1;
      raw_params.df_csc.num_lines = 480;
      /* These are unit test values. For real case, use
       * correct values in this table
       */
      raw_params.df_csc.csc.coeff[0] = csc_coef_val;
      raw_params.df_csc.csc.coeff[1].decimal = 1;
      raw_params.df_csc.csc.coeff[2].decimal = 2;
      raw_params.df_csc.csc.coeff[3].decimal = 3;
      raw_params.df_csc.csc.coeff[4].decimal = 4;
      raw_params.df_csc.csc.coeff[5].decimal = 5;
      raw_params.df_csc.csc.coeff[6].decimal = 6;
      raw_params.df_csc.csc.coeff[7].decimal = 7;
      raw_params.df_csc.csc.coeff[8].decimal = 8;
      raw_params.df_csc.csc.coeff[9].decimal = 9;
      raw_params.df_csc.csc.coeff[10].decimal = 10;
      raw_params.df_csc.csc.coeff[11].decimal = 11;
      raw_params.df_csc.csc.coeff[12].decimal = 12;
      raw_params.df_csc.csc.coeff[13].decimal = 13;
      raw_params.df_csc.csc.coeff[14].decimal = 14;
      raw_params.df_csc.csc.coeff[15].decimal = 15;

	} else {
      raw_params.df_csc.df_or_csc = 0;
      raw_params.df_csc.csc.en = 0;
	}

	/* vertical line defect correction */
	if (vldfc_en) {
      raw_params.dfc.en = 1;
      // correction method
      raw_params.dfc.corr_mode = CCDC_VDFC_HORZ_INTERPOL_IF_SAT;
      // not pixels upper than the defect corrected
      raw_params.dfc.corr_whole_line = 1;
      raw_params.dfc.def_level_shift = CCDC_VDFC_SHIFT_2;
      raw_params.dfc.def_sat_level = 20;
      raw_params.dfc.num_vdefects = 7;
      for (i = 0; i < raw_params.dfc.num_vdefects; i++) {
        raw_params.dfc.table[i].pos_vert = i;
        raw_params.dfc.table[i].pos_horz = i + 1;
        raw_params.dfc.table[i].level_at_pos = i + 5;
        raw_params.dfc.table[i].level_up_pixels = i + 6;
        raw_params.dfc.table[i].level_low_pixels = i + 7;
      }
      printf("DFC enabled\n");
	} else {
      raw_params.dfc.en = 0;
	}

	if (en_culling) {

      printf("Culling enabled\n");
      raw_params.culling.hcpat_odd  = 0xaa;
      raw_params.culling.hcpat_even = 0xaa;
      raw_params.culling.vcpat = 0x55;
      raw_params.culling.en_lpf = 1;
	} else {
      raw_params.culling.hcpat_odd  = 0xFF;
      raw_params.culling.hcpat_even = 0xFF;
      raw_params.culling.vcpat = 0xFF;
	}

	raw_params.col_pat_field0.olop = CCDC_GREEN_BLUE;
	raw_params.col_pat_field0.olep = CCDC_BLUE;
	raw_params.col_pat_field0.elop = CCDC_RED;
	raw_params.col_pat_field0.elep = CCDC_GREEN_RED;

	raw_params.col_pat_field1.olop = CCDC_GREEN_BLUE;
	raw_params.col_pat_field1.olep = CCDC_BLUE;
	raw_params.col_pat_field1.elop = CCDC_RED;
	raw_params.col_pat_field1.elep = CCDC_GREEN_RED;
	raw_params.data_size = CCDC_12_BITS;
	raw_params.data_shift = CCDC_NO_SHIFT;

	printf("VPFE_CMD_S_CCDC_RAW_PARAMS, size = %d, address = %p\n", sizeof(raw_params),
           &raw_params);
	if (-1 == ioctl(capt_fd, VPFE_CMD_S_CCDC_RAW_PARAMS, &raw_params)) {
      printf("InitDevice:ioctl:VPFE_CMD_S_CCDC_PARAMS, %p", &raw_params);
      return -1;
	}

	return 0;
}

int set_data_format(int fdCapture, struct capt_std_params input_std_params)
{
  int ret,width,height;
  v4l2_std_id  cur_std,std;
  struct v4l2_format fmt;
  struct v4l2_input input;
  struct v4l2_crop crop;
  struct v4l2_control ctrl;

  int pixelformat;
  int failCount = 0;
  char *camera_input = "Camera";
 
  std    = input_std_params.std;
  width  = input_std_params.image_width;
  height = input_std_params.image_height;
  cur_std = std;    
  printf("set_data_format:setting std to 0x%llx\n", cur_std);

	// first set the input
	input.type = V4L2_INPUT_TYPE_CAMERA;
	input.index = 0;
	while (-EINVAL != ioctl(fdCapture, VIDIOC_ENUMINPUT, &input)) {
		printf("input.name = %s\n", input.name);
		if (!strcmp((char*)input.name, (char*)camera_input)) {
            printf("Found the input %s \n", camera_input);
			break;
        }
		input.index++;
	}
    int flickerHz = 60;
    
    printf("Setting input %d\n", input.index);
	if (-1 == ioctl (fdCapture, VIDIOC_S_INPUT, &input.index)) {
		perror("ioctl:VIDIOC_S_INPUT failed\n");
		return -1;
	}

	printf ("InitDevice:ioctl:VIDIOC_S_INPUT, selected input\n");
    ctrl.id = V4L2_CID_EXPOSURE;
   if(input_std_params.std == V4L2_STD_720P_30
      ||input_std_params.std == V4L2_STD_720P_50
      ||input_std_params.std == V4L2_STD_720P_60)
        {
     if(flickerHz == 50)
        ctrl.value = 0x2A9;
     else
        ctrl.value = 0x2F2;
      }
   else if(input_std_params.std == V4L2_STD_1080P_30
      ||input_std_params.std == V4L2_STD_1080P_50
      ||input_std_params.std == V4L2_STD_1080P_60
      ||input_std_params.std == V4L2_STD_1080I_30
      ||input_std_params.std == V4L2_STD_1080I_50
      ||input_std_params.std == V4L2_STD_1080I_60)
        {
     if(flickerHz == 50)
        ctrl.value = 0x3E9;
     else
        ctrl.value = 0x456;
      }
  else
    //     ctrl.value = 0x5F4;
    ctrl.value = 0x2F2;

	if (-1 == ioctl (fdCapture, VIDIOC_S_CTRL, &ctrl)) {
		perror("ioctl:VIDIOC_S_EXPOSURE failed\n");
		return -1;
	}


    int i;
    struct v4l2_fmtdesc fmtdesc;

    printf("------------------------------------------------\n");
    printf("Enumerting supported formats \n");
    for (i=0; ; i++) {
      memset(&fmtdesc,0,sizeof(struct v4l2_fmtdesc));
        fmtdesc.index = i;
	    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	    if (-1 == ioctl (fdCapture, VIDIOC_ENUM_FMT, &fmtdesc)) {
            break;
	         }
        printf("\tpixelformat: %d", fmtdesc.pixelformat);
        printf("\tdescription: %s\n", fmtdesc.description);
    }



    memset(&fmt,0,sizeof(struct v4l2_format));
    
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl (fdCapture, VIDIOC_G_FMT, &fmt)) {
		perror("ioctl:VIDIOC_G_FMT failed\n");
		return -1;
	}
    printf("Default OUTPUT FROM VIDIOC_G_FMT:fmt.fmt.pix...\n");
    printf("\twidth = %d\n", fmt.fmt.pix.width);    
    printf("\theight = %d\n", fmt.fmt.pix.height);
    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8)
        printf("\tpixelformat = SBGGR8 (%d)\n", fmt.fmt.pix.pixelformat);    
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SGBRG8)
        printf("\tpixelformat = SGBGR8 (%d)\n", fmt.fmt.pix.pixelformat);
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SGRBG8)
        printf("\tpixelformat = SGRBG8 (%d)\n", fmt.fmt.pix.pixelformat);    
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SGRBG10)
        printf("\tpixelformat = SGRBG10 (%d)\n", fmt.fmt.pix.pixelformat);
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12)
        printf("\tpixelformat = NV12 (%d)\n", fmt.fmt.pix.pixelformat);    
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY)
        printf("\tpixelformat = UYVY (%d)\n", fmt.fmt.pix.pixelformat);    
    else
        printf("\t********** pixelformat = unknown (%d)\n", fmt.fmt.pix.pixelformat);
    printf("\tbytesperline = %d\n", fmt.fmt.pix.bytesperline);    
    printf("\tsizeimage = %d\n", fmt.fmt.pix.sizeimage);    

#if 0
    do {
        printf("Query the current standard \n");
            ret = ioctl(fdCapture, VIDIOC_QUERYSTD, &cur_std);

            if (ret == -1 && errno == EAGAIN) {
                usleep(1);
                failCount++;
                printf("\ttrying again ...\n");
            }
        } while (ret == -1 && errno == EAGAIN && failCount < 5);
    
   if((cur_std & std) == 0){
        printf("Not support current standard \n");
        exit (0);
        }

	if (-1 == ioctl(fdCapture, VIDIOC_S_STD, &std)) {
		printf("set_data_format:unable to set standard automatically\n");
		return -1;
	} else
		printf("\nS_STD Done\n");

#endif

	printf("\nCalling configCCDCraw()\n");
	ret = configCCDCraw(fdCapture);
	if (ret < 0) {
		perror("configCCDCraw error\n");
		return -1;
	} else {
		printf("\nconfigCCDCraw Done\n");
	}

    int output_format = 1;
    
    //	CLEAR(fmt);
    memset(&fmt,0,sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
    /*	if (output_format) {
		printf("Setting format to V4L2_PIX_FMT_NV12 at capture\n");
        //		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
        //	fmt.fmt.pix.bytesperline = width;
	}
	else {
		printf("Setting format to V4L2_PIX_FMT_UYVY at capture\n");
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        }
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
*/
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR16;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    pixelformat = fmt.fmt.pix.pixelformat;
    //	fmt.fmt.pix.field = V4L2_FIELD_ANY;
      fmt.fmt.pix.field = V4L2_FIELD_NONE;
     ret= 0;
    ret = ioctl(fdCapture, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
        printf("Error:VIDIOC_S_FMT: ret is %d\n",ret);
		perror("set_data_format:ioctl:VIDIOC_S_FMT");
		return -1;
	} else
		printf("\nS_FMT Done\n");
    if (pixelformat != fmt.fmt.pix.pixelformat) {
        printf("ERRRRRRRRRRR\n");
        exit (0);
    }
    
#if 1
    //    CLEAR(fmt);
    memset(&fmt,0,sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl (fdCapture, VIDIOC_G_FMT, &fmt)) {
		perror("ioctl:VIDIOC_G_FMT failed\n");
		return -1;
	}

    printf("After Setting OUTPUT FROM VIDIOC_G_FMT ...\n");
    printf("\twidth = %d\n", fmt.fmt.pix.width);    
    printf("\theight = %d\n", fmt.fmt.pix.height);
    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12)
        printf("\tpixelformat = NV12\n");    
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY)
        printf("\tpixelformat = UYVY\n");    
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8)
        printf("\tpixelformat = SBGGR8 (%d)\n", fmt.fmt.pix.pixelformat);
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR16)
        printf("\tpixelformat = SBGGR8 (%d)\n", fmt.fmt.pix.pixelformat);
    else
        printf(" \t******* pixelformat = unknown\n");
    printf("\tbytesperline = %d\n", fmt.fmt.pix.bytesperline);    
    printf("\tsizeimage = %d\n", fmt.fmt.pix.sizeimage);    
    printf("------------------------------------------------\n");
#endif
#if 0
	if (en_crop) {
		printf("******Cropping the input @%d,%d,%d,%d***********\n", crop_default.top,
			crop_default.left, crop_default.width, crop_default.height);
        //		input_std_params.crop = crop_default;
        //		CLEAR(crop);
        memset(&crop,0,sizeof(struct v4l2_crop));
		crop.c = input_std_params.crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == ioctl(fdCapture, VIDIOC_S_CROP, &crop)) {
			perror("set_data_format:ioctl:VIDIOC_S_CROP");
			return -1;
		} else
			printf("\nS_CROP Done\n");

	}
#endif
	return 0;
}

static int InitCaptureBuffers(int fdCapture, int num_bufs)
{
  struct v4l2_requestbuffers req;

  req.count = num_bufs;//APP_NUM_BUFS/2;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  
  if (-1 == ioctl(fdCapture, VIDIOC_REQBUFS, &req)) {
    perror("InitCaptureBuffers:ioctl:VIDIOC_REQBUFS\n");
    return -1;
  } else
    printf("\nREQBUF Done\n");

  if (req.count != num_bufs) {
    printf("Error:VIDIOC_REQBUFS failed for capture\n");
    printf("\t num_bufs expected is %d,Availed is %d\n",num_bufs,req.count);
    return -1;
  }
  return 0;
}

int start_capture_streaming(int fdCapture,struct buf_info *capture_buffers,int num_buf)
{
	int i = 0;
	enum v4l2_buf_type type;
    if(0 != InitCaptureBuffers(fdCapture,num_buf))
      return -1;
    
	for (i = 0; i < num_buf; i++) {
		struct v4l2_buffer buf;
        //		CLEAR(buf);
        memset(&buf,0,sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.length = capture_buffers[i].buf_size;
		buf.m.userptr = (unsigned long)capture_buffers[i].user_addr;
		printf("Queing buffer:%d\n", i);

		if (-1 == ioctl(fdCapture, VIDIOC_QBUF, &buf)) {
			perror("StartStreaming:VIDIOC_QBUF failed");
			return -1;
		} else
			printf("\nQ_BUF Done\n");

	}
	/* all done , get set go */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(fdCapture, VIDIOC_STREAMON, &type)) {
		perror("StartStreaming:ioctl:VIDIOC_STREAMON:");
		return -1;
	} else
		printf("\nSTREAMON Done\n");

	return 0;
}


int init_camera_capture(int capt_fd,struct capt_std_params capt_params)
{

  int ret = 0;
  struct v4l2_capability cap;

  /*Is capture supported? */
  if (-1 == ioctl(capt_fd, VIDIOC_QUERYCAP, &cap)) {
    perror("init_camera_capture:ioctl:VIDIOC_QUERYCAP:");
    close(capt_fd);
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    printf("InitDevice:capture is not supported on:%s\n",
           CAPTURE_DEVICE);
    close(capt_fd);
    return -1;
  }
  /*is MMAP-IO supported? */
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    printf("InitDevice:IO method MMAP is not supported on:%s\n",
           CAPTURE_DEVICE);
    close(capt_fd);
    return -1;
  }

  printf("setting data format\n");
  if (set_data_format(capt_fd, capt_params) < 0) {
    printf("SetDataFormat failed\n");
    close(capt_fd);
    return -1;
  }
  /*
	printf("initializing capture buffers\n");
	if (InitCaptureBuffers(capt_fd) < 0) {
    printf("InitCaptureBuffers failed\n");
    close(capt_fd);
    return -1;
	}
    ret = start_capture_streaming(capt_fd);
	if (ret) {
    printf("Failed to start capture streaming\n");
    return ret;
    }*/
  printf("Capture initialized\n");
  return capt_fd;
}

int capture_open(void)
{
  int capt_fd;
  if ((capt_fd = open(CAPTURE_DEVICE, O_RDWR | O_NONBLOCK, 0)) <= -1) {

    perror("init_camera_capture:open::");
    return -1;
  }
  return capt_fd;
  
}

int cleanup_capture(int fd)
{
	int err = 0;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
		perror("cleanup_capture :ioctl:VIDIOC_STREAMOFF");
		err = -1;
	}

	if (close(fd) < 0) {
		perror("Error in closing device\n");
		err = -1;
	}
	return err;
}

/*******************************************************************************
 * allocate_user_buffers() allocate buffer using CMEM
 ******************************************************************************/
int allocate_user_buffers(struct buf_info *capture_buffers,int num_buf,unsigned long buf_size)
{
	void *pool;
	int i;
    
	CMEM_AllocParams  alloc_params;
	printf("calling cmem utilities for allocating frame buffers\n");
	CMEM_init();

	alloc_params.type = CMEM_POOL;
	alloc_params.flags = CMEM_NONCACHED;
	alloc_params.alignment = 32;
	pool = CMEM_allocPool(0, &alloc_params);

	if (NULL == pool) {
		printf("Failed to allocate cmem pool\n");
		return -1;
	}
	printf("Allocating capture buffers :buf size = %d \n", buf_size);

	for (i=0; i < num_buf; i++) {
		capture_buffers[i].user_addr = CMEM_alloc(buf_size, &alloc_params);
		if (capture_buffers[i].user_addr) {
			capture_buffers[i].phy_addr = CMEM_getPhys(capture_buffers[i].user_addr);
			if (0 == capture_buffers[i].phy_addr) {
				printf("Failed to get phy cmem buffer address\n");
				return -1;
			}
		} else {
			printf("Failed to allocate cmem buffer\n");
			return -1;
		}
        capture_buffers[i].buf_size = buf_size;
		printf("Got %p from CMEM, phy = %p\n", capture_buffers[i].user_addr,
			(void *)capture_buffers[i].phy_addr);
	}
/*
	printf("Allocating display buffers :buf size = %d \n", buf_size);
	for (i=0; i < APP_NUM_BUFS/2; i++) {
		display_buffers[i].user_addr = CMEM_alloc(buf_size, &alloc_params);
		if (display_buffers[i].user_addr) {
			display_buffers[i].phy_addr = CMEM_getPhys(display_buffers[i].user_addr);
			if (0 == display_buffers[i].phy_addr) {
				printf("Failed to get phy cmem buffer address\n");
				return -1;
			}
		} else {
			printf("Failed to allocate cmem buffer\n");
			return -1;
		}
		printf("Got %p from CMEM, phy = %p\n", display_buffers[i].user_addr,
			(void *)display_buffers[i].phy_addr);
	}
*/
	return 0;
}
