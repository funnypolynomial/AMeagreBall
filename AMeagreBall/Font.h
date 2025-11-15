#pragma once

#define FONT_CHAR_HEIGHT    16
#define FONT_CHAR_BASELINE  14 // height without descender
#define FONT_CHAR_WIDTH     8
#define FONT_TICK_CHAR      '\x80' // not part of the original font
#define FONT_SEND_BACK_CHAR '\x81' // not part of the original font
#define FONT_ALT_TOGGLE     '\x01' // toggle to/from Alt foreground colour

namespace Font
{
  // two globals to reduce the number of parms below!
  extern COLOUR _AltForegroundColour; // alt colour, see FONT_ALT_TOGGLE
  extern uint8_t _scaleFactor;        // 1 or 2
  void DrawString(int x, int y, const char* pStr, COLOUR fore, COLOUR back, 
                  bool dither = false, int trailingGapPixels = 0, bool progmem = false);
}
