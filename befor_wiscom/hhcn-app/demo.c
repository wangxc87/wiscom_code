#define _FIFO_DVR_TO_QT_ "/opt/dvr_rdk/ti814x/dvr_to_qt_fifo"
#define FIFO_BUF_SIZE 1024
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
//notes: change socket to pipe by wj  0418


#include <demo.h> 
//#include <demo_spl_usecases.h>
#include <demo_vcap_venc_vdec_vdis.h> //villion

#include <linux/input.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
#include <malloc.h>

//#define HELLO_WORLD_SERVER_PORT 7777   
//#define BUFFER_SIZE 1024   

#include "adc_read.h"
//#include "demos/link_api_sd_demo/sd_demo/SD_demo.h"
//#include "demos/link_api_sd_demo/sd_demo/SD_demo.h"

extern char chrLightNum[10];
Vsys_AllocBufInfo bufInfo[2];

Demo_Info gDemo_info = {
	.curDisplaySeqId = VDIS_DISPLAY_SEQID_DEFAULT,
};

int GetFrameBuf()
{
	AlgLink_OsdChWinParams OsdParam;
	int bufAlign = 128;
	int i,status =0;
	int bufsize = 224*30*2;
	status = Vsys_allocBuf(OSD_BUF_HEAP_SR_ID2, bufsize, bufAlign, &bufInfo[0]);
	if(status!=OSA_SOK){
		printf("bufInfo0:Vsys_allocBuf failed ...\n");
		return -1;
	}else
		printf("bufInfo0:Vsys_allocBuf physAddr 0x%x,virtAddr 0x%x.\n",
			bufInfo[0].physAddr,bufInfo[0].virtAddr);
	status = Vsys_allocBuf(OSD_BUF_HEAP_SR_ID2,bufsize,bufAlign,&bufInfo[1]);
	if(status!=OSA_SOK){
		printf("bufInfo1:Vsys_allocBuf failed ...\n");
		return -1;
	}else
		printf("bufInfo1:Vsys_allocBuf physAddr 0x%x,virtAddr 0x%x.\n",
			bufInfo[1].physAddr,bufInfo[1].virtAddr);
		
	memset(&OsdParam,0,sizeof(AlgLink_OsdChWinParams));
	OsdParam.chId = 0;	
	OsdParam.winPrm[0].addr[0][0]= bufInfo[0].physAddr;
	OsdParam.winPrm[1].addr[0][0]= bufInfo[1].physAddr;
	status=System_linkControl(SYSTEM_LINK_ID_ALG_0,
		ALG_LINK_OSD_CMD_GET_FULL_FRAME,
		&OsdParam,sizeof(AlgLink_OsdChWinParams), FALSE);  
	FILE *fd0,*fd1;
	do{
		usleep(10);
	}while(OsdParam.winPrm[0].height);
	
	int height = OsdParam.winPrm[0].height;
	int width = OsdParam.winPrm[0].width;
	printf("%s:GET FrameInfo width %d,height %d,format %d.\n",
		__func__,width,height,OsdParam.winPrm[0].format);
	fd0=fopen("/home/root/chId0.data","w");
	if(fd0 ==NULL){
		printf("EER:Open chId0.data failed.\n");
		status = -1;
		goto out;
		
	}
	fwrite(bufInfo[0].virtAddr,1,height*width,fd0);
	fclose(fd0);
out:	for(i=0;i<2;i++)
	Vsys_freeBuf(OSD_BUF_HEAP_SR_ID,bufInfo[i].virtAddr,bufsize);
	return status;
}
#define SR2_VIRTUAL_ADDR 0x40653380 /*0xaf000000*/
#define SR2_PHYSICAL_ADDR 0xbf7d7380
#define SharedRegion_INVALIDREGIONID    (0xFFFF)

int main()
{
	Bool done = FALSE;
	char ch;
	printf("\nEnter Into Main 20130614++++\n");
	gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
	gDemo_info.audioInitialized = FALSE;

	int remoteId;  //wxc@tech-5d,test id
	remoteId = SharedRegion_getId(SR2_VIRTUAL_ADDR);
	if(remoteId == SharedRegion_INVALIDREGIONID)
		printf("ERR:SharedRegion GetId(virtual) failed.\n");
	else
		printf("SR2 ID(virtual) is 0x%x++++\n",remoteId);
	
	remoteId = SharedRegion_getId(SR2_PHYSICAL_ADDR);
	if(remoteId == SharedRegion_INVALIDREGIONID)
		printf("ERR:SharedRegion GetId(physical) failed.\n");
	else
		printf("SR2 ID(physical) is 0x%x++++\n",remoteId);
	
	Demo_runEx(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE);

	return 0;
}

Int32 Demo_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData)
{
	if (eventId == VSYS_EVENT_VIDEO_DETECT) {
		printf(" \n");
		printf(" DEMO: Received event VSYS_EVENT_VIDEO_DETECT [0x%04x]\n", eventId);

		Demo_captureGetVideoSourceStatus();
	}

	if (eventId == VSYS_EVENT_TAMPER_DETECT) {
		Demo_captureGetTamperStatus(pPrm);
	}

	if (eventId == VSYS_EVENT_MOTION_DETECT) {
		Demo_captureGetMotionStatus(pPrm);
	}

	if (eventId == VSYS_EVENT_DECODER_ERROR) {
		//Demo_decodeErrorStatus(pPrm);
	}
#if USE_FBDEV
	if (eventId == VSYS_EVENT_LIGHT_NUM) //villion
	{

		Demo_DSPLightsCountingStatus(pPrm);
	}
#endif
	return 0;
} 

int Demo_runEx(int demoId)
{
	int status;
	Bool done = FALSE;
	char ch;
	start_recording = FALSE;
	gDemo_info.scdTileConfigInitFlag = FALSE;
	status = Demo_startStop(demoId, TRUE);
	int countlight = 0;
	int countrecording = 0;
	if (status < 0) {
		printf(" WARNING: This demo is NOT curently supported !!!\n");
		return status;
	}

	//sleep(1);

	Demo_displaySettingsEx(demoId);


	int iChannel = 0; //图像
/*	int iPhotonFlag = 0; //计数
	int iGain = 0; //增益
	int iVideo = 0; //视频
	int iOSD = 0; //OSD
	int iVisibleLight = 0; //可见光
	int iUltra = 0; //紫外线


	int callight = 2;
*/
	//Key Define
	int keys_fd;

	struct input_event keyEvent;
	keys_fd = open("/dev/input/event0", O_RDONLY);
	fcntl(keys_fd, F_SETFL, O_NONBLOCK);

	Demo_displaySettingsEx(demoId);
	//EnableGraphic();
	Demo_displayChangeChannel(demoId, 2); //0:可见光通道 1：紫外 2：叠加
	//system("/home/root/saFbdevDisplay");
//	GetFrameBuf();
	while (1) {
		usleep(50);
		if (read(keys_fd, &keyEvent, sizeof(keyEvent)) == sizeof(keyEvent)) {
			if (keyEvent.type == 4) {
				printf("keyEvent.value %d\n", keyEvent.value);

				if (keyEvent.value == 8) //图像/拍照
				{
					GetFrameBuf();
				} else if (keyEvent.value == 9) //紫外
				{
					system("/home/root/saFbdevDisplay");
				} else if (keyEvent.value == 10) //确认
				{
					break;
				} else if (keyEvent.value == 11)//功能
				{
				} else if (keyEvent.value == 16)//光子计数
				{
				} else if (keyEvent.value == 17)//+号
				{
					break;
/*
					//EnableDisplay();
					//DisableGraphic();
					Vdis_start();
					//EnableGraphic();
					Vdec_start();
					Venc_start();
					Vcap_start();
*/

				} else if (keyEvent.value == 25)//-号
				{
					Vcap_stop();
					Venc_stop();
					Vdec_stop();

					//DisableGraphic(); //wxc
					Vdis_stop(); //villion
					//EnableDisplay();
					//EnableGraphic();
				} else if (keyEvent.value == 18)//视频
				{/*
					system("killall -9 MyQt");
					Demo_startStop(demoId, FALSE);
					system("echo 1 > /sys/class/gpio/gpio47/value");

					start_recording = !start_recording;
					gDemo_info.scdTileConfigInitFlag = FALSE;
					status = Demo_startStop(demoId, TRUE);
					system("echo 0 > /sys/class/gpio/gpio47/value");
					Demo_displaySettingsEx(demoId);
					system("/opt/dvr_rdk/ti814x/runsec.sh");
*/
				} else if (keyEvent.value == 19) //增益
				{

				} else if (keyEvent.value == 24) //显示(三屏切换)
				{
					iChannel += 1;
					if (iChannel == 3) iChannel = 0;
					if (iChannel == 0) {
						Demo_displaySettingsEx(demoId);
						Demo_displayChangeChannel(demoId, 0);
						printf("Current Channel is : Visable Light.\n");
					} else if (iChannel == 1) {
						Demo_displaySettingsEx(demoId);
						Demo_displayChangeChannel(demoId, 1);
						printf("Current Channel is : UV Light.\n");
					} else if (iChannel == 2) {
						Demo_displaySettingsEx(demoId);
						Demo_displayChangeChannel(demoId, 2);
						printf("Current Channel is : Composite Light.\n");
					}
				} else if (keyEvent.value == 26) //可见
				{

				} else if (keyEvent.value == 27) //OSD
				{
				}
			}
		}

	}
	close(keys_fd);
	Demo_startStop(demoId, FALSE);
	return 0;
}

int Demo_startStop(int demoId, Bool startDemo)
{

	if (startDemo) {
		gDemo_info.maxVcapChannels = 0;
		gDemo_info.maxVdisChannels = 0;
		gDemo_info.maxVencChannels = 0;
		gDemo_info.maxVdecChannels = 0;

		gDemo_info.audioEnable = FALSE;
		gDemo_info.isAudioPathSet = FALSE;
		gDemo_info.audioCaptureActive = FALSE;
		gDemo_info.audioPlaybackActive = FALSE;
		gDemo_info.audioPlaybackChNum = 0;
		gDemo_info.audioCaptureChNum = 0;
		gDemo_info.osdEnable = FALSE;
		gDemo_info.curDisplaySeqId = VDIS_DISPLAY_SEQID_DEFAULT;
	}

	if (startDemo) {

		gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;

		VcapVencVdecVdis_start(TRUE, TRUE, demoId);
	} else {
		VcapVencVdecVdis_stop();
	}

	if (startDemo) {
		Vsys_registerEventHandler(Demo_eventHandler, NULL);
	}

	return 0;
}

char Demo_getChar()
{
	char buffer[MAX_INPUT_STR_SIZE];

	fflush(stdin);
	fgets(buffer, MAX_INPUT_STR_SIZE, stdin);

	return(buffer[0]);
}

int Demo_getChId(char *string, int maxChId)
{
	char inputStr[MAX_INPUT_STR_SIZE];
	int chId;

	printf(" \n");
	printf(" Select %s CH ID [0 .. %d] : ", string, maxChId - 1);

	fflush(stdin);
	fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

	chId = atoi(inputStr);

	if (chId < 0 || chId >= maxChId) {
		chId = 0;

		printf(" \n");
		printf(" WARNING: Invalid CH ID specified, defaulting to CH ID = %d \n", chId);
	} else {
		printf(" \n");
		printf(" Selected CH ID = %d \n", chId);
	}

	printf(" \n");

	return chId;
}

int Demo_getIntValue(char *string, int minVal, int maxVal, int defaultVal)
{
	char inputStr[MAX_INPUT_STR_SIZE];
	int value;

	printf(" \n");
	printf(" Enter %s [Valid values, %d .. %d] : ", string, minVal, maxVal);

	fflush(stdin);
	fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

	value = atoi(inputStr);

	if (value < minVal || value > maxVal) {
		value = defaultVal;
		printf(" \n");
		printf(" WARNING: Invalid value specified, defaulting to value of = %d \n", value);
	} else {
		printf(" \n");
		printf(" Entered value = %d \n", value);
	}

	printf(" \n");

	return value;
}

Bool Demo_getFileWriteEnable()
{
	char inputStr[MAX_INPUT_STR_SIZE];
	Bool enable;

	printf(" \n");
	printf(" Enable file write (YES - y / NO - n) : ");

	inputStr[0] = 0;

	fflush(stdin);
	fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

	enable = FALSE;

	if (inputStr[0] == 'y' || inputStr[0] == 'Y') {
		enable = TRUE;
	}

	printf(" \n");
	if (enable)
		printf(" File write ENABLED !!!\n");
	else
		printf(" File write DISABLED !!!\n");
	printf(" \n");
	return enable;
}

Bool Demo_isPathValid(const char* absolutePath)
{

	if (access(absolutePath, F_OK) == 0) {

		struct stat status;
		stat(absolutePath, &status);

		return(status.st_mode & S_IFDIR) != 0;
	}
	return FALSE;
}

int Demo_getFileWritePath(char *path, char *defaultPath)
{
	int status = 0;

	printf(" \n");
	printf(" Enter file write path : ");

	//fflush(stdin);
	//fgets(path, MAX_INPUT_STR_SIZE, stdin);

	printf(" \n");
	strcpy(path, "/home/video/"); //villion
	/* remove \n from the path name */
	path[ strlen(path) - 1 ] = 0;



	if (!Demo_isPathValid(path)) {
		printf(" WARNING: Invalid path [%s], trying default path [%s] ...\n", path, defaultPath);

		strcpy(path, defaultPath);

		if (!Demo_isPathValid(path)) {
			printf(" WARNING: Invalid default path [%s], file write will FAIL !!! \n", path);

			status = -1;
		}
	}

	if (status == 0) {
		printf(" Selected file write path [%s] \n", path);
	}

	printf(" \n");

	return 0;


}

