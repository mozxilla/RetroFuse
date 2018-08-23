/* virtualkeyboard.c: The virtual keyboard widget
   Copyright (c) 2008 Fredrick Meunier

   $Id: virtualkeyboard.c 3690 2008-06-23 23:03:27Z fredm $

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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fuse.h"
#include "ui/uidisplay.h"
#include "ui/fb/fbkeyboard.h"
#include "widget_internals.h"

#include "unistd.h"
#include "ctype.h"

#include "stdlib.h"
#include "string.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#include "ui/fb/fbdisplay.h"
#include "sound.h"

#define MENU_MSG_INFO -1
#define MENU_VIRTUAL_KEYBOARD 0
#define MENU_GAME_SELECT 1
#define MENU_GAME_INFO 2
#define MENU_SETTINGS 3
#define MENU_SD_GAME_SELECT 4
#define MENU_SAVE_GAME 5
#define MENU_LOAD_GAME 6
#define MENU_GAME_FAV 7
#define MENU_GAME_REMAP_KEYS 8
#define MENU_GAME_REMAP_KEYS2 9
#define MENU_HALL_OF_FAME 10

int menuWidgetType;

int isSDCardGame = 0;
int isCapsLockOn = 0;
int isSymOn = 0;
char lastFilePath[255] = "";

int batteryChargingLevel[] = { 3920, 3960, 3976, 3992, 4016, 4056, 4088, 4128, 4160, 4192 };
int batteryDischargingLevel[] = { 4056, 3992, 3920, 3864, 3808, 3768, 3736, 3720, 3696, 3664, 3620 };

enum E_VIRTUAL_KEYBOARD_BUTTON_TYPE
{
  E_VKB_BUTTON_TYPE_SPECTRUM_KEY=0,
  E_VKB_BUTTON_TYPE_MOVE_KEYBOARD,
};

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

enum E_KEYMAP_FUNCTION
{
  // spectrum key events
  E_KEYMAPFUNC_NONE=-1,
  E_KEYMAPFUNC_KEY_1,
  E_KEYMAPFUNC_KEY_2,
  E_KEYMAPFUNC_KEY_3,
  E_KEYMAPFUNC_KEY_4,
  E_KEYMAPFUNC_KEY_5,
  E_KEYMAPFUNC_KEY_6,
  E_KEYMAPFUNC_KEY_7,
  E_KEYMAPFUNC_KEY_8,
  E_KEYMAPFUNC_KEY_9,
  E_KEYMAPFUNC_KEY_0,
  E_KEYMAPFUNC_KEY_Q,     //10
  E_KEYMAPFUNC_KEY_W,
  E_KEYMAPFUNC_KEY_E,
  E_KEYMAPFUNC_KEY_R,
  E_KEYMAPFUNC_KEY_T,
  E_KEYMAPFUNC_KEY_Y,
  E_KEYMAPFUNC_KEY_U,
  E_KEYMAPFUNC_KEY_I,
  E_KEYMAPFUNC_KEY_O,
  E_KEYMAPFUNC_KEY_P,
  E_KEYMAPFUNC_KEY_A, //20
  E_KEYMAPFUNC_KEY_S,
  E_KEYMAPFUNC_KEY_D,
  E_KEYMAPFUNC_KEY_F,
  E_KEYMAPFUNC_KEY_G,
  E_KEYMAPFUNC_KEY_H,
  E_KEYMAPFUNC_KEY_J,
  E_KEYMAPFUNC_KEY_K,
  E_KEYMAPFUNC_KEY_L,
  E_KEYMAPFUNC_KEY_ENTER,
  E_KEYMAPFUNC_KEY_CAPS_SHIFT,    //30
  E_KEYMAPFUNC_KEY_Z,
  E_KEYMAPFUNC_KEY_X,
  E_KEYMAPFUNC_KEY_C,
  E_KEYMAPFUNC_KEY_V,
  E_KEYMAPFUNC_KEY_B,
  E_KEYMAPFUNC_KEY_N,
  E_KEYMAPFUNC_KEY_M,
  E_KEYMAPFUNC_KEY_SYMBOL_SHIFT,
  E_KEYMAPFUNC_KEY_SPACE,
  E_KEYMAPFUNC_KEY_ESCAPE,                //40

  // non-mappable emulator events
  E_KEYMAPFUNC_EMULATOR_VIRTUAL_KEYBOARD_CURSOR_UP,       //41
  E_KEYMAPFUNC_EMULATOR_VIRTUAL_KEYBOARD_CURSOR_DOWN,
  E_KEYMAPFUNC_EMULATOR_VIRTUAL_KEYBOARD_CURSOR_LEFT,
  E_KEYMAPFUNC_EMULATOR_VIRTUAL_KEYBOARD_CURSOR_RIGHT,
  E_KEYMAPFUNC_EMULATOR_VIRTUAL_KEYBOARD_PRESS_KEY,

  // mappable emulator events
  E_KEYMAPFUNC_EMULATOR_MAIN_MENU,                                        // 46 ! THIS MUST BE THE FIRST MAPPABLE EVENT !
  E_KEYMAPFUNC_EMULATOR_TOGGLE_VIRTUAL_KEYBOARD,
  E_KEYMAPFUNC_EMULATOR_RESET_SPECTRUM,
  //      E_KEYMAPFUNC_EMULATOR_FILE_BROWSER,             //TODO
  E_KEYMAPFUNC_EMULATOR_SAVE_CURRENT_SLOT_SNAPSHOT,
  E_KEYMAPFUNC_EMULATOR_LOAD_CURRENT_SLOT_SNAPSHOT,
  E_KEYMAPFUNC_EMULATOR_FAST_FWD_EMULATION,
  E_KEYMAPFUNC_EMULATOR_NEXT_GRAPHIC_FILTER,
  E_KEYMAPFUNC_EMULATOR_PREV_GRAPHIC_FILTER,
  //E_KEYMAPFUNC_EMULATOR_NEXT_RZX,
  // add more here

  NUM_KEYMAPFUNCTIONS,
};

enum VIRTUAL_KEYBOARD_STATE {
  E_VKS_ACCEPTING_INPUT=0,
  E_VKS_POSITIONING_MODE,
};

typedef struct {
  int type;
  int button_enum;
  int keymap_function;
  int button_up;
  int button_down;
  int button_left;
  int button_right;
  int x_pos;
  int y_pos;
  int width;
  int height;
  int toggle_button_is_pressed_down;
  char text[8];
  int flags;
} VIRTUAL_KEYBOARD_BUTTON;

typedef struct {
  VIRTUAL_KEYBOARD_BUTTON buttons[NUM_VIRTUAL_KEYBOARD_BUTTONS];
  int is_active;
  int state;
  int was_active;                         // was it active before we took control?
  int get_keymap_mode;                    // are we getting a key to map to the joypad in the Map Spectrum Keys menu?

  int key_held;                           // which key is being held
  int cur_button_enum;                    // which button the cursor is on
  int send_key_events;

  VIRTUAL_KEYBOARD_BUTTON* cur_button;

  int x_pos;
  int y_pos;
  int width;
  int height;
} VIRTUAL_KEYBOARD;

#define VKB_FLAG_TOGGLE_BUTTON  0x1

const int VIRTUAL_KEYBOARD_WIDTH=270;
const int VIRTUAL_KEYBOARD_HEIGHT=80;

const char * keyDefinition[] = {"Start...", "Up..", "Down...", "Left...", "Right...", "Button F...", "Button 1...", "Button 2...", "Button S...", "Yellow Btn...", "Green Btn...", "Blue Btn..."};

char keyBuffer[64] = {'\xb', '\x00'};
VIRTUAL_KEYBOARD virtual_keyboard;
//int isKeyboardWidget;
//int isGameSelectWidget;
//int isDisplayGameInfoWidget;
//int isSettingsWidget;
//int isSDGameSelectWidget;

typedef struct {
  char *title;
  char *filename;
  char *M;
  char *keys;
  char *description;
  char *hint;
  char *pokes;
  u_int8_t isFavourite;
} GAME_DATA;

GAME_DATA *gameData = NULL;
int gameListOffset = 0;
int selectedGameOnScreen = 0;
int currentGameId = -1;
int selectedLetter = E_VKB_G;
int numGamesFound;
#define MAX_GAME_NUM 1100
#define NUM_GAMES_ON_SCREEN 44
#define MAX_HINT_LINE_LENGTH 46
#define MAX_HINT_LINES 4
#define HINT_LINE_START 18

#define MAX_SETTINGS_ITEMS 4
int currentSettingsItem = 0;
int currentSaveGame = 0;
int currentLoadGame = 0;

GAME_DATA *sdGameData = NULL;
int sdGameListOffset = 0;
int sdSelectedGameOnScreen = 0;
int sdCurrentGameId = -1;
int sdNumGamesFound;

int lastGameLoadFromSD = -1;
int lastGameLoaded = -1;

int favListOffset = 0;
int selectedFavGame = 0;
int favGamesFound = 0;

char lastGameFilename[256];
char lastKeyMapping[256];
int currentKeyMapIndex = 0;
short lastSelectedKey;
char lastSelectedKeyName[24];

#define MAX_NAMES_NUM 4580
#define NUM_NAMES_ON_SCREEN 44
int hofIndex = 0;

void
SetCurrentVirtualKeyboardButton(int button)
{
  selectedLetter = button;
  virtual_keyboard.cur_button_enum = button;
  if (button == E_VKB_NONE)
    virtual_keyboard.cur_button = 0;
  else
    virtual_keyboard.cur_button = &virtual_keyboard.buttons[button];
}

void
MoveVirtualKeyboardCursorLeft( void )
{
  if (virtual_keyboard.cur_button)
    SetCurrentVirtualKeyboardButton(virtual_keyboard.cur_button->button_left);
}

void
MoveVirtualKeyboardCursorRight( void )
{
  if (virtual_keyboard.cur_button)
    SetCurrentVirtualKeyboardButton(virtual_keyboard.cur_button->button_right);
}

void
MoveVirtualKeyboardCursorUp( void )
{
  if (virtual_keyboard.cur_button)
    SetCurrentVirtualKeyboardButton(virtual_keyboard.cur_button->button_up);
}


void
MoveVirtualKeyboardCursorDown( void )
{
  if (virtual_keyboard.cur_button)
    SetCurrentVirtualKeyboardButton(virtual_keyboard.cur_button->button_down);
}

void
DrawVirtualKeyboardButton( VIRTUAL_KEYBOARD_BUTTON* button, int x, int y )
{
  //char text_col   = 15; /* Bright White */
  char text_col   = 0; /* Black */
  char bkgnd_col  = 12; /* Bright Green */
  //char button_col = 15; /* Bright White */
  char button_col = 0; /* Black */

  x += virtual_keyboard.x_pos;
  y += virtual_keyboard.y_pos;

  /* dont draw toggle buttons if getting keymap key */
  if( button->flags & VKB_FLAG_TOGGLE_BUTTON
      && button->toggle_button_is_pressed_down &&
      !virtual_keyboard.get_keymap_mode ) {
    bkgnd_col = 7; /* White */
    if( button->button_enum == virtual_keyboard.cur_button_enum ) {
      text_col = 15; /* Bright White */
    } else {
      text_col = 0;  /* Black */
    }
    widget_draw_rectangle_solid( x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                 y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                 button->width, button->height, bkgnd_col );
  } else if( button->button_enum == virtual_keyboard.cur_button_enum ) {
    if (isSymOn && (button->button_enum == E_VKB_SYMBOL_SHIFT))
        bkgnd_col = 11;
    if (isCapsLockOn && (button->button_enum == E_VKB_CAPS_SHIFT))
        bkgnd_col = 11;

    widget_draw_rectangle_solid( x + 1 + 8*DISPLAY_BORDER_WIDTH_COLS,
                                 y + 1 + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                 button->width-2, button->height-2, bkgnd_col );
  } else {
      bkgnd_col = 15;
    if (isSymOn && (button->button_enum == E_VKB_SYMBOL_SHIFT))
        bkgnd_col = 10;
    if (isCapsLockOn && (button->button_enum == E_VKB_CAPS_SHIFT))
        bkgnd_col = WIDGET_COLOUR_HIGHLIGHT;

    widget_draw_rectangle_solid( x + 1 + 8*DISPLAY_BORDER_WIDTH_COLS,
                                 y + 1 + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                 button->width-2, button->height-2, bkgnd_col );
  }

  if( virtual_keyboard.state == E_VKS_POSITIONING_MODE ) {
    text_col = 7;   /* White */
    button_col = 7; /* White */
  }

  widget_draw_rectangle_outline_rounded( x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                         y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                         button->width, button->height,
                                         button_col );

  widget_printstring( virtual_keyboard.x_pos+button->x_pos+button->width/2-
                        ( widget_stringwidth( button->text ))/2,
                      virtual_keyboard.y_pos+button->y_pos+button->height/2-4,
                      text_col, button->text );
}

void
InitVirtualKeyboardButton( int button_type, int button_enum,
                           int keymap_function, int button_up, int button_down,
                           int button_left, int button_right, int x_pos,
                           int y_pos, int width, int height, char *text,
                           int flags )
{
  if (button_type >= NUM_VIRTUAL_KEYBOARD_BUTTONS)
    return;
  {
    VIRTUAL_KEYBOARD_BUTTON* button;

    button = &virtual_keyboard.buttons[button_enum];

    button->type = button_type;
    button->button_enum = button_enum;
    button->keymap_function = keymap_function;
    button->button_up = button_up;
    button->button_down = button_down;
    button->button_left = button_left;
    button->button_right = button_right;
    button->x_pos = x_pos;
    button->y_pos = y_pos;
    button->width = width;
    button->height = height;
    strcpy(button->text, text);
    button->toggle_button_is_pressed_down = 0;
    button->flags = flags;
  }
}

void
InitVirtualKeyboardButtons( void )
{
  int cur_x=0;
  int cur_y=8;

  int width=16;
  int height=16;
  int button_x_spacing = 3;
  int button_y_spacing = 3;
  int button_x_gap=width + button_x_spacing;
  int button_y_gap=height + button_y_spacing;

  char* symbol_shift_str = "SHIFT";
  char* enter_str = "ENTER";
  char* space_str = "SPC";
  char* caps_str = "CAPS";
  char* move_str = "=";
  char *sym_str = "SYM";

  int symbol_shift_button_width = strlen(symbol_shift_str)*8+4;
  int enter_button_width = strlen(enter_str)*8+4;
  int space_button_width = strlen(space_str)*8+4+13;
  int caps_button_width = strlen(caps_str)*8+4+6;
  int move_button_width = strlen(move_str)*8+4;
  int sym_button_width = strlen(sym_str)*8+4+6;

  int letters_start_x = 4;

  cur_x = letters_start_x;

  // u,                           d,              l,                              r
  if (isSymOn)
      InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_1,
                                    E_KEYMAPFUNC_KEY_1,  E_VKB_CAPS_SHIFT,
                                    E_VKB_Q,        E_VKB_0/*E_VKB_MOVE_KEYBOARD*/,
                                    E_VKB_2, cur_x, cur_y, width, height, "'", 0);

  else
      InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_1,
                                E_KEYMAPFUNC_KEY_1,  E_VKB_CAPS_SHIFT,
                                E_VKB_Q,        E_VKB_0/*E_VKB_MOVE_KEYBOARD*/,
                                E_VKB_2, cur_x, cur_y, width, height, "1", 0);

  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_2,
                            E_KEYMAPFUNC_KEY_2,  E_VKB_Z,
                            E_VKB_W,        E_VKB_1,
                            E_VKB_3, cur_x, cur_y, width, height, "2", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_3,
                            E_KEYMAPFUNC_KEY_3,  E_VKB_X,
                            E_VKB_E,        E_VKB_2,
                            E_VKB_4, cur_x, cur_y, width, height, "3", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_4,
                            E_KEYMAPFUNC_KEY_4,  E_VKB_C,
                            E_VKB_R,        E_VKB_3,
                            E_VKB_5, cur_x, cur_y, width, height, "4", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_5,
                            E_KEYMAPFUNC_KEY_5,  E_VKB_V,
                            E_VKB_T,        E_VKB_4,
                            E_VKB_6, cur_x, cur_y, width, height, "5", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_6,
                            E_KEYMAPFUNC_KEY_6,  E_VKB_B,
                            E_VKB_Y,        E_VKB_5,
                            E_VKB_7, cur_x, cur_y, width, height, "6", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_7,
                            E_KEYMAPFUNC_KEY_7,  E_VKB_N,
                            E_VKB_U,        E_VKB_6,
                            E_VKB_8, cur_x, cur_y, width, height, "7", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_8,
                            E_KEYMAPFUNC_KEY_8,  E_VKB_M,
                            E_VKB_I,        E_VKB_7,
                            E_VKB_9, cur_x, cur_y, width, height, "8", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_9,
                            E_KEYMAPFUNC_KEY_9,  E_VKB_SYMBOL_SHIFT,
                            E_VKB_O,        E_VKB_8,
                            E_VKB_0, cur_x, cur_y, width, height, "9", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_0,
                            E_KEYMAPFUNC_KEY_0,  E_VKB_SPACE,
                            E_VKB_P,        E_VKB_9,
                            E_VKB_1/*E_VKB_MOVE_KEYBOARD*/,
                            cur_x, cur_y, width, height, "0", 0);
  cur_x += button_x_gap;

  //cur_x += 4;
  //InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_MOVE_KEYBOARD, E_VKB_MOVE_KEYBOARD, E_KEYMAPFUNC_NONE,                    E_VKB_MOVE_KEYBOARD,E_VKB_MOVE_KEYBOARD, E_VKB_0, E_VKB_1, cur_x, cur_y, move_button_width, height, move_str, 0);

  cur_x = letters_start_x + 10;
  cur_y += button_y_gap;

  // U    D               L                       R
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_Q,
                            E_KEYMAPFUNC_KEY_Q,
                            E_VKB_1, E_VKB_A, E_VKB_P,
                            E_VKB_W, cur_x, cur_y, width, height, "Q", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_W,
                            E_KEYMAPFUNC_KEY_W,
                            E_VKB_2, E_VKB_S, E_VKB_Q,
                            E_VKB_E, cur_x, cur_y, width, height, "W", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_E,
                            E_KEYMAPFUNC_KEY_E,
                            E_VKB_3, E_VKB_D,       E_VKB_W,
                            E_VKB_R, cur_x, cur_y, width, height, "E", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_R,
                            E_KEYMAPFUNC_KEY_R,
                            E_VKB_4, E_VKB_F,       E_VKB_E,
                            E_VKB_T, cur_x, cur_y, width, height, "R", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_T,
                            E_KEYMAPFUNC_KEY_T,
                            E_VKB_5, E_VKB_G,       E_VKB_R,
                            E_VKB_Y, cur_x, cur_y, width, height, "T", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_Y,
                            E_KEYMAPFUNC_KEY_Y,
                            E_VKB_6, E_VKB_H,       E_VKB_T,
                            E_VKB_U, cur_x, cur_y, width, height, "Y", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_U,
                            E_KEYMAPFUNC_KEY_U,
                            E_VKB_7, E_VKB_J, E_VKB_Y,
                            E_VKB_I, cur_x, cur_y, width, height, "U", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_I,
                            E_KEYMAPFUNC_KEY_I,
                            E_VKB_8, E_VKB_K, E_VKB_U,
                            E_VKB_O, cur_x, cur_y, width, height, "I", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_O,
                            E_KEYMAPFUNC_KEY_O,
                            E_VKB_9, E_VKB_L,       E_VKB_I,
                            E_VKB_P, cur_x, cur_y, width, height, "O", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_P,
                            E_KEYMAPFUNC_KEY_P,
                            E_VKB_0, E_VKB_ENTER,E_VKB_O,
                            E_VKB_Q, cur_x, cur_y, width, height, "P", 0);
  cur_x += button_x_gap;

  cur_x = letters_start_x + 20;
  cur_y += button_y_gap;

  //      U                       D                                       L                   R
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_A,
                            E_KEYMAPFUNC_KEY_A,
                            E_VKB_Q,        E_VKB_CAPS_SHIFT, E_VKB_ENTER,
                            E_VKB_S, cur_x, cur_y, width, height, "A", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_S,
                            E_KEYMAPFUNC_KEY_S,
                            E_VKB_W,        E_VKB_Z, E_VKB_A,
                            E_VKB_D, cur_x, cur_y, width, height, "S", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_D,
                            E_KEYMAPFUNC_KEY_D,
                            E_VKB_E,        E_VKB_X, E_VKB_S,
                            E_VKB_F, cur_x, cur_y, width, height, "D", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_F,
                            E_KEYMAPFUNC_KEY_F,
                            E_VKB_R,        E_VKB_C, E_VKB_D,
                            E_VKB_G, cur_x, cur_y, width, height, "F", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_G,
                            E_KEYMAPFUNC_KEY_G,
                            E_VKB_T,        E_VKB_V, E_VKB_F,
                            E_VKB_H, cur_x, cur_y, width, height, "G", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_H,
                            E_KEYMAPFUNC_KEY_H,
                            E_VKB_Y,        E_VKB_B, E_VKB_G,
                            E_VKB_J, cur_x, cur_y, width, height, "H", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_J,
                            E_KEYMAPFUNC_KEY_J,
                            E_VKB_U,        E_VKB_N, E_VKB_H,
                            E_VKB_K, cur_x, cur_y, width, height, "J", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_K,
                            E_KEYMAPFUNC_KEY_K,
                            E_VKB_I,        E_VKB_M, E_VKB_J,
                            E_VKB_L, cur_x, cur_y, width, height, "K", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_L,
                            E_KEYMAPFUNC_KEY_L,
                            E_VKB_O,        E_VKB_SYMBOL_SHIFT, E_VKB_K,
                            E_VKB_ENTER, cur_x, cur_y, width, height, "L", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_ENTER,
                            E_KEYMAPFUNC_KEY_ENTER,
                            E_VKB_P,        E_VKB_SPACE,     E_VKB_L,
                            E_VKB_A, cur_x, cur_y, enter_button_width, height, enter_str, 0);
  cur_x += enter_button_width+button_x_spacing;

  cur_x = letters_start_x;
  cur_y += button_y_gap;

  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_CAPS_SHIFT,
                            E_KEYMAPFUNC_KEY_CAPS_SHIFT,
                            E_VKB_A,                E_VKB_1, E_VKB_SPACE,
                            E_VKB_Z, cur_x, cur_y, caps_button_width, height, caps_str, VKB_FLAG_TOGGLE_BUTTON);
  cur_x += caps_button_width+button_x_spacing;
  //  U   D                                                                   L                                                       R
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_Z,
                            E_KEYMAPFUNC_KEY_Z,
                            E_VKB_S,        E_VKB_2, E_VKB_CAPS_SHIFT,
                            E_VKB_X, cur_x, cur_y, width, height, "Z", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_X,
                            E_KEYMAPFUNC_KEY_X,
                            E_VKB_D,        E_VKB_3,       E_VKB_Z,
                            E_VKB_C, cur_x, cur_y, width, height, "X", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_C,
                            E_KEYMAPFUNC_KEY_C,
                            E_VKB_F,        E_VKB_4,       E_VKB_X,
                            E_VKB_V, cur_x, cur_y, width, height, "C", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_V,
                            E_KEYMAPFUNC_KEY_V,
                            E_VKB_G,        E_VKB_5,    E_VKB_C,
                            E_VKB_B, cur_x, cur_y, width, height, "V", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_B,
                            E_KEYMAPFUNC_KEY_B,
                            E_VKB_H,        E_VKB_6,    E_VKB_V,
                            E_VKB_N, cur_x, cur_y, width, height, "B", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_N,
                            E_KEYMAPFUNC_KEY_N,
                            E_VKB_J,        E_VKB_7,    E_VKB_B,
                            E_VKB_M, cur_x, cur_y, width, height, "N", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_M,
                            E_KEYMAPFUNC_KEY_M,
                            E_VKB_K,        E_VKB_8,    E_VKB_N,
                            E_VKB_SYMBOL_SHIFT, cur_x, cur_y, width, height, "M", 0);
  cur_x += button_x_gap;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_SYMBOL_SHIFT,
                            E_KEYMAPFUNC_KEY_SYMBOL_SHIFT,
                            E_VKB_L, E_VKB_9,        E_VKB_M,
                            E_VKB_SPACE, cur_x, cur_y, sym_button_width, height, sym_str, 0);
  cur_x += sym_button_width+button_x_spacing;
  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_SPACE,
                            E_KEYMAPFUNC_KEY_SPACE,
                            E_VKB_ENTER, E_VKB_0,        E_VKB_SYMBOL_SHIFT,
                            E_VKB_CAPS_SHIFT, cur_x, cur_y, space_button_width, height, space_str, 0);

  // U                            D           L                                                       R
//  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_CAPS_SHIFT,
//                            E_KEYMAPFUNC_KEY_CAPS_SHIFT,
//                            E_VKB_X,                E_VKB_2, E_VKB_SYMBOL_SHIFT,
//                            E_VKB_SPACE, cur_x, cur_y, caps_button_width, height, caps_str, VKB_FLAG_TOGGLE_BUTTON);
//  cur_x += caps_button_width+button_x_spacing;
//  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_SPACE,
//                            E_KEYMAPFUNC_KEY_SPACE,
//             InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_SPACE,
//  E_KEYMAPFUNC_KEY_SPACE,
//  E_VKB_B, E_VKB_5,        E_VKB_CAPS_SHIFT,
//  E_VKB_SYMBOL_SHIFT, cur_x, cur_y, space_button_width, height, space_str, 0);
//cur_x += space_button_width+button_x_spacing;                 E_VKB_B, E_VKB_5,        E_VKB_CAPS_SHIFT,
//                            E_VKB_SYMBOL_SHIFT, cur_x, cur_y, space_button_width, height, space_str, 0);
//  cur_x += space_button_width+button_x_spacing;
//  InitVirtualKeyboardButton(E_VKB_BUTTON_TYPE_SPECTRUM_KEY, E_VKB_SYMBOL_SHIFT,
//                            E_KEYMAPFUNC_KEY_SYMBOL_SHIFT,
//                            E_VKB_ENTER, E_VKB_9,    E_VKB_SPACE,
//                            E_VKB_CAPS_SHIFT, cur_x, cur_y, symbol_shift_button_width, height, symbol_shift_str, VKB_FLAG_TOGGLE_BUTTON);
//  cur_x += symbol_shift_button_width+button_x_spacing;
}

void
InitVirtualKeyboard( void )
{
  InitVirtualKeyboardButtons();

  virtual_keyboard.width = VIRTUAL_KEYBOARD_WIDTH;
  virtual_keyboard.height = VIRTUAL_KEYBOARD_HEIGHT;
  virtual_keyboard.x_pos = 0;
  virtual_keyboard.y_pos = 80;

  virtual_keyboard.state = E_VKS_ACCEPTING_INPUT;
  virtual_keyboard.was_active = 0;

  virtual_keyboard.key_held = E_VKB_NONE;
  SetCurrentVirtualKeyboardButton(selectedLetter);
  virtual_keyboard.send_key_events = 1;
  virtual_keyboard.get_keymap_mode = 0;

  //      virtual_keyboard.surface=NULL;
}

void cleanText(char *text) {
    int l = strlen(text)-1;

    if (l > 2 && text[0] == 'H')
        return;

    while (l > 0) {
        if (!isalnum(text[l])) {
            text[l] = 0;
            l--;
        }
        else
            return;
    }
}

void initGamesData()
{
    char defaultKeys[] = "K:EN 1;Q;A;O;P;M;1;1;;;;;;;;;;;;;;;;";
    char defaultDesc[] = "D:;Up;Down;Left;Down;Fire;Choose 1;Choose 1;;;;;;;;;;;;;;;";

    printf("\nInitializing games data....");
    if (gameData == NULL) {
        gameData = malloc(sizeof(GAME_DATA) * (MAX_GAME_NUM+1));
//        printf("\nReserving %d bytes", sizeof(GAME_DATA) * (MAX_GAME_NUM));

        char line[255];
        int i=0;

        FILE *f = fopen("games_data.txt", "rt");
        if (f && gameData) {
            gameData[i].description = NULL;
            gameData[i].filename = NULL;
            gameData[i].hint = NULL;
            gameData[i].keys = NULL;
            gameData[i].M = NULL;
            gameData[i].title = NULL;
            gameData[i].pokes = NULL;
            gameData[i].isFavourite = 0;
            fgets(&line, 255, f);

            while (strlen(&line) > 0 && i < MAX_GAME_NUM) {

                cleanText(&line);

                if (strlen(&line) > 2) {
                    if (strncmp("T:", &line, 2) == 0) {
//                        printf("\n1");
                        if (gameData[i].title != NULL) {
                            // New game found
//                            printf("\nFound game %d", i);
                            if (gameData[i].keys == NULL) {
                                gameData[i].keys = malloc(strlen(defaultKeys)+1);
                                if (gameData[i].keys)
                                    strcpy(gameData[i].keys, defaultKeys);
                            }
                            if (gameData[i].description == NULL) {
                                gameData[i].description = malloc(strlen(defaultDesc)+1);
                                if (gameData[i].description)
                                    strcpy(gameData[i].description, defaultDesc);
                            }

                            i++;
                            gameData[i].description = NULL;
                            gameData[i].filename = NULL;
                            gameData[i].hint = NULL;
                            gameData[i].keys = NULL;
                            gameData[i].M = NULL;
                            gameData[i].pokes = NULL;
                            gameData[i].isFavourite = 0;                            
                        }
                        gameData[i].title = malloc(strlen(&line[2])+1);
                        if (gameData[i].title)
                            strcpy(gameData[i].title, &line[2]);
                    }
                    if (strncmp("F:", &line, 2) == 0) {
//                        printf("\n2");
                        gameData[i].filename = malloc(strlen(&line[2])+1);
                        if (gameData[i].filename)
                            strcpy(gameData[i].filename, &line[2]);
                    }
                    if (strncmp("M:", &line, 2) == 0) {
//                        printf("\n3");
                        gameData[i].M = malloc(strlen(&line[2])+1);
                        if (gameData[i].M)
                            strcpy(gameData[i].M, &line[2]);
                    }
                    if (strncmp("K:", &line, 2) == 0) {
//                        printf("\n4");
                        gameData[i].keys = malloc(strlen(&line[2])+1);
                        if (gameData[i].keys)
                            strcpy(gameData[i].keys, &line[2]);
                    }
                    if (strncmp("D:", &line, 2) == 0) {
//                        printf("\n5");
                        gameData[i].description = malloc(strlen(&line[2])+1);
                        if (gameData[i].description)
                            strcpy(gameData[i].description, &line[2]);
                    }
                    if (strncmp("H:", &line, 2) == 0) {
//                        printf("\n6");
                        gameData[i].hint = malloc(strlen(&line[2])+1);
                        if (gameData[i].hint)
                            strcpy(gameData[i].hint, &line[2]);
                    }
                    if (strncmp("P:", &line, 2) == 0) {
                        gameData[i].pokes = malloc(strlen(&line[2])+1);
                        if (gameData[i].pokes)
                            strcpy(gameData[i].pokes, &line[2]);
                    }
//                    printf("\n-");
                }
                if (feof(f))
                    line[0] = 0;
                else
                    fgets(&line, 255, f);
            }
            fclose(f);

            numGamesFound = i;
            i++;
            gameData[i].description = NULL;
            gameData[i].filename = NULL;
            gameData[i].hint = NULL;
            gameData[i].keys = NULL;
            gameData[i].M = NULL;
            gameData[i].title = NULL;
            gameData[i].pokes = NULL;
            gameData[i].isFavourite = 0;
        }

        // Load favourite menu
#ifdef __arm__
        char fav[] = "/media/favourite.txt";
#else
        char fav[] = "/home/./test/favourite.txt";
#endif

        int favId;
        f = fopen(fav, "rt");
        favGamesFound = 0;

        if (f) {
            while (!feof(f)) {
                if (fscanf(f, "%d\n", &favId)) {
                    printf("\nFavourite %d", favId);
                    gameData[favId].isFavourite = 1;
                    favGamesFound++;
                }
            }
            fclose(f);
        }
        printf("\nInitialized.");
    }
    else
        printf("\nAready initialized.");
    fflush(stdout);
}

void initSDGamesData()
{
    if (!sdGameData) {
        sdCurrentGameId = 0;
        sdGameListOffset = 0;
        sdNumGamesFound = 0;
        sdSelectedGameOnScreen = 0;
        printf("\nInitializing SD games data....");
        sdGameData = malloc(sizeof(GAME_DATA) * (MAX_GAME_NUM+1));

#ifdef __arm__
        char sdFolder[] = "/media/games_data.txt";
#else
        char sdFolder[] = "/home/./test/games_data.txt";
        system("if [ ! -e /home/./test/games_data.txt ]; then for f in /home/./test/*.zxk; do (cat $f; echo) >> /home/./test/games_data.txt; done; fi");
#endif

        char line[255];
        int i=0;

        FILE *f = fopen(sdFolder, "rt");
        if (f && sdGameData) {
            sdGameData[i].description = NULL;
            sdGameData[i].filename = NULL;
            sdGameData[i].hint = NULL;
            sdGameData[i].keys = NULL;
            sdGameData[i].M = NULL;
            sdGameData[i].title = NULL;
            sdGameData[i].pokes = NULL;
            fgets(&line, 255, f);

            while (strlen(&line) > 0 && i < MAX_GAME_NUM) {

                cleanText(&line);
    //            printf("\nRead %s", line);

                if (strlen(&line) > 2) {
                    if (strncmp("T:", &line, 2) == 0) {
    //                        printf("\n1");
                        if (sdGameData[i].title != NULL) {
                            // New game found
                            i++;
                            sdGameData[i].description = NULL;
                            sdGameData[i].filename = NULL;
                            sdGameData[i].hint = NULL;
                            sdGameData[i].keys = NULL;
                            sdGameData[i].M = NULL;
                            sdGameData[i].pokes = NULL;
                        }
                        sdGameData[i].title = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].title)
                            strcpy(sdGameData[i].title, &line[2]);
                        printf("\nFound game %s", sdGameData[i].title);
                    }
                    if (strncmp("F:", &line, 2) == 0) {
    //                        printf("\n2");
                        sdGameData[i].filename = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].filename)
                            strcpy(sdGameData[i].filename, &line[2]);
                    }
                    if (strncmp("M:", &line, 2) == 0) {
    //                        printf("\n3");
                        sdGameData[i].M = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].M)
                            strcpy(sdGameData[i].M, &line[2]);
                    }
                    if (strncmp("K:", &line, 2) == 0) {
    //                        printf("\n4");
                        sdGameData[i].keys = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].keys)
                            strcpy(sdGameData[i].keys, &line[2]);
                    }
                    if (strncmp("D:", &line, 2) == 0) {
    //                        printf("\n5");
                        sdGameData[i].description = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].description)
                            strcpy(sdGameData[i].description, &line[2]);
                    }
                    if (strncmp("H:", &line, 2) == 0) {
    //                        printf("\n6");
                        sdGameData[i].hint = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].hint)
                            strcpy(sdGameData[i].hint, &line[2]);
                    }
                    if (strncmp("P:", &line, 2) == 0) {
                        sdGameData[i].pokes = malloc(strlen(&line[2])+1);
                        if (sdGameData[i].pokes)
                            strcpy(sdGameData[i].pokes, &line[2]);
                    }
    //                    printf("\n-");
                }
                if (feof(f))
                    line[0] = 0;
                else
                    fgets(&line, 255, f);
            }
            fclose(f);

            if (i == 0) {
                printf("\nNo games found");
            }
            else
                i++;

            sdNumGamesFound = i;
            sdGameData[i].description = NULL;
            sdGameData[i].filename = NULL;
            sdGameData[i].hint = NULL;
            sdGameData[i].keys = NULL;
            sdGameData[i].M = NULL;
            sdGameData[i].title = NULL;
            sdGameData[i].pokes = NULL;
        }
        else {
            printf("\nNo games found");
            sdNumGamesFound = 0;
            sdGameData[0].description = NULL;
            sdGameData[0].filename = NULL;
            sdGameData[0].hint = NULL;
            sdGameData[0].keys = NULL;
            sdGameData[0].M = NULL;
            sdGameData[0].title = NULL;
            sdGameData[0].pokes = NULL;
        }
        printf("\nSD Initialized.");
    }
    else
        printf("\nSD Already initialized");
}

void displaySettings() {
    int batteryState = vegaReadReg(0x80044000, 0xc0);

    int batteryVoltageLevel = vegaReadReg(0x80044000, 0xe0);
//    printf("\nValue before %d", batteryVoltageLevel);

    if ((batteryState & 2) != 0) {
        // Switching off charging to improve battery charging level detection
        printf("\nSwitching off charging");
        vegaWriteReg(0x80044000, 0x30, 0x1073f);
    }

    // Clear screen
    printf("\nDisplaySettings %d", currentSettingsItem);
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+20,
                                 virtual_keyboard.y_pos+20,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;

    if ((batteryState & 2) != 0)
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, "Battery is charging");
    else
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, "Battery not charging");
    y+=10;

    usleep(80000);
    batteryVoltageLevel = vegaReadReg(0x80044000, 0xe0);
//    printf("\nValue after %d", batteryVoltageLevel);
    batteryVoltageLevel = batteryVoltageLevel >> 16;
    batteryVoltageLevel = batteryVoltageLevel & 0x3ff;
    batteryVoltageLevel = batteryVoltageLevel * 8;

    char batteryPct;

//    if ((batteryState & 2) != 0) {
//        if (batteryVoltageLevel < batteryChargingLevel[0])
//            batteryPct = 0;
//        else {
//            int i;
//            for (i=9;i>=0;i--) {
//                if (batteryVoltageLevel >= batteryChargingLevel[i]) {
//                    batteryPct = (i+1)*10;
//                    break;
//                }

//            }
//        }
//    }
//    else {
        int i;
        for (i=0;i<11;i++) {
            if (batteryVoltageLevel >= batteryDischargingLevel[i]) {
                batteryPct = (10-i)*10;
                break;
            }
        }
//    }

    char customString[40];
    char percentage[10];
    if (batteryVoltageLevel < 3640) {
        sprintf(percentage, "[---]");
    }
    else if (batteryPct < 40) {
        sprintf(percentage, "[#--]");
    }
    else if (batteryPct < 80) {
        sprintf(percentage, "[##-]");
    }
    else
        sprintf(percentage, "[###]");

    sprintf(&customString[0], "Battery Level: %s (%d mV)", percentage, batteryVoltageLevel);
    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    y+= 20;

    sprintf(&customString[0], "Sound Volume Level: %d", vegaGetDACVolume());
    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    if (currentSettingsItem == 0) {
        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
            y + 8*DISPLAY_BORDER_HEIGHT_COLS,
            stringWidth - x, 8,
            WIDGET_COLOUR_HIGHLIGHT );
        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    }
    y += 10;

    int vegaAudio = vegaGetAudio();

    if (vegaAudio == 0)
        sprintf(&customString[0], "Audio: Speakers");
    else if (vegaAudio == 1)
        sprintf(&customString[0], "Audio: Headphones");

    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    if (currentSettingsItem == 1) {
        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
            y + 8*DISPLAY_BORDER_HEIGHT_COLS,
            stringWidth - x, 8,
            WIDGET_COLOUR_HIGHLIGHT );
        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    }
    y += 10;

    sprintf(&customString[0], "Brightness Level: %d", vegaGetBrightnessLevel());
    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    if (currentSettingsItem == 2) {
        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
            y + 8*DISPLAY_BORDER_HEIGHT_COLS,
            stringWidth -x, 8,
            WIDGET_COLOUR_HIGHLIGHT );
        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    }
    y += 10;

    sprintf(&customString[0], "Contrast Level: %d", vegaGetContrastLevel());
    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    if (currentSettingsItem == 3) {
        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
            y + 8*DISPLAY_BORDER_HEIGHT_COLS,
            stringWidth - x, 8,
            WIDGET_COLOUR_HIGHLIGHT );
        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    }
    y+= 10;

    y+= 20;
    sprintf(&customString[0], "Use D-Pad (Up,Down,Left,Right)");
    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
    y+= 10;
    sprintf(&customString[0], "to navigate and change settings");
    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);

    int scale = machine_current->timex ? 2 : 1;
    uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                    scale * DISPLAY_SCREEN_HEIGHT );
    uidisplay_frame_end();

    if ((batteryState & 2) != 0) {
        // Switching charging back on
        printf("\nSwitching charging back on");
        vegaWriteReg(0x80044000, 0x30, 0x73f); // charge max 780mA - stop 80mA
    }
}

void displayGameList(int offset) {
    char path[64];
    getcwd(&path, 64);
    strcat(&path, "/roms/");

    int i=0;
    int x, y;
    int stringWidth;
    int resetY = 0;

    if (offset < 0)
        offset = 0;
    if (offset > MAX_GAME_NUM)
        offset = MAX_GAME_NUM;

    int limit = offset + NUM_GAMES_ON_SCREEN;

    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width+5,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    while (offset < limit) {
        if (gameData[offset].filename) {
            char filePath[256];
            strcpy(&filePath, &path);
            strncat(&filePath, gameData[offset].filename, 256 - 64);

            x = -16;
            y = i*8;
            if (i >= NUM_GAMES_ON_SCREEN / 2) {
                if (resetY == 0)
                    resetY = i;
                x = -16 + (virtual_keyboard.width/2) + 1;
                y = (i-resetY)*8;
            }
            if (access(&filePath, F_OK) != -1) {
                if (gameData[offset].isFavourite == 1)
                    stringWidth = widget_printstring(x, y, 10, gameData[offset].title);
                else
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, gameData[offset].title);

                if (i == selectedGameOnScreen) {
                    if (i >= NUM_GAMES_ON_SCREEN / 2) {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16 - (virtual_keyboard.width/2)-1, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }
                    else {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }

                    if (gameData[offset].isFavourite == 1)
                        stringWidth = widget_printstring(x, y, 10, gameData[offset].title);
                    else
                        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, gameData[offset].title);
                }
            }
            i++;
        }
        else
            break;
        offset++;
    }

    y= 184;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "Use D-Pad (Up,Down,Left,Right) to navigate through");
    y+= 10;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "games. F to play, 1 to mark as favourite");
}

void displaySDCardGameList(int offset) {
#ifdef __arm__
    char path[128] = "/media/";
#else
    char path[128] = "/home/./test/";
#endif

    int i=0;
    int x, y;
    int stringWidth;
    int resetY = 0;

    if (offset < 0)
        offset = 0;
    if (offset > MAX_GAME_NUM)
        offset = MAX_GAME_NUM;

    int limit = offset + NUM_GAMES_ON_SCREEN;
    if (limit > sdNumGamesFound)
        limit = sdNumGamesFound;

    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width+5,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    while (offset < limit) {
        if (sdGameData[offset].filename) {
            char filePath[256];
            strcpy(&filePath, &path);
            strncat(&filePath, sdGameData[offset].filename, 256 - 64);

            x = -16;
            y = i*8;
            if (i >= NUM_GAMES_ON_SCREEN / 2) {
                if (resetY == 0)
                    resetY = i;
                x = -16 + (virtual_keyboard.width/2) + 1;
                y = (i-resetY)*8;
            }
            if (access(&filePath, F_OK) != -1) {
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, sdGameData[offset].title);
                if (i == sdSelectedGameOnScreen) {
                    if (i >= NUM_GAMES_ON_SCREEN / 2) {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16 - (virtual_keyboard.width/2)-1, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }
                    else {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }

                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, sdGameData[offset].title);
                }
            }
            i++;
        }
        else
            break;
        offset++;
    }

    y= 184;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "Use D-Pad (Up,Down,Left,Right) to navigate through");
    y+= 10;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "games. F to play, 1 to mark as favourite");
}

void displayHOF(int offset) {

    int i=0;
    int x, y;
    int stringWidth;
    int column1 = 0;

    if (offset < 0)
        offset = 0;
    if (offset > MAX_NAMES_NUM)
        offset = MAX_NAMES_NUM - NUM_NAMES_ON_SCREEN;

    int limit = offset + NUM_NAMES_ON_SCREEN;

    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width+5,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    FILE *f = fopen("credits.txt", "rt");
    char credit[255];
    x = -16;
    y = i*8;

    if (f) {
        while (offset > 0) {
            fgets(credit, 255, f);
            offset--;
            limit--;
        }
        while (offset < limit) {
            fgets(credit, 255, f);

            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, credit);
            column1++;

            if (column1 == 1) {
                x = -16 + (virtual_keyboard.width/2) + 1;
            }
            else {
                column1 = 0;
                x = -16;
                y += 8;
            }
            i++;
            offset++;
            if (feof(f))
                break;
        }
        fclose(f);
    }

    y= 184;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "This is the list of people that made Vega+ a reality!");
    y+= 10;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "Use Left, Right to browse.");
}

void displayFavList(int offset) {
    char path[64];
    getcwd(&path, 64);
    strcat(&path, "/roms/");

    int i=0;
    int j=0;
    int x, y;
    int stringWidth;
    int resetY = 0;

    if (offset < 0)
        offset = 0;
    if (offset > MAX_GAME_NUM)
        offset = MAX_GAME_NUM;

    int limit = offset + NUM_GAMES_ON_SCREEN;

    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width+5,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    for (j=0;j<numGamesFound;j++) {
        if (gameData[j].isFavourite == 1 && offset > 0)
            offset--;
        else if (gameData[j].isFavourite == 1 && offset == 0) {
            char filePath[256];
            strcpy(&filePath, &path);
            strncat(&filePath, gameData[j].filename, 256 - 64);

            x = -16;
            y = i*8;
            if (i >= NUM_GAMES_ON_SCREEN / 2) {
                if (resetY == 0)
                    resetY = i;
                x = -16 + (virtual_keyboard.width/2) + 1;
                y = (i-resetY)*8;
            }
            if (access(&filePath, F_OK) != -1) {
                if (gameData[j].isFavourite == 1)
                    stringWidth = widget_printstring(x, y, 10, gameData[j].title);
                else
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, gameData[j].title);

                if (i == selectedFavGame) {
                    if (i >= NUM_GAMES_ON_SCREEN / 2) {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16 - (virtual_keyboard.width/2)-1, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }
                    else {
                        widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                                                    y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                                                    stringWidth+16, 8,
                                                    WIDGET_COLOUR_HIGHLIGHT );
                    }

                    if (gameData[j].isFavourite == 1)
                        stringWidth = widget_printstring(x, y, 10, gameData[j].title);
                    else
                        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, gameData[j].title);
                }
            }
            i++;
        }
        if (i == NUM_GAMES_ON_SCREEN)
            break;
    }

    y = 184;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "Use D-Pad (Up,Down,Left,Right) to navigate through");
    y+= 10;
    stringWidth = widget_printstring(-16, y, WIDGET_COLOUR_FOREGROUND, "games. F to play, 1 to mark as favourite");
}

void displayGameInfo() {

    int x, y;
    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    if (currentGameId >= 0) {
        x = 10;
        y = 10;

        char title[] = "Game info:";
        char hint[] = "Hint:";
        char keys[] = "Game Control:";
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, title);
        x += 60;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, gameData[currentGameId].title);

        y+= 10;
        x= 10;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, hint);
        x += 30;

        if (gameData[currentGameId].hint) {
            int newLine = 32;
            char hintLine[32+1];
            char *start = &gameData[currentGameId].hint[0];

            int currentLine = 0;

            while (currentLine < 32) {
                hintLine[0] = 0;
                if (strlen(start) > 32) {
                    while (start[newLine] != ' ' && (newLine) > 0)
                        newLine--;
                    if (newLine == 0)
                        newLine = 32;

                    strncat(&hintLine, start, newLine);
                    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &hintLine);
                    start += newLine + 1;
                    newLine = 32;
                }
                else {
                    strncat(&hintLine, start, newLine);
                    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &hintLine);
                    break;
                }
                currentLine++;
                y += 10;
            }
        }

        y+= 10;
        x= 10;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keys);
        x += 60;

        char *gameControl = gameData[currentGameId].description;
        char currentKey[255];
        int i;
        int j=0;
        int currentKeyNum = 0;

        x = 10;
        int altX = 140;
        int altY = y;

        if (gameControl) {
            for (i=0;i<strlen(gameControl);i++) {
                if (gameControl[i] != ';') {
                    currentKey[j] = gameControl[i];
                    j++;
                }
                else {
                    if (j > 0) {
                        currentKey[j] = 0;
                        if (currentKeyNum > 11) {
                            altY += 10;
                            int altX2 = widget_printstring(altX, altY, WIDGET_COLOUR_FOREGROUND, "- ");
                            widget_printstring(altX2, altY, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
                        }
                        else {
                            y+= 10;
                            x = 10;
                            widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keyDefinition[currentKeyNum]);
                            x += 82;
                            x = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
                            x += 10;

                            if (x > altX)
                                altX = x;
                        }
                    }
                    j=0;
                    currentKeyNum++;
                }
            }
        }
        if (j > 0) {
            currentKey[j] = 0;
            if (currentKeyNum > 11) {
                altY += 10;
                int altX2 = widget_printstring(altX, altY, WIDGET_COLOUR_FOREGROUND, "- ");
                widget_printstring(altX2, altY, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
            }
            else {
                y+= 10;
                x = 10;
                widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keyDefinition[currentKeyNum]);
                x += 82;

                widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
            }
        }
        currentKeyNum++;
    }
}

void displaySDGameInfo() {

    int x, y;
    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    if (sdCurrentGameId >= 0) {
        x = 10;
        y = 10;

        char title[] = "Game info:";
        char hint[] = "Hint:";
        char keys[] = "Game Control:";
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, title);
        x += 60;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, sdGameData[sdCurrentGameId].title);

        y+= 10;
        x= 10;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, hint);
        x += 30;

        if (sdGameData[sdCurrentGameId].hint) {
            int newLine = 32;
            char hintLine[32+1];
            char *start = &sdGameData[sdCurrentGameId].hint[0];

            int currentLine = 0;

            while (currentLine < 32) {
                hintLine[0] = 0;
                if (strlen(start) > 32) {
                    while (start[newLine] != ' ' && (newLine) > 0)
                        newLine--;
                    if (newLine == 0)
                        newLine = 32;

                    strncat(&hintLine, start, newLine);
                    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &hintLine);
                    start += newLine + 1;
                    newLine = 32;
                }
                else {
                    strncat(&hintLine, start, newLine);
                    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &hintLine);
                    break;
                }
                currentLine++;
                y += 10;
            }
        }

        y+= 10;
        x= 10;
        widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keys);
        x += 60;

        char *gameControl = sdGameData[sdCurrentGameId].description;
        char currentKey[255];
        int i;
        int j=0;
        int currentKeyNum = 0;

        x = 10;
        int altX = 140;
        int altY = y;

        if (gameControl) {
            for (i=0;i<strlen(gameControl);i++) {
                if (gameControl[i] != ';') {
                    currentKey[j] = gameControl[i];
                    j++;
                }
                else {
                    if (j > 0) {
                        currentKey[j] = 0;
                        if (currentKeyNum > 11) {
                            altY += 10;
                            int altX2 = widget_printstring(altX, altY, WIDGET_COLOUR_FOREGROUND, "- ");
                            widget_printstring(altX2, altY, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
                        }
                        else {
                            y+= 10;
                            x = 10;
                            widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keyDefinition[currentKeyNum]);
                            x += 82;
                            x = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
                            x += 10;

                            if (x > altX)
                                altX = x;
                        }
                    }
                    j=0;
                    currentKeyNum++;
                }
            }
        }
        if (j > 0) {
            currentKey[j] = 0;
            if (currentKeyNum > 11) {
                altY += 10;
                int altX2 = widget_printstring(altX, altY, WIDGET_COLOUR_FOREGROUND, "- ");
                widget_printstring(altX2, altY, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
            }
            else {
                y+= 10;
                x = 10;
                widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, keyDefinition[currentKeyNum]);
                x += 82;

                widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, &currentKey[0]);
            }
        }
        currentKeyNum++;
    }
}

void displaySaveGameMenu() {
    // Clear screen
    printf("\nDisplaySettings %d", currentSettingsItem);
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+20,
                                 virtual_keyboard.y_pos+20,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;
    int i;
    char customString[40];

    for (i=0;i<8;i++) {
        sprintf(customString, "Save state %d", i+1);
        if (i == currentSaveGame) {
            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
            widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                stringWidth - x, 8,
                WIDGET_COLOUR_HIGHLIGHT );
            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
        }
        else {
            char gameState[255];
#ifdef __arm__
            if (lastGameLoadFromSD == 1 && lastGameLoaded >=0)
                sprintf(gameState, "/media/%s.s0%d", sdGameData[lastGameLoaded].filename, i+1);
            else
                sprintf(gameState, "/media/%s.s0%d", gameData[lastGameLoaded].filename, i+1);
#else
            if (lastGameLoadFromSD == 1 && lastGameLoaded >=0)
                sprintf(gameState, "/home/./test/%s.s0%d", sdGameData[lastGameLoaded].filename, i+1);
            else
                sprintf(gameState, "/home/./test/%s.s0%d", gameData[lastGameLoaded].filename, i+1);
#endif
            if (access(&gameState, F_OK) != -1)
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
            else
                stringWidth = widget_printstring(x, y, 10, customString);
        }
        y+= 10;
    }
}

void displayLoadGameMenu() {
    // Clear screen
    printf("\nDisplaySettings %d", currentSettingsItem);
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+20,
                                 virtual_keyboard.y_pos+20,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;
    int i;
    char customString[40];

    for (i=0;i<8;i++) {
        sprintf(customString, "Load state %d", i+1);
        if (i == currentLoadGame) {
            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
            widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                stringWidth - x, 8,
                WIDGET_COLOUR_HIGHLIGHT );
            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
        }
        else {
            char gameState[255];
#ifdef __arm__
            if (lastGameLoadFromSD == 1 && lastGameLoaded >=0)
                sprintf(gameState, "/media/%s.s0%d", sdGameData[lastGameLoaded].filename, i+1);
            else
                sprintf(gameState, "/media/%s.s0%d", gameData[lastGameLoaded].filename, i+1);
#else
            if (lastGameLoadFromSD == 1 && lastGameLoaded >=0)
                sprintf(gameState, "/home/./test/%s.s0%d", sdGameData[lastGameLoaded].filename, i+1);
            else
                sprintf(gameState, "/home/./test/%s.s0%d", gameData[lastGameLoaded].filename, i+1);
#endif
            if (access(&gameState, F_OK) != -1)
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
            else
                stringWidth = widget_printstring(x, y, 10, customString);
        }
        y+= 10;
    }
}

void displayRemapKeysMenu() {


    // Clear screen
    printf("\nDisplaySettings %d", currentSettingsItem);
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+30,
                                 virtual_keyboard.y_pos+30,
                                 virtual_keyboard.width+5,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;   

    char customString[255];


    if (lastGameLoadFromSD != -1) {
        char keySequenceBuffer[32];
        char *keySequence;
        char key[32];
        char *keyDef = &lastKeyMapping;
        int currentKey = 0;
        char mappedKey[32];

        printf("\nLast Key Mapping %s", lastKeyMapping);

        while (getToken(keyDef, &keySequenceBuffer, ';', 31)) {
            key[0] = 0;
            mappedKey[0] = 0;
            printf ("\nKey Sequence is %s", &keySequenceBuffer);
            keySequence = &keySequenceBuffer;
            if (getToken(keySequence, &key, ' ', 31)) {
                printf("\nKey is %s", &key);
                strcpy(mappedKey, key);

                if (strlen(keySequence) > 0)
                    keySequence++;
            }

            switch(currentKey) {
            case 0:
                break;
            case 1:
                sprintf(customString, "Up");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 0) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 2:
                sprintf(customString, "Down");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 1) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 3:
                sprintf(customString, "Left");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 2) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }

                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 4:
                sprintf(customString, "Right");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 3) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 5:
                sprintf(customString, "F");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 4) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 6:
                sprintf(customString, "1");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 5) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 7:
                sprintf(customString, "2");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 6) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 8:
                sprintf(customString, "S");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 7) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 9:
                sprintf(customString, "Yellow");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 8) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 10:
                sprintf(customString, "Green");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 9) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            case 11:
                sprintf(customString, "Blue");
                stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                if (currentKeyMapIndex == 10) {
                    widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                        y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                        stringWidth - x, 8,
                        WIDGET_COLOUR_HIGHLIGHT );
                    stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
                }
                sprintf(customString, "%s", &mappedKey);
                stringWidth = widget_printstring(x + 60, y, WIDGET_COLOUR_FOREGROUND, customString);
                y+= 10;
                break;
            default:
                break;
            }
            currentKey++;

            keyDef += strlen(&keySequenceBuffer);
            if (strlen(keyDef) > 0)
                keyDef ++;
        }        

        y+= 10;
        sprintf(customString, "Save mapping on SD Card");
        stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
        if (currentKeyMapIndex == 11) {
            widget_draw_rectangle_solid(x + 8*DISPLAY_BORDER_WIDTH_COLS,
                y + 8*DISPLAY_BORDER_HEIGHT_COLS,
                stringWidth - x, 8,
                WIDGET_COLOUR_HIGHLIGHT );
            stringWidth = widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, customString);
        }
    }
}

int
widget_virtualkeyboard_draw( void *data )
{
  int i;
//  printf("\nVirtualKeyboard draw");

//  printf ("\n1 - %d", data);
//  printf ("\n2 - %d", (int)data);

  menuWidgetType = data;
  printf("\nMenuWidgetType now %d", menuWidgetType);

  if (data == MENU_VIRTUAL_KEYBOARD) {
//      isKeyboardWidget = 1;
//      isGameSelectWidget = 0;
//      isDisplayGameInfoWidget = 0;
//      isSettingsWidget = 0;
//      isSDGameSelectWidget = 0;
      InitVirtualKeyboard();

      // The following sets the cirtual keyboard into the key selection
      // mode
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                 virtual_keyboard.y_pos/8-1,
                                 virtual_keyboard.width/8+2,
                                 virtual_keyboard.height/8+3 );

      for (i=0; i<NUM_VIRTUAL_KEYBOARD_BUTTONS; i++) {
        DrawVirtualKeyboardButton(&virtual_keyboard.buttons[i],
                                  virtual_keyboard.buttons[i].x_pos,
                                  virtual_keyboard.buttons[i].y_pos);
      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_GAME_SELECT){
//      isKeyboardWidget = 0;
//      isGameSelectWidget = 1;
//      isDisplayGameInfoWidget = 0;
//      isSettingsWidget = 0;
//      isSDGameSelectWidget = 0;
      // The following sets the cirtual keyboard into the key selection
      // mode
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = -16;
      virtual_keyboard.y_pos = -8;
      virtual_keyboard.width = 292;
      virtual_keyboard.height = 204;

      initGamesData();

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                 virtual_keyboard.y_pos/8-1,
                                 virtual_keyboard.width/8+2,
                                 virtual_keyboard.height/8+3 );

      widget_printstring(-22, -16, WIDGET_COLOUR_TITLE, "Game List");
      displayGameList(gameListOffset);

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_GAME_INFO) {
//      isKeyboardWidget = 0;
//      isGameSelectWidget = 0;
//      isDisplayGameInfoWidget = 1;
//      isSettingsWidget = 0;
//      isSDGameSelectWidget = 0;
      // The following sets the cirtual keyboard into the key selection
      // mode
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = 4;
      virtual_keyboard.y_pos = 4;
      virtual_keyboard.width = 260;
      virtual_keyboard.height = 180;

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                 virtual_keyboard.y_pos/8-1,
                                 virtual_keyboard.width/8+2,
                                 virtual_keyboard.height/8+3 );

      widget_printstring(-6, -8, WIDGET_COLOUR_TITLE, "Game Info");

      if (isSDCardGame == 0)
          displayGameInfo();
      else
          displaySDGameInfo();

      uidisplay_area( 0, 0, DISPLAY_ASPECT_WIDTH,
                      DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_SETTINGS) {
//      isKeyboardWidget = 0;
//      isGameSelectWidget = 0;
//      isDisplayGameInfoWidget = 0;
//      isSettingsWidget = 1;
//      isSDGameSelectWidget = 0;

      virtual_keyboard.x_pos = 30;
      virtual_keyboard.y_pos = 20;
      virtual_keyboard.width = 200;
      virtual_keyboard.height = 120;

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                 virtual_keyboard.y_pos/8-1,
                                 virtual_keyboard.width/8+2,
                                 virtual_keyboard.height/8+3 );


      widget_printstring(20, 8, WIDGET_COLOUR_TITLE, "Settings");


      displaySettings();
      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_SD_GAME_SELECT) {
//      isKeyboardWidget = 0;
//      isGameSelectWidget = 0;
//      isDisplayGameInfoWidget = 0;
//      isSettingsWidget = 0;
//      isSDGameSelectWidget = 1;
      // The following sets the cirtual keyboard into the key selection
      // mode
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = -16;
      virtual_keyboard.y_pos = -8;
      virtual_keyboard.width = 292;
      virtual_keyboard.height = 204;

      initSDGamesData();

      // Load favourite menu
#ifdef __arm__
      char fav[] = "/media/games_data.txt";
#else
      char fav[] = "/home/./test/games_data.txt";
#endif

      if (access(&fav, F_OK) != -1) {
          widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                     virtual_keyboard.y_pos/8-1,
                                     virtual_keyboard.width/8+2,
                                     virtual_keyboard.height/8+3 );

          widget_printstring(-22, -16, WIDGET_COLOUR_TITLE, "SD Card Games");
          displaySDCardGameList(sdGameListOffset);
      }
      else {
          msgInfo("SD Card not found", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_SAVE_GAME) {
      if (lastGameLoadFromSD < 0) {
          msgInfo("Save game state", "Please start a game before\nsaving its state");
          return;
      }
      else {
#ifdef __arm__
          char fav[] = "/media/games_data.txt";
#else
          char fav[] = "/home/./test/games_data.txt";
#endif

          if (access(&fav, F_OK) != -1) {
              virtual_keyboard.x_pos = 30;
              virtual_keyboard.y_pos = 20;
              virtual_keyboard.width = 200;
              virtual_keyboard.height = 120;

              widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                         virtual_keyboard.y_pos/8-1,
                                         virtual_keyboard.width/8+2,
                                         virtual_keyboard.height/8+3 );

              widget_printstring(20, 8, WIDGET_COLOUR_TITLE, "Save game state");
              displaySaveGameMenu();
          }
          else {
              msgInfo("SD Card not found", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
          }
      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_LOAD_GAME) {
      if (lastGameLoadFromSD < 0) {
              msgInfo("Load game state", "Please start a game before\nloading its state");
              return;
          }
          else {
#ifdef __arm__
          char fav[] = "/media/games_data.txt";
#else
          char fav[] = "/home/./test/games_data.txt";
#endif

          if (access(&fav, F_OK) != -1) {
              virtual_keyboard.x_pos = 30;
              virtual_keyboard.y_pos = 20;
              virtual_keyboard.width = 200;
              virtual_keyboard.height = 120;

              widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                         virtual_keyboard.y_pos/8-1,
                                         virtual_keyboard.width/8+2,
                                         virtual_keyboard.height/8+3 );


              widget_printstring(20, 8, WIDGET_COLOUR_TITLE, "Load game state");
              displayLoadGameMenu();
          }
          else {
              msgInfo("SD Card not found", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
          }
      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_GAME_FAV) {
      // The following sets the cirtual keyboard into the key selection
      // mode
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = -16;
      virtual_keyboard.y_pos = -8;
      virtual_keyboard.width = 292;
      virtual_keyboard.height = 204;

      initGamesData();

      // Load favourite menu
#ifdef __arm__
      char fav[] = "/media/games_data.txt";
#else
      char fav[] = "/home/./test/games_data.txt";
#endif

      if (access(&fav, F_OK) != -1) {
          widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                     virtual_keyboard.y_pos/8-1,
                                     virtual_keyboard.width/8+2,
                                     virtual_keyboard.height/8+3 );

          widget_printstring(-22, -16, WIDGET_COLOUR_TITLE, "Favourite Game List");
          displayFavList(favListOffset);
      }
      else {
          msgInfo("SD Card not found", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_GAME_REMAP_KEYS) {
      printf("\nKM: %s", lastKeyMapping);
      // Sanitize lastKeymapping
      int i = 0;
      int parms = 0;
      while (lastKeyMapping[i] != 0) {
          if (lastKeyMapping[i] == ';')
              parms++;
          i++;
      }
      while (parms < 12) {
          lastKeyMapping[i] = ';';
          parms++;
          i++;
      }
      lastKeyMapping[i] = 0;
      printf("\nSAN KM: %s", lastKeyMapping);

      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = -16;
      virtual_keyboard.y_pos = -8;
      virtual_keyboard.width = 292;
      virtual_keyboard.height = 204;

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                 virtual_keyboard.y_pos/8-1,
                                 virtual_keyboard.width/8+2,
                                 virtual_keyboard.height/8+3 );

      widget_printstring(-22, -16, WIDGET_COLOUR_TITLE, "Remapping Keys");

      displayRemapKeysMenu();

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (data == MENU_GAME_REMAP_KEYS2) {
    InitVirtualKeyboard();
    int i=0;

    // The following sets the cirtual keyboard into the key selection
    // mode
    virtual_keyboard.send_key_events = 0;
    virtual_keyboard.get_keymap_mode = 1;

    widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                               virtual_keyboard.y_pos/8-1,
                               virtual_keyboard.width/8+2,
                               virtual_keyboard.height/8+3 );

    for (i=0; i<NUM_VIRTUAL_KEYBOARD_BUTTONS; i++) {
      DrawVirtualKeyboardButton(&virtual_keyboard.buttons[i],
                                virtual_keyboard.buttons[i].x_pos,
                                virtual_keyboard.buttons[i].y_pos);
    }

    int scale = machine_current->timex ? 2 : 1;
    uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                    scale * DISPLAY_SCREEN_HEIGHT );
    uidisplay_frame_end();
  }
  else if (data == MENU_HALL_OF_FAME) {
      virtual_keyboard.send_key_events = 0;
      virtual_keyboard.get_keymap_mode = 1;

      virtual_keyboard.x_pos = -16;
      virtual_keyboard.y_pos = -8;
      virtual_keyboard.width = 292;
      virtual_keyboard.height = 204;

      widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
               virtual_keyboard.y_pos/8-1,
               virtual_keyboard.width/8+2,
               virtual_keyboard.height/8+3 );

      widget_printstring(-22, -16, WIDGET_COLOUR_TITLE, "Hall of Fame");

      displayHOF(hofIndex);

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  return 0;
}

char getSymKey(char key)
{
    switch (key)
    {
    case E_VKB_1:
        return 40;
    }
    return E_VKB_0;
}

void loadExtraINIFile(char *fileName)
{
    // Search for extra INI file in SD Card
#ifdef __arm__
    char filePath[255] = "";
    sprintf(filePath, "/media/%s.ini", fileName);
#else
    char filePath[255] = "";
    sprintf(filePath, "/home/./test/%s.ini", fileName);
#endif
    // Try to load ini file
    char line[255];
    FILE *f = fopen(filePath, "rt");
    char M[255] = "";
    char pokes[255] = "";
    char keys[255] = "";
    if (f) {
        printf("\nFound INI file on SD Card");
        fgets(&line, 255, f);

        while (strlen(&line) > 0) {
            cleanText(&line);
            if (strncmp("M:", &line, 2) == 0)
                strcpy(M, &line[2]);
            if (strncmp("K:", &line, 2) == 0)
                strcpy(keys, &line[2]);
            if (strncmp("P:", &line, 2) == 0)
                strcpy(pokes, &line[2]);

            if (feof(f))
                line[0] = 0;
            else
                fgets(&line, 255, f);
        }
        fclose(f);

        if (M[0] != 0) {
            printf("\nSetting machine type %s", &M[0]);
            machine_select_id(&M[0]);
        }
        if (pokes[0] != 0)
            vegaSetPokes(pokes);
        if (keys[0] != 0) {
            strcpy(lastKeyMapping, keys);
            remapKeys(keys);
        }
    }
}

void doPleaseWait()
{
    virtual_keyboard.x_pos = 30;
    virtual_keyboard.y_pos = 20;
    virtual_keyboard.width = 200;
    virtual_keyboard.height = 120;

    widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                               virtual_keyboard.y_pos/8-1,
                               virtual_keyboard.width/8+2,
                               virtual_keyboard.height/8+3 );


    widget_printstring(20, 8, WIDGET_COLOUR_TITLE, "Loading game...");


    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+20,
                                 virtual_keyboard.y_pos+20,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;

    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, "Please wait.");

    int scale = machine_current->timex ? 2 : 1;
    uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                    scale * DISPLAY_SCREEN_HEIGHT );
    uidisplay_frame_end();
}

void loadGame(char *fileName)
{
    printf("Doing utils_open_file %s\n", fileName);
    fflush(stdout);    
    utils_open_file(fileName, tape_can_autoload(), NULL );    
}

void msgInfo(char *title, char *text)
{
    menuWidgetType = MENU_MSG_INFO;
    virtual_keyboard.x_pos = 30;
    virtual_keyboard.y_pos = 20;
    virtual_keyboard.width = 200;
    virtual_keyboard.height = 120;

    widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                               virtual_keyboard.y_pos/8-1,
                               virtual_keyboard.width/8+2,
                               virtual_keyboard.height/8+3 );


    widget_printstring(20, 8, WIDGET_COLOUR_TITLE, title);


    // Clear screen
    widget_draw_rectangle_solid( virtual_keyboard.x_pos+20,
                                 virtual_keyboard.y_pos+20,
                                 virtual_keyboard.width,
                                 virtual_keyboard.height,
                                 WIDGET_COLOUR_BACKGROUND );

    int stringWidth;
    int x = 24;
    int y = 30;

    char oneLine[255];
    int i = 0;
    int j = 0;
    while (text[i] != 0) {
        if (text[i] == '\n') {
            oneLine[j] = 0;
            widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, oneLine);
            y+= 10;
            j = 0;
        }
        else {
            oneLine[j] = text[i];
            j++;
        }
        i++;
    }
    oneLine[j] = 0;
    widget_printstring(x, y, WIDGET_COLOUR_FOREGROUND, oneLine);

    int scale = machine_current->timex ? 2 : 1;
    uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                    scale * DISPLAY_SCREEN_HEIGHT );
    uidisplay_frame_end();
}

void
widget_virtualkeyboard_keyhandler( input_key key )
{
  int cursor_pressed = 0;
  int previous_button_enum = virtual_keyboard.cur_button_enum;
  int i, j, keyNum;

  if (menuWidgetType == MENU_VIRTUAL_KEYBOARD) {
      switch( key ) {

    #if 0
      case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
        widget_dialog_with_border( 1, 2, 30, 2 + 20 );
        widget_general_show_all( &widget_options_settings );
        break;
    #endif

      case INPUT_KEY_Escape:
          widget_end_all( WIDGET_FINISHED_CANCEL );
          display_refresh_all();
          vegaSetAudio(vegaGetAudio());
          return;
          break;

      case INPUT_KEY_Up:
      case INPUT_KEY_7:
      case INPUT_JOYSTICK_UP:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorUp();
        break;

      case INPUT_KEY_Down:
      case INPUT_KEY_6:
      case INPUT_JOYSTICK_DOWN:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorDown();
        break;

      case INPUT_KEY_Left:
      case INPUT_KEY_5:
      case INPUT_JOYSTICK_LEFT:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorLeft();
        break;

      case INPUT_KEY_Right:
      case INPUT_KEY_8:
      case INPUT_JOYSTICK_RIGHT:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorRight();
        break;

      case INPUT_KEY_Return:
        if (isSymOn) {
            keyBuffer[0] = getSymKey(virtual_keyboard.cur_button_enum);
            keyBuffer[1] = '\x00';
        }
        else {
            keyBuffer[0] = virtual_keyboard.cur_button_enum;
            keyBuffer[1] = '\x00';
        }
        setVirtualKeys(&keyBuffer[0]);
        if (virtual_keyboard.cur_button_enum == E_VKB_CAPS_SHIFT)
            if (isCapsLockOn == 0)
                isCapsLockOn = 1;
        else
                isCapsLockOn = 0;
        if (virtual_keyboard.cur_button_enum == E_VKB_SYMBOL_SHIFT)
            if (isSymOn == 0)
                isSymOn = 1;
        else
                isSymOn = 0;

        if (virtual_keyboard.cur_button->button_enum == E_VKB_SYMBOL_SHIFT) {
//            DrawVirtualKeyboardButton(virtual_keyboard.cur_button,
//                                      virtual_keyboard.cur_button->x_pos,
//                                      virtual_keyboard.cur_button->y_pos);
            InitVirtualKeyboardButtons();

            // The following sets the cirtual keyboard into the key selection
            // mode
            int i;
            widget_dialog_with_border( virtual_keyboard.x_pos/8-1,
                                       virtual_keyboard.y_pos/8-1,
                                       virtual_keyboard.width/8+2,
                                       virtual_keyboard.height/8+3 );

            for (i=0; i<NUM_VIRTUAL_KEYBOARD_BUTTONS; i++) {
              DrawVirtualKeyboardButton(&virtual_keyboard.buttons[i],
                                        virtual_keyboard.buttons[i].x_pos,
                                        virtual_keyboard.buttons[i].y_pos);
            }

            int scale = machine_current->timex ? 2 : 1;
            uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                            scale * DISPLAY_SCREEN_HEIGHT );
            uidisplay_frame_end();
        }
        else {
            widget_end_all( WIDGET_FINISHED_OK );
            display_refresh_all();
            vegaSetAudio(vegaGetAudio());
        }

        return;
        break;

      default:	/* Keep gcc happy */
        break;

      }

      if( cursor_pressed ) {
        DrawVirtualKeyboardButton(&virtual_keyboard.buttons[previous_button_enum],
                                  virtual_keyboard.buttons[previous_button_enum].x_pos,
                                  virtual_keyboard.buttons[previous_button_enum].y_pos);

        DrawVirtualKeyboardButton(&virtual_keyboard.buttons[virtual_keyboard.cur_button_enum],
                                  virtual_keyboard.buttons[virtual_keyboard.cur_button_enum].x_pos,
                                  virtual_keyboard.buttons[virtual_keyboard.cur_button_enum].y_pos);

        int scale = machine_current->timex ? 2 : 1;
        uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                        scale * DISPLAY_SCREEN_HEIGHT );
        uidisplay_frame_end();
        return;
      }
  }
  else if (menuWidgetType == MENU_SETTINGS) {
      switch( key ) {
          case INPUT_KEY_Escape:
              widget_end_all( WIDGET_FINISHED_CANCEL );
              display_refresh_all();
              vegaSetAudio(vegaGetAudio());
              return;
              break;

          case INPUT_KEY_Up:
          case INPUT_KEY_7:
          case INPUT_JOYSTICK_UP:
            currentSettingsItem --;
            if (currentSettingsItem < 0)
                currentSettingsItem = 0;

            displaySettings();
            break;

          case INPUT_KEY_Down:
          case INPUT_KEY_6:
          case INPUT_JOYSTICK_DOWN:
            currentSettingsItem++;
            if (currentSettingsItem >= MAX_SETTINGS_ITEMS-1)
                currentSettingsItem = MAX_SETTINGS_ITEMS-1;

            displaySettings();
            break;

          case INPUT_KEY_Left:
          case INPUT_KEY_5:
          case INPUT_JOYSTICK_LEFT:
            if (currentSettingsItem == 0) {
                vegaDACVolumeDown();
            }
            else if (currentSettingsItem == 1) {
                if (vegaGetAudio() == 1)
                    vegaSetAudio(0);
            }
            else if (currentSettingsItem == 2) {
                vegaBrightnessDown();
            }
            else if (currentSettingsItem == 3) {
                vegaContrastDown();
            }
            displaySettings();
            break;

          case INPUT_KEY_Right:
          case INPUT_KEY_8:
          case INPUT_JOYSTICK_RIGHT:
            if (currentSettingsItem == 0) {
                vegaDACVolumeUp();
            }
            else if (currentSettingsItem == 1) {
                if (vegaGetAudio() == 0)
                    vegaSetAudio(1);
            }
            else if (currentSettingsItem == 2) {
                vegaBrightnessUp();
            }
            else if (currentSettingsItem == 3) {
                vegaContrastUp();
            }
            displaySettings();
          break;
//          case INPUT_KEY_Return:
//              if (currentSettingsItem == 4) {
//                  if (sdGameData && sdGameData[sdGameListOffset+sdSelectedGameOnScreen].pokes != NULL) {
//                      printf("\nApplying pokes:");
//                      char poke[24] = "set ";
//                      int j=4;
//                      int i=0;
//                      while (sdGameData[sdGameListOffset+sdSelectedGameOnScreen].pokes[i] != 0) {
//                          if (sdGameData[sdGameListOffset+sdSelectedGameOnScreen].pokes[i] == ';') {
//                              poke[j] = 0;
//                              printf("\n%s", &poke[0]);
//                              debugger_command_evaluate(&poke[0]);
//                              j = 4;
//                          }
//                          else {
//                              poke[j] = sdGameData[sdGameListOffset+sdSelectedGameOnScreen].pokes[i];
//                              if (poke[j] == ',')
//                                  poke[j] = ' ';
//                              j++;
//                          }
//                          i++;
//                      }
//                      if (j != 4) {
//                          poke[j] = 0;
//                          printf("\n%s", &poke[0]);
//                          debugger_command_evaluate(&poke[0]);
//                      }
//                  }
//                  widget_end_all( WIDGET_FINISHED_CANCEL );
//                  display_refresh_all();
//              }

          default:	/* Keep gcc happy */
            break;

      }
  }
  else if (menuWidgetType == MENU_GAME_SELECT || menuWidgetType == MENU_SD_GAME_SELECT){
      switch( key ) {

    #if 0
      case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
        widget_dialog_with_border( 1, 2, 30, 2 + 20 );
        widget_general_show_all( &widget_options_settings );
        break;
    #endif

      case INPUT_KEY_Escape:
        widget_end_all( WIDGET_FINISHED_CANCEL );
        display_refresh_all();
        vegaSetAudio(vegaGetAudio());
        return;
        break;

      case INPUT_KEY_Up:
      case INPUT_KEY_7:
      case INPUT_JOYSTICK_UP:
          if (menuWidgetType == MENU_GAME_SELECT) {
              selectedGameOnScreen--;
              if (selectedGameOnScreen < 0)
                  selectedGameOnScreen = 0;

              displayGameList(gameListOffset);
          }
          if (menuWidgetType == MENU_SD_GAME_SELECT) {
              sdSelectedGameOnScreen--;
              if (sdSelectedGameOnScreen < 0)
                  sdSelectedGameOnScreen = 0;

              displaySDCardGameList(sdGameListOffset);
          }
        break;

      case INPUT_KEY_Down:
      case INPUT_KEY_6:
      case INPUT_JOYSTICK_DOWN:
          if (menuWidgetType == MENU_GAME_SELECT) {
              selectedGameOnScreen++;
              if (selectedGameOnScreen == NUM_GAMES_ON_SCREEN)
                  selectedGameOnScreen = NUM_GAMES_ON_SCREEN-1;

              if (gameListOffset + selectedGameOnScreen >= numGamesFound)
                  selectedGameOnScreen = numGamesFound - gameListOffset - 1;

              displayGameList(gameListOffset);
          }
          if (menuWidgetType == MENU_SD_GAME_SELECT) {
              sdSelectedGameOnScreen++;
              if (sdSelectedGameOnScreen == NUM_GAMES_ON_SCREEN)
                  sdSelectedGameOnScreen = NUM_GAMES_ON_SCREEN-1;

              if (sdGameListOffset + sdSelectedGameOnScreen >= sdNumGamesFound)
                  sdSelectedGameOnScreen = sdNumGamesFound - sdGameListOffset - 1;

              displaySDCardGameList(sdGameListOffset);
          }
          break;

      case INPUT_KEY_Left:
      case INPUT_KEY_5:
      case INPUT_JOYSTICK_LEFT:
          if (menuWidgetType == MENU_GAME_SELECT) {
              if (selectedGameOnScreen >= NUM_GAMES_ON_SCREEN / 2)
                  selectedGameOnScreen -= NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (gameListOffset > NUM_GAMES_ON_SCREEN) {
                      gameListOffset -= NUM_GAMES_ON_SCREEN;
                      selectedGameOnScreen += NUM_GAMES_ON_SCREEN / 2;
                  }
                  else
                      gameListOffset = 0;
              }
              displayGameList(gameListOffset);
          }
          if (menuWidgetType == MENU_SD_GAME_SELECT) {
              if (sdSelectedGameOnScreen >= NUM_GAMES_ON_SCREEN / 2)
                  sdSelectedGameOnScreen -= NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (sdGameListOffset > NUM_GAMES_ON_SCREEN) {
                      sdGameListOffset -= NUM_GAMES_ON_SCREEN;
                      sdSelectedGameOnScreen += NUM_GAMES_ON_SCREEN / 2;
                  }
                  else
                      sdGameListOffset = 0;
              }
              displaySDCardGameList(sdGameListOffset);
          }
        break;

      case INPUT_KEY_Right:
      case INPUT_KEY_8:
      case INPUT_JOYSTICK_RIGHT:
          if (menuWidgetType == MENU_GAME_SELECT) {
              if (selectedGameOnScreen < NUM_GAMES_ON_SCREEN / 2)
                  selectedGameOnScreen += NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (gameListOffset + NUM_GAMES_ON_SCREEN <= numGamesFound) {
                      gameListOffset += NUM_GAMES_ON_SCREEN;
                      selectedGameOnScreen -= NUM_GAMES_ON_SCREEN / 2;
                  }
              }

              if (gameListOffset > (MAX_GAME_NUM-NUM_GAMES_ON_SCREEN))
                  gameListOffset = MAX_GAME_NUM-NUM_GAMES_ON_SCREEN;

              if (gameListOffset + selectedGameOnScreen >= numGamesFound)
                  selectedGameOnScreen = numGamesFound - gameListOffset - 1;

              displayGameList(gameListOffset);
          }
          if (menuWidgetType == MENU_SD_GAME_SELECT) {
              if (sdSelectedGameOnScreen < NUM_GAMES_ON_SCREEN / 2)
                  sdSelectedGameOnScreen += NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (sdGameListOffset + NUM_GAMES_ON_SCREEN <= sdNumGamesFound) {
                      sdGameListOffset += NUM_GAMES_ON_SCREEN;
                      sdSelectedGameOnScreen -= NUM_GAMES_ON_SCREEN / 2;
                  }
              }

              if (sdGameListOffset > (MAX_GAME_NUM-NUM_GAMES_ON_SCREEN))
                  sdGameListOffset = MAX_GAME_NUM-NUM_GAMES_ON_SCREEN;

              if (sdGameListOffset + sdSelectedGameOnScreen >= sdNumGamesFound)
                  sdSelectedGameOnScreen = sdNumGamesFound - sdGameListOffset - 1;

              displaySDCardGameList(sdGameListOffset);
          }
      break;
      case INPUT_KEY_S:
          if (menuWidgetType == MENU_GAME_SELECT) {
              if (gameData[gameListOffset+selectedGameOnScreen].isFavourite == 0)
                  gameData[gameListOffset+selectedGameOnScreen].isFavourite = 1;
              else
                  gameData[gameListOffset+selectedGameOnScreen].isFavourite = 0;

              // Save favourite menu
#ifdef __arm__
              char fav[] = "/media/favourite.txt";
#else
              char fav[] = "/home/./test/favourite.txt";
#endif
              int i;
              FILE *f = fopen(fav, "wt");

              printf("\nSaving %s", fav);
              favGamesFound = 0;             
              if (f) {
                  for (i=0;i<numGamesFound;i++) {
                      if (gameData[i].isFavourite == 1) {
                          fprintf(f, "%d\n", i);
                          favGamesFound++;
                      }
                  }
                  printf("\nSaved");
                  fclose(f);
                  displayGameList(gameListOffset);
              }
              else {
                  msgInfo("Error saving data in SD Card", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
                  return;
              }
          }
          break;
      case INPUT_KEY_Return:
        if (menuWidgetType == MENU_GAME_SELECT) {
            doPleaseWait();
            isSDCardGame = 0;
            long pages = sysconf(_SC_PHYS_PAGES);
            long page_size = sysconf(_SC_PAGE_SIZE);
            long freemem = pages * page_size;

            printf ("\nFREEMEM %ld", freemem);
            char path[64];
            getcwd(&path, 64);
            strcat(&path, "/roms/");
            fflush(stdout);

            printf("Index: %d", gameListOffset+selectedGameOnScreen);
            fflush(stdout);

            printf ("\nGame: %s", gameData[gameListOffset+selectedGameOnScreen].title);
            fflush(stdout);
            strcpy(lastGameFilename, gameData[gameListOffset+selectedGameOnScreen].filename);
//            printf("\n1");
//            fflush(stdout);
            currentGameId = gameListOffset+selectedGameOnScreen;

            vegaSetPokes(gameData[gameListOffset+selectedGameOnScreen].pokes);
//            printf("\n2");
//            fflush(stdout);

            strcpy(lastKeyMapping, gameData[gameListOffset+selectedGameOnScreen].keys);
//            printf("\n3");
//            fflush(stdout);

            remapKeys(gameData[gameListOffset+selectedGameOnScreen].keys);
//            printf("\n4");
//            fflush(stdout);

            clearAudioCache();
            printf("\nSetting machine type %s", &gameData[gameListOffset+selectedGameOnScreen].M[0]);            
            fflush(stdout);
            machine_select_id(&gameData[gameListOffset+selectedGameOnScreen].M[0]);

//            printf("\n1");
//            fflush(stdout);
            loadExtraINIFile(gameData[gameListOffset+selectedGameOnScreen].filename);
//            printf("\n2");
//            fflush(stdout);

            strcpy(&lastFilePath, &path);
            strncat(&lastFilePath, gameData[gameListOffset+selectedGameOnScreen].filename, 256 - 64);
            lastGameLoadFromSD = 0;
            lastGameLoaded = gameListOffset+selectedGameOnScreen;
            printf("\n3");
            fflush(stdout);

            loadGame(&lastFilePath);
            fflush(stdout);

            widget_end_all( WIDGET_FINISHED_OK );
            display_refresh_all();
            if (vegaIsTvOUTModeEnabled() == 1)
                vegaSetAudio(1);
            vegaSetAudio(vegaGetAudio());

            fflush(stdout);
            return;
        }
        if (menuWidgetType == MENU_SD_GAME_SELECT) {
            isSDCardGame = 1;

            if (sdNumGamesFound > 0) {
                doPleaseWait();
                long pages = sysconf(_SC_PHYS_PAGES);
                long page_size = sysconf(_SC_PAGE_SIZE);
                long freemem = pages * page_size;

                printf ("\nSD GAME FREEMEM %ld", freemem);
    #ifdef __arm__
                char path[255] = "/media/";
    #else
                char path[255] = "/home/./test/";
    #endif
                printf ("\nGame: %s", sdGameData[sdGameListOffset+sdSelectedGameOnScreen].title);
                strcpy(lastGameFilename, sdGameData[sdGameListOffset+sdSelectedGameOnScreen].filename);
                currentGameId = sdGameListOffset+sdSelectedGameOnScreen;

                vegaSetPokes(sdGameData[sdGameListOffset+sdSelectedGameOnScreen].pokes);
                strcpy(lastKeyMapping, sdGameData[sdGameListOffset+sdSelectedGameOnScreen].keys);
                remapKeys(sdGameData[sdGameListOffset+sdSelectedGameOnScreen].keys);
                clearAudioCache();
                printf("\nSetting machine type %s", &sdGameData[sdGameListOffset+sdSelectedGameOnScreen].M[0]);
                machine_select_id(&sdGameData[sdGameListOffset+sdSelectedGameOnScreen].M[0]);

                loadExtraINIFile(sdGameData[sdGameListOffset+sdSelectedGameOnScreen].filename);

                strcpy(&lastFilePath, &path);
                strncat(&lastFilePath, sdGameData[sdGameListOffset+sdSelectedGameOnScreen].filename, 256 - 64);
                lastGameLoadFromSD = 1;
                lastGameLoaded = sdGameListOffset+sdSelectedGameOnScreen;
                printf("\nLoading game %s", lastFilePath);
                fflush(stdout);
                loadGame(&lastFilePath);
            }

            widget_end_all( WIDGET_FINISHED_OK );
            display_refresh_all();

            if (vegaIsTvOUTModeEnabled() == 1)
                vegaSetAudio(1);
            vegaSetAudio(vegaGetAudio());

            fflush(stdout);
            return;
        }        break;

      default:	/* Keep gcc happy */
        break;

      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (menuWidgetType == MENU_SAVE_GAME || menuWidgetType == MENU_LOAD_GAME) {
      int gameDetected = 0;
      char gameState[255];

      int selectedGameIndex;
      if (menuWidgetType == MENU_SAVE_GAME)
          selectedGameIndex = currentSaveGame;
      else
          selectedGameIndex = currentLoadGame;

      switch( key ) {
      case INPUT_KEY_Up:
      case INPUT_KEY_7:
      case INPUT_JOYSTICK_UP:
          if (menuWidgetType == MENU_SAVE_GAME) {
              currentSaveGame--;
              if (currentSaveGame < 0)
                  currentSaveGame = 0;

              displaySaveGameMenu();
          }
          if (menuWidgetType == MENU_LOAD_GAME) {
              currentLoadGame--;
              if (currentLoadGame < 0)
                  currentLoadGame = 0;

              displayLoadGameMenu();
          }
          break;

      case INPUT_KEY_Down:
      case INPUT_KEY_6:
      case INPUT_JOYSTICK_DOWN:
          if (menuWidgetType == MENU_SAVE_GAME) {
              currentSaveGame++;
              if (currentSaveGame > 8)
                  currentSaveGame = 8;

              displaySaveGameMenu();
          }
          if (menuWidgetType == MENU_LOAD_GAME) {
              currentLoadGame++;
              if (currentLoadGame > 8)
                  currentLoadGame = 8;

              displayLoadGameMenu();
          }
          break;

      case INPUT_KEY_Return:
          if (lastGameLoadFromSD == 1 && lastGameLoaded >=0) {
#ifdef __arm__
              sprintf(gameState, "/media/%s.s0%d", sdGameData[lastGameLoaded].filename, selectedGameIndex+1);
#else
              sprintf(gameState, "/home/./test/%s.s0%d", sdGameData[lastGameLoaded].filename, selectedGameIndex+1);
#endif
              gameDetected = 1;
          }
          else if (lastGameLoadFromSD == 0 && lastGameLoaded >=0) {
#ifdef __arm__
              sprintf(gameState, "/media/%s.s0%d", gameData[lastGameLoaded].filename, selectedGameIndex+1);
#else
              sprintf(gameState, "/home/./test/%s.s0%d", gameData[lastGameLoaded].filename, selectedGameIndex+1);
#endif
              gameDetected = 1;
          }

          if (gameDetected == 1) {
              printf("\nDetected game name: %s", gameState);

              if (menuWidgetType == MENU_SAVE_GAME) {
                  FILE *f = fopen(&gameState, "wt");
                  if (f) {
                      fclose(f);
                      snapshot_write( gameState );
                  }
                  else {
                      msgInfo("Error saving data in SD Card", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
                      return;
                  }
              }
              else {
                  doPleaseWait();
                  clearAudioCache();
                  if (isSDCardGame && sdCurrentGameId >=0)
                      machine_select_id(&sdGameData[sdGameListOffset+sdSelectedGameOnScreen].M[0]);
                  else
                      machine_select_id(&gameData[gameListOffset+selectedGameOnScreen].M[0]);

                  loadGame(&gameState);

                  widget_end_all( WIDGET_FINISHED_OK );
                  display_refresh_all();
                  if (vegaIsTvOUTModeEnabled() == 1)
                      vegaSetAudio(1);
                  vegaSetAudio(vegaGetAudio());

                  fflush(stdout);
                  return;
              }
          }

          widget_end_all( WIDGET_FINISHED_OK );
          display_refresh_all();
          if (vegaIsTvOUTModeEnabled() == 1)
              vegaSetAudio(1);
          vegaSetAudio(vegaGetAudio());

          fflush(stdout);
          return;
          break;

      case INPUT_KEY_Escape:
        widget_end_all( WIDGET_FINISHED_CANCEL );
        display_refresh_all();
        vegaSetAudio(vegaGetAudio());
        return;
        break;



      default:	/* Keep gcc happy */
        break;

      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (menuWidgetType == MENU_GAME_FAV) {
      switch( key ) {
          case INPUT_KEY_Escape:
              widget_end_all( WIDGET_FINISHED_CANCEL );
              display_refresh_all();
              vegaSetAudio(vegaGetAudio());
              return;
              break;

          case INPUT_KEY_Up:
          case INPUT_KEY_7:
          case INPUT_JOYSTICK_UP:
              selectedFavGame--;
              if (selectedFavGame < 0)
                  selectedFavGame = 0;

              displayFavList(favListOffset);
              break;

          case INPUT_KEY_Down:
          case INPUT_KEY_6:
          case INPUT_JOYSTICK_DOWN:
              selectedFavGame++;
              if (selectedFavGame == NUM_GAMES_ON_SCREEN)
                  selectedFavGame = NUM_GAMES_ON_SCREEN-1;

              if (favListOffset + selectedFavGame >= favGamesFound)
                  selectedFavGame = favGamesFound - favListOffset - 1;

              displayFavList(favListOffset);
              break;

          case INPUT_KEY_Left:
          case INPUT_KEY_5:
          case INPUT_JOYSTICK_LEFT:
              if (selectedFavGame >= NUM_GAMES_ON_SCREEN / 2)
                  selectedFavGame -= NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (favListOffset > NUM_GAMES_ON_SCREEN) {
                      favListOffset -= NUM_GAMES_ON_SCREEN;
                      selectedFavGame += NUM_GAMES_ON_SCREEN / 2;
                  }
                  else
                      favListOffset = 0;
              }
              displayFavList(favListOffset);
              break;

          case INPUT_KEY_Right:
          case INPUT_KEY_8:
          case INPUT_JOYSTICK_RIGHT:
              if (selectedFavGame < NUM_GAMES_ON_SCREEN / 2)
                  selectedFavGame += NUM_GAMES_ON_SCREEN / 2;
              else {
                  if (favListOffset + NUM_GAMES_ON_SCREEN <= favGamesFound) {
                      favListOffset += NUM_GAMES_ON_SCREEN;
                      selectedFavGame -= NUM_GAMES_ON_SCREEN / 2;
                  }
              }

              if (favListOffset > (MAX_GAME_NUM-NUM_GAMES_ON_SCREEN))
                  favListOffset = MAX_GAME_NUM-NUM_GAMES_ON_SCREEN;

              if (favListOffset + selectedFavGame >= favGamesFound)
                  selectedFavGame = favGamesFound - favListOffset - 1;

              displayFavList(favListOffset);
              break;
//          case INPUT_KEY_S:
//              if (menuWidgetType == MENU_GAME_SELECT) {
//                  if (gameData[gameListOffset+selectedGameOnScreen].isFavourite == 0)
//                      gameData[gameListOffset+selectedGameOnScreen].isFavourite = 1;
//                  else
//                      gameData[gameListOffset+selectedGameOnScreen].isFavourite = 0;

//                  // Save favourite menu
//    #ifdef __arm__
//                  char fav[] = "/media/favourite.txt";
//    #else
//                  char fav[] = "/home/./test/favourite.txt";
//    #endif
//                  int i;
//                  FILE *f = fopen(fav, "wt");

//                  if (f) {
//                      for (i=0;i<numGamesFound;i++) {
//                          if (gameData[i].isFavourite == 1)
//                              fprintf(f, "%d\n", i);
//                      }
//                      fclose(f);
//                  }
//                  displayGameList(gameListOffset);
//              }
//              break;
          case INPUT_KEY_Return:
              doPleaseWait();
              isSDCardGame = 0;
              long pages = sysconf(_SC_PHYS_PAGES);
              long page_size = sysconf(_SC_PAGE_SIZE);
              long freemem = pages * page_size;




              printf ("\nFREEMEM %ld", freemem);
              char path[64];
              getcwd(&path, 64);
              strcat(&path, "/roms/");
              fflush(stdout);


              int j = 0;
              int skip = favListOffset + selectedFavGame;

              for (j=0;j<numGamesFound;j++) {
                  if (gameData[j].isFavourite == 1) {
                      if (skip > 0)
                          skip--;
                      else {
                          currentGameId = j;
                          break;
                      }
                  }
              }

              printf ("\nGame: %s", gameData[currentGameId].title);
              strcpy(lastGameFilename, gameData[currentGameId].filename);
              vegaSetPokes(gameData[currentGameId].pokes);

              vegaSetPokes(gameData[currentGameId].pokes);
              strcpy(lastKeyMapping, gameData[currentGameId].keys);
              remapKeys(gameData[currentGameId].keys);
              clearAudioCache();
              printf("\nSetting machine type %s", &gameData[currentGameId].M[0]);
              machine_select_id(&gameData[currentGameId].M[0]);

              loadExtraINIFile(gameData[currentGameId].filename);

              strcpy(&lastFilePath, &path);
              strncat(&lastFilePath, gameData[currentGameId].filename, 256 - 64);
              lastGameLoadFromSD = 0;
              lastGameLoaded = currentGameId;
              loadGame(&lastFilePath);
              fflush(stdout);

              widget_end_all( WIDGET_FINISHED_OK );
              display_refresh_all();
              if (vegaIsTvOUTModeEnabled() == 1)
                  vegaSetAudio(1);
              vegaSetAudio(vegaGetAudio());

              fflush(stdout);
              return;
              break;

          default:	/* Keep gcc happy */
              break;

          }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (menuWidgetType == MENU_GAME_REMAP_KEYS) {
      switch( key ) {
      case INPUT_KEY_Up:
      case INPUT_KEY_7:
      case INPUT_JOYSTICK_UP:
          currentKeyMapIndex--;
          if (currentKeyMapIndex < 0)
              currentKeyMapIndex = 0;

          displayRemapKeysMenu();
          break;
      case INPUT_KEY_Down:
      case INPUT_KEY_6:
      case INPUT_JOYSTICK_DOWN:
          currentKeyMapIndex++;
          if (currentKeyMapIndex > 11)
              currentKeyMapIndex = 11;

          displayRemapKeysMenu();
          break;
      case INPUT_KEY_Return:
          if (currentKeyMapIndex < 11) {
              widget_do( WIDGET_TYPE_VIRTUALKEYBOARD, MENU_GAME_REMAP_KEYS2);

              char newKeyMapping[255];
              j=0;
              keyNum = 0;
              for (i=0;i<255;i++) {
                  if ((currentKeyMapIndex+1) == keyNum) {
                      newKeyMapping[j] = lastSelectedKeyName[0];
                      j++;
                      if (lastSelectedKeyName[1] != 0) {
                          newKeyMapping[j] = lastSelectedKeyName[1];
                          j++;
                      }
                      while (lastKeyMapping[i] != ';' && lastKeyMapping[i] != 0 && i < 254)
                          i++;
                  }
                  newKeyMapping[j] = lastKeyMapping[i];
                  j++;
                  if (lastKeyMapping[i] == ';')
                      keyNum++;
                  if (lastKeyMapping[i] == 0)
                      break;
              }
              printf("\nOldKeyMap %s\nNewKeyMap %s", lastKeyMapping, newKeyMapping);
              strcpy(lastKeyMapping, newKeyMapping);
              remapKeys(lastKeyMapping);

              displayRemapKeysMenu();
              break;
         }
         else {
              // Saving mapping file
#ifdef __arm__
              char filePath[255] = "";
              sprintf(filePath, "/media/%s.ini", lastGameFilename);
#else
              char filePath[255] = "";
              sprintf(filePath, "/home/./test/%s.ini", lastGameFilename);
#endif
              printf("\nSaving %s", filePath);
              FILE *f = fopen(filePath, "wt");
              if (f) {
                  printf("\nSaving INI file on SD Card");
                  fprintf(f, "K:%s", lastKeyMapping);
                  fclose(f);
              }
              else {
                  msgInfo("Error saving data in SD Card", "Please make sure that an SD Card\nis inserted in the Micro-SD slot.\nThe card must be formatted using\nFAT32.");
                  return;
              }

              widget_end_all( WIDGET_FINISHED_OK );
              display_refresh_all();
              if (vegaIsTvOUTModeEnabled() == 1)
                  vegaSetAudio(1);
              vegaSetAudio(vegaGetAudio());

              fflush(stdout);
              return;
         }

      case INPUT_KEY_Escape:
        widget_end_all( WIDGET_FINISHED_CANCEL );
        display_refresh_all();
        vegaSetAudio(vegaGetAudio());
        return;
        break;



      default:	/* Keep gcc happy */
        break;

      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else if (menuWidgetType == MENU_GAME_REMAP_KEYS2) {
      switch( key ) {
      case INPUT_KEY_Escape:
          lastSelectedKeyName[0] = 0;
          widget_end_all( WIDGET_FINISHED_CANCEL );
          display_refresh_all();
          vegaSetAudio(vegaGetAudio());
          return;
          break;

      case INPUT_KEY_Up:
      case INPUT_KEY_7:
      case INPUT_JOYSTICK_UP:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorUp();
        break;

      case INPUT_KEY_Down:
      case INPUT_KEY_6:
      case INPUT_JOYSTICK_DOWN:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorDown();
        break;

      case INPUT_KEY_Left:
      case INPUT_KEY_5:
      case INPUT_JOYSTICK_LEFT:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorLeft();
        break;

      case INPUT_KEY_Right:
      case INPUT_KEY_8:
      case INPUT_JOYSTICK_RIGHT:
        cursor_pressed = 1;
        MoveVirtualKeyboardCursorRight();
        break;

      case INPUT_KEY_Return:
          if (virtual_keyboard.cur_button->text[0] == 'E' &&
              virtual_keyboard.cur_button->text[1] == 'N') {
              strcpy(lastSelectedKeyName, "EN");
          }
          else if (virtual_keyboard.cur_button->text[0] == 'C' &&
                   virtual_keyboard.cur_button->text[1] == 'A') {
              strcpy(lastSelectedKeyName, "CS");
          }
          else if (virtual_keyboard.cur_button->text[0] == 'S' &&
                   virtual_keyboard.cur_button->text[1] == 'Y') {
              strcpy(lastSelectedKeyName, "SS");
          }
          else if (virtual_keyboard.cur_button->text[0] == 'S' &&
                   virtual_keyboard.cur_button->text[1] == 'P') {
              strcpy(lastSelectedKeyName, "SP");
          }
          else {
              strcpy(lastSelectedKeyName, virtual_keyboard.cur_button->text);
          }

          lastSelectedKey = virtual_keyboard.cur_button_enum;
          printf("\nSelected button %s", lastSelectedKeyName);
          widget_end_widget( WIDGET_FINISHED_OK );
          display_refresh_all();

        return;
        break;

      default:	/* Keep gcc happy */
        break;

      }

      if( cursor_pressed ) {
        DrawVirtualKeyboardButton(&virtual_keyboard.buttons[previous_button_enum],
                                  virtual_keyboard.buttons[previous_button_enum].x_pos,
                                  virtual_keyboard.buttons[previous_button_enum].y_pos);

        DrawVirtualKeyboardButton(&virtual_keyboard.buttons[virtual_keyboard.cur_button_enum],
                                  virtual_keyboard.buttons[virtual_keyboard.cur_button_enum].x_pos,
                                  virtual_keyboard.buttons[virtual_keyboard.cur_button_enum].y_pos);

        int scale = machine_current->timex ? 2 : 1;
        uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                        scale * DISPLAY_SCREEN_HEIGHT );
        uidisplay_frame_end();
        return;
      }
  }
  else if (menuWidgetType == MENU_HALL_OF_FAME) {
      switch( key ) {
          case INPUT_KEY_Left:
          case INPUT_KEY_5:
          case INPUT_JOYSTICK_LEFT:
              if (hofIndex > 0)
                  hofIndex -= NUM_NAMES_ON_SCREEN;
              if (hofIndex < 0)
                  hofIndex = 0;
              displayHOF(hofIndex);
              break;

          case INPUT_KEY_Right:
          case INPUT_KEY_8:
          case INPUT_JOYSTICK_RIGHT:
              if (hofIndex + NUM_NAMES_ON_SCREEN < MAX_NAMES_NUM)
                  hofIndex += NUM_NAMES_ON_SCREEN;

              displayHOF(hofIndex);
              break;
      case INPUT_KEY_Escape:
      case INPUT_KEY_Return:
        widget_end_all( WIDGET_FINISHED_CANCEL );
        display_refresh_all();
        vegaSetAudio(vegaGetAudio());
        return;
        break;

      default:	/* Keep gcc happy */
        break;

      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
  else {
      switch( key ) {

      case INPUT_KEY_Escape:
      case INPUT_KEY_Return:
        widget_end_all( WIDGET_FINISHED_CANCEL );
        display_refresh_all();
        vegaSetAudio(vegaGetAudio());
        return;
        break;

      default:	/* Keep gcc happy */
        break;

      }

      int scale = machine_current->timex ? 2 : 1;
      uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      uidisplay_frame_end();
  }
}
