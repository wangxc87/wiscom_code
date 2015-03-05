#include <stdio.h>
#include <termios.h>
#include <term.h>
#include <unistd.h>


static   struct   termios   initial_settings;
static   struct   termios   new_settings;
static   int   peek_character   =   -1;

void   init_keyboard()
{
  tcgetattr(STDIN_FILENO,   &initial_settings);
  new_settings   =   initial_settings;
  new_settings.c_lflag   &=   ~ICANON;
  new_settings.c_lflag   &=   ~ECHO;
  new_settings.c_lflag   &=   ~ISIG;
  new_settings.c_cc[VMIN]   =   1;
  new_settings.c_cc[VTIME]   =   0;
  tcsetattr(STDIN_FILENO,   TCSANOW,   &new_settings);

}

void   close_keyboard()
{
  tcsetattr(0,   TCSANOW,   &initial_settings);
}

int   kbhit()
{
  char   ch;
  int   nread;
  if   (peek_character   !=   -1)   {
    return   1;
  }
  new_settings.c_cc[VMIN]   =   0;
  tcsetattr(STDIN_FILENO,   TCSANOW,   &new_settings);
  nread   =   read(STDIN_FILENO,   &ch,   1);
  new_settings.c_cc[VMIN]   =   1;
  tcsetattr(STDIN_FILENO,   TCSANOW,   &new_settings);

  if   (nread   ==   1)   {
    peek_character   =   ch;
    return   1;
  }
  return   0;
}

int   readch()
{
  char   ch;
  if   (peek_character   !=   -1)   {
    ch   =   peek_character;
    peek_character   =   -1;
    return   ch;
  }
  read(0,   &ch,   1);
  return   ch;
}

void exit_s()
{
  close_keyboard();
  //    exit(0);
}
void PrintInfo()
{
  printf("---press 'q' to quit---\n");
}


int ChkKeyBrdAndSetGain()
{
  int ch=0;

  if   (kbhit())   {
    ch   =   readch();
    switch (ch){
    case 'q':
    case 'Q':
      return -1;
      break;
    case 'w':
    case 'W':
            return 'w';
            break;
            
      /*    case 'D':
      if (digital_global_gain < 4095 - 16 ) digital_global_gain +=16;
      if (red_gain < 4095 - 16 ) red_gain +=16;
      if (green_gain < 4095 - 16 ) green_gain +=16;
      if (blue_gain < 4095 - 16 ) blue_gain +=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'd':
      if (digital_global_gain > 16 ) digital_global_gain -=16;
      if (red_gain > 16 ) red_gain -=16;
      if (green_gain > 16 ) green_gain -=16;
      if (blue_gain > 16 ) blue_gain -=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'R':
      if (red_gain < 4095 - 16 ) red_gain +=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'r':
      if (red_gain > 16 ) red_gain -=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'G':
      if (green_gain < 4095 - 16 ) green_gain +=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'g':
      if (green_gain > 16 ) green_gain -=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'B':
      if (blue_gain < 4095 - 16 ) blue_gain +=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 'b':
      if (blue_gain > 16 ) blue_gain -=16;
      if(SetDigitalGain(digital_global_gain,red_gain,green_gain,blue_gain)<0)return -1; // 0-4095
      break;
    case 's':
    case 'S':
      save_yuv=1;
      break;
    case 'w':
    case 'W':
      if (show_frame == 0) {
        updateIpipeParam(wb_table_def,rgb1_table_calc,rgb2_table_def);
        red_gain = wb_table_def[0];
        green_gain = wb_table_def[1];
        blue_gain = wb_table_def[3];

        show_frame = 1;
        wb_adjust = 0;
      } else {
        show_frame = 0;
        wb_adjust = 1;
        wb_target_level = 128;
      }
      break;*/
    default:
      break;
    }
    PrintInfo();
  }
  return 0;
}

