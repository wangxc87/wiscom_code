/*
 * demo: audio loopback
 *
 * audio_loopback application used for the LSP demo
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>
#include<string.h>
#include<sys/ioctl.h>
#include<linux/soundcard.h>
#include<poll.h>
#include <interf_enc.h>
#include "mkv_mix.c"


#define PLAYBACK 0
#define CAPTURE 1
int rc;

int buffer_size;

snd_pcm_t *handle[2];
snd_pcm_hw_params_t *params;
unsigned int val;
int dir;
snd_pcm_uframes_t frames;

pthread_t recorder_thread, player_thread;
int pipe_desc[2];

void *recorder(void *arg);
void *player(void *arg);
void rdfile(void);
void wrtomkvfile(void);
void *amr=NULL;
unsigned char wavhead[]={
//00000000   52 49 46 46  C4 D2 1E 00  57 41 56 45  66 6D 74 20  10 00 00 00  RIFF....WAVEfmt ....
//00000014   01 00 01 00  80 3E 00 00  00 7D 00 00  02 00 10 00  64 61 74 61
    0x52,0x49,0x46,0x46,0xc4,0xd2,0x1e,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,0x10,0x00,0x00,0x00,
	0x01,0x00,0x01,0x00,0x80,0x3e,0x00,0x00,0x00,0x7d,0x00,0x00,0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,
	0x00,0x00,0x00,0x00
	};
typedef struct { 
	unsigned short  format_tag; 
	unsigned short  channels;           /* 1 = mono, 2 = stereo */ 
	unsigned long   samplerate;         /* typically: 44100, 32000, 22050, 11025 or 8000*/ 
	unsigned long   bytes_per_second;   /* SamplesPerSec * BlockAlign*/ 
	unsigned short  blockalign;         /* Channels * (BitsPerSample / 8)*/ 
	unsigned short  bits_per_sample;    /* 16 or 8 */ 
}WAVEAUDIOFORMAT; 


int main()
{
	int i = 0;

#if 0
	if (pipe(pipe_desc)) {
		printf("audio_loopback: unable to create pipe\n");
		exit(0);
	}
#endif

#if 0 
	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle[PLAYBACK], "default",
			  SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		printf("audio_loopback: unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}
#else
	amr = Encoder_Interface_init(0);

	/* Open PCM device for record. */
	rc = snd_pcm_open(&handle[CAPTURE], "default",
			  SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		printf("audio_loopback: unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}
#endif
	//for (i = 0; i < 2; i++) {
#if 1 //record
	for (i = 1; i < 2; i++) {
#else
	for (i = 0; i < 1; i++) {
#endif //play
		/* Allocate a hardware parameters object. */
		snd_pcm_hw_params_alloca(&params);

		/* Fill it in with default values. */
		snd_pcm_hw_params_any(handle[i], params);

		/* Set the desired hardware parameters. */

		/* Interleaved mode */
		snd_pcm_hw_params_set_access(handle[i], params,
				     SND_PCM_ACCESS_RW_INTERLEAVED);

		/* Signed 16-bit little-endian format */
		snd_pcm_hw_params_set_format(handle[i], params,
					     SND_PCM_FORMAT_S16_LE);

		/* Two channels (stereo) */
		//snd_pcm_hw_params_set_channels(handle[i], params, 2);
		snd_pcm_hw_params_set_channels(handle[i], params, 1);

		/* 44100 bits/second sampling rate (CD quality) */
		//val = 44100;
	//	val = 16000; //lzcx
		val = 8000; //lzcx
		snd_pcm_hw_params_set_rate_near(handle[i], params, &val, &dir);

		/* Set period size */
		frames = 160*2*4;
		snd_pcm_hw_params_set_period_size_near(handle[i],
						       params, &frames, &dir);

		/* Write the parameters to the driver */
		rc = snd_pcm_hw_params(handle[i], params);
		if (rc < 0) {
			printf("audio_loopback: unable to set hw parameters: %s\n", snd_strerror(rc));
			exit(1);
		}
	}
	/* Use a buffer large enough to hold one period */
	
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	//buffer_size = frames * 4;	/* 2 bytes/sample, 2 channels */
	//buffer_size = frames * 2;	/* 2 bytes/sample, 1 channels */
	buffer_size = frames ;	/* 2 bytes/sample, 1 channels */

//	pthread_create(&recorder_thread, NULL, recorder, NULL);
//	pthread_create(&player_thread, NULL, player, NULL);
//	pthread_join(recorder_thread, NULL);
//	pthread_join(player_thread, NULL);
#if 10 
//wrtofile();
wrtomkvfile();
#else
rdfile();
#endif
	printf("111\n");
	Encoder_Interface_exit(amr);
	printf("222\n");

	printf("audio_loopback: Both threads are exited...\n");
	return 0;

}
void rdfile(void)
{
	unsigned char *buffer,*buffer1;
	FILE *fp;
	int i,time;
	buffer = (unsigned char *)malloc(buffer_size);
	if (!buffer) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
		return ;
	}
	buffer1 = (unsigned char *)malloc(buffer_size);
	if (!buffer1) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
		free(buffer);
		return ;
	}

	fp = fopen("1.wav","rb");
	if(NULL == fp) {
		printf("err open file\n");
		free(buffer);
		free(buffer1);
		return;
	}
	time = 0;
	while(1){
		printf("time %d\n",time);
		if (1 != fread(buffer, buffer_size , 1 , fp)){
			printf("audio_loopback: error reading from the file\n");
			break;
		}
		rc = snd_pcm_writei(handle[PLAYBACK], buffer, frames);//1280
		if (rc == -EAGAIN)
			continue;
		if (rc == -EPIPE) {
			/* EPIPE means underrun */
			printf("audio_loopback: underrun occurred\n");
			snd_pcm_prepare(handle[PLAYBACK]);
			/*snd_pcm_recover(handle[PLAYBACK], rc, 0);*/
		} else if (rc < 0) {
			printf("audio_loopback: error from writei: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			printf("audio_loopback: short write, write %d frames\n", rc);
		}
		time++;
		if(time > 256) break;
		
	}


		free(buffer);
		free(buffer1);
		fclose(fp);

}

void wrtomkvfile(void)
{
	unsigned short *buffer[4];
	unsigned char *buffer1;
	FILE *fp;
	int i,w,n;
	unsigned char buf[500]; //test
	short inbuf0[160];
	short inbuf1[160];
	short inbuf2[160];
	short inbuf3[160];
	WAVEAUDIOFORMAT format;   
	printf("hello\n");
#if 0
	for(i=0;i<4;i++){
		buffer[i] = (unsigned char *)malloc(160*2);
		if (!buffer[i]) {
			printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
			goto quit1;
		}
	}
#endif
	buffer[0] = inbuf0;
	buffer[1] = inbuf1;
	buffer[2] = inbuf2;
	buffer[3] = inbuf3;
	buffer1 = (unsigned char *)malloc(buffer_size);
	if (!buffer1) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
			goto quit1;
	}
#if 0
	amr = Encoder_Interface_init(0);
	if(NULL == amr) {
		printf("amr enc err\n");
			goto quit2;
	}
#endif

	fp = fopen("0.amr","w+");
	if(NULL == fp) {
		printf("file open err\n");
		goto quit3;
	}

    /*write mkv head*/
#if 1
	fwrite((char *)&mkv_ebml_head,sizeof(mkv_ebml_head),1,fp);
	fwrite((char *)&mkv_segment,sizeof(mkv_segment),1,fp);

	mkv_ebml_void.ebml_void_size[1] = 0xFF &(sizeof(mkv_seek_head) + sizeof(mkv_seek_entry_seek) + sizeof(mkv_seek_entry_cue)-sizeof(mkv_ebml_void));
	mkv_ebml_void.ebml_void_size[0] = 0x40 |((sizeof(mkv_seek_head) + sizeof(mkv_seek_entry_seek) + sizeof(mkv_seek_entry_cue)-sizeof(mkv_ebml_void))>>8);
	fwrite((char *)&mkv_ebml_void,sizeof(mkv_ebml_void),1,fp);
	fseek(fp,sizeof(mkv_seek_head)+sizeof(mkv_seek_entry_seek)+sizeof(mkv_seek_entry_cue)-sizeof(mkv_ebml_void),SEEK_CUR);

	mkv_segment_info.info_size[0] = 0x40 |((sizeof(mkv_segment_info) -sizeof(mkv_segment_info.info_id) -sizeof(mkv_segment_info.info_size))>>8);
	mkv_segment_info.info_size[1] = 0xFF&(sizeof(mkv_segment_info) -sizeof(mkv_segment_info.info_id) -sizeof(mkv_segment_info.info_size));
	fwrite((char *)&mkv_segment_info,sizeof(mkv_segment_info),1,fp);

	/*add more track info here*/
//	mkv_h264_track.track_entry_size[0] = 0x40 |((sizeof(mkv_h264_track) - 3)>>8);
//	mkv_h264_track.track_entry_size[1] = 0xFF&(sizeof(mkv_h264_track) - 3);
	mkv_amr_track1.track_entry_size[0] = 0x40 |((sizeof(mkv_amr_track1) - 3)>>8);
	mkv_amr_track1.track_entry_size[1] = 0xFF&(sizeof(mkv_amr_track1) - 3);
	mkv_amr_track2.track_entry_size[0] = 0x40 |((sizeof(mkv_amr_track2) - 3)>>8);
	mkv_amr_track2.track_entry_size[1] = 0xFF&(sizeof(mkv_amr_track2) - 3);
	mkv_amr_track3.track_entry_size[0] = 0x40 |((sizeof(mkv_amr_track3) - 3)>>8);
	mkv_amr_track3.track_entry_size[1] = 0xFF&(sizeof(mkv_amr_track3) - 3);
	mkv_amr_track4.track_entry_size[0] = 0x40 |((sizeof(mkv_amr_track4) - 3)>>8);
	mkv_amr_track4.track_entry_size[1] = 0xFF&(sizeof(mkv_amr_track4) - 3);


	mkv_srt_track.track_entry_size[0] = 0x40 |((sizeof(mkv_srt_track) - 3)>>8);
	mkv_srt_track.track_entry_size[1] = 0xFF&(sizeof(mkv_srt_track) - 3);

	/*update the mkv_track struct*/
//	mkv_track.track_size[0] = 0x40 |((sizeof(mkv_h264_track)+sizeof(mkv_srt_track) \
				+sizeof(mkv_amr_track1) +sizeof(mkv_amr_track2) +sizeof(mkv_amr_track3) +sizeof(mkv_amr_track4) ) >>8);
//	mkv_track.track_size[1] = 0xFF&(sizeof(mkv_h264_track)+sizeof(mkv_srt_track) \
			+sizeof(mkv_amr_track1) +sizeof(mkv_amr_track2) +sizeof(mkv_amr_track3) +sizeof(mkv_amr_track4) );
	mkv_track.track_size[0] = 0x40 |((sizeof(mkv_srt_track) \
						+sizeof(mkv_amr_track1) +sizeof(mkv_amr_track2) +sizeof(mkv_amr_track3) +sizeof(mkv_amr_track4) ) >>8);
	mkv_track.track_size[1] = 0xFF&(sizeof(mkv_srt_track) \
			+sizeof(mkv_amr_track1) +sizeof(mkv_amr_track2) +sizeof(mkv_amr_track3) +sizeof(mkv_amr_track4) );


	/*
	   mkv_track.track_size[0] = 0x40 |((sizeof(mkv_h264_track) + sizeof(mkv_XXX_track) + ...)>>8);
	   mkv_track.track_size[1] = 0xFF&(sizeof(mkv_h264_track) + sizeof(mkv_XXX_track) + ...);
	 */
	fwrite((char *)&mkv_track,sizeof(mkv_track),1,fp);
//	fwrite(fp,(char *)&mkv_h264_track,sizeof(mkv_h264_track));
	fwrite((char *)&mkv_amr_track1,sizeof(mkv_amr_track1),1,fp);
	fwrite((char *)&mkv_amr_track2,sizeof(mkv_amr_track2),1,fp);
	fwrite((char *)&mkv_amr_track3,sizeof(mkv_amr_track3),1,fp);
	fwrite((char *)&mkv_amr_track4,sizeof(mkv_amr_track4),1,fp);

	/*
	   write(fp,(char *)&mkv_xxx_track,sizeof(mkv_xxx_track));
	 */
	fwrite((char *)&mkv_srt_track,sizeof(mkv_srt_track),1,fp);

	printf("write head over\n");
#endif
	while(1) {

		rc = snd_pcm_readi(handle[CAPTURE], buffer1, frames);
		if (rc == -EAGAIN)
			continue;
		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			printf("audio_loopback: overrun occurred\n");
			snd_pcm_prepare(handle[CAPTURE]);
			/*snd_pcm_recover(handle[CAPTURE], rc, 0);*/
		} else if (rc < 0) {
			printf("audio_loopback: error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			printf("audio_loopback: short read, read %d frames\n", rc);
		}

		w = 0;
		for(i=0 ; i < buffer_size/2 ; i+=4) {
			((short *)(buffer[0]))[w]  = ((short *)buffer1)[i+3]; //lzcx ch0
			((short *)(buffer[1]))[w]  = ((short *)buffer1)[i+0]; //lzcx ch1
			((short *)(buffer[2]))[w]  = ((short *)buffer1)[i+2]; //lzcx ch2
			((short *)(buffer[3]))[w]  = ((short *)buffer1)[i+1]; //lzcx ch3
			w++;
		}


		//encode
#if  0 
		for(i=0 ; i<4; i++) {
			//		memset(buffer1,0x00,1280);
			//		printf("111\n");
			n = Encoder_Interface_Encode(amr, MR475, (short *)(buffer[i]), buf, 0);
			//		printf("111222\n");
			if (fwrite(buf, n, 1 , fp) != 1) {
				printf("err write file\n");
				goto quit4;
			}
		}
#else
		n = Encoder_Interface_Encode(amr, MR475, inbuf0, buf, 0);
		if (fwrite(buf, n, 1 , fp) != 1) {
			printf("err write file\n");
			goto quit4;
		}

		n = Encoder_Interface_Encode(amr, MR475, inbuf1, buf, 0);
		if (fwrite(buf, n, 1 , fp) != 1) {
			printf("err write file\n");
			goto quit4;
		}

		n = Encoder_Interface_Encode(amr, MR475, inbuf2, buf, 0);
		if (fwrite(buf, n, 1 , fp) != 1) {
			printf("err write file\n");
			goto quit4;
		}
		n = Encoder_Interface_Encode(amr, MR475, inbuf3, buf, 0);
		if (fwrite(buf, n, 1 , fp) != 1) {
			printf("err write file\n");
			goto quit4;
		}
#endif
	}
quit4:
	fclose(fp);
quit3:
quit2:
	if(NULL != buffer1)
		free(buffer1);

quit1:
	return;
#if 0
	for(i=0 ; i<4 ; i++){
		if(NULL != buffer[i])
			free(buffer[i]);
	}
#endif

}

void wrtofile(void)
{
	unsigned char *buffer[4],*buffer1;
	FILE *fp[4];
	int i,time,len,length,w;
    WAVEAUDIOFORMAT format;   
	for(i=0;i<4;i++){
	buffer[i] = (unsigned char *)malloc(buffer_size);
	if (!buffer[i]) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
		return ;
	}
	}
	buffer1 = (unsigned char *)malloc(buffer_size);
	if (!buffer1) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");

		for(i=0;i<4;i++)
			free(buffer[i]);
		return ;
	}

	fp[0] = fopen("0.wav","w+");
	if(NULL == fp[0]) return;
	fp[1] = fopen("1.wav","w+");
	if(NULL == fp[1]) return;
	fp[2] = fopen("2.wav","w+");
	if(NULL == fp[2]) return;
	fp[3] = fopen("3.wav","w+");
	if(NULL == fp[3]) return;

#if 0 
		if (fwrite(wavhead, 40, 1 , fp) != 1) {
			goto exit1;
		}
#else
		for(i=0;i<4;i++){
	 format.format_tag = 1;   
     format.channels = 1;   
     format.samplerate = 8000;//16000;   
     format.bits_per_sample = 16;   
     format.blockalign = 1*(16/8);//channels * (resolution/8);   
     format.bytes_per_second = format.samplerate * format.blockalign;   
     fseek(fp[i], 0, SEEK_SET);   
	 fwrite("RIFF\0\0\0\0WAVEfmt ", sizeof(char), 16, fp[i]);
	 length = 16;   
	 fwrite(&length, 1, sizeof(long), fp[i]); /* Length of Format (always 16) */   
	 fwrite(&format, 1, sizeof(format), fp[i]);   
     fwrite("data\0\0\0\0", sizeof(char), 8, fp[i]); /* Write data chunk */  
		}
#endif
	time = 0;
	while(1) {
		rc = snd_pcm_readi(handle[CAPTURE], buffer1, frames);
		if (rc == -EAGAIN)
			continue;
		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			printf("audio_loopback: overrun occurred\n");
			snd_pcm_prepare(handle[CAPTURE]);
			/*snd_pcm_recover(handle[CAPTURE], rc, 0);*/
		} else if (rc < 0) {
			printf("audio_loopback: error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			printf("audio_loopback: short read, read %d frames\n", rc);
		}

#if 10 
#if 1 
		w = 0;
		for(i=0 ; i < buffer_size/2 ; i+=4){
			((unsigned short *)(buffer[0]))[w]  = ((unsigned short *)buffer1)[i+3]; //lzcx ch0
			((unsigned short *)(buffer[1]))[w]  = ((unsigned short *)buffer1)[i+1]; //lzcx ch1
			((unsigned short *)(buffer[2]))[w]  = ((unsigned short *)buffer1)[i+0]; //lzcx ch2
			((unsigned short *)(buffer[3]))[w]  = ((unsigned short *)buffer1)[i+2]; //lzcx ch3
			w++;
		}
		
		for(i=0;i<4;i++)
		if (fwrite(buffer[i], w*2, 1 , fp[i]) != 1) {
#else
			w = 0;
			for(i=0 ; i < buffer_size ; i+=4){
				((unsigned char *)buffer1)[w++]  = ((unsigned char *)buffer)[i+3]; //lzcx
			}

			if (fwrite(buffer1, w, 1 , fp) != 1) {

#endif
#else
		if (fwrite(buffer, buffer_size, 1 , fp) != 1) {
#endif
			printf("audio_loopback: error writing to the pipe\n");
			break;
		}		
		time++;
		if(time > 1000) break;
	}
	for(i=0;i<4;i++){
	    fseek(fp[i],0x4,SEEK_SET);
		len = 2048*1001/4+44-8;

		fwrite(&len,1,4,fp[i]);
	    fseek(fp[i],40,SEEK_SET);
		len = 2048*1001/4;
		fwrite(&len,1,4,fp[i]);
	}
exit1:
	for(i=0;i<4;i++){
		fclose(fp[i]);
		free(buffer[i]);
	}
		free(buffer1);
}
void *recorder(void *arg)
{
	unsigned char *buffer;
	
	buffer = (unsigned char *)malloc(buffer_size);
	if (!buffer) {
		printf("audio_loopback: could not allocate memory for the recorder audio buffer\n");
		goto recorder_thread_exit;
	}
	rc = 0;

	while(1) {
		rc = snd_pcm_readi(handle[CAPTURE], buffer, frames);
		if (rc == -EAGAIN)
			continue;
		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			printf("audio_loopback: overrun occurred\n");
			snd_pcm_prepare(handle[CAPTURE]);
			/*snd_pcm_recover(handle[CAPTURE], rc, 0);*/
		} else if (rc < 0) {
			printf("audio_loopback: error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			printf("audio_loopback: short read, read %d frames\n", rc);
		}
		if (write(pipe_desc[1], buffer, buffer_size) != buffer_size) {
			printf("audio_loopback: error writing to the pipe\n");
			break;
		}		
	}

recorder_thread_exit:
	printf("audio_loopback: recorder thread exited\n");
	free(buffer);
	pthread_exit(0);
}

void *player(void *arg)
{
	unsigned char *buffer;
	
	buffer = (unsigned char *)malloc(buffer_size);
	if (!buffer) {
		printf("audio_loopback: could not allocate memory for the player audio buffer\n");
		goto player_thread_exit;
	}
	rc = 0;

	while(1){
		if (read(pipe_desc[0], buffer, buffer_size) != buffer_size) {
			printf("audio_loopback: error reading from the pipe\n");
			break;
		}
		rc = snd_pcm_writei(handle[PLAYBACK], buffer, frames);
		if (rc == -EAGAIN)
			continue;
		if (rc == -EPIPE) {
			/* EPIPE means underrun */
			printf("audio_loopback: underrun occurred\n");
			snd_pcm_prepare(handle[PLAYBACK]);
			/*snd_pcm_recover(handle[PLAYBACK], rc, 0);*/
		} else if (rc < 0) {
			printf("audio_loopback: error from writei: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			printf("audio_loopback: short write, write %d frames\n", rc);
		}
		
	}

player_thread_exit:
	printf("audio_loopback: player thread exited...\n");
	free(buffer);
	pthread_exit(0);
}
