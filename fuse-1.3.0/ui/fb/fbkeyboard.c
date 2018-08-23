/* fbkeyboard.c: routines for dealing with the linux fbdev display
   Copyright (c) 2000-2004 Philip Kendall, Matan Ziv-Av, Darren Salt

   $Id: fbkeyboard.c 4109 2009-12-27 06:15:10Z fredm $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <linux/kd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui/ui.h"

#include "input.h"
#include <debugger/debugger.h>
#include "fbdisplay.h"

#define BUTTON_1 0
#define BUTTON_UP 1
#define BUTTON_DOWN 2
#define BUTTON_LEFT 3
#define BUTTON_RIGHT 4
#define BUTTON_2 5
#define BUTTON_3 6
#define BUTTON_4 7
#define BUTTON_YELLOW 8
#define BUTTON_BLUE 9
#define BUTTON_GREEN 10
#define BUTTON_POWER 11
#define BUTTON_BOOTKEY 12
#define BUTTON_VOL_DOWN 13
#define BUTTON_VOL_UP 14
int powerKeyPressed = 0;
int bootKeyPressed = 0;
#define KEY_REPEAT_DELAY 4
#define BATTERY_INFO_PRINT_DELAY 100
#define MIN_BATTERY_CHARGE_INIT_LCD 3600
#define MIN_BATTERY_CHARGE_RED_LIGHT 3640

#define PRINT_BATTERY_STATUS 0

short *current_key_button1;
short *current_key_button2;
short *current_key_button3;
short *current_key_button4;
short *current_key_left;
short *current_key_right;
short *current_key_up;
short *current_key_down;
short *current_key_yellow;
short *current_key_green;
short *current_key_blue;

short key_button1[10];
short key_button2[10];
short key_button3[10];
short key_button4[10];
short key_left[10];
short key_right[10];
short key_up[10];
short key_down[10];
short key_yellow[10];
short key_green[10];
short key_blue[10];

short startSequence[10];
short alt_key_button1[10];
short alt_key_button2[10];
short alt_key_button3[10];
short alt_key_button4[10];
short alt_key_left[10];
short alt_key_right[10];
short alt_key_up[10];
short alt_key_down[10];
short alt_key_yellow[10];
short alt_key_green[10];
short alt_key_blue[10];
int currentKeyMapping = 0;
int altKeyMappingUsed = 0;

// According to keysyms.c
enum E_VIRTUAL_KEYBOARD_BUTTON {
  E_VKB_NONE=-1,
  E_VKB_1=2,
  E_VKB_2,
  E_VKB_3,
  E_VKB_4,
  E_VKB_5,
  E_VKB_6,
  E_VKB_7,
  E_VKB_8,
  E_VKB_9,
  E_VKB_0,
  E_VKB_Q=16,
  E_VKB_W,
  E_VKB_E,
  E_VKB_R,
  E_VKB_T,
  E_VKB_Y,
  E_VKB_U,
  E_VKB_I,
  E_VKB_O,
  E_VKB_P,
  E_VKB_A=30,
  E_VKB_S,
  E_VKB_D,
  E_VKB_F,
  E_VKB_G,
  E_VKB_H,
  E_VKB_J,
  E_VKB_K,
  E_VKB_L,
  E_VKB_ENTER=28,
  E_VKB_CAPS_SHIFT=58,
  E_VKB_Z=44,
  E_VKB_X,
  E_VKB_C,
  E_VKB_V,
  E_VKB_B,
  E_VKB_N,
  E_VKB_M,
  E_VKB_SYMBOL_SHIFT,
  E_VKB_SPACE=57,

  NUM_VIRTUAL_KEYBOARD_BUTTONS=60,
};
int popupScreenAfterBoot = 100;


#ifdef __arm__

#define LITTLE_KEYS 0
#define LEFT_KEYS 1
#define RIGHT_KEYS 2
#define VOLUME_KEYS 3

// Keys: Button1;Up;Down;Left;Right;Button2?;Button3?;Button4?;???.....

#include "stdlib.h"
#include "string.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/mman.h>

unsigned int value = 0xF555FFAA;
int gpio4, gpio6, gpio7, gpio30;
int gpio21;
int gpio14, gpio15, gpio26, gpio27;
char gpioValue;

int usbKeyboard = -1;

//unsigned int base = 0x80018000;
//unsigned int offset = 0x100;
//unsigned int pagesize = 0x2000;
char keyPressed[16];
int readingState = 0;
int isPowerSupplyConnected = 0;
unsigned char batteryMonFreq = 0;
int resetPowerCharger = 0;
int backlightStatus = 0;
int chargerResetCounter = 1000;

//int reg_read(int *reg_block, int offset, int mask, int position)
//{
//    int actual = *(reg_block + (offset/4));
//    actual &= mask;
//    actual >>= position;

//    return actual;
//}

//void reg_write(int *reg_block, int offset, int value)
//{
//    *(reg_block + (offset/4)) = value;
//}

void setDirection(int gpioNum, int dir)
{
        char buf[64];
        sprintf(&buf[0], "/sys/class/gpio/gpio%d/direction", gpioNum);

//      printf("\nUsing %s", &buf[0]);
        int gpio = open(buf, O_WRONLY);
        if (dir == 1)
            write(gpio, "in", 2);
        else
            write(gpio, "out", 3);
        close(gpio);
}

void setKeyScan(int mode)
{
    if (mode == LEFT_KEYS) {
        setDirection(7, 0);
        write(gpio7, "0", 1);
        setDirection(4, 1);
        setDirection(6, 1);
        setDirection(30, 1);
        }
    else if (mode == LITTLE_KEYS) {
        setDirection(6, 0);
        write(gpio6, "0", 1);
        setDirection(4, 1);
        setDirection(7, 1);
        setDirection(30, 1);
    }
    else if (mode == RIGHT_KEYS){
        setDirection(4, 0);
        write(gpio4, "0", 1);
        setDirection(6, 1);
        setDirection(7, 1);
        setDirection(30, 1);
    }
    else {
        setDirection(30, 0);
        write(gpio30, "0", 1);
        setDirection(4, 1);
        setDirection(6, 1);
        setDirection(7, 1);
    }
}

static const char *const evval[3] = {
    "RELEASED",
    "PRESSED ",
    "REPEATED"
};

void checkForUSBKeyboard() {
    const char *dev = "/dev/input/event0";

    if (usbKeyboard == -1) {
        usbKeyboard = open(dev, O_RDONLY | O_NONBLOCK);
        if (usbKeyboard != -1) {
            printf("\nUSB Keyboard detected\n");
            fflush(stdout);
        }
    }
}

#endif

static struct termios old_ts;
static int old_kbmode = K_XLATE;
static int got_old_ts = 0;
unsigned short virtualKeys[64] = {'\x00'};
int keyDelay = 0;
int doRelease = 0;

int fbkeyboard_init(void)
{
  struct termios ts;
  struct stat st;
  int i = 1;

  /* First, set up the keyboard */
  if( fstat( STDIN_FILENO, &st ) ) {
    fprintf( stderr, "%s: couldn't stat stdin: %s\n", fuse_progname,
         strerror( errno ) );
    return 1;
  }

  /* check for character special, major 4, minor 0..63 */
  if( !isatty(STDIN_FILENO) || !S_ISCHR(st.st_mode) ||
      ( st.st_rdev & ~63 ) != 0x0400 ) {
    fprintf( stderr, "%s: stdin isn't a local tty\n", fuse_progname );
//    return 1;
  }

  tcgetattr( STDIN_FILENO, &old_ts );
  ioctl( STDIN_FILENO, KDGKBMODE, &old_kbmode );
  got_old_ts = 1;

  /* We need non-blocking semi-cooked keyboard input */
  if( ioctl( STDIN_FILENO, FIONBIO, &i ) ) {
    fprintf( stderr, "%s: can't set stdin nonblocking: %s\n", fuse_progname,
         strerror( errno ) );
    return 1;
  }
  if( ioctl( STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW ) ) {
    fprintf( stderr, "%s: can't set keyboard into medium-raw mode: %s\n",
         fuse_progname, strerror( errno ) );
//    return 1;
  }

  /* Add in a bit of voodoo... */
  ts = old_ts;
  ts.c_cc[VTIME] = 0;
  ts.c_cc[VMIN] = 1;
  ts.c_lflag &= ~(ICANON | ECHO | ISIG);
  ts.c_iflag = 0;
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &ts );
  tcsetpgrp( STDIN_FILENO, getpgrp() );

#ifdef __arm__
  for (i=0;i<16;i++)
      keyPressed[i] = 0;

// Set GPIO for PIN4 and PIN5
vegaWriteReg(0x80018000, 0x100, 0xF555FFAA);

//  int fd = open("/dev/mem", O_RDWR|O_SYNC);
//  if (fd < 0) {
//       printf("\nCannot access memory.\n");
//       return -1;
//  }

//  int page = getpagesize();
//  int length = page * ((pagesize + page - 1) / page);
//  int *regs = (int *)mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);

//  if (regs  == MAP_FAILED) {
//   printf("regs mmap error\n");
//   return -1;
//  }
//  reg_write(regs, offset, value);

  // Setup Power Key
//  base = 0x80044000;
//  offset = 0xc0;
//  pagesize = 0x2000;

//  page = getpagesize();
//  length = page * ((pagesize + page - 1) / page);

  // Setup Boot Key
  char buf[255];
  int gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "21");
  write(gpio, buf, strlen(buf));
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio21/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "in", 2);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio21/value");
  gpio21 = open(buf, O_RDONLY);

  // Setup 4-bit matrix
  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "4");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio4/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "out", 3);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio4/value");
  gpio4 = open(buf, O_WRONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "6");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio6/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "out", 3);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio6/value");
  gpio6 = open(buf, O_WRONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "7");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio7/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "out", 3);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio7/value");
  gpio7 = open(buf, O_WRONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "14");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio14/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "in", 2);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio14/value");
  gpio14 = open(buf, O_RDONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "15");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio15/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "in", 2);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio15/value");
  gpio15 = open(buf, O_RDONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "26");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio26/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "in", 2);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio26/value");
  gpio26 = open(buf, O_RDONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "27");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio27/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "in", 2);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio27/value");
  gpio27 = open(buf, O_RDONLY);

  gpio = open("/sys/class/gpio/export", O_WRONLY);
  sprintf(buf, "30");
  write(gpio, buf, strlen(buf));
  close(gpio);
  sprintf(buf, "/sys/class/gpio/gpio30/direction");
  gpio = open(buf, O_WRONLY);
  write(gpio, "out", 3);
  close(gpio);

  sprintf(buf, "/sys/class/gpio/gpio30/value");
  gpio30 = open(buf, O_WRONLY);

  printf("\nSwitching green led off");
  vegaSetLed(0, 1);
#endif
  remapKeys(NULL);

  return 0;
}

int fbkeyboard_end(void)
{
  int i = 0;

  ioctl( STDIN_FILENO, FIONBIO, &i );
  if( got_old_ts ) {
    tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_ts );
    ioctl( STDIN_FILENO, KDSKBMODE, old_kbmode );
  }

  return 0;
}

void setVirtualKeys(unsigned char *keys)
{
  printf("\nSetVirtualKeys");
//  int i;
//  for (i=0;keys[i] != 0; i++)
//      printf("K: %x", keys[i]);
  strncpy((char *) &virtualKeys, (char *) keys, 63);
  virtualKeys[63] = 0;
  keyDelay = 10;
}

void setUSBKeys(unsigned short *keys)
{
  printf("\nSetUSBKeys");

  if (virtualKeys[0] == 0) {
      strncpy((short *) &virtualKeys, (short *) keys, 63);
      virtualKeys[63] = 0;
      keyDelay = 4;
  }
}

short interpretKey(char *key)
{
    if (strcmp(key, "SP") == 0)
        return INPUT_KEY_space;
    if (strcmp(key, "EN") == 0)
        return INPUT_KEY_Return;
    if (strcmp(key, "CS") == 0)
        return INPUT_KEY_Caps_Lock;
    if (strcmp(key, "SS") == 0)
        return keysyms_remap(51);

    if (strcmp(key, "0") == 0)
        return INPUT_KEY_0;
    if (strcmp(key, "1") == 0)
        return INPUT_KEY_1;
    if (strcmp(key, "2") == 0)
        return INPUT_KEY_2;
    if (strcmp(key, "3") == 0)
        return INPUT_KEY_3;
    if (strcmp(key, "4") == 0)
        return INPUT_KEY_4;
    if (strcmp(key, "5") == 0)
        return INPUT_KEY_5;
    if (strcmp(key, "6") == 0)
        return INPUT_KEY_6;
    if (strcmp(key, "7") == 0)
        return INPUT_KEY_7;
    if (strcmp(key, "8") == 0)
        return INPUT_KEY_8;
    if (strcmp(key, "9") == 0)
        return INPUT_KEY_9;

    if (strcmp(key, "A") == 0)
        return INPUT_KEY_a;
    if (strcmp(key, "B") == 0)
        return INPUT_KEY_b;
    if (strcmp(key, "C") == 0)
        return INPUT_KEY_c;
    if (strcmp(key, "D") == 0)
        return INPUT_KEY_d;
    if (strcmp(key, "E") == 0)
        return INPUT_KEY_e;
    if (strcmp(key, "F") == 0)
        return INPUT_KEY_f;
    if (strcmp(key, "G") == 0)
        return INPUT_KEY_g;
    if (strcmp(key, "H") == 0)
        return INPUT_KEY_h;
    if (strcmp(key, "I") == 0)
        return INPUT_KEY_i;
    if (strcmp(key, "J") == 0)
        return INPUT_KEY_j;
    if (strcmp(key, "K") == 0)
        return INPUT_KEY_k;
    if (strcmp(key, "L") == 0)
        return INPUT_KEY_l;
    if (strcmp(key, "M") == 0)
        return INPUT_KEY_m;
    if (strcmp(key, "N") == 0)
        return INPUT_KEY_n;
    if (strcmp(key, "O") == 0)
        return INPUT_KEY_o;
    if (strcmp(key, "P") == 0)
        return INPUT_KEY_p;
    if (strcmp(key, "Q") == 0)
        return INPUT_KEY_q;
    if (strcmp(key, "R") == 0)
        return INPUT_KEY_r;
    if (strcmp(key, "S") == 0)
        return INPUT_KEY_s;
    if (strcmp(key, "T") == 0)
        return INPUT_KEY_t;
    if (strcmp(key, "U") == 0)
        return INPUT_KEY_u;
    if (strcmp(key, "V") == 0)
        return INPUT_KEY_v;
    if (strcmp(key, "W") == 0)
        return INPUT_KEY_w;
    if (strcmp(key, "X") == 0)
        return INPUT_KEY_x;
    if (strcmp(key, "Y") == 0)
        return INPUT_KEY_y;
    if (strcmp(key, "Z") == 0)
        return INPUT_KEY_z;

    printf("\nCannot detect key %s", key);
    return INPUT_KEY_NONE;
}

int getToken(char *str, char *token, char sep, int maxTokenLength)
{
    int i=0;
    while (str[i] != 0 && i < maxTokenLength) {
        if (str[i] == sep)
            break;
        else {
            token[i] = str[i];
            i++;
        }
    }
    token[i] = 0;

    if (str[i] == 0 && i == 0)
        return 0;

    return 1;
}

void mapKeys(int useAltKeyMapping) {
    if (useAltKeyMapping == 1) {
        current_key_button1 = alt_key_button1;
        current_key_button2 = alt_key_button2;
        current_key_button3 = alt_key_button3;
        current_key_button4 = alt_key_button4;
        current_key_left = alt_key_left;
        current_key_right = alt_key_right;
        current_key_up = alt_key_up;
        current_key_down = alt_key_down;
        current_key_blue = alt_key_blue;
        current_key_yellow = alt_key_yellow;
        current_key_green = alt_key_green;
    }
    else {
        current_key_button1 = key_button1;
        current_key_button2 = key_button2;
        current_key_button3 = key_button3;
        current_key_button4 = key_button4;
        current_key_left = key_left;
        current_key_right = key_right;
        current_key_up = key_up;
        current_key_down = key_down;
        current_key_blue = key_blue;
        current_key_yellow = key_yellow;
        current_key_green = key_green;
    }
}


void remapKeys(char *keyDef) {
    key_button1[0] = 0;
    key_button2[0] = 0;
    key_button3[0] = 0;
    key_button4[0] = 0;
    key_left[0] = 0;
    key_right[0] = 0;
    key_up[0] = 0;
    key_down[0] = 0;
    key_yellow[0] = 0;
    key_green[0] = 0;
    key_blue[0] = 0;

    startSequence[0] = 0;
    alt_key_button1[0] = 0;
    alt_key_button2[0] = 0;
    alt_key_button3[0] = 0;
    alt_key_button4[0] = 0;
    alt_key_left[0] = 0;
    alt_key_right[0] = 0;
    alt_key_up[0] = 0;
    alt_key_down[0] = 0;
    alt_key_yellow[0] = 0;
    alt_key_green[0] = 0;
    alt_key_blue[0] = 0;

    if (keyDef) {
        printf("\nRemap Keys: %s", keyDef);
        altKeyMappingUsed = 0;
        short *currentKeys = &startSequence;

        char keySequenceBuffer[32];
        char *keySequence;
        char key[32];

        while (getToken(keyDef, &keySequenceBuffer, ';', 31)) {
            int currentKeysSize = 0;
            printf ("\nKey Sequence is %s", &keySequenceBuffer);
            keySequence = &keySequenceBuffer;
            while (getToken(keySequence, &key, ' ', 31)) {
                printf("\nKey is %s %x", &key, interpretKey(&key));

                if (currentKeys)
                    currentKeys[currentKeysSize++] = interpretKey(&key);

                keySequence += strlen(&key);
                if (strlen(keySequence) > 0)
                    keySequence++;
            }
            if (currentKeys) {
                printf("\nset zero to pos %d", currentKeysSize);
                currentKeys[currentKeysSize] = 0;

                if (currentKeysSize == 2) {
                    int i;
                    for (i=0;currentKeys[i] != 0; i++)
                        printf("CK %x ", currentKeys[i]);
                    for (i=0;key_yellow[i] != 0; i++)
                        printf("Y %x ", key_yellow[i]);
                }
            }

            if (currentKeys == &startSequence)
                currentKeys = &key_up;
            else if (currentKeys == &key_up)
                currentKeys = &key_down;
            else if (currentKeys == &key_down)
                currentKeys = &key_left;
            else if (currentKeys == &key_left)
                currentKeys = &key_right;
            else if (currentKeys == &key_right)
                currentKeys = &key_button1;
            else if (currentKeys == &key_button1)
                currentKeys = &key_button2;
            else if (currentKeys == &key_button2)
                currentKeys = &key_button3;
            else if (currentKeys == &key_button3)
                currentKeys = &key_button4;
            else if (currentKeys == &key_button4)
                currentKeys = &key_yellow;
            else if (currentKeys == &key_yellow)
                currentKeys = &key_green;
            else if (currentKeys == &key_green)
                currentKeys = &key_blue;
            else if (currentKeys == &key_blue)
                currentKeys = &alt_key_up;
            else if (currentKeys == &alt_key_up) {
                altKeyMappingUsed = 1;
                currentKeys = &alt_key_down;
            }
            else if (currentKeys == &alt_key_down)
                currentKeys = &alt_key_left;
            else if (currentKeys == &alt_key_left)
                currentKeys = &alt_key_right;
            else if (currentKeys == &alt_key_right)
                currentKeys = &alt_key_button1;
            else if (currentKeys == &alt_key_button1)
                currentKeys = &alt_key_button2;
            else if (currentKeys == &alt_key_button2)
                currentKeys = &alt_key_button3;
            else if (currentKeys == &alt_key_button3)
                currentKeys = &alt_key_button4;
            else if (currentKeys == &alt_key_button4)
                currentKeys = &alt_key_yellow;
            else if (currentKeys == &alt_key_yellow)
                currentKeys = &alt_key_green;
            else if (currentKeys == &alt_key_green)
                currentKeys = &alt_key_blue;
            else
                currentKeys = NULL;

            keyDef += strlen(&keySequenceBuffer);
            if (strlen(keyDef) > 0)
                keyDef ++;
        }

        int i;
        printf("\nRemapped keys:");
        printf("\nUP: ");
        for (i=0;key_up[i] != 0; i++)
            printf("%x ", key_up[i]);
        printf("\nDOWN: ");
        for (i=0;key_down[i] != 0; i++)
            printf("%x ", key_down[i]);
        printf("\nLEFT: ");
        for (i=0;key_left[i] != 0; i++)
            printf("%x ", key_left[i]);
        printf("\nRIGHT: ");
        for (i=0;key_right[i] != 0; i++)
            printf("%x ", key_right[i]);
        printf("\nButton 1: ");
        for (i=0;key_button1[i] != 0; i++)
            printf("%x ", key_button1[i]);
        printf("\nButton 2: ");
        for (i=0;key_button2[i] != 0; i++)
            printf("%x ", key_button2[i]);
        printf("\nButton 3: ");
        for (i=0;key_button3[i] != 0; i++)
            printf("%x ", key_button3[i]);
        printf("\nButton 4: ");
        for (i=0;key_button4[i] != 0; i++)
            printf("%x ", key_button4[i]);
        printf("\nYellow: ");
        for (i=0;key_yellow[i] != 0; i++)
            printf("%x ", key_yellow[i]);
        printf("\nGreen: ");
        for (i=0;key_green[i] != 0; i++)
            printf("%x ", key_green[i]);
        printf("\nBlue: ");
        for (i=0;key_blue[i] != 0; i++)
            printf("%x ", key_blue[i]);
        if (altKeyMappingUsed == 1) {
            printf("\nALT_UP: ");
            for (i=0;alt_key_up[i] != 0; i++)
                printf("%x ", alt_key_up[i]);
            printf("\nALT_DOWN: ");
            for (i=0;alt_key_down[i] != 0; i++)
                printf("%x ", alt_key_down[i]);
            printf("\nALT_LEFT: ");
            for (i=0;alt_key_left[i] != 0; i++)
                printf("%x ", alt_key_left[i]);
            printf("\nALT_RIGHT: ");
            for (i=0;alt_key_right[i] != 0; i++)
                printf("%x ", alt_key_right[i]);
            printf("\nALT_Button 1: ");
            for (i=0;alt_key_button1[i] != 0; i++)
                printf("%x ", alt_key_button1[i]);
            printf("\nALT_Button 2: ");
            for (i=0;alt_key_button2[i] != 0; i++)
                printf("%x ", alt_key_button2[i]);
            printf("\nALT_Button 3: ");
            for (i=0;alt_key_button3[i] != 0; i++)
                printf("%x ", alt_key_button3[i]);
            printf("\nALT_Button 4: ");
            for (i=0;alt_key_button4[i] != 0; i++)
                printf("%x ", alt_key_button4[i]);
            printf("\nALT_Yellow: ");
            for (i=0;alt_key_yellow[i] != 0; i++)
                printf("%x ", alt_key_yellow[i]);
            printf("\nALT_Green: ");
            for (i=0;alt_key_green[i] != 0; i++)
                printf("%x ", alt_key_green[i]);
            printf("\nALT_Blue: ");
            for (i=0;alt_key_blue[i] != 0; i++)
                printf("%x ", alt_key_blue[i]);
        }
    }
    else
        printf("\nRemap Keys Sequence Empty!");
    mapKeys(0);
}

void sendKey(short *keySequence, int eventType)
{
    input_event_t fuse_event;

    int i;

    for (i=0;i<10;i++) {
        if (keySequence[i] == 0)
            return;

        fuse_event.type = eventType;
        fuse_event.types.key.native_key = keySequence[i];
        fuse_event.types.key.spectrum_key = keySequence[i];

//        if (eventType == INPUT_EVENT_KEYPRESS)
//            printf("\nPressing key %x", keySequence[i]);
//        if (eventType == INPUT_EVENT_KEYRELEASE)
//            printf("\nReleasing key %x", keySequence[i]);
        input_event( &fuse_event );
    }
}

void activateButton(int button, int type)
{
    switch (button) {

    case BUTTON_UP:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nUp Key pressed");
        else
            printf("\nUp Key released");
        fflush(stdout);

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Up;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Up;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_up, type);
        break;
    case BUTTON_LEFT:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nLeft Key pressed");
        else
            printf("\nLeft Key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Left;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Left;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_left, type);
        break;
    case BUTTON_RIGHT:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nRight Key pressed");
        else
            printf("\nRight Key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Right;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Right;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_right, type);
        break;
    case BUTTON_DOWN:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nDown Key pressed");
        else
            printf("\nDown Key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Down;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Down;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_down, type);
        break;
    case BUTTON_1:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nButton 1 pressed");
        else
            printf("\nButton 1 released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_button1, type);
        break;
    case BUTTON_4:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nButton 4 pressed");
        else
            printf("\nButton 4 released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_button4, type);
        break;
    case BUTTON_2:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nButton 2 pressed");
        else
            printf("\nButton 2 released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_S;
            fuse_event.types.key.spectrum_key = INPUT_KEY_S;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_button2, type);
        break;
    case BUTTON_3:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nButton 3 pressed");
        else
            printf("\nButton 3 released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_button3, type);
        break;
    case BUTTON_POWER:
        switch (type) {
        case INPUT_EVENT_KEYPRESS:
            printf("\nPowerKey pressed %d", powerKeyPressed);
            fflush(stdout);
            powerKeyPressed++;
            if (powerKeyPressed > 4) {
                printf("\nExiting");
                fflush(stdout);
                if (ui_widget_level > -1) {
                    input_event_t fuse_event;

                    fuse_event.type = INPUT_EVENT_KEYPRESS;
                    fuse_event.types.key.native_key = INPUT_KEY_Escape;
                    fuse_event.types.key.spectrum_key = INPUT_KEY_Escape;

                    input_event( &fuse_event );
                }
                fuse_exit(0);
            }
            break;
        case INPUT_EVENT_KEYRELEASE:
            powerKeyPressed = 0;
            printf("\nPowerKey released");
            fflush(stdout);
            if (ui_widget_level > -1) {
                input_event_t fuse_event;

                fuse_event.type = INPUT_EVENT_KEYPRESS;
                fuse_event.types.key.native_key = INPUT_KEY_Escape;
                fuse_event.types.key.spectrum_key = INPUT_KEY_Escape;

                input_event( &fuse_event );
            }
            else {
                input_event_t fuse_event;

                fuse_event.type = INPUT_EVENT_KEYPRESS;
                fuse_event.types.key.native_key = INPUT_KEY_F1;
                fuse_event.types.key.spectrum_key = INPUT_KEY_F1;

                input_event( &fuse_event );
            }
            break;
        }
        break;
    case BUTTON_BOOTKEY:
        switch (type) {
        case INPUT_EVENT_KEYPRESS:
            printf("\nBootKey pressed");
            bootKeyPressed++;

            if (bootKeyPressed > 3) {
                input_event_t fuse_event;

                fuse_event.type = INPUT_EVENT_KEYPRESS;
                fuse_event.types.key.native_key = INPUT_KEY_F2;
                fuse_event.types.key.spectrum_key = INPUT_KEY_F2;

                input_event( &fuse_event );
            }
            break;
        case INPUT_EVENT_KEYRELEASE:
            printf("\nBootKey released");
            if (bootKeyPressed > 3) {
                ; // Nothing to do
            }
            else {
                if (ui_widget_level > -1) {
                    input_event_t fuse_event;

                    fuse_event.type = INPUT_EVENT_KEYPRESS;
                    fuse_event.types.key.native_key = INPUT_KEY_Escape;
                    fuse_event.types.key.spectrum_key = INPUT_KEY_Escape;

                    input_event( &fuse_event );
                }
                else {
                    if (altKeyMappingUsed == 1) {
                        if (currentKeyMapping == 0)
                            currentKeyMapping = 1;
                        else
                            currentKeyMapping = 0;
                        mapKeys(currentKeyMapping);
                    }
                    else {
                        input_event_t fuse_event;

                        fuse_event.type = INPUT_EVENT_KEYPRESS;
                        fuse_event.types.key.native_key = INPUT_KEY_F3;
                        fuse_event.types.key.spectrum_key = INPUT_KEY_F3;

                        input_event( &fuse_event );
                    }
                }
            }
            bootKeyPressed = 0;
            break;
        }
        break;
    case BUTTON_YELLOW:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nYellow key pressed");
        else
            printf("\nYellow key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_yellow, type);
        break;
    case BUTTON_BLUE:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nBlue key pressed");
        else
            printf("\nBlue key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_blue, type);
        break;
    case BUTTON_GREEN:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\nGreen key pressed");
        else
            printf("\nGreen key released");

        if (ui_widget_level > -1) {
            input_event_t fuse_event;

            fuse_event.type = type;
            fuse_event.types.key.native_key = INPUT_KEY_Return;
            fuse_event.types.key.spectrum_key = INPUT_KEY_Return;

            input_event( &fuse_event );
        }
        else
            sendKey(current_key_green, type);
        break;
    case BUTTON_VOL_UP:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\Volume UP pressed");
        else {
            printf("\nVolume UP released");
            vegaDACVolumeUp();
        }
        break;
    case BUTTON_VOL_DOWN:
        if (type == INPUT_EVENT_KEYPRESS)
            printf("\Volume DOWN pressed");
        else {
            printf("\nVolume DOWN released");
            vegaDACVolumeDown();
        }
        break;
    }
    fflush(stdout);
}

void
keyboard_update( void )
{
  unsigned short keybuf[64];
  static int ignore = 0;

  if (popupScreenAfterBoot > 0) {
      popupScreenAfterBoot--;
      if (popupScreenAfterBoot == 0) {
          popupScreenAfterBoot = 0;
          vegaSetDefaultVolume();
          printf("\npopupScreenAfterBoot");
          fflush(stdout);
          input_event_t fuse_event;

          fuse_event.type = INPUT_EVENT_KEYPRESS;
          fuse_event.types.key.native_key = INPUT_KEY_F1;
          fuse_event.types.key.spectrum_key = INPUT_KEY_F1;

          input_event( &fuse_event );
      }
  }

#ifdef __arm__
  readingState++;
  if (readingState == 7)
      readingState = 1;

  // Check for Power Key and Boot key
  if (readingState == 1) {
      // Check for Power key
      value = vegaReadReg(0x80044000, 0xc0) & 0x100000;

      if (value != 0) {
          keyPressed[0] += 1;
          if (keyPressed[0] == 1 || keyPressed[0] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_POWER, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[0] != 0) {
              keyPressed[0] = 0;
              activateButton(BUTTON_POWER, INPUT_EVENT_KEYRELEASE);
          }
      }

      // Check for Boot key
      lseek(gpio21, 0, SEEK_SET);
      read(gpio21, &gpioValue, 1);
      if (gpioValue == '1') {
          keyPressed[1] += 1;
          if (keyPressed[1] == 1 || keyPressed[1] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_BOOTKEY, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[1] != 0) {
              keyPressed[1] = 0;
              activateButton(BUTTON_BOOTKEY, INPUT_EVENT_KEYRELEASE);
          }
      }

      // Battery monitor code
      int batteryState = vegaReadReg(0x80044000, 0xc0);
      if ((batteryState & 2) != 0) {
          if (isPowerSupplyConnected == 0) {
              resetPowerCharger = 2;
              printf("\nPower supply detected - charging battery");
              fflush(stdout);
              isPowerSupplyConnected = 1;
              vegaWriteReg(0x80044000, 0x30, 0x10420);
              vegaWriteReg(0x80044000, 0x30, 0x420); // was 73f charge max 780mA - stop 80mA
              batteryMonFreq = BATTERY_INFO_PRINT_DELAY - 10;
              vegaSetLed(0, 0);

              int batteryStatus = vegaReadReg(0x80044000, 0xe0);
              batteryStatus = batteryStatus >> 16;
              batteryStatus = batteryStatus & 0x3ff;
              batteryStatus = batteryStatus * 8;
              if (PRINT_BATTERY_STATUS) {
                  printf("\nBattery status: %d", batteryStatus);
                  fflush(stdout);
              }
              if (batteryStatus > MIN_BATTERY_CHARGE_INIT_LCD && backlightStatus == 0) {
                  printf("\nSetting backlight ON");
                  backlightStatus = 1;
                  if (vegaIsTvOUTModeEnabled() == 0) {
                      vegaSetBacklight(1);
                      resetPowerCharger = 2;                      
                  }
                  fflush(stdout);
              }
              if (batteryStatus > MIN_BATTERY_CHARGE_RED_LIGHT)
                  vegaSetLed(1, 1);
              else
                  vegaSetLed(1, 0);
          }
      }
      else if (isPowerSupplyConnected == 1) {
          printf("\nPower supply disconnected - running from battery");
          fflush(stdout);
          isPowerSupplyConnected = 0;
          vegaWriteReg(0x80044000, 0x30, 0x10420);
          vegaSetLed(0, 1);

          int batteryStatus = vegaReadReg(0x80044000, 0xe0);
          batteryStatus = batteryStatus >> 16;
          batteryStatus = batteryStatus & 0x3ff;
          batteryStatus = batteryStatus * 8;
          if (PRINT_BATTERY_STATUS) {
              printf("\nBattery status: %d", batteryStatus);
              fflush(stdout);
          }
          if (batteryStatus > MIN_BATTERY_CHARGE_INIT_LCD && backlightStatus == 0) {
              printf("\nSetting backlight ON");
              fflush(stdout);
              backlightStatus = 1;
              if (vegaIsTvOUTModeEnabled() == 0) {
                  vegaSetBacklight(1);
              }
          }
          if (batteryStatus > MIN_BATTERY_CHARGE_RED_LIGHT)
              vegaSetLed(1, 1);
          else
              vegaSetLed(1, 0);          
      }
      batteryMonFreq++;
      if (chargerResetCounter >= 0)
          chargerResetCounter--;
//      else
//          printf("\nReady to reset charger...");

      if (resetPowerCharger == 1) {
          vegaWriteReg(0x80044000, 0x10, 0x403f583);
          resetPowerCharger = 0;
          printf("\nReset Charger step 2");
      }
      if (resetPowerCharger == 2) {
          vegaWriteReg(0x80044000, 0x10, 0x413f583);
          resetPowerCharger = 1;
          printf("Reset Charger step 1");
          batteryMonFreq = BATTERY_INFO_PRINT_DELAY - 10;
      }

      if (batteryMonFreq > BATTERY_INFO_PRINT_DELAY) {
          int batteryStatus = vegaReadReg(0x80044000, 0xe0);
          batteryStatus = batteryStatus >> 16;
          batteryStatus = batteryStatus & 0x3ff;
          batteryStatus = batteryStatus * 8;

          if (PRINT_BATTERY_STATUS) {
              printf("\nBattery status: %d", batteryStatus);
              fflush(stdout);
          }
          batteryMonFreq = 0;

          if (batteryStatus < 4100 &&
                (batteryState & 2) != 0 && chargerResetCounter < 0) {
              printf("\nBattery status < 4100 and power supply connected -Resetting power charge");
              fflush(stdout);
              resetPowerCharger = 2;
              chargerResetCounter = 1000;
          }

          if (batteryStatus > MIN_BATTERY_CHARGE_INIT_LCD && backlightStatus == 0) {
              printf("\nSetting backlight ON");
              fflush(stdout);
              backlightStatus = 1;
              if (vegaIsTvOUTModeEnabled() == 0) {
                  vegaSetBacklight(1);
              }
          }
          if (batteryStatus > MIN_BATTERY_CHARGE_RED_LIGHT)
              vegaSetLed(1, 1);
          else
              vegaSetLed(1, 0);          
      }
  }

  // Check for LEFT KEYS
  if (readingState == 2) {
      setKeyScan(LEFT_KEYS);
      lseek(gpio14, 0, SEEK_SET);
      read(gpio14, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[2] += 1;
          if (keyPressed[2] == 1 || keyPressed[2] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_1, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[2] != 0) {
              keyPressed[2] = 0;
              activateButton(BUTTON_1, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio15, 0, SEEK_SET);
      read(gpio15, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[3] += 1;
          if (keyPressed[3] == 1 || keyPressed[3] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_4, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[3] != 0) {
              keyPressed[3] = 0;
              activateButton(BUTTON_4, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio26, 0, SEEK_SET);
      read(gpio26, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[4] += 1;
          if (keyPressed[4] == 1 || keyPressed[4] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_2, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[4] != 0) {
              keyPressed[4] = 0;
              activateButton(BUTTON_2, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio27, 0, SEEK_SET);
      read(gpio27, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[5] += 1;
          if (keyPressed[5] == 1 || keyPressed[5] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_3, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[5] != 0) {
              keyPressed[5] = 0;
              activateButton(BUTTON_3, INPUT_EVENT_KEYRELEASE);
          }
      }
  }

  // Check for RIGHT KEYS
  if (readingState == 3) {
      setKeyScan(RIGHT_KEYS);
      lseek(gpio14, 0, SEEK_SET);
      read(gpio14, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[9] += 1;
          if (keyPressed[9] == 1 || keyPressed[9] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_UP, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[9] != 0) {
              keyPressed[9] = 0;
              activateButton(BUTTON_UP, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio15, 0, SEEK_SET);
      read(gpio15, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[10] += 1;
          if (keyPressed[10] == 1 || keyPressed[10] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_LEFT, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[10]  > 0) {
              keyPressed[10] = 0;
              activateButton(BUTTON_LEFT, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio26, 0, SEEK_SET);
      read(gpio26, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[11] += 1;
          if (keyPressed[11] == 1 || keyPressed[11] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_RIGHT, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[11] != 0) {
              keyPressed[11] = 0;
              activateButton(BUTTON_RIGHT, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio27, 0, SEEK_SET);
      read(gpio27, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[12] += 1;
          if (keyPressed[12] == 1 || keyPressed[12] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_DOWN, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[12] != 0) {
              keyPressed[12] = 0;
              activateButton(BUTTON_DOWN, INPUT_EVENT_KEYRELEASE);
          }
      }
  }

  // Check for Little keys
  if (readingState == 4) {
      setKeyScan(LITTLE_KEYS);
      lseek(gpio15, 0, SEEK_SET);
      read(gpio15, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[6] += 1;
          if (keyPressed[6] == 1 || keyPressed[6] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_YELLOW, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[6] != 0) {
              keyPressed[6] = 0;
              activateButton(BUTTON_YELLOW, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio26, 0, SEEK_SET);
      read(gpio26, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[7] += 1;
          if (keyPressed[7] == 1 || keyPressed[7] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_BLUE, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[7] != 0) {
              keyPressed[7] = 0;
              activateButton(BUTTON_BLUE, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio27, 0, SEEK_SET);
      read(gpio27, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[8] += 1;
          if (keyPressed[8] == 1 || keyPressed[8] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_GREEN, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[8] != 0) {
              keyPressed[8] = 0;
              activateButton(BUTTON_GREEN, INPUT_EVENT_KEYRELEASE);
          }
      }
  }

  // Check for Volume keys
  if (readingState == 5) {
      setKeyScan(VOLUME_KEYS);
      lseek(gpio14, 0, SEEK_SET);
      read(gpio14, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[13] += 1;
          if (keyPressed[13] == 1 || keyPressed[13] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_VOL_UP, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[13] != 0) {
              keyPressed[13] = 0;
              activateButton(BUTTON_VOL_UP, INPUT_EVENT_KEYRELEASE);
          }
      }
      lseek(gpio15, 0, SEEK_SET);
      read(gpio15, &gpioValue, 1);
      if (gpioValue == '0') {
          keyPressed[14] += 1;
          if (keyPressed[14] == 1 || keyPressed[14] % KEY_REPEAT_DELAY == 0)
              activateButton(BUTTON_VOL_DOWN, INPUT_EVENT_KEYPRESS);
      }
      else {
          if (keyPressed[14] != 0) {
              keyPressed[14] = 0;
              activateButton(BUTTON_VOL_DOWN, INPUT_EVENT_KEYRELEASE);
          }
      }
  }
  // Check for USB Keyboard
  if (readingState == 6) {
      if (usbKeyboard == -1)
          checkForUSBKeyboard();
      else {
          struct input_event ev;
          ssize_t n;
          input_event_t fuse_event;

          n = read(usbKeyboard, &ev, sizeof ev);
          while (n > 0) {
              unsigned short usbKeys[32];
              int usbKeyLength = 0;
              if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2) {
                  switch (ev.code) {
                  // Menu Keys
                  case 0x1:
                      fuse_event.types.key.native_key = INPUT_KEY_Escape;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Escape;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_Escape;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Escape;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  case 0x3b:
                      fuse_event.type = INPUT_EVENT_KEYPRESS;
                      fuse_event.types.key.native_key = INPUT_KEY_F1;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_F1;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_F1;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_F1;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  case 0x69:
                      usbKeys[usbKeyLength++] = INPUT_KEY_Left;
                      fuse_event.type = INPUT_EVENT_KEYPRESS;
                      fuse_event.types.key.native_key = INPUT_KEY_Left;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Left;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_Left;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Left;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  case 0x67:
                      usbKeys[usbKeyLength++] = INPUT_KEY_Up;
                      fuse_event.type = INPUT_EVENT_KEYPRESS;
                      fuse_event.types.key.native_key = INPUT_KEY_Up;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Up;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_Up;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Up;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  case 0x6a:
                      usbKeys[usbKeyLength++] = INPUT_KEY_Right;
                      fuse_event.type = INPUT_EVENT_KEYPRESS;
                      fuse_event.types.key.native_key = INPUT_KEY_Right;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Right;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_Right;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Right;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  case 0x6c:
                      usbKeys[usbKeyLength++] = INPUT_KEY_Down;
                      fuse_event.type = INPUT_EVENT_KEYPRESS;
                      fuse_event.types.key.native_key = INPUT_KEY_Down;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Down;
                      input_event( &fuse_event );

                      fuse_event.type = INPUT_EVENT_KEYRELEASE;
                      fuse_event.types.key.native_key = INPUT_KEY_Down;
                      fuse_event.types.key.spectrum_key = INPUT_KEY_Down;
                      input_event( &fuse_event );
                      ev.value = 0;
                      break;
                  // Other Keys
                  case 0x2:
                      usbKeys[usbKeyLength++] = E_VKB_1;
                      break;
                  case 0x3:
                      usbKeys[usbKeyLength++] = E_VKB_2;
                      break;
                  case 0x4:
                      usbKeys[usbKeyLength++] = E_VKB_3;
                      break;
                  case 0x5:
                      usbKeys[usbKeyLength++] = E_VKB_4;
                      break;
                  case 0x6:
                      usbKeys[usbKeyLength++] = E_VKB_5;
                      break;
                  case 0x7:
                      usbKeys[usbKeyLength++] = E_VKB_6;
                      break;
                  case 0x8:
                      usbKeys[usbKeyLength++] = E_VKB_7;
                      break;
                  case 0x9:
                      usbKeys[usbKeyLength++] = E_VKB_8;
                      break;
                  case 0x0a:
                      usbKeys[usbKeyLength++] = E_VKB_9;
                      break;
                  case 0x0b:
                      usbKeys[usbKeyLength++] = E_VKB_0;
                      break;

                  case 0x10:
                      usbKeys[usbKeyLength++] = E_VKB_Q;
                      break;
                  case 0x11:
                      usbKeys[usbKeyLength++] = E_VKB_W;
                      break;
                  case 0x12:
                      usbKeys[usbKeyLength++] = E_VKB_E;
                      break;
                  case 0x13:
                      usbKeys[usbKeyLength++] = E_VKB_R;
                      break;
                  case 0x14:
                      usbKeys[usbKeyLength++] = E_VKB_T;
                      break;
                  case 0x15:
                      usbKeys[usbKeyLength++] = E_VKB_Y;
                      break;
                  case 0x16:
                      usbKeys[usbKeyLength++] = E_VKB_U;
                      break;
                  case 0x17:
                      usbKeys[usbKeyLength++] = E_VKB_I;
                      break;
                  case 0x18:
                      usbKeys[usbKeyLength++] = E_VKB_O;
                      break;
                  case 0x19:
                      usbKeys[usbKeyLength++] = E_VKB_P;
                      break;

                  case 0x1e:
                      usbKeys[usbKeyLength++] = E_VKB_A;
                      break;
                  case 0x1f:
                      usbKeys[usbKeyLength++] = E_VKB_S;
                      break;
                  case 0x20:
                      usbKeys[usbKeyLength++] = E_VKB_D;
                      break;
                  case 0x21:
                      usbKeys[usbKeyLength++] = E_VKB_F;
                      break;
                  case 0x22:
                      usbKeys[usbKeyLength++] = E_VKB_G;
                      break;
                  case 0x23:
                      usbKeys[usbKeyLength++] = E_VKB_H;
                      break;
                  case 0x24:
                      usbKeys[usbKeyLength++] = E_VKB_J;
                      break;
                  case 0x25:
                      usbKeys[usbKeyLength++] = E_VKB_K;
                      break;
                  case 0x26:
                      usbKeys[usbKeyLength++] = E_VKB_L;
                      break;
                  case 0x1c:
                      usbKeys[usbKeyLength++] = E_VKB_ENTER;
                      break;

                  case 0x2a:
                      usbKeys[usbKeyLength++] = 4096;
                      break;
                  case 0x2c:
                      usbKeys[usbKeyLength++] = E_VKB_Z;
                      break;
                  case 0x2d:
                      usbKeys[usbKeyLength++] = E_VKB_X;
                      break;
                  case 0x2e:
                      usbKeys[usbKeyLength++] = E_VKB_C;
                      break;
                  case 0x2f:
                      usbKeys[usbKeyLength++] = E_VKB_V;
                      break;
                  case 0x30:
                      usbKeys[usbKeyLength++] = E_VKB_B;
                      break;
                  case 0x31:
                      usbKeys[usbKeyLength++] = E_VKB_N;
                      break;
                  case 0x32:
                      usbKeys[usbKeyLength++] = E_VKB_M;
                      break;
                  case 0xf:
                      usbKeys[usbKeyLength++] = E_VKB_S;
                      break;
                  case 0x33:
                      usbKeys[usbKeyLength++] = 51; // ,
                      break;
                  case 0x34:
                      usbKeys[usbKeyLength++] = 52; // .
                      break;
                  case 0x27:
                      usbKeys[usbKeyLength++] = 39; // ;
                      break;
                  case 0x28:
                      if (ev.value == 1) {
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = 0x101;
                          fuse_event.types.key.spectrum_key = 0x101;
                          input_event( &fuse_event );
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = E_VKB_Z;
                          fuse_event.types.key.spectrum_key = E_VKB_Z;
                          input_event( &fuse_event );
                      }
                      else {
                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = E_VKB_Z;
                          fuse_event.types.key.spectrum_key = E_VKB_Z;
                          input_event( &fuse_event );

                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = 0x101;
                          fuse_event.types.key.spectrum_key = 0x101;
                          input_event( &fuse_event );
                      }
                      ev.value = 0;
                      break;
                  case 0x0d:
                      if (ev.value == 1) {
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = 0x102;
                          fuse_event.types.key.spectrum_key = 0x102;
                          input_event( &fuse_event );
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = E_VKB_Z;
                          fuse_event.types.key.spectrum_key = E_VKB_Z;
                          input_event( &fuse_event );
                      }
                      else {
                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = E_VKB_Z;
                          fuse_event.types.key.spectrum_key = E_VKB_Z;
                          input_event( &fuse_event );

                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = 0x102;
                          fuse_event.types.key.spectrum_key = 0x102;
                          input_event( &fuse_event );
                      }
                      ev.value = 0;
                      break;
                  case 0x0c:
                      usbKeys[usbKeyLength++] = 12; // -
                      break;


                  case 0xe:
//                      usbKeys[usbKeyLength++] = 4096;
//                      usbKeys[usbKeyLength++] = E_VKB_0;
                      if (ev.value == 1) {
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = 4096;
                          fuse_event.types.key.spectrum_key = 4096;
                          input_event( &fuse_event );
                          fuse_event.type = INPUT_EVENT_KEYPRESS;
                          fuse_event.types.key.native_key = KEYBOARD_0;
                          fuse_event.types.key.spectrum_key = KEYBOARD_0;
                          input_event( &fuse_event );
                      }
                      else {
                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = KEYBOARD_0;
                          fuse_event.types.key.spectrum_key = KEYBOARD_0;
                          input_event( &fuse_event );

                          fuse_event.type = INPUT_EVENT_KEYRELEASE;
                          fuse_event.types.key.native_key = 4096;
                          fuse_event.types.key.spectrum_key = 4096;
                          input_event( &fuse_event );
                      }
                      ev.value = 0;
                      break;
                  case 0x39:
                      usbKeys[usbKeyLength++] = E_VKB_SPACE;
                      break;
                  }

                  if (ev.value == 1) {
                      usbKeys[usbKeyLength] = 0;
                      setUSBKeys(&usbKeys);
                  }
              }
              printf("\nRead %d %d", n, usbKeyboard);
//              if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
//                  printf("\n%s 0x%04x (%d)", evval[ev.value], (int)ev.code, (int)ev.code);
              n = read(usbKeyboard, &ev, sizeof ev);
          }
          if (errno == 19) {
              printf("\nUSB Keyboard removed");
              close(usbKeyboard);
              usbKeyboard = -1;
          }
          fflush(stdout);
      }
  }
#endif

  int isVirtualKey = 0;

  while( 1 ) {
    ssize_t available, i;

    if (virtualKeys[0] != 0 && keyDelay >0) {
//        printf("\nDelaying...");
        keyDelay--;
    }

    if (virtualKeys[0] != 0 && keyDelay == 0) {
        isVirtualKey = 1;
        if (doRelease == 1) {
            keybuf[0] = virtualKeys[0] | 0x80;
            printf("\nVirtual keybuf release %d", keybuf[0]);
            fflush(stdout);
            keybuf[1] = 0;
            available = 1;
            doRelease = 0;
            if (virtualKeys[1] == 0)
                virtualKeys[0] = 0;
            else {
                int j=0;
                while (virtualKeys[j+1] != 0) {
                    virtualKeys[j] = virtualKeys[j+1];
                    j++;
                }
                virtualKeys[j] = 0;
                keyDelay = 8;
            }
        }
        else {
            keybuf[0] = virtualKeys[0];
            printf("\nVirtual keybuf press %d", keybuf[0]);
            keybuf[1] = 0;
            available = 1;
            keyDelay = 2;
            doRelease = 1;
        }
    }
    else
        available = read( STDIN_FILENO, &keybuf, sizeof( keybuf ) );

    if( available <= 0 ) return;

    for( i = 0; i < available; i++ ) {
        if( ignore ) {
            ignore--;
        }
        else if( ( keybuf[i] & 0x7f ) == 0 ) {
            ignore = 2; /* ignore extended keysyms */
        }
        else {
            input_key fuse_keysym;
            input_event_t fuse_event;

#ifdef __arm__
            if (keybuf[i] < 4096)
                fuse_keysym = keysyms_remap( keybuf[i] & 0x7f );
            else
                fuse_keysym = keybuf[i];
            fuse_keysym = keysyms_remap( keybuf[i] & 0x7f );
#else
            fuse_keysym = keysyms_remap( keybuf[i] & 0x7f );
#endif

            if( fuse_keysym == INPUT_KEY_NONE ) continue;

            fuse_event.type = ( keybuf[i] & 0x80 ) ?
                                  INPUT_EVENT_KEYRELEASE :
                                  INPUT_EVENT_KEYPRESS;

            if (isVirtualKey == 0) {
                // Now you can check typedef enum input_key in input.h
                switch(fuse_keysym) {
                case INPUT_KEY_Up:
                    activateButton(BUTTON_UP, fuse_event.type);
                    break;
                case INPUT_KEY_Left:
                    activateButton(BUTTON_LEFT, fuse_event.type);
                    break;
                case INPUT_KEY_Right:
                    activateButton(BUTTON_RIGHT, fuse_event.type);
                    break;
                case INPUT_KEY_Down:
                    activateButton(BUTTON_DOWN, fuse_event.type);
                    break;
                case INPUT_KEY_q:
                    activateButton(BUTTON_1, fuse_event.type);
                    break;
                case INPUT_KEY_w:
                    activateButton(BUTTON_2, fuse_event.type);
                    break;
                case INPUT_KEY_e:
                    activateButton(BUTTON_3, fuse_event.type);
                    break;
                case INPUT_KEY_r:
                    activateButton(BUTTON_4, fuse_event.type);
                    break;
                case INPUT_KEY_a:
                    activateButton(BUTTON_YELLOW, fuse_event.type);
                    break;
                case INPUT_KEY_s:
                    activateButton(BUTTON_BLUE, fuse_event.type);
                    break;
                case INPUT_KEY_d:
                    activateButton(BUTTON_GREEN, fuse_event.type);
                    break;
                case INPUT_KEY_Escape:
                    activateButton(BUTTON_POWER, fuse_event.type);
                    break;
                case INPUT_KEY_F1:
                    activateButton(BUTTON_BOOTKEY, fuse_event.type);
                    break;
                default:
                    printf("\nKey not recognized %x", fuse_keysym);
                }
            }
            else {
                fuse_event.types.key.native_key = fuse_keysym;
                fuse_event.types.key.spectrum_key = fuse_keysym;

                input_event( &fuse_event );
//                printf("\nSending %d %d", fuse_event.type, fuse_event.types.key.native_key);
//                fflush(stdout);
            }
        }
     }
  }
}


