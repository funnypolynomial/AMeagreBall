#include "Boing.h"
#include "Workbench.h"
#include "Font.h"
#include "Ball.h"
#include "Config.h"
#include "Menu.h"

namespace Menu
{
  const int kMenuLineGap = 2; // between lines, half the gap at top and bottom
  byte _menuWidth = 0, _menuHeight = 0, _menuRows = 0;
  int _menuX = 0, _menuY = 0;
  // live items are provided on-the-fly (vs progmem), are displayed differently and aren't selectable
  LiveItemFunc _menuLiveItem = NULL;

  char* LiveItem(int item)
  {
    // get a live item string, or NULL
    if (_menuLiveItem)
      return _menuLiveItem(item);
    else
      return NULL;
  }

  void UpdateMenuItem(int x, int y, int item, const char* pStrings_P, uint16_t ticked, uint16_t disabled, bool highlighted)
  {
    // re-draw a single item
    char buffer[32];
    uint16_t mask = 1 << item;
    buffer[0] = (ticked & mask) ? CFG_MENU_TICK : ' ';
    COLOUR fore = highlighted ? CFG_MENU_SEL_FOREGND : CFG_MENU_ITEM_FOREGND;
    COLOUR back = highlighted ? CFG_MENU_SEL_BACKGND : CFG_MENU_ITEM_BACKGND;
    char* pLive = LiveItem(item);
    if (pLive)
    {
      strcpy(buffer + 1, pLive);
      fore = CFG_MENU_LIVE_FOREGND;
      back = CFG_MENU_LIVE_BACKGND;
    }
    else
      strcpy_P(buffer + 1, Utility::GetMString(pStrings_P, item));
    if (disabled & mask)
    {
      fore = CFG_MENU_GHOST_FOREGND;
      back = CFG_MENU_GHOST_BACKGND;
    }
    x++;
    y += 1 + kMenuLineGap / 2 + item * (FONT_CHAR_HEIGHT + kMenuLineGap);
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y - 1, _menuWidth - 1, 1), back); // line above
    Font::DrawString(x, y, buffer, fore, back, false, _menuWidth - (int)strlen(buffer)*FONT_CHAR_WIDTH - 1); // pad to end
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y + FONT_CHAR_HEIGHT, _menuWidth - 1, 1), back); // line below
  }

  void DrawMenu(int x, int y, const char* pStrings_P, uint16_t ticked, uint16_t disabled)
  {
    // paint the menu items and outer rectangle
    int cols = 0;
    const char* pStr = pStrings_P;
    // get dims
    _menuRows = 0;
    int item = 0;
    while (pgm_read_byte(pStr))
    {
      char* pLive = LiveItem(item++);
      if (pLive)
        cols = (int)max(cols, (int)strlen(pLive));
      else
        cols = (int)max(cols, (int)strlen_P(pStr));
      pStr += strlen_P(pStr) + 1;
      _menuRows++;
    }
    _menuWidth = FONT_CHAR_WIDTH * (cols + 2) + 2;
    _menuHeight = FONT_CHAR_HEIGHT * _menuRows + kMenuLineGap * (_menuRows + 1) - 1;
    // frame
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y, _menuWidth, 1), CFG_MENU_FRAME); // borders...
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y + _menuHeight, _menuWidth, 1), CFG_MENU_FRAME);
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x, y, 1, _menuHeight), CFG_MENU_FRAME);
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + _menuWidth, y, 1, _menuHeight), CFG_MENU_FRAME);
    // paint text
    pStr = pStrings_P;
    item = 0;
    while (pgm_read_byte(pStr))
    {
      UpdateMenuItem(x, y, item, pStrings_P, ticked, disabled, false);
      item++;
      pStr += strlen_P(pStr) + 1;
    }
  }

  int MenuHitTest(int x, int y, int touchX, int touchY, uint16_t disabled)
  {
    // return menu item under (x, y) or -1
    if (x < touchX && touchX < (x + _menuWidth) && (y + kMenuLineGap / 2) < touchY && touchY < (y + _menuHeight - kMenuLineGap))
    {
      int item = min((touchY - y - kMenuLineGap / 2) / (FONT_CHAR_HEIGHT + kMenuLineGap), _menuRows - 1);
      if (!(disabled & 1 << item))
        return item;
    }
    return -1;
  }

  int Select(int x, int y, const char* pStrings_P, uint16_t ticked, uint16_t disabled, uint16_t labels, LiveItemFunc live)
  {
    // draw the menu and track the touch
    // lsb of uint16_t bitsets correponds to menu item #0
    // ticked: shown with tick
    // disabled: show dithered, unselectable
    // label: shown as normal but not selectable. if any, menu not erased
    // returns selection or -1
    // -2 denotes a touch outside the menu
    int prevItem = -1;
    _menuLiveItem = live;
    _menuX = x;
    _menuY = y;
    DrawMenu(x, y, pStrings_P, ticked, disabled); // full paint
    int touchX, touchY;
    while (Utility::GetStableTouch(touchX, touchY)) // held
    {
      int thisItem = MenuHitTest(x, y, touchX, touchY, disabled | labels);
      if (thisItem == -1 && (touchX < _menuX || touchX >(_menuX + _menuWidth) || touchY < _menuY || touchY >(_menuY + _menuHeight)))
      {
        prevItem = -2;
        break;
      }
      if (thisItem != prevItem)
      {
        if (prevItem != -1)
          UpdateMenuItem(x, y, prevItem, pStrings_P, ticked, disabled, false);
        if (thisItem != -1)
          UpdateMenuItem(x, y, thisItem, pStrings_P, ticked, disabled, true);
      }
      prevItem = thisItem;
    }
    // released. clear, return last item
    if (!labels)
      Erase();
    return prevItem;
  }

  void Erase()
  {
    if (_menuWidth)
      Ball::ClearBackground(_menuX, _menuY, _menuWidth + 1, _menuHeight + kMenuLineGap);
    _menuWidth = -1;
  }

}
