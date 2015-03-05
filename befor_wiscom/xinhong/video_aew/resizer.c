#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>


#ifdef DEBUG_resizer
#define  resizer_debug(fmt, args...) printf("%s: " fmt,__func__,## args)
#else
#define  resizer_debug(fmt, args...)
#endif

char *dev_name_rsz = "/dev/davinci_resizer";

int resizer_open(void)
{
  int resizer_fd;
  int mode = O_RDWR;
  resizer_fd = open(dev_name_rsz, mode);
  if(resizer_fd <= 0) {
    perror("Cannot open resizer device\n");
    return -1;
  }
  return resizer_fd;
}
/*chain : 1  from preview ; 0 from DDR*/
int resizer_config(int resizer_fd,unsigned long user_mode,void *config,int chain)
{
  unsigned long oper_mode;
  struct rsz_channel_config rsz_chan_config; // resizer channel config
  struct rsz_single_shot_config rsz_ss_config,*pss; // single shot mode configuration
  struct rsz_continuous_config rsz_cont_config,*pcon;
  
  if (ioctl(resizer_fd,RSZ_S_OPER_MODE, &user_mode) < 0) {
    perror("Can't set operation mode\n");
    goto resize_clean;
  }
  if (ioctl(resizer_fd, RSZ_G_OPER_MODE, &oper_mode) <0)  {
      perror("Can't get operation mode\n");
      goto resize_clean;
  }
    if (oper_mode == IMP_MODE_SINGLE_SHOT)
    {
      pss = (struct rsz_single_shot_config *)config;
      printf("RESIZER: Operating mode changed successfully to Single Shot\n");
    }
    else {
      pcon = (struct rsz_continuous_config *)config;
      printf("RESIZER: Operating mode changed successfully to Continous\n");
    }
    
    resizer_debug("Setting default configuration in Resizer\n");
    bzero(&rsz_ss_config, sizeof(struct rsz_single_shot_config));
    rsz_chan_config.oper_mode = oper_mode;
    rsz_chan_config.chain = chain;
    rsz_chan_config.len = 0;
    rsz_chan_config.config = NULL; /* to set defaults in driver */
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) {
      perror("Error in setting default configuration for single shot mode\n");
      goto resize_clean;
    }




    if(IMP_MODE_SINGLE_SHOT == oper_mode)
    {
      resizer_debug("default configuration setting in Resizer successfull\n");
      
      bzero(&rsz_ss_config, sizeof(struct rsz_single_shot_config));
      rsz_chan_config.oper_mode = oper_mode;
      rsz_chan_config.chain = chain;
      rsz_chan_config.len = sizeof(struct rsz_single_shot_config);
      rsz_chan_config.config = &rsz_ss_config;

      if (ioctl(resizer_fd, RSZ_G_CONFIG, &rsz_chan_config) < 0) {
        perror("Error in getting resizer channel configuration from driver\n");
        goto resize_clean;
      }
      resizer_debug("default values at driver\n");
      resizer_debug("\trsz_ss_config.output1.enable = %d\n", rsz_ss_config.output1.enable);
      resizer_debug("\trsz_ss_config.output1.width = %d, \n", rsz_ss_config.output1.width);
      resizer_debug("\trsz_ss_config.output1.height = %d\n", rsz_ss_config.output1.height);
      resizer_debug("\trsz_ss_config.output2.enable = %d\n", rsz_ss_config.output1.enable);
      resizer_debug("\trsz_ss_config.output2.width = %d\n", rsz_ss_config.output1.width);
      resizer_debug("\trsz_ss_config.output2.height = %d\n", rsz_ss_config.output1.height);
      if(rsz_ss_config.input.pix_fmt == IPIPE_YUV420SP)
        resizer_debug("\trsz_ss_config.input.pix_fmt = IPIPE_YUV420SP\n");
      else if(rsz_ss_config.input.pix_fmt == IPIPE_UYVY)
        resizer_debug("\trsz_ss_config.input.pix_fmt = IPIPE_UYVY\n");
      else
        resizer_debug("\t-----rsz_ss_config.input.pix_fmt = Unknown pix_fmt");
    
      // in the chain mode only output configurations are valid
      // input params are set at the previewer
      /*    printf("Changing output width to %d\n", width);
            printf("Changing output height to %d\n", height);
      */
    
      rsz_ss_config.output1.pix_fmt = pss->output1.pix_fmt;
      if(0 == rsz_ss_config.output1.pix_fmt)
        rsz_ss_config.output1.pix_fmt = IPIPE_YUV420SP;
      rsz_ss_config.output1.enable = 1;
      rsz_ss_config.output1.width  = pss->output1.width;
      rsz_ss_config.output1.height = pss->output1.height;
      //    rsz_ss_config.output1.h_flip = pss->output1.h_flip;
      //    rsz_ss_config.output1.v_flip = pss->output1.v_flip;

      rsz_ss_config.output2.enable = pss->output2.enable;
      rsz_ss_config.output2.width  = pss->output2.width;
      rsz_ss_config.output2.height = pss->output2.height;
      rsz_ss_config.output2.pix_fmt = pss->output2.pix_fmt;
      if(0  == rsz_ss_config.output2.pix_fmt)
        rsz_ss_config.output2.pix_fmt = IPIPE_UYVY;
      rsz_ss_config.output2.h_flip = pss->output2.h_flip;
      rsz_ss_config.output2.v_flip = pss->output2.v_flip;
      if(0 == chain){
        rsz_ss_config.input.image_width = pss->input.image_width;
        rsz_ss_config.input.image_height = pss->input.image_height;
        rsz_ss_config.input.ppln = rsz_ss_config.input.image_width + 8;
        rsz_ss_config.input.lpfr = rsz_ss_config.input.image_height + 10;
        rsz_ss_config.input.pix_fmt = pss->input.pix_fmt;
        if(0  == rsz_ss_config.input.pix_fmt)
          rsz_ss_config.input.pix_fmt = IPIPE_UYVY;
      }
    
      rsz_chan_config.oper_mode = oper_mode;
      rsz_chan_config.chain = chain;
      rsz_chan_config.len = sizeof(struct rsz_single_shot_config);
    }
    else
    {
      	// set configuration to chain resizer with preview
	resizer_debug("default configuration setting in Resizer successfull\n");
	bzero(&rsz_cont_config, sizeof(struct rsz_continuous_config));
	rsz_chan_config.oper_mode = user_mode;
	rsz_chan_config.chain = 1;
	rsz_chan_config.len = sizeof(struct rsz_continuous_config);
	rsz_chan_config.config = &rsz_cont_config;

	if (ioctl(resizer_fd, RSZ_G_CONFIG, &rsz_chan_config) < 0) {
		perror("Error in getting resizer channel configuration from driver\n");
        goto resize_clean;
        
	}

	// we can ignore the input spec since we are chaining. So only
	// set output specs
	rsz_cont_config.output1.enable = pcon->output1.enable;

    rsz_cont_config.output2.enable = pcon->output2.enable;
    rsz_cont_config.output2.width  = pcon->output2.width;
    rsz_cont_config.output2.height = pcon->output2.height;
    
    rsz_cont_config.output2.pix_fmt = pcon->output2.pix_fmt;
    if(0 == rsz_cont_config.output2.pix_fmt)
      rsz_cont_config.output2.pix_fmt = IPIPE_UYVY;//IPIPE_YUV420SP

    //	rsz_cont_config.output1.h_flip = 1;
	rsz_chan_config.oper_mode = user_mode;
	rsz_chan_config.chain = 1;
	rsz_chan_config.len = sizeof(struct rsz_continuous_config);
	rsz_chan_config.config = &rsz_cont_config;
    }
    
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) {
      perror("Error in setting default configuration for single shot mode\n");
      goto resize_clean;
    }



    bzero(&rsz_ss_config, sizeof(struct rsz_single_shot_config));
    rsz_chan_config.oper_mode = oper_mode;
    rsz_chan_config.chain = chain;
    rsz_chan_config.len = sizeof(struct rsz_single_shot_config);
    rsz_chan_config.config = &rsz_ss_config;

    if (ioctl(resizer_fd, RSZ_G_CONFIG, &rsz_chan_config) < 0) {
      perror("Error in getting resizer channel configuration from driver\n");
      goto resize_clean;
    }
    resizer_debug("Modified values at driver\n");
    resizer_debug("\trsz_ss_config.output1.enable = %d\n", rsz_ss_config.output1.enable);
    resizer_debug("\trsz_ss_config.output1.width = %d, \n", rsz_ss_config.output1.width);
    resizer_debug("\trsz_ss_config.output1.height = %d\n", rsz_ss_config.output1.height);
    resizer_debug("\trsz_ss_config.output2.enable = %d\n", rsz_ss_config.output1.enable);
    resizer_debug("\trsz_ss_config.output2.width = %d\n", rsz_ss_config.output1.width);
    resizer_debug("\trsz_ss_config.output2.height = %d\n", rsz_ss_config.output1.height);
    if(rsz_ss_config.input.pix_fmt == IPIPE_YUV420SP)
      resizer_debug("\trsz_ss_config.input.pix_fmt = IPIPE_YUV420SP\n");
    else if(rsz_ss_config.input.pix_fmt == IPIPE_UYVY)
      resizer_debug("\trsz_ss_config.input.pix_fmt = IPIPE_UYVY\n");
    else
      resizer_debug("\t-----rsz_ss_config.input.pix_fmt = Unknown pix_fmt");
    
    return 0;
resize_clean:
    return -1;
}


  int resizer_convert(int resizer_fd,struct imp_convert convert)
{
  /*	convert.in_buff.buf_type = IMP_BUF_IN;
	convert.in_buff.index = -1;
	convert.in_buff.offset = (unsigned int)input_buffer.user_addr;
	convert.in_buff.size = size;
	convert.out_buff1.buf_type = IMP_BUF_OUT1;
	convert.out_buff1.index = -1;
	convert.out_buff1.offset = (unsigned int)output1_buffer.user_addr;
	convert.out_buff1.size = output1_size;
	if (second_output) {
		bzero(output2_buffer.user_addr, output2_size);
		convert.out_buff2.buf_type = IMP_BUF_OUT2;
		convert.out_buff2.index = -1;
		convert.out_buff2.offset = (unsigned int)output2_buffer.user_addr;
		convert.out_buff2.size = output2_size;
        }*/
	if (ioctl(resizer_fd, RSZ_RESIZE, &convert) < 0) {
		perror("Error in doing preview\n");
		
        return -1;
	} 
    return 0;
}













