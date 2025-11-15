#include "Boing.h"
#include "Font.h"
#include "Workbench.h"

namespace Workbench
{
  const byte kWhiteByte = CFG_WORKBENCH_FOREGND_BYTE;
  const byte kBlackByte = 0x00;
  const int kFrameBotThickness = 18;
  const int kFrameLeftThickness = 2;// 17; vs E/F on workbench window
  const int kFrameRightThickness = 16;
  const int kAppFrameThickness = 2;
  const int kCloseOffset = 4;
  const int kBackFrontOffset = -53;
  const int kTitleOffset = 2;
  int _backFrontWidthIcon = 0;

  #include "IconData.h"

  void DrawIcon(int x, int y, const uint8_t* pIcon, bool bottomRight = false)
  {
    // draw a 2bpp icon image
    uint8_t w = pgm_read_byte(pIcon++);
    uint8_t h = pgm_read_byte(pIcon++);
    if (bottomRight)
    {
      x -= w;
      y -= h;
    }
    h = min(h, LCD_HEIGHT - y);
    int size = LCD_BEGIN_FILL(x, y, w, h);
    while (size)
    {
      uint8_t run = pgm_read_byte(pIcon++);
      if (!run)
        break;
      uint8_t colour = ICON_PALETTE_IDX(run);
      uint8_t len = ICON_RUN_LEN(run);
      if (colour == 3)
        LCD_FILL_COLOUR(len, CFG_ICON_RED);
      else if (colour == 2)
        LCD_FILL_BYTE(len, kWhiteByte);
      else if (colour == 1)
        LCD_FILL_COLOUR(len, CFG_WORKBENCH_BACKGND);
      else
        LCD_FILL_BYTE(len, kBlackByte);
      size -= len;
    }
  }

  void Init()
  {
    // init workbench
    _backFrontWidthIcon = pgm_read_byte(backFrontIcon);
  }

  void DrawWindowFrame(int x, int y, int width, int height, WindowType type,
                       int* pClientX, int* pClientY, int* pClientW, int* pClientH)
  {
    // draw a workbench-style window. if folder, partial folder window
    int top = kFrameTopThickness;
    int bot = type == DitheredFolder ? kFrameBotThickness : kAppFrameThickness;
    int left = type == DitheredFolder ? kFrameLeftThickness : kAppFrameThickness;
    int right = type == DitheredFolder ? kFrameRightThickness : kAppFrameThickness;
    if (type == Workbench || type == WorkbenchFast)
      left = right = 0;

    // update client rect if requested:
    if (pClientX) *pClientX = x + left;
    if (pClientY) *pClientY = y + top;
    if (pClientW) *pClientW = width - left - right;
    if (pClientH) *pClientH = height - top - bot;

    LCD_FILL_BYTE(LCD_BEGIN_FILL(x, y, width, top), kWhiteByte); // top
    if (type != AppFast && type != WorkbenchFast)
    {
      if (type != Workbench)
        DrawIcon(x + kCloseOffset, y, closeIcon);
      DrawIcon(x + width + kBackFrontOffset, y, backFrontIcon);
    }
    if (type == DitheredFolder)
    {
      // 2 hz lines
      int LHS = x + kCloseOffset + pgm_read_byte(closeIcon) + kTitleOffset;
      int RHS = x + width + kBackFrontOffset - kTitleOffset;
      int len = RHS - LHS;
      len -= len % 4;
      for (int bar = 0; bar < 2; bar++)
      {
        LCD_BEGIN_FILL(LHS, y + 4 + bar*8, len + 1, 4);
        for (int row = 0; row < 4; row++)
        {
          if (row >= 2)
            LCD_FILL_BYTE(1, kWhiteByte);
          for (int i = 0; i < len; i += 4)
            if ((row / 2) % 2)
            {
              LCD_FILL_BYTE(1, kWhiteByte);
              LCD_FILL_COLOUR(3, CFG_WORKBENCH_BACKGND);
            }
            else
            {
              LCD_FILL_COLOUR(3, CFG_WORKBENCH_BACKGND);
              LCD_FILL_BYTE(1, kWhiteByte);
            }
          if (row < 2)
            LCD_FILL_BYTE(1, kWhiteByte);
        }
      }
    }

    if (type == Workbench || type == WorkbenchFast)
    {
      if ((height - top) > 0)
        LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + left, y + top, width - left - right, height - top), CFG_WORKBENCH_BACKGND); // body
    }
    else if (height > top)
    {
      LCD_FILL_BYTE(LCD_BEGIN_FILL(x, y + top, left, height - top), kWhiteByte); // lhs
      LCD_FILL_BYTE(LCD_BEGIN_FILL(x + width - right, y + top, right, height - top), kWhiteByte); // rhs
      if (type == DitheredFolder)
      {
        LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + left, y + top, width - left - right, height - top), CFG_WORKBENCH_BACKGND); // body
        // scroll bar
        DrawIcon(x + width - right, y + top, upArrowIcon); // arrow
        int scrollY = y + top + pgm_read_byte(upArrowIcon + 1);
        if (scrollY < LCD_HEIGHT - 2)
        {
          const int kScrollMargin = 2;
          int scrollWidth = right - 2*kScrollMargin;
          LCD_BEGIN_FILL(x + width - right + kScrollMargin, scrollY, scrollWidth, LCD_HEIGHT - scrollY);
          LCD_FILL_COLOUR(2*scrollWidth, CFG_WORKBENCH_BACKGND); // bar top end
          scrollY += 2;
          LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + width - right + kScrollMargin, scrollY, kScrollMargin, LCD_HEIGHT - scrollY), CFG_WORKBENCH_BACKGND); // bar left
          LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + width - right + scrollWidth, scrollY, kScrollMargin, LCD_HEIGHT - scrollY), CFG_WORKBENCH_BACKGND);   // bar right
        }
      }
      else
      {
        LCD_FILL_BYTE(LCD_BEGIN_FILL(x + left, y + height - bot, width - left - right, bot), kWhiteByte); // bot
        LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + left, y + top, width - left - right, height - top - bot), CFG_WORKBENCH_BACKGND); // body
        if (type != AppFast)
          DrawIcon(x + width, y + height, sizeIcon, true);
      }
    }
  }

  void DrawWindowTitle(int x, int y, const char* pTitle, WindowType type, bool progmem)
  {
    // Draw title on window of type
    if (type == Workbench)
      x += kCloseOffset;
    else
      x += kCloseOffset + pgm_read_byte(closeIcon) + kTitleOffset;
    Font::DrawString(x, y + kTitleOffset, pTitle, CFG_WORKBENCH_BACKGND, RGB(255, 255, 255), type == DitheredFolder, 2, progmem);
  }

  WindowHit HitTest(int touchX, int touchY, int x, int y, int w, int h, WindowType )
  {
    // determine where on the window the touch was
    if (x <= touchX && touchX <= (x + w) && y <= touchY && touchY <= (y + h))
    {
      // in window
      if (touchY <= (y + kFrameTopThickness))
      {
        // in title bar
        if ((x + w + kBackFrontOffset) <= touchX && touchX <= (x + w + kBackFrontOffset + _backFrontWidthIcon / 2))
          return SendBehindHit;
        else
          return TitleHit;
      }
      else
        return BodyHit;
    }
    return NoHit;
  }

  void DrawBallIcon(int x, int y)
  {
    // Draw the "boing" icon
    DrawIcon(x, y, boingIcon);
  }

}
