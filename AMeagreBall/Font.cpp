#include "Boing.h"
#include "Font.h"
namespace Font
{
  COLOUR _AltForegroundColour = RGB(200, 200, 200); // alternate text colour enabled/disabled by FONT_ALT_TOGGLE
  uint8_t _scaleFactor = 1;

  // Characters extracted from "Kickstart 1.2" image at https://heckmeck.de/blog/amiga-topaz-1.4/  using extract_font.py

  // Font data as PROGMEM table
  #include "FontData.h"

  // Extra char(s)
  static const uint8_t fontExtras[] PROGMEM =
  {
    // tick (for menu)
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000110,
    0b00000110,
    0b00000110,
    0b00000110,
    0b00000110,
    0b00001100,
    0b11001100,
    0b01101100,
    0b00111100,
    0b00011000,
    0b00000000,
    0b00000000,
    0b00000000,

    // send to back icon
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111100,
    0b10000100,
    0b10111111,
    0b10111111,
    0b10111111,
    0b11111111,
    0b00111111,
    0b00111111,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,

  };

#define READ(_ptr) (progmem?pgm_read_byte(_ptr):*_ptr)
  void DrawString(int x, int y, const char* pStr, COLOUR fore, COLOUR back, bool dither, int trailingGapPixels, bool progmem)
  {
    // draw the string at x, y. in the colours. if dither mask pixels, add a gap at the end, progmem or normal ram
    bool altColour = false;
    while (READ(pStr))
    {
      const uint8_t* pRow = font + FONT_CHAR_HEIGHT * (READ(pStr) - ' ');
      uint8_t ch = READ(pStr);
      if (ch >= (uint8_t)FONT_TICK_CHAR)
        pRow = fontExtras + FONT_CHAR_HEIGHT*(ch - (uint8_t)FONT_TICK_CHAR);
      else if (ch == FONT_ALT_TOGGLE)
      {
        altColour = !altColour;
        pStr++;
        continue;
      }
      LCD_BEGIN_FILL(x, y, _scaleFactor*FONT_CHAR_WIDTH, _scaleFactor*FONT_CHAR_HEIGHT);
      for (int row = 0; row < FONT_CHAR_HEIGHT; row++)
      {
        uint8_t rowBits = pgm_read_byte(pRow++);
        uint8_t scaleRowBits = rowBits;
        if (dither)
          rowBits &= ((row / 2) % 2) ? 0b11101111 : 0b10111011;
        for (int scale = 0; scale < _scaleFactor; scale++)
        {
          for (int col = 0; col < FONT_CHAR_WIDTH; col++)
          {
            if (rowBits & 0x80)
              LCD_FILL_COLOUR(_scaleFactor, altColour ? _AltForegroundColour : fore);
            else
              LCD_FILL_COLOUR(_scaleFactor, back);
            rowBits <<= 1;
          }
          rowBits = scaleRowBits;
        }
      }
      pStr++;
      x += _scaleFactor*FONT_CHAR_WIDTH;
    }
    if (trailingGapPixels > 0)
      LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y, _scaleFactor*trailingGapPixels, _scaleFactor*FONT_CHAR_HEIGHT), back);
  }
}
