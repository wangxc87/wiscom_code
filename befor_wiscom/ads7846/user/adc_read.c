#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define ADC_DEV  "/dev/ads7846_adc"
/*
#define ADC_OFFSET 1608
#define ADC_RATE 1598
*/
#define ADC_OFFSET 1610
#define ADC_RATE 955
#define LOOP 20
#define ADJ 2
static float voltage_adjust(int data)
{
        int offset = ADC_OFFSET;
        int k = ADC_RATE;
        float vout =0.0;
        vout =data - offset;
        vout =vout*1.00/k;
        return vout;
        
}
 int  adc_open(void)
{
        int fd;
        fd = open(ADC_DEV,O_RDWR);
        
        if(fd < 0){
                printf("Err:open %s failed..\n",ADC_DEV);
                return -1;
        }
        return fd;
        
}
 int adc_read(int fd,int *buf)
{
        int loop = LOOP;
        int tmp,adc_data,total;
        float *vout;
        int k = ADJ;
	int ret = 0;
        while(loop--){
                ret = read(fd,&adc_data,1);
                if(ret != sizeof(unsigned int)){
                        printf("Err:read failed...\n");
                        return -1;
                }
                total +=adc_data;
                usleep(10);
        }
        tmp = total/LOOP;
        loop =LOOP;
        total = 0;
        *buf = voltage_adjust(tmp) *k;
        return 0;
}
 int adc_close(int fd)
{
        close(fd);
        return 0;
}


int main(void)
{
        int ret;
        int fd;
        int adc_data;
        int flags;
        int quit =1;

        int loop =20;
        int total =0;
        int tmp;
        
        fd = open(ADC_DEV,O_RDWR);
        
        if(fd < 0){
                printf("Err:open %s failed..\n",ADC_DEV);
                return -1;
        }
        
        while(quit){
                while(loop--){
                        ret = read(fd,&adc_data,1);
                        if(ret != sizeof(unsigned int)){
                                printf("Err:read failed...\n");
                                goto err_out;
                        }
                        total +=adc_data;
                        usleep(50000);
                        printf("%d\t",adc_data);
                        
                }
                tmp = total/20;
                
                printf("\nAverage Data read is %d\n",tmp);
                loop =20;
                total = 0;
                
                printf("Vin is %fv\n\n",voltage_adjust(tmp));
                sleep(1);
                
        }
        close(fd);
        return 0;
err_out:
        close(fd);
        return -1;
}
