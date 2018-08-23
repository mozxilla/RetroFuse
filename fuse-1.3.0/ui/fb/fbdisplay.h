/* fbdisplay.h: Routines for dealing with the framebuffer display
   Copyright (c) 2003 Philip Kendall

   $Id: fbdisplay.h 2889 2007-05-26 17:45:08Z zubzero $

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

#ifndef FUSE_FBDISPLAY_H
#define FUSE_FBDISPLAY_H

int fbdisplay_init( void );
int fbdisplay_end( void );

int vegaIsTvOUTModeEnabled( void );
int vegaWriteReg(int base, int offset, int value);
int vegaReadReg(int base, int offset);
void vegaSetLCD(char brightness, char contrast);
void vegaSetLed(int greenOrRed, int onOrOff);
void vegaSetBacklight(int offOrOn);

int vegaGetDACVolume();
void vegaDACVolumeUp();
void vegaDACVolumeDown();
void vegaSetDefaultVolume();
void vegaSetAudio(int status);
int vegaGetAudio();
int vegaGetBrightnessLevel();
void vegaBrightnessUp();
void vegaBrightnessDown();
int vegaGetContrastLevel();
void vegaContrastUp();
void vegaContrastDown();

void vegaSetPokes(char *str);
void vegaApplyPokes();


#endif			/* #ifndef FUSE_FBDISPLAY_H */
