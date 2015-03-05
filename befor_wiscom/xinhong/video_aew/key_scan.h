#include <stdio.h>
#include <termios.h>
#include <term.h>
#include <unistd.h>

void   init_keyboard();

void   close_keyboard();

int   kbhit();

void exit_s();

int ChkKeyBrdAndSetGain();
