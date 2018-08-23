/* fbdisplay.c: Routines for dealing with the linux fbdev display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Darren Salt,
			   Witold Filipczyk
   Copyright (c) 2015 Stuart Brady

   $Id: fbdisplay.c 5434 2016-05-01 04:22:45Z fredm $

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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "fbdisplay.h"
#include "fuse.h"
#include "display.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "settings.h"

int pCs = 32+21;
int pClk = 32+19;
int pMosi = 32+20;
int gpioCs, gpioClk, gpioMosi;

char gamePokes[255] = "";

void setGPIODirection(int gpioNum, int dir)
{
        char buf[64];
        sprintf(&buf[0], "/sys/class/gpio/gpio%d/direction", gpioNum);

//      printf("\nUsing %s", &buf[0]);
        int gpio = open(buf, O_WRONLY);
        if (gpio) {
            if (dir == 1)
                write(gpio, "in", 2);
            else
                write(gpio, "out", 3);
            close(gpio);
        }
        else
            printf("Error writing direction %d %d\n", gpioNum, dir);
}

void spi_write(unsigned int val)
{
    unsigned int mask;

    write(gpioMosi, "0", 1);
    write(gpioClk, "0", 1);
    write(gpioCs, "0", 1);

    for (mask = 0x00800000; mask != 0; mask >>= 1) {
        write(gpioClk, "0", 1);
        if (val & mask)
            write(gpioMosi, "1", 1);
        else
            write(gpioMosi, "0", 1);

        write(gpioClk, "1", 1);
    }

    usleep(10);
    write(gpioMosi, "0", 1);
    write(gpioClk, "0", 1);
    write(gpioCs, "1", 1);
}

int init_pinmux_spi()
{
    int ret = -1;
    char buf[64];
    int gpio;

    gpio = open("/sys/class/gpio/export", O_WRONLY);
    if (gpio > 0) {
        sprintf(buf, "52");
        write(gpio, buf, strlen(buf));
        close(gpio);
    }
    else
        printf("Error exporting gpio52\n");
    setGPIODirection(pMosi, 0);
    gpioMosi = open("/sys/class/gpio/gpio52/value", O_RDWR);
    if (gpioMosi > 0)
        write(gpioMosi, "0", 1);
    else
        printf ("Error opening gpio52\n");

    gpio = open("/sys/class/gpio/export", O_WRONLY);
    if (gpio > 0) {
        sprintf(buf, "51");
        write(gpio, buf, strlen(buf));
        close(gpio);
    }
    else
        printf("Error exporting gpio51\n");
    setGPIODirection(pClk, 0);
    gpioClk = open("/sys/class/gpio/gpio51/value", O_RDWR);
    if (gpioClk > 0)
        write(gpioClk, "0", 1);
    else
        printf ("Error opening gpio51\n");

    gpio = open("/sys/class/gpio/export", O_WRONLY);
    if (gpio > 0) {
        sprintf(buf, "53");
        write(gpio, buf, strlen(buf));
        close(gpio);
    }
    else
        printf("Error exporting gpio53\n");
    setGPIODirection(pCs, 0);
    gpioCs = open("/sys/class/gpio/gpio53/value", O_RDWR);
    if (gpioCs > 0)
        write(gpioCs, "1", 1);
    else
        printf ("Error opening gpio53\n");

    return 0;
}

void write_reg(unsigned short reg, unsigned short val)
{
//    pr_debug("%s: writing %x to %x\n", __func__, reg, val);
    spi_write(0x00700000 | reg);
    spi_write(0x00720000 | val);
}

void init_panel_hw(unsigned short arg)
{
    int i;
    const unsigned short seq[] = {
        0x01, 0x6300,
//      0x01, 0x2100,
        0x02, 0x0200,
        0x03, 0x8286,
        0x04, 0x04c7,
        0x05, 0xa800,
        0x08, 0x06ff,
        0x0a, arg,
        0x0b, 0xd400,
        0x0d, 0x3229,
        0x0E, 0x1200,
        0x0f, 0x0000,
        0x16, 0x9f80,
        0x17, 0x2212,
        0x1e, 0x00fc,
        0x30, 0x0000,
        0x31, 0x0707,
        0x32, 0x0206,
        0x33, 0x0001,
        0x34, 0x0105,
        0x35, 0x0000,
        0x36, 0x0707,
        0x37, 0x0100,
        0x3a, 0x0502,
        0x3b, 0x0502
    };

    for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i += 2)
        write_reg(seq[i], seq[i + 1]);
}

void vegaSetLCD(char brightness, char contrast)
{
    init_pinmux_spi();
    unsigned short r;
    r = brightness;
    r = r << 8;
    r += contrast;
    printf("\nsetLCD %x | %x -> %x", brightness, contrast, r);
    init_panel_hw(r);
}

void vegaSetLed(int greenOrRed, int onOrOff)
{
    char buf[64];
    int gpio;

    gpio = open("/sys/class/gpio/export", O_WRONLY);
    if (gpio > 0) {
        if (greenOrRed == 0)
            sprintf(buf, "16");
        else
            sprintf(buf, "17");
        write(gpio, buf, strlen(buf));
        close(gpio);
    }    
    if (greenOrRed == 0)
        setGPIODirection(16, 0);
    else
        setGPIODirection(17, 0);

    if (greenOrRed == 0)
        gpio = open("/sys/class/gpio/gpio16/value", O_RDWR);
    else
        gpio = open("/sys/class/gpio/gpio17/value", O_RDWR);

    if (gpio > 0) {
        if (onOrOff == 0)
            write(gpio, "0", 1);
        else
            write(gpio, "1", 1);
        close(gpio);
    }
}

void vegaSetBacklight(int offOrOn)
{
    char buf[64];
    int gpio;

    gpio = open("/sys/class/gpio/export", O_WRONLY);
    if (gpio > 0) {
        sprintf(buf, "60");
        write(gpio, buf, strlen(buf));
        close(gpio);
    }
    setGPIODirection(60, 0);
    gpio = open("/sys/class/gpio/gpio60/value", O_RDWR);
    if (gpio > 0) {
        if (offOrOn == 0)
            write(gpio, "0", 1);
        else
            write(gpio, "1", 1);
        close(gpio);
    }
}

#define MAX_VOLUME 7
int audioState = 0;
int currentDACVolume = 7;
int DACvolumeLevel[] = { 0x2000000, 0x29e009e, 0x2ae00ae, 0x2be00be, 0x2ce00ce, 0x2de00de, 0x2ee00ee, 0x2fe00fe };


int vegaGetDACVolume()
{
    return currentDACVolume;
}

void vegaSetDefaultVolume()
{
    audioState = 0;
    currentDACVolume = 4;
    printf("\nDAC Volume default %x %d", DACvolumeLevel[currentDACVolume], currentDACVolume);
#ifdef __arm__
    vegaWriteReg(0x80048000, 0x30, DACvolumeLevel[currentDACVolume]);
    vegaWriteReg(0x80048000, 0x74, 0x1); // Disable HP
    vegaWriteReg(0x80048000, 0x78, 0x1000000); // Enable Speakers
#endif
}

void vegaDACVolumeDown()
{
    if (currentDACVolume > 0)
        currentDACVolume--;
    printf("\nDAC Volume down %x %d", DACvolumeLevel[currentDACVolume], currentDACVolume);
#ifdef __arm__
    vegaWriteReg(0x80048000, 0x30, DACvolumeLevel[currentDACVolume]);
#endif
}

void vegaDACVolumeUp()
{
    if (currentDACVolume < MAX_VOLUME)
        currentDACVolume++;
    printf("\nDAC Volume up %x %d", DACvolumeLevel[currentDACVolume], currentDACVolume);
#ifdef __arm__
    vegaWriteReg(0x80048000, 0x30, DACvolumeLevel[currentDACVolume]);
#endif
}

void vegaSetAudio(int status)
{
#ifdef __arm__
    if (status == 0) {
        vegaWriteReg(0x80048000, 0x74, 0x1); // Disable HP
        vegaWriteReg(0x80048000, 0x78, 0x1000000); // Enable Speakers
    }
    else {
        vegaWriteReg(0x80048000, 0x74, 0x1000000); // Disable Speakers
        vegaWriteReg(0x80048000, 0x78, 0x1); // Enable HP
    }
#endif
    audioState = status;
    printf("\nAudio now %d", audioState);
    fflush(stdout);
}

int vegaGetAudio()
{
//    if (vegaReadReg(0x80048000, 0x70) & 1)
//        return 1;
//    return 0;
    return audioState;
}


#define MAX_BRIGHTNESS_LEVEL 7
#define MAX_CONTRAST_LEVEL 7

char currentBrightnessLevel = MAX_BRIGHTNESS_LEVEL;
int currentContrastLevel = MAX_CONTRAST_LEVEL;

char contrastLevel[] = { 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x10 };
char brightnessLevel[] = { 0x8, 0x10, 0x17, 0x20, 0x27, 0x30, 0x37, 0x40 };

int vegaGetBrightnessLevel()
{
    return currentBrightnessLevel;
}

void vegaBrightnessUp()
{
    if (currentBrightnessLevel < MAX_BRIGHTNESS_LEVEL)
        currentBrightnessLevel++;
    printf("\Brightness level %x, %d", brightnessLevel[currentBrightnessLevel], currentBrightnessLevel);
    vegaSetLCD(brightnessLevel[currentBrightnessLevel], contrastLevel[currentContrastLevel]);
}

void vegaBrightnessDown()
{
    if (currentBrightnessLevel > 0)
        currentBrightnessLevel--;
    printf("\Brightness level %x, %d", brightnessLevel[currentBrightnessLevel], currentBrightnessLevel);
    vegaSetLCD(brightnessLevel[currentBrightnessLevel], contrastLevel[currentContrastLevel]);
}

int vegaGetContrastLevel()
{
    return currentContrastLevel;
}

void vegaContrastUp()
{
    if (currentContrastLevel < MAX_CONTRAST_LEVEL)
        currentContrastLevel++;
    printf("\Contrast level %x, %d", contrastLevel[currentContrastLevel], currentContrastLevel);
    vegaSetLCD(brightnessLevel[currentBrightnessLevel], contrastLevel[currentContrastLevel]);
}

void vegaContrastDown()
{
    if (currentContrastLevel > 0)
        currentContrastLevel--;
    printf("\Contrast level %x, %d", contrastLevel[currentContrastLevel], currentContrastLevel);
    vegaSetLCD(brightnessLevel[currentBrightnessLevel], contrastLevel[currentContrastLevel]);
}

void vegaSetPokes(char *str)
{
    if (str)
        strcpy(&gamePokes[0], str);
    else
        gamePokes[0] = 0;
}

void vegaApplyPokes()
{
    if (gamePokes[0] != 0) {
        printf("\nApplying pokes:");
        char poke[24] = "set ";
        int j=4;
        int i=0;
        while (gamePokes[i] != 0) {
            if (gamePokes[i] == ';') {
                poke[j] = 0;
                printf("\n%s", &poke[0]);
                debugger_command_evaluate(&poke[0]);
                j = 4;
            }
            else {
                poke[j] = gamePokes[i];
                if (poke[j] == ',')
                    poke[j] = ' ';
                j++;
            }
            i++;
        }
        if (j != 4) {
            poke[j] = 0;
            printf("\n%s", &poke[0]);
            debugger_command_evaluate(&poke[0]);
        }
    }
    else
        printf("\nNo pokes founds");
}

/* A copy of every pixel on the screen */
libspectrum_word
  fbdisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t fbdisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* The environment variable specifying which device to use */
static const char * const DEVICE_VARIABLE = "FRAMEBUFFER";

/* The device we will use if device_env_variable is not specified */
static const char * const DEFAULT_DEVICE = "/dev/fb0";

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* Are we in a Timex display mode? */
static int hires;

static void register_scalers( void );

/* probably 0rrrrrgggggbbbbb */
static short rgbs[16], greys[16];

static int fb_fd = -1;		/* The framebuffer's file descriptor */
static libspectrum_word *gm = 0;

static struct fb_fix_screeninfo fixed;
static struct fb_var_screeninfo orig_display, display;
static int got_orig_display = 0;
static int changed_palette = 0;

unsigned long fb_resolution; /* == xres << 16 | yres */
#define FB_RES(X,Y) ((X) << 16 | (Y))
#define FB_WIDTH (fb_resolution >> 16)
#define IF_FB_WIDTH(X) ((fb_resolution >> 16) == (X))
#define IF_FB_HEIGHT(Y) ((fb_resolution & 0xFFFF) == (Y))
#define IF_FB_RES(X, Y) (fb_resolution == FB_RES ((X), (Y)))

typedef struct {
  int xres, yres, pixclock;
  int left_margin, right_margin, upper_margin, lower_margin;
  int hsync_len, vsync_len;
  int sync, doublescan;
} fuse_fb_mode_t;

/* Test monitor is a Compaq V50, default settings
 * A = working on ATI RV100 (Radeon 7000) (2.6.x, old radeonfb)
 * M = working on Matrox MGA2064W (Millennium I) (2.6.x, matroxfb)
 * Large/tall/wide = size indication (overscan)
 * The 640x480 modes are from /etc/fb.modes and should work anywhere.
 *
 *    x    y  clock   lm  rm  tm  bm   hl  vl  s  d
 */
static const fuse_fb_mode_t fb_modes_320[] = {
  { 320, 240, 64104,  46, 28, 14,  4,  20,  3, 3, 1 }, /* 320x240-72  72.185 M wide */
  { 0 } /* end of list */
};
static const fuse_fb_mode_t fb_modes_640[] = {
  { 640, 480, 37037, 69, 12, 39,  5,  63,  5, 0, 0 },
  { 0 } /* end of list */
};
static const fuse_fb_mode_t fb_modes_doublescan_alt[] = {
  { 640, 240, 39722,  36, 12, 18,  7,  96,  2, 1, 1 }, /* 640x240-60  60.133 AM large */
  { 640, 480, 32052,  96, 56, 28,  9,  40,  3, 0, 0 }, /* 640x480-72  72.114 */
  { 640, 480, 39722,  48, 16, 33, 10,  96,  2, 0, 0 }, /* 640x480-60  59.940 std */
  { 320, 240, 79444,  18,  6, 18,  7,  48,  2, 1, 1 }, /* 320x240-60  60.133 AM large */
  { 0 } /* end of list */
};
/* Modes not used but which may work are listed here.
 *    x    y  clock   lm  rm  tm  bm   hl  vl  s  d
  { 640, 480, 22272,  48, 32, 17, 22, 128, 12, 0, 0 }, ** 640x480-100 99.713
  { 640, 480, 25057, 120, 32, 14, 25,  40, 14, 0, 0 }, ** 640x480-90  89.995
  { 640, 480, 31747, 120, 16, 16,  1,  64,  3, 0, 0 }, ** 640x480-75  74.998
  { 320, 240, 80000,  40, 28,  9,  2,  20,  3, 0, 1 }, ** 320x240-60  60.310 M tall
  { 320, 240, 55555,  52, 16, 12,  0,  28,  2, 0, 1 }, ** 320x240-85  85.177 M
 */

static unsigned short red16[256], green16[256], blue16[256], transp16[256];
static struct fb_cmap orig_cmap = {0, 256, red16, green16, blue16, transp16};

static int fb_set_mode( void );

int uidisplay_init( int width, int height )
{
  hires = ( width == 640 ? 1 : 0 );

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  register_scalers();

  display_ui_initialised = 1;

  display_refresh_all();

  printf("\nUIDISPLAY Hires %d", hires);
  printf("\nUIDISPLAY image w h %d %d", image_width, image_height);

  return 0;
}

static void
register_scalers( void )
{
  scaler_register_clear();
  scaler_select_bitformat( 565 );		/* 16bit always */
  scaler_register( SCALER_NORMAL );
}

static void
linear_palette(struct fb_cmap *p_cmap)
{
  int i;
  int rcols = 1 << display.red.length;
  int gcols = 1 << display.green.length;
  int bcols = 1 << display.blue.length;

  for (i = 0; i < rcols; i++)
    p_cmap->red[i] = (65535 / (rcols - 1)) * i;

  for (i = 0; i < gcols; i++)
    p_cmap->green[i] = (65535 / (gcols - 1)) * i;

  for (i = 0; i < bcols; i++)
    p_cmap->blue[i] = (65535 / (bcols - 1)) * i;
}

int fbdisplay_init(void)
{
  int i;
  const char *dev;

  static libspectrum_word r16[256], g16[256], b16[256];
  static struct fb_cmap fb_cmap = {
    0, 256, r16, g16, b16, NULL
  };

  dev = getenv( DEVICE_VARIABLE );
  if( !dev || !*dev ) dev = DEFAULT_DEVICE;

  fb_fd = open( dev, O_RDWR | O_EXCL );
  if( fb_fd == -1 ) {
    fprintf( stderr, "%s: couldn't open framebuffer device '%s'\n",
	     fuse_progname, dev );
    return 1;
  }
  if( ioctl( fb_fd, FBIOGET_FSCREENINFO, &fixed )        ||
      ioctl( fb_fd, FBIOGET_VSCREENINFO, &orig_display )    ) {
    fprintf( stderr, "%s: couldn't read info from framebuffer device '%s'\n",
             fuse_progname, dev );
    return 1;
  }
  got_orig_display = 1;

  if( fb_set_mode() ) return 1;

  printf("\nSelected Mode %d %d %d %d %d %d", display.xres, display.yres, display.xres_virtual, display.yres_virtual, display.bits_per_pixel, hires);


//  fputs( "\x1B[H\x1B[J", stdout );	/* clear tty */
  memset( gm, 0, display.xres_virtual * display.yres_virtual * 4 );

//  display.activate = FB_ACTIVATE_NOW;
//  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
//    fprintf( stderr, "%s: couldn't set mode for framebuffer device '%s'\n",
//	     fuse_progname, dev );
//    return 1;
//  }

//  ioctl( fb_fd, FBIOGET_VSCREENINFO, &display);
  for( i = 0; i < 16; i++ ) {
    int v = ( i & 8 ) ? 0xff : 0xbf;
    int c;
     rgbs[i] = ( ( i & 1 ) ? (v >> (8 - display.blue.length)) << display.blue.offset  : 0 )
           | ( ( i & 2 ) ? (v >> (8 - display.red.length)) << display.red.offset   : 0 )
           | ( ( i & 4 ) ? (v >> (8 - display.green.length)) << display.green.offset : 0 );

     c = (( i & 1 ) ? (v * 0.114) : 0.0) +
     (( i & 2) ? (v * 0.299) : 0.0) +
     (( i & 4) ? (v * 0.587) : 0.0) + 0.5;
     greys[i] = (c >> (8 - display.red.length) << display.red.offset)
     | (c >> (8 - display.green.length) << display.green.offset)
     | (c >> (8 - display.blue.length) << display.blue.offset);
  }
  linear_palette(&fb_cmap);

  if (orig_display.bits_per_pixel == 8 || fixed.visual == FB_VISUAL_DIRECTCOLOR) {
    ioctl( fb_fd, FBIOGETCMAP, &orig_cmap);
    changed_palette = 1;
  }
  ioctl( fb_fd, FBIOGET_FSCREENINFO, &fixed);
  if ( fixed.visual == FB_VISUAL_DIRECTCOLOR) {
    ioctl( fb_fd, FBIOPUTCMAP, &fb_cmap );
  }
//  sleep( 1 ); /* give the monitor time to sync before we start emulating */

//  fputs( "\x1B[?25l", stdout );		/* hide cursor */
  fflush( stdout );

  return 0;
}

static int
fb_select_mode( const fuse_fb_mode_t *fb_mode )
{
//  printf("\nfbSelectMode %d %d %d", fb_mode->xres, fb_mode->yres, fb_mode->doublescan );

  memset (&display, 0, sizeof (struct fb_var_screeninfo));
  display.xres_virtual = display.xres = fb_mode->xres;
  display.yres_virtual = display.yres = fb_mode->yres;
  display.xoffset = display.yoffset = 0;
  display.grayscale = 0;
  display.nonstd = 0;
  display.accel_flags = 0;
  display.pixclock = fb_mode->pixclock;
  display.left_margin = fb_mode->left_margin;
  display.right_margin = fb_mode->right_margin;
  display.upper_margin = fb_mode->upper_margin;
  display.lower_margin = fb_mode->lower_margin;
  display.hsync_len = fb_mode->hsync_len;
  display.vsync_len = fb_mode->vsync_len;
  display.sync = fb_mode->sync;
  display.vmode &= ~FB_VMODE_MASK;
  if( fb_mode->doublescan ) display.vmode |= FB_VMODE_DOUBLE;
  display.vmode |= FB_VMODE_CONUPDATE;

  display.bits_per_pixel = 16;
  display.red.length = display.green.length = display.blue.length = 5;

  display.red.offset = 0;
  display.green.offset = 5;
  display.blue.offset = 10;

  gm = mmap( 0, fixed.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0 );
  if( gm == (void*)-1 ) {
    fprintf (stderr, "%s: couldn't mmap for framebuffer: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  display.bits_per_pixel = 32;
  display.red.length = display.green.length = display.blue.length = 8;

  display.red.offset = 0;
  display.green.offset = 8;
  display.blue.offset = 16;

  display.activate = FB_ACTIVATE_TEST;
  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
    munmap( gm, fixed.smem_len );
    return 1;
  }

  display.bits_per_pixel = 16;
  display.red.length = display.green.length = display.blue.length = 5;

  display.red.offset = 0;
  display.green.offset = 5;
  display.blue.offset = 10;

  fb_resolution = FB_RES( display.xres, display.yres );
  return 0;			/* success */
}

int vegaWriteReg(int base, int offset, int value)
{
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) {
        printf("\nCannot access memory.\n");
        return -1;
    }

    int page = getpagesize();
    int length = page * ((0x2000 + page - 1) / page);
    int *regs = (int *)mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);

    if (regs  == MAP_FAILED) {
        close(fd);
        printf("regs mmap error\n");
        return -1;
    }
    *(regs + (offset/4)) = value;
//    printf("\nWrite REG %x, %x = %x", base, offset, value);

    munmap(regs, length);
    close(fd);
    return 0;
}

int vegaReadReg(int base, int offset)
{
#ifdef __arm__
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) {
        printf("\nCannot access memory.\n");
        return -1;
    }

    int page = getpagesize();
    int length = page * ((0x2000 + page - 1) / page);
    int *regs = (int *)mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);

    if (regs  == MAP_FAILED) {
        close(fd);
        printf("regs mmap error\n");
        return -1;
    }

    int actual = *(regs + (offset/4));
//    printf("\Read REG %x, %x = %x", base, offset, actual);

    munmap(regs, length);
    close(fd);
    return actual;
#else
    return 0;
#endif
}

int vegaIsTvOUTModeEnabled( void )
{
#ifdef __arm__
    int f = open("/sys/class/graphics/fb0/virtual_size", O_RDONLY);
    if (f) {
        char s[20];
        int r = read(f, &s[0], 10);
        s[r] = 0;
        printf("\nFB Mode found %s", &s[0]);
        close(f);
        if (s[0] == '6') {
            return 1;
        }
    }
    return 0;
#else
    return 0;
#endif
}

static int
fb_set_mode( void )
{
  size_t i;

  const fuse_fb_mode_t *fb_modes = fb_modes_320;

  if (vegaIsTvOUTModeEnabled() == 1)
      fb_modes = fb_modes_640;
  vegaSetLCD(0x40, 0x10);

  fb_select_mode(fb_modes);
  return 0;
}

int
uidisplay_hotswap_gfx_mode( void )
{
  return 0;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int start, int width, int height)
{
  int y;
  const short *colours = settings_current.bw_tv ? greys : rgbs;
  uint8_t r5, g6, b5;
  libspectrum_word gb, ar;
  libspectrum_word color;

  switch( fb_resolution ) {
  case FB_RES( 640, 480 ):
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

        for( i = 0, point = gm + y * display.xres_virtual + x;
             i < width;
             i++, point++ )
          *point = colours[fbdisplay_image[y][x+i]];
      } else {
        // This is used by TV-OUT
        for( i = 0, point = gm + 2 * (y+20) * display.xres_virtual + x * 2;
             i < width;
             i++, point += 2 ) {
            short col = colours[fbdisplay_image[y][x+i]];

            b5 = ((((col >> 10) & 0x1F) * 527) + 23) >> 6;
            g6 = ((((col >> 5) & 0x1F) * 527) + 23) >> 6;
            r5 = (((col & 0x1F) * 527) + 23) >> 6;

            short ic = (r5 << 11) | (g6 << 5) | b5;

          *  point       = *( point +     display.xres_virtual ) =
          *( point + 1 ) = *( point + 1 + display.xres_virtual ) =
            ic;
        }
      }
    }
    break;

  case FB_RES( 640, 240 ):
    if( hires ) { start >>= 1; height >>= 1; }
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

    for ( i = 0, point = gm + y * display.xres_virtual + x;
          i < width;
          i++, point++ )
      *point = colours[fbdisplay_image[y*2][x+i]];

      } else {

    for( i = 0, point = gm + y * display.xres_virtual + x * 2;
         i < width;
         i++, point+=2 )
      *point = *(point+1) = colours[fbdisplay_image[y][x+i]];

      }
    }
    break;

  case FB_RES( 320, 240 ):
    if( hires ) { start >>= 1; height >>= 1; x >>= 1; width >>= 1; }
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

    /* Drop every second pixel */
    for ( i = 0, point = gm + y * display.xres_virtual + x;
          i < width;
          i++, point++ )
      *point = colours[fbdisplay_image[y*2][(x+i)*2]];

      } else {
        // This is used by LCD
        for( i = 0, point = gm + 2 * y * display.xres_virtual + (2*x);
         i < width;
             i++) {

            color = colours[fbdisplay_image[y][x+i]];

            b5 = ((((color >> 10) & 0x1F) * 527) + 23) >> 6;
            g6 = ((((color >> 5) & 0x1F) * 527) + 23) >> 6;
            r5 = (((color & 0x1F) * 527) + 23) >> 6;

            gb = g6;
            gb = (gb << 8) | b5;
            ar = r5;

            *point++ = gb;
            *point++ = ar;
        }

      }

    }
    break;

  default:;		/* Shut gcc up */
  }
}

int
uidisplay_end( void )
{
  return 0;
}

int
fbdisplay_end( void )
{
  if( fb_fd != -1 ) {
    if( got_orig_display ) {
      ioctl( fb_fd, FBIOPUT_VSCREENINFO, &orig_display );
      if (changed_palette) {
        ioctl( fb_fd, FBIOPUTCMAP, &orig_cmap);
	changed_palette = 0;
      }
    }
    close( fb_fd );
    fb_fd = -1;
    fputs( "\x1B[H\x1B[J\x1B[?25h", stdout );	/* clear screen, show cursor */
  }

  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    fbdisplay_image[y  ][x  ] = colour;
    fbdisplay_image[y  ][x+1] = colour;
    fbdisplay_image[y+1][x  ] = colour;
    fbdisplay_image[y+1][x+1] = colour;
  } else {
    fbdisplay_image[y][x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      fbdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      fbdisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      fbdisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      fbdisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      fbdisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      fbdisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      fbdisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      fbdisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      fbdisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      fbdisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      fbdisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      fbdisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      fbdisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      fbdisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      fbdisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      fbdisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    fbdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    fbdisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    fbdisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    fbdisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    fbdisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    fbdisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    fbdisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    fbdisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4; y <<= 1;

  for( i=0; i<2; i++,y++ ) {
    fbdisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    fbdisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    fbdisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    fbdisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    fbdisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    fbdisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    fbdisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    fbdisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    fbdisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    fbdisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    fbdisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    fbdisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    fbdisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    fbdisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    fbdisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    fbdisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

void
uidisplay_frame_save( void )
{
  /* FIXME: Save current framebuffer state as the widget UI wants to scribble
     in here */
}

void
uidisplay_frame_restore( void )
{
  /* FIXME: Restore saved framebuffer state as the widget UI wants to draw a
     new menu */
}
