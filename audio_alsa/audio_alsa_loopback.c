#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <alsa/asoundlib.h>
 
#define Int8 char
#define Int32 int
#define UInt32 unsigned int
#define OSA_EFAIL -1
#define OSA_SOK  0

#define     AUD_DEVICE_PRINT_ERROR_AND_RETURN(str, err, hdl)        \
        fprintf (stderr, "\n\r [host] AUDIO >> " str, snd_strerror (err));  \
        snd_pcm_close (hdl);    \
        return  -1;

static snd_pcm_t *gAlsa_captureHdle = NULL;
static snd_pcm_t *gAlsa_playHdle = NULL;

#define PCM_SAMPLE_FORMT  SND_PCM_FORMAT_S16_LE


Int32 Audio_initCaptureDevice (Int8 *device, Int32 numChannels, UInt32 sampleRate, UInt32 pcm_format)
{
    snd_pcm_hw_params_t *hw_params;
    Int32 err;
    snd_pcm_t       *alsa_handle;
    Int32 resample;
    static snd_pcm_uframes_t    frames;
    Int32 dir;

    if ((err = snd_pcm_open (&alsa_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        printf("%s:Cannot open audio device %s (%s)\n", __func__, device, snd_strerror(err));
        return  OSA_EFAIL;
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot allocate hardware parameter structure (%s)\n", err, alsa_handle);
    }

    if ((err = snd_pcm_hw_params_any (alsa_handle, hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot initialize hardware parameter structure (%s)\n", err, alsa_handle);
    }

    if ((err = snd_pcm_hw_params_set_access (alsa_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot set access type (%s)\n", err, alsa_handle);
    }

    if ((err = snd_pcm_hw_params_set_format (alsa_handle, hw_params, pcm_format)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot set sample format (%s)\n", err, alsa_handle);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (alsa_handle, hw_params, &sampleRate, 0)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot set sample rate (%s)\n", err, alsa_handle);
    }

    if ((err = snd_pcm_hw_params_set_channels (alsa_handle, hw_params, numChannels)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot set channel count (%s)\n", err, alsa_handle);
    }

    resample = 1;        
    snd_pcm_hw_params_set_rate_resample(alsa_handle, hw_params, resample);  
#ifdef CONFIG_WISCOM
    frames = 320;   //一次采集多少帧
#else
    frames = 1024;  
#endif
    err = snd_pcm_hw_params_set_period_size_near(alsa_handle, hw_params, &frames, &dir);        
    if (err < 0)        
    {            
       printf("AUDIO >> Unable to set period size %li err: %s\n", frames, snd_strerror(err));            
    }

    if ((err = snd_pcm_hw_params (alsa_handle, hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot set parameters (%s)\n", err, alsa_handle);
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (alsa_handle)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("\n\nAUDIO >>  cannot prepare audio interface for use (%s)\n", err, alsa_handle);
    }

    gAlsa_captureHdle  = alsa_handle;
    printf(("\n\nAUDIO: AUDIO CAPTURE DEVICE Init Done!!!!!\n"));
    usleep(100);
    return err;
}


static Int32  Audio_captureData(char *buffer, Int32 numSamples)
{
    Int32 err = OSA_EFAIL;

    if (gAlsa_captureHdle){
        if ((err = snd_pcm_readi (gAlsa_captureHdle, buffer, numSamples)) != numSamples){
            return -1;
        }
    }else{
        return -1;
    }
    return 0;
}

static Int32 Audio_deInitCaptureDevice(void)
{
    if (gAlsa_captureHdle)
    {
        snd_pcm_drain(gAlsa_captureHdle);
        snd_pcm_close(gAlsa_captureHdle);
        gAlsa_captureHdle = NULL;
        printf(("AUDIO: Capture device deInit done....\n"));
    }
    printf(("\n\nAUDIO: AUDIO CAPTURE DEVICE De_Init Done!!!!!\n"));
    return OSA_SOK;
}

        //playback

struct audio_playObj {
    snd_pcm_uframes_t frames_per_period;
    int bufSizeByte_per_period;
};

static struct audio_playObj gAudio_playObj;

Int32 Audio_initPlayBackDevice (Int8 *device, Int32 channels, UInt32 sample_rate, UInt32 pcm_format)
{
    Int32 rc;
    snd_pcm_hw_params_t *params;
    UInt32 val, rate;
    Int32 ret;
    int dir;
    snd_pcm_uframes_t bufferSizeMax = (4 * 1024);
    Int32 size;
    snd_pcm_uframes_t period_size;

    memset(&gAudio_playObj, 0, sizeof(struct audio_playObj));
    
    /* Open AIC device for playback. */
    rc = snd_pcm_open(&gAlsa_playHdle, device,
            SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        printf( " Unable to open pcm device: %s.\n", snd_strerror(rc));
        return -1;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(gAlsa_playHdle, params);

    /* Set the desired hardware parameters. */
    /* Interleaved mode */

    snd_pcm_hw_params_set_access(gAlsa_playHdle, params,
                   SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(gAlsa_playHdle, params,
                           pcm_format);

    rc = snd_pcm_hw_params_set_channels(gAlsa_playHdle, params, channels);
    if (rc < 0)
    {
        printf( " Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(rc));
    }

    dir = 0;
    ret = snd_pcm_hw_params_set_rate_near(gAlsa_playHdle, params,
                               &sample_rate, &dir);
    if (ret != 0)
    {
        printf( " The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n", sample_rate, ret);
    }
    else
    {
        printf( " The rate %d Hz set for play back hardware, ret - %d\n", sample_rate, ret);
    }


    period_size = 320; //1024 定义frames
 
    rc = snd_pcm_hw_params_set_period_size_near(gAlsa_playHdle,
            params, &period_size, &dir);
    if (rc < 0) {
        printf( " Unable to set period size %u err: %s\n", (UInt32)period_size, snd_strerror(rc));
        return -1;
    }

    if ((rc = snd_pcm_hw_params_set_buffer_size(gAlsa_playHdle, params, bufferSizeMax)) < 0) {
        printf( "%s: cannot set buffer size (%lu)\n", __func__, bufferSizeMax);
    }

   
    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(gAlsa_playHdle, params);
    if (rc < 0)
    {
        printf("\n Unable to set hw parameters: (%s)\n", snd_strerror(rc));
        snd_pcm_close(gAlsa_playHdle);
        gAlsa_playHdle = NULL;
        return -1;
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &period_size,
                                    &dir);

    size = period_size * 2; /* 2 bytes/sample, 1 channel */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    snd_pcm_hw_params_get_rate(params, &rate, &dir);
    
    
    gAudio_playObj.frames_per_period = period_size;
    gAudio_playObj.bufSizeByte_per_period = size;
    
    snd_pcm_prepare(gAlsa_playHdle);
    
    printf( " Audio Playback Device Opened,"
           " period size: %u, period time: %d, rate: %d\n", 
            (UInt32)period_size, val, rate);
    
    return 0;
}

Int32 Audio_deInitPlayBackDevice(void)
{
    if (gAlsa_playHdle)
    {
        snd_pcm_drain(gAlsa_playHdle);
        snd_pcm_close(gAlsa_playHdle);
        gAlsa_playHdle = NULL;
        printf( " Playback Device Closed\n");
    }
    return 0;    
}

Int32 Audio_playPcm(char *pcm_buf, Int32 buf_size)
{
    Int32 ret = 0;
    Int32 numBytes = 0;
    char *play_buf = pcm_buf;
    Int32 samples_played = 0;

    snd_pcm_uframes_t frames_per_period = gAudio_playObj.frames_per_period;

    if(!pcm_buf)
        return -1;

    if(!gAlsa_playHdle)
         return -1;

     numBytes = buf_size;
     samples_played = 0;

     while ((numBytes >= gAudio_playObj.bufSizeByte_per_period)) {
        ret = snd_pcm_writei(gAlsa_playHdle, play_buf + samples_played, frames_per_period); //每period读入的帧数
        samples_played += gAudio_playObj.bufSizeByte_per_period;
        numBytes -= gAudio_playObj.bufSizeByte_per_period;
        
        if (ret == -EPIPE) {
            /* EPIPE means underrun */
            printf("%s: Error-Underrun occurred", __func__);
            snd_pcm_prepare(gAlsa_playHdle);          
            return -1;
        } else if (ret < 0) {
            printf( "%s: Could not write (writei returned error %s)",
                    __func__, snd_strerror(ret));
            return -1;
        } else if (ret != (Int32) frames_per_period) {
            printf("%s: Error-short write, wrote %d frames", __func__,ret);
            return -1;
        }
    }        

    return 0;
}


//demo
#define AUDIO_DEVICE_NAME "plughw:0,1"
 static int done = 0;
 void signal_fxn(int signo)
 {
     if(signo == SIGQUIT)
         done = 1;
 }

void usage(char *argv)
{
    printf("Info: %s %s\n", __TIME__, __DATE__);
    printf("  %s -m 0/1/2 -t 0/1 -f ./test.pcm\n", argv);
    printf("\tm: test mode, valude 0 1 2\n");
    printf("\t    0: capture playback loop mode [default mode].\n");
    printf("\t    1: capture mode\n");
    printf("\t    2: play mode\n");    
    printf("\tt: capture/play format, 0:pcm, 1:ulaw, 2:alaw.\n");
    printf("\tf: file path\n");
}
 Int32 main(int argc, char **argv )
{
    Int32 ret = 0;
    Int32 sample_nums = 0;
    char pcm_buf[4*1024];
    Int32 buf_size = 0;
    Int32 count = 0;
    snd_pcm_format_t sample_format = SND_PCM_FORMAT_S16_LE;

    FILE *file = NULL;
    char file_path[128];
    Int32 capture_enable = 1;
    Int32 play_enable = 1;
    Int32 savefile_enable = 0;
    
    char *opt_strings = "m:t:f:h";
    Int32 ch;
    char exe_name[128];
    char *mode = NULL;
    
    strcpy(exe_name, argv[0]);
    
    while(1){
        if(argc < 2)
            break;
        ch = getopt(argc, argv, opt_strings);
        if(ch == -1)
            break;
        switch(ch){
        case 'm':
            switch(atoi(optarg)){
            case 1:
                capture_enable = 1;
                play_enable = 0;
                mode = "wb";
                printf("capture mode.\n");
                break;
            case 2:
                capture_enable = 0;
                play_enable = 1;
                printf("playback mode.\n");
                mode = "rb";
                break;
            case 0:
            default:
                capture_enable = 1;
                play_enable = 1;
                //                savefile_enable = 0;
                printf("loopback mode.\n");
                break;                
            }
            break;
        case 't':
            switch(atoi(optarg)){
            case 1:
                sample_format = SND_PCM_FORMAT_MU_LAW;
                printf("sample_format is mu_law.\n");
                printf("**Warnning this format Invalid, exit***\n");
                return -1;
                break;
            case 2:
                sample_format = SND_PCM_FORMAT_A_LAW;
                printf("sample_format is a_law.\n");
                printf("**Warnning this format Invalid, exit***\n");
                return -1;
            case 0:
            default:
                sample_format = SND_PCM_FORMAT_S16_LE;
                printf("sample_format is s16_le.\n");
            }
            break;
        case 'f':
            strcpy(file_path, optarg);
                savefile_enable = 1;            
            break;
        case 'h':
            usage(exe_name);
            return 0;
        default:
            break;
        }
    }

    printf("INFO: Test start, press <C+\\> to Quit..\n");
    signal(SIGQUIT, signal_fxn);

    if(play_enable && capture_enable){
        savefile_enable = 0;
    }else{
        if(!savefile_enable){
            usage(exe_name);
            return -1;
        }
    }

    if(savefile_enable && mode){
        file = fopen(file_path, mode);
        if(!file){
            printf("File <%s> Open Error.\n", file_path);
            return -1;
        }else
            printf("File <%s> Open OK.\n", file_path);
        
    }

    if(capture_enable){
    ret = Audio_initCaptureDevice(AUDIO_DEVICE_NAME, 1, 8000, sample_format);
    if(ret < 0){
        printf("audio initCaptureDevice failed.\n");
        if(!file)
            fclose(file);
        return -1;
    }
    }

    if(play_enable){
    ret = Audio_initPlayBackDevice(AUDIO_DEVICE_NAME, 1, 8000, sample_format);
    if(ret < 0){
        printf("audio initPlayDevice failed.\n");
        Audio_deInitCaptureDevice();
        if(!file)
            fclose(file);
        return -1;
    }
    }

    while(!done){
        sample_nums = 320;
        if(capture_enable){
        ret = Audio_captureData(pcm_buf, sample_nums);
        if(ret < 0){
            printf("Error-audio capture data error.\n");
            break;
        }
        }

        if(sample_format == SND_PCM_FORMAT_S16_LE)
            buf_size = sample_nums * 2; //1CH 16bit
        else
            buf_size = sample_nums;

        if(savefile_enable){
            if(capture_enable && file){
                ret = fwrite(pcm_buf, 1, buf_size, file);
                if(ret < 0){
                    printf("Error- write file failed, return-%d.\n", ret);
                    break;
                }
            }
            if(play_enable && file){
                ret = fread(pcm_buf, 1, buf_size, file);
                if(!ret){
                    printf("Error- read file failed, return-%d.\n", ret);
                    break;
                }
            }
        }

        if(play_enable){
        ret = Audio_playPcm(pcm_buf, buf_size);
        if(ret < 0){
            printf("audio playPcm failed\n");
        }
        }
        
        if((count % 50) == 0)
            printf("Capture Info [%d]: capture samples-%d, bufSize-%d Bytes.\n",
                   count, sample_nums, buf_size);
        if(count < 0xffffff)
            count ++;
        else
            count = 0;
        
    }

    if(capture_enable)
        Audio_deInitCaptureDevice();

    if(play_enable)
        Audio_deInitPlayBackDevice();

    if(!file)
            fclose(file);
    printf("****Test Over****\n");
    return 0;
}
