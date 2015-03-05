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

char *dev_name_prev = "/dev/davinci_previewer";

int preview_open(void)
{
  int preview_fd;
  int mode = O_RDWR;
  preview_fd = open(dev_name_prev, mode);
	if(preview_fd <= 0) {
		printf("Cannot open previewer device\n");
        return -1;
	}
    return preview_fd;
}


int preview_config(int preview_fd,unsigned long user_mode,void *config,int chain)
{
  int oper_mode;
  struct prev_channel_config prev_chan_config;
  struct prev_single_shot_config prev_ss_config,*pss;
  struct prev_continuous_config pre_con_config,*pcon;
  if(IMP_MODE_SINGLE_SHOT == user_mode)
    pss = (struct prev_single_shot_config *)config;
  else 
    pcon = (struct prev_continuous_config *)config;
  
	if (ioctl(preview_fd, PREV_S_OPER_MODE, &user_mode) < 0) {
		perror("Can't set operation mode\n");
		goto preview_clean;
	}
	if (ioctl(preview_fd, PREV_G_OPER_MODE, &oper_mode) < 0) {
		perror("Can't get operation mode\n");
		goto preview_clean;
	}

	if (oper_mode == IMP_MODE_SINGLE_SHOT) 
		printf("Operating mode changed successfully to single shot in previewer\n");
	else 
		printf("Operating mode changed successfully to continue in previewer\n");



	printf("Setting default configuration in previewer\n");
	prev_chan_config.oper_mode = oper_mode;
	prev_chan_config.len = 0;
	prev_chan_config.config = NULL; /* to set defaults in driver */
	if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) {
		perror("Error in setting default configuration\n");
		goto preview_clean;
	}
	printf("default configuration setting in previewer successfull\n");
    if(IMP_MODE_SINGLE_SHOT == oper_mode)
    {
      prev_chan_config.oper_mode = oper_mode;
      prev_chan_config.len = sizeof(struct prev_single_shot_config);
      prev_chan_config.config = &prev_ss_config;
      if (ioctl(preview_fd, PREV_G_CONFIG, &prev_chan_config) < 0) {
		perror("Error in getting configuration from driver\n");
		goto preview_clean;
      }
    if(prev_ss_config.output.pix_fmt == IPIPE_YUV420SP)
      printf("\tprev_ss_config.output.pix_fmt = IPIPE_YUV420SP\n");
    else if(prev_ss_config.output.pix_fmt == IPIPE_UYVY)
      printf("\tprev_ss_config.output.pix_fmt = IPIPE_UYVY\n");
    else
      printf("\t-----prev_ss_config.output.pix_fmt = Unknown pix_fmt");
    


	
      prev_chan_config.oper_mode = oper_mode;
      prev_chan_config.len = sizeof(struct prev_single_shot_config);
      prev_chan_config.config = &prev_ss_config;

      prev_ss_config.input.image_width = pss->input.image_width;
      prev_ss_config.input.image_height = pss->input.image_height;
      prev_ss_config.input.ppln= pss->input.image_width * 1.5;
      prev_ss_config.input.lpfr = pss->input.image_height + 10;
      /*
      prev_ss_config.input.vst = 0;
      prev_ss_config.input.hst = 0;
      */
      /*if (format_flag == 0) {
		prev_ss_config.input.pix_fmt = IPIPE_BAYER;
		prev_ss_config.input.data_shift = IPIPEIF_5_1_BITS11_0;
        }
        else if (format_flag == 1)
		prev_ss_config.input.pix_fmt = IPIPE_BAYER_8BIT_PACK_ALAW;
        else if (format_flag == 2)
		prev_ss_config.input.pix_fmt = IPIPE_BAYER_8BIT_PACK_DPCM;
      */
      if(0 == pss->input.pix_fmt )
      	prev_ss_config.input.pix_fmt = IPIPE_BAYER;
      else
        prev_ss_config.input.pix_fmt = pss->input.pix_fmt;

      prev_ss_config.input.data_shift = IPIPEIF_5_1_BITS11_0;

      if(0 == chain)
        //  prev_ss_config.output.pix_fmt = pss->input.pix_fmt;
        prev_ss_config.bypass = 0;
      
/*
	Gr R
	B Gb
*/

	/* Some reason, MT9P031 color pattern is flipped horizontally
	 * for some of the scan resolution
	 */
      int regular_col_pat = 2;
      switch(regular_col_pat){
      case 0:
            prev_ss_config.input.colp_elep= IPIPE_BLUE;       // b gb
            prev_ss_config.input.colp_elop= IPIPE_GREEN_BLUE; // gr r
            prev_ss_config.input.colp_olep= IPIPE_GREEN_RED;
            prev_ss_config.input.colp_olop= IPIPE_RED;
            break;
      case 1:
            prev_ss_config.input.colp_elep= IPIPE_GREEN_BLUE;  // gb b 
            prev_ss_config.input.colp_elop= IPIPE_BLUE;        // r gr
            prev_ss_config.input.colp_olep= IPIPE_RED;
            prev_ss_config.input.colp_olop= IPIPE_GREEN_RED;
            break;
      case 2:
            prev_ss_config.input.colp_elep= IPIPE_GREEN_RED;// gr r
            prev_ss_config.input.colp_elop= IPIPE_RED;      // b  gb
            prev_ss_config.input.colp_olep= IPIPE_BLUE;
            prev_ss_config.input.colp_olop= IPIPE_GREEN_BLUE;
            break;
      case 3:
              prev_ss_config.input.colp_elep= IPIPE_GREEN_RED;// r   gr
              prev_ss_config.input.colp_elop= IPIPE_RED;      // gb  b
              prev_ss_config.input.colp_olep= IPIPE_BLUE;
              prev_ss_config.input.colp_olop= IPIPE_GREEN_BLUE;
              break;

	}

      printf("Prev_ss_config.input color...\n");
      printf("---IPIE_RED 0 IPIE_GREEN_RED 1 IPIE_GREEN_BULE  IPIE_BLUE 3--\n");
      
      printf("\tprev_ss_config.input.colp_elep = %d\n",prev_ss_config.input.colp_elep);
      printf("\tprev_ss_config.input.colp_elop = %d\n",prev_ss_config.input.colp_elop);
      printf("\tprev_ss_config.input.colp_olep = %d\n",prev_ss_config.input.colp_olep);
      printf("\tprev_ss_config.input.colp_olop = %d\n",prev_ss_config.input.colp_olop);
      

      
    }
    
	if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) {
		perror("Error in setting default configuration\n");
		goto preview_clean;
        }
    return 0;
 preview_clean:
    return -1;
}



int preview_querybuf(int preview_fd,struct imp_reqbufs req_buf,struct imp_buffer *buf_in)
{
  int buf_type = req_buf.buf_type ;
  int num_bufs = req_buf.count ;
   int i;
  
  if (ioctl(preview_fd, PREV_REQBUF, &req_buf) < 0) {
    perror("Error in PREV_REQBUF for IMP_BUF_IN\n");
    return -1;
  } 
		
  for (i = 0; i < num_bufs; i++) {
    buf_in[i].index = i;
    buf_in[i].buf_type = buf_type;
    if (ioctl(preview_fd, PREV_QUERYBUF, &buf_in[i]) < 0) {
      perror("Error in PREV_QUERYBUF for IMP_BUF_IN\n ");
      return -1;
    }
  }
  printf("PREV_QUERYBUF successful for IMP_BUF_IN\n");
  return 0;
}
 
int preview_convert(int preview_fd,struct imp_convert convert)
{
  /*    memcpy(input_buffer,in_buf,(WIDTH*HEIGHT*BYTESPERLINE));
	convert.in_buff.buf_type = IMP_BUF_IN;
	convert.in_buff.index = 0;
	convert.in_buff.offset = buf_in[0].offset;
	convert.in_buff.size = buf_in[0].size;
	convert.out_buff1.buf_type = IMP_BUF_OUT1;
	convert.out_buff1.index = 0;
	convert.out_buff1.offset = buf_out1[0].offset;
	convert.out_buff1.size = buf_out1[0].size;
	if (second_out_en) {
		convert.out_buff2.buf_type = IMP_BUF_OUT2;
		convert.out_buff2.index = 0;
		convert.out_buff2.offset = buf_out2[0].offset;
		convert.out_buff2.size = buf_out2[0].size;
        }*/
	if (ioctl(preview_fd, PREV_PREVIEW, &convert) < 0) {
		perror("Error in doing preview\n");
        return -1;
	}
    return 0;
}
                    
int wb_set(int preview_fd,struct prev_wb *wb,int flag)
{
        int ret = 0;
        
        struct prev_module_param mod_param;
//        struct prev_wb wb;
        struct prev_cap cap;
        cap.index =0 ;

        ret = ioctl(preview_fd , PREV_ENUM_CAP, &cap);
        if (ret < 0) {
                return -1;
        }
        // find the defaults for this module
        cap.module_id = PREV_WB;
        strcpy(mod_param.version,cap.version);
        mod_param.module_id = cap.module_id;


        //wb.wb.wb2_dgn = global_gain;
/*        wb.gain_r.integer = U4_9_INT(red_gain);
        wb.gain_r.decimal = U4_9_DEC(red_gain);


        wb.gain_gr.integer = U4_9_INT(green_gain);
        wb.gain_gr.decimal = U4_9_DEC(green_gain);


        wb.gain_gb.integer =  U4_9_INT(green_gain);
        wb.gain_gb.decimal = U4_9_DEC(green_gain);


        wb.gain_b.integer =  U4_9_INT(blue_gain);
        wb.gain_b.decimal = U4_9_DEC(blue_gain);


        wb.ofst_r = wb_table_calc[4];
        wb.ofst_gr = wb_table_calc[5];
        wb.ofst_gb = wb_table_calc[6];
        wb.ofst_b = wb_table_calc[7];
*/
        mod_param.len = sizeof(struct prev_wb);
        mod_param.param = wb;
        switch (flag){
        case 0:
                 if (ioctl(preview_fd, PREV_G_PARAM, &mod_param) < 0) {
                        printf("Error in Getting %s params from driver\n", cap.module_name);
  
                        return -1;
                
                }
                 break;
        case 1:
                if (ioctl(preview_fd, PREV_S_PARAM, &mod_param) < 0) {
                        printf("Error in Setting %s params from driver\n", cap.module_name);
  
                        return -1;
                
                }
                break;
        default:
                printf("flag value invalid..Input 0 or 1");
                break;
                
        }
        
//        printf("gain  G=%d    R=%d    B=%d\n",green_gain,red_gain,blue_gain);

        return ret;
}

