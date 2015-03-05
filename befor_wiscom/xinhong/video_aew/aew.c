#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/* Kernel header file, prefix path comes from makefile */
#include <media/davinci/dm365_aew.h>
#include <media/davinci/dm365_a3_hw.h>
#include <linux/videodev2.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/vpfe_types.h>
#include <media/davinci/dm365_ccdc.h>
#include "aew.h"


#define DRIVER_NAME	"/dev/dm365_aew"
#define U4_9_INT(D_IN)	((((unsigned short) D_IN) & 0X1E00) >> 9)
#define U4_9_DEC(D_IN)	(((unsigned short )D_IN) & 0X01FF)


int aew_config(int fd,int *buf_size)
{
	struct aew_configuration *config;
	int result;
	
	/* Allocate Memory For Configuration Structure */
	config = (struct aew_configuration *)
            malloc(sizeof(struct aew_configuration));
	/* Configure Window Parameters */
	config->window_config.width = 32;
	config->window_config.height = 32;
	config->window_config.hz_line_incr = 4;
	config->window_config.vt_line_incr = 4;
	config->window_config.vt_cnt = 32;//8;
	config->window_config.hz_cnt = 32;//8;
	config->window_config.vt_start =32;
	config->window_config.hz_start = 32;
	/* Configure black window parameter */
	config->blackwindow_config.height = 2;
	config->blackwindow_config.vt_start =0;

//    config->out_format == AEW_OUT_SUM_ONLY;
    
	/* Enable ALaw */
	config->alaw_enable = H3A_AEW_DISABLE;//EH3A_AEW_ENABLE
	/* Set Saturation limit */
	config->saturation_limit = 1023;
	/* Call AEW_S_PARAM to set Parameters */
//	printf("\n error no instance 1 %d", errno);
	result = ioctl(fd, AEW_S_PARAM, config);
    if(result < 0){
            printf("ioctl AEW_S_PARAM fail \n");
            return -1;
	}
    else{
            printf("\n IOCTL S_PARAM Return Value : %d", result);
//            printf("\n error no %d", errno);
            *buf_size = result;
    }

	ioctl(fd, AEW_DISABLE);
	if (config)
		free(config);
	/* Close Driver File */
	/* Call AEW_ENABLE to enable AEW Engine */
/*	result = ioctl(fd, AEW_ENABLE);
	printf("\n IOCTL AEW_ENABLE  : %d", result);
*/
//	close(fd);
	return 0;
}

static int aew_read(int fd,void *buffer,int buff_size)
{
        int result;
        
        if (buff_size < 0) {
                printf("\n Buffer size is invalid");
                return -1;
        }

        if (buffer == NULL) {
                printf("\n Alloction in failure");
                return -1;
        }

        /* Enable the engine */
        if (ioctl(fd, AEW_ENABLE) < 0) {
                printf("ioctl AEW_ENABLE fail. \n");
                return -1;
        }

        result = read(fd, (void *)buffer, buff_size);
        printf("\n RESULT OF READ %d", result);
        if (result < 0) {
                printf("\n Read Failed");
                ioctl(fd, AEW_DISABLE);
                return -1;
        }
        ioctl(fd, AEW_DISABLE);

        return 0;;
    
}
/*
struct aew_sum 
{
        unsigned short sub_samples0;
        unsigned short sub_samples1;
        unsigned short sub_samples2;
        unsigned short sub_samples3;
        unsigned short saturator0;
        unsigned short saturator1;
        unsigned short saturator2;
        unsigned short saturator3;
        unsigned int  sum0;
        unsigned int sum1;
        unsigned int sum2;
        unsigned int sum3;
};
*/
        
int aew_ctrl(int fd,struct prev_wb *wb)
{
        int ret;
        int buf_size;
        int win_number,pixel_per_win,pixel_number;
        struct aew_configuration *config;
        long sub_samp0,sub_samp1,sub_samp2,sub_samp3;
        void *buf;
        config = (struct aew_configuration *)malloc(sizeof(struct aew_configuration));
        ret = ioctl(fd,AEW_G_PARAM,config);
        if(ret < 0){
                printf("IOCTL AEW_GPARAM Fail..\n");
                free(config);
                return -1;
        }
        buf_size = ret;
        buf = (void *)malloc(buf_size);
        if (buf == NULL) {
                printf("\n Alloction in failure");
                free(config);
                return -1;
        }


        win_number = config->window_config.vt_cnt * config->window_config.hz_cnt;
        pixel_per_win = (config->window_config.width /config->window_config.hz_line_incr)
                * (config->window_config.height /config->window_config.vt_line_incr);
        pixel_number = win_number * pixel_per_win;
        free(config);
        
/*        struct aew_sum *aew_sum;
          aew_sum = (struct aew_sum *)malloc(sizeof(strcut aew_sum)*win_number);*/
        ret = aew_read(fd,buf,buf_size);
        if(0 > ret){
                printf("AEW read Fail...\n");
                free(buf);
                return -1;
        }
        int i,j;
        unsigned short *tmp;
        int grey;
        float k0,k1,k2,k3;
        int average0,average1,average2,average3;

        
        tmp = (unsigned short *)buf;
        sub_samp0 = 0;
        sub_samp1 = 0;
        sub_samp2 = 0;
        sub_samp3 = 0;
        
        for(i=0;i<buf_size;){
                sub_samp0 += *(tmp++);
                sub_samp1 += *(tmp++);
                sub_samp2 += *(tmp++);
                sub_samp3 += *tmp;
                tmp += 12;
                i += 32;
        }
        average0 = sub_samp0 /pixel_number;
        average1 = sub_samp1 /pixel_number;
        average2 = sub_samp2 /pixel_number;
        average3 = sub_samp3 /pixel_number;
        grey = (average0 + average1 + average2 +average3)/4;
        k0 = (grey *1.0) /average0;
        k1 = (grey *1.0) /average1;
        k2 = (grey *1.0) /average2;
        k3 = (grey *1.0) /average3;
        printf("\ngrey %d,average0 %d,averag1 %d,averag2 %d,average3 %d",grey,average0,average1,average2,average3);
        
        printf("\nk0 %f,k1 %f,k2 %f,k3 %f\n",k0,k1,k2,k3);

//        k2 = k3=k1 = k0 =1;
        
        int gain =1024;        
        wb->gain_gr.integer = U4_9_INT(k0 *gain);
        wb->gain_gr.decimal = U4_9_DEC(k0 *gain);
        
        wb->gain_r.integer = U4_9_INT(k1*gain);
        wb->gain_r.decimal = U4_9_DEC(k1*gain);

        wb->gain_b.integer = U4_9_INT(k2*gain);
        wb->gain_b.decimal = U4_9_DEC(k2*gain);

        wb->gain_gb.integer = U4_9_INT(k3*gain);
        wb->gain_gb.decimal = U4_9_DEC(k3*gain);

        free(buf);
        return 0;
        
}


                        
                                                    

int aew_save(int fd,int buff_size)
{
	unsigned short int *k11;
    int result;
    int count = 0;
	FILE *fp1;
    void *buffer_stat;
	fp1 = fopen("aew_samp_dm365.txt", "w");
	printf("\n Buffer size s %d", buff_size);
	if (buff_size < 0) {
		printf("\n error in StartLoop");
		printf("\n Buffer size is invalid");
		return -1;
	}

	printf("\n Allocating the buffer");
	buffer_stat = (void *)malloc(buff_size);
	if (buffer_stat == NULL) {
		printf("\n error in StartLoop");
		printf("\n Alloction in failure");
		return -1;
	}

	/* Enable the engine */
	if (ioctl(fd, AEW_ENABLE) < 0) {
		printf("\n error in StartLoop");
		printf("ioctl AEW_ENABLE fail. \n");
		return -1;
	}

	result = read(fd, (void *)buffer_stat, buff_size);
	printf("\n RESULT OF READ %d", result);
	if (result < 0) {
		printf("\n error in StartLoop");
		printf("\n Read Failed");
        ioctl(fd, AEW_DISABLE);
        return -1;
        
	}
	ioctl(fd, AEW_DISABLE);

	if (result > 0) {
		printf("\n Output File : aew_samp_dm365.txt");

		k11 = (unsigned short int *)buffer_stat;

		for (count = 1; count <= buff_size / 2; count++) {
			fprintf(fp1, "%d  ", (unsigned short int)k11[count-1]);
            if(0 == count%4)
                    fprintf(fp1,"\n");

            if(0 == count%16)
                    fprintf(fp1,"\n");
            
			/*printf("\n@%d@",(unsigned short int)k11[count]);*/
		}

	} else {
		printf("\n error in StartLoop");
		printf("\nNOT ABLE TO READ");
	}
    printf("\n");
    
	return 0;
}

int aew_open(void)
{
	int fd;
	/* Open AEW Driver */
	fd = open(DRIVER_NAME, O_RDWR);
	if (fd < 0) {
		printf("\n Error in opening device file");
		return -1;
	}
return fd;
}
