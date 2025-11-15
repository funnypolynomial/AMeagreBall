#pragma once
// Small version uses 240x320 LCD Touch Screen Shield, https://www.jaycar.co.nz/240x320-lcd-touch-screen-for-arduino/p/XC4630, SPFD5408 interface?
// --OR-- later revision of above, HX8347i, see XC4630_HX8347i define
// --OR-- even later revision of above, UC8230?, see XC4630_UC8230 define
#include "LCD.h"
// The LCD interface
// Initialise
#define LCD_INIT() lcd.init();
// Define a window to fill with pixels at (_x,_y) width _w, height _h
// Returns the number of pixels to fill (unsigned long)
#define LCD_BEGIN_FILL(_x,_y,_w,_h) lcd.beginFill(_x,_y,_w,_h)
// Sends _sizeUL (unsigned long) pixels of the 16-bit colour
#define LCD_FILL_COLOUR(_sizeUL, _colorWord) lcd.fillColour(_sizeUL, _colorWord)
// Sends _sizeUL (unsigned long) pixels of the 8-bit colour.
// The byte is duplicated, 0xFF and 0x00 really only make sense. Slightly faster than above.
#define LCD_FILL_BYTE(_sizeUL, _colorByte) lcd.fillByte(_sizeUL, _colorByte)
// Sends a single white pixel
#define LCD_ONE_WHITE() lcd.OneWhite()
// Sends a single black pixel
#define LCD_ONE_BLACK() lcd.OneBlack()
// True if there is a touch. Returns position in (int) _x, _y
#define LCD_GET_TOUCH(_x, _y) lcd.getTouch(_x, _y)
// Scroll left/right number of columns
#define LCD_SCROLL(_left,_cols) {lcd.setScroll(_left);lcd.scrollBy(_cols);}


// The whole LCD
#define LCD_WIDTH  320
#define LCD_HEIGHT 240

#define BALL_SHADOW_WIDTH 20
// there seems to be an asymmetry in the left & right sides
#define BALL_LIMIT_LEFT   16
#define BALL_LIMIT_RIGHT  (LCD_WIDTH - 25) // Note: allow for shadow, there is no checking/clipping
#define BALL_LIMIT_TOP    20
#define BALL_LIMIT_BOTTOM (LCD_HEIGHT - 16) // Affects how far "forward" the ball looks
