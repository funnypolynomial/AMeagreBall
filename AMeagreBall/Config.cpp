#include <EEPROM.h>
#include "Boing.h"
#include "Ball.h"
#include "Font.h"
#include "Menu.h"
#include "RTC.h"
#include "Clock.h"
#include "Config.h"

namespace Config
{
  ClockStyle _clockStyle = NoClock;
  bool       _DLSOn = false;
  int        _DigitalX = 150;
  int        _DigitalY = 30;
  int        _AnalogX = 150;
  int        _AnalogY = 30;
  bool       _WorkbenchMimimized = false;
#define CHECK_CHAR 'A' // in EEPROM to show we've written
enum  // EEPROM indices
{
  IDX_CHECK, IDX_STYLE, IDX_DLS, 
  IDX_DX_LO, IDX_DX_HI, IDX_DY_LO, DX_DY_HI, // Digital clock x & y 2 bytes
  IDX_AX_LO, IDX_AX_HI, IDX_AY_LO, DX_AY_HI,  // Analog clock x & y 2 bytes
  IDX_WK_MIN // Workbench minimized
};
#define READ(_lo) (EEPROM.read(_lo) + (EEPROM.read(_lo + 1) << 8))
#define WRITE(_lo, _val) EEPROM.write(_lo, _val & 0xFF); EEPROM.write(_lo + 1, _val >> 8);

#define MENU_HOTSPOT_SIZE 54

  void Init()
  {
    // Load settings from EEPROM
    Load();
  }

  void Reset()
  {
    // rrestore settings to defaults
    _clockStyle = NoClock;
    _DLSOn = false;
    _DigitalX = 150;
    _DigitalY = 30;
    _AnalogX = 150;
    _AnalogY = 30;
    _WorkbenchMimimized = false;
  }

  void Load()
  {
    // Load settings from EEPROM
    if (EEPROM.read(IDX_CHECK) != CHECK_CHAR)
    {
      // not present, reset & save
      Reset();
      Save();
    }
    else
    {
      _clockStyle = (ClockStyle)EEPROM.read(IDX_STYLE);
      _DLSOn = EEPROM.read(IDX_DLS);
      _DigitalX = READ(IDX_DX_LO);
      _DigitalY = READ(IDX_DY_LO);
      _AnalogX = READ(IDX_AX_LO);
      _AnalogY = READ(IDX_AY_LO);
      _WorkbenchMimimized = EEPROM.read(IDX_WK_MIN);
    }
  }

  void Save()
  {
    // Write settings to EEPROM
    EEPROM.write(IDX_CHECK, CHECK_CHAR);
    EEPROM.write(IDX_STYLE, _clockStyle);
    EEPROM.write(IDX_DLS,   _DLSOn);
    WRITE(IDX_DX_LO, _DigitalX);
    WRITE(IDX_DY_LO, _DigitalY);
    WRITE(IDX_AX_LO, _AnalogX);
    WRITE(IDX_AY_LO, _AnalogY);
    EEPROM.write(IDX_WK_MIN, _WorkbenchMimimized?1:0);
  }

#ifdef CFG_HAVE_RTC
  static const char pHelp[] PROGMEM =
    //     1234567890123456789012345678901234567890
    MSTR("Ball only: \x01just bouncing ball.")
    MSTR("Analog clock: \x01window with hr/min hands.")
    MSTR("Digital clock: \x01window with HH:MM.")
    MSTR("Workbench clock: \x01workbench with HH:MM.")
    MSTR("Hide clock: \x01hide/show clock window.")
    MSTR("\x01Or touch in clock window or \x01\x81\x01 to \x01hide\x01.")
    MSTR("\x01Touch anywhere to \x01show.")
    MSTR("Set time: \x01set time with Set/Adj menu.")
    MSTR("Daylight saving: \x01toggle DLS.")
    MSTR("Randomize: \x01seed PRNG from time.")
    MSTR("Reset: \x01reset windows & ball.")
    MSTR("\x01Move clock window by \x01""dragging\x01 title bar.")
    MSTR("\x01Workbench clock has \x01two\x01 positions.")
    ;
#else
  static const char pHelp[] PROGMEM =
    //     1234567890123456789012345678901234567890
    MSTR("Randomize: \x01seed PRNG from millis().")
    MSTR("(No real time clock)")
    ;
#endif

  void ShowHelp()
  {
    // Show help text
    LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
    const char* pLine = pHelp;
    int y = 1;
    while (pgm_read_byte(pLine))
    {
      Font::DrawString(0, y, pLine, RGB(255, 255, 255), RGB(0, 0, 0), false, 0, true);
      pLine += strlen_P(pLine) + 1;
      y += FONT_CHAR_HEIGHT + 1;
    }
    int touchX, touchY;
    while (!Utility::GetStableTouch(touchX, touchY))
      ;
    Ball::RepaintBackground();
  }

#define MENU_BIT(_menu) (1 << _menu)
  enum TimeItems { Hour, Min, Next, Plus, Minus };
  static const char pSetTimeMenu[] PROGMEM = MSTR("Hour:") MSTR("Min :") MSTR("Set/Next") MSTR("Adjust+") MSTR("Adjust-");

  // live menu items
  char buffHH[8];
  char buffMM[8];
  char* LiveItem(int item)
  {
    if (item == Hour)
      return buffHH;
    else if (item == Min)
      return buffMM;
    else
      return NULL;
  }

  void SetTime()
  {
    // drive a "menu" to set the time
    rtc.ReadTime(true);
    if (Config::_DLSOn)
    {
      rtc.m_Hour24++;
      if (rtc.m_Hour24 == 24)
        rtc.m_Hour24 = 0;
    }
    uint8_t hh = rtc.m_Hour24;
    uint8_t mm = rtc.m_Minute;
    int item = Hour;
    uint8_t* pField = &hh;
    uint16_t labels = MENU_BIT(Hour) | MENU_BIT(Min);
    uint16_t ticked = MENU_BIT(Hour);
    bool update = true;
    bool save = false;
    while (true)
    {
      if (update)
      {
        strcpy_P(buffHH, Utility::GetMString(pSetTimeMenu, Hour));
        strcpy_P(buffMM, Utility::GetMString(pSetTimeMenu, Min));
        Utility::Format(hh, buffHH + strlen(buffHH), '0', true);
        Utility::Format(mm, buffMM + strlen(buffMM), '0', true);
        update = false;
      }
      int sel = Menu::Select(1, 1, pSetTimeMenu, ticked, 0u, labels, LiveItem);
      if (sel == Plus)
      {
        // inc current field
        (*pField)++;
        if ((item == Hour && hh > 23) || (item == Min && mm > 59))
          *pField = 0;
        update = true;
      }
      else if (sel == Minus)
      {
        // dec current field
        (*pField)--;
        if ((*pField) == 255)
        {
          if (item == Hour)
            *pField = 23;
          else
            *pField = 59;
        }
        update = true;
      }
      else if (sel == Next)
      {
        // next field
        if (item == Hour)
        {
          item = Min;
          pField = &mm;
          ticked = MENU_BIT(Min);
        }
        else
        {
          save = true;
          break; // done
        }
      }
      else if (sel == -2)
        break;
    }

    if (save)
    {
      if (Config::_DLSOn)
      {
        hh--;
        if (hh == 255)
          hh = 23;
      }
      rtc.m_Hour24 = hh;
      rtc.m_Minute = mm;
      rtc.m_Second = 0;
      rtc.WriteTime();
    }
    Menu::Erase();
  }

  void CheckPause()
  {
#ifdef DEBUG
    // check touch top-right to pause, anywhere else to resume (for tear-free photos)
    // a bit of a hack, so pause doesn't work with hidden clock, shows instead
    int touchX, touchY;
    if (Utility::GetStableTouch(touchX, touchY) && touchX > (LCD_WIDTH - MENU_HOTSPOT_SIZE) && touchY < MENU_HOTSPOT_SIZE)
    {
      while (Utility::GetStableTouch(touchX, touchY))
        ;
      while (!Utility::GetStableTouch(touchX, touchY))
        ;
    }
#endif    
  }
  
#ifdef CFG_HAVE_RTC
  enum MenuItems { Ball, Analog, Digital, Workbench, Hide, Set, DLS, Randomize, ResetAll, Help };
  static const char pMenuItems[] PROGMEM = MSTR("Ball only") MSTR("Analog clock") MSTR("Digital clock") MSTR("Workbench clock") MSTR("Hide clock") MSTR("Set Time...") MSTR("Daylight saving") MSTR("Randomize")  MSTR("Reset") MSTR("Help");
  bool Loop()
  {
    CheckPause();
    // check for menu selection
    int touchX, touchY;
    if (Utility::GetStableTouch(touchX, touchY) && touchX < MENU_HOTSPOT_SIZE && touchY < MENU_HOTSPOT_SIZE)
    {
      uint16_t ticked = 0;
      uint16_t disabled = 0;
      if (Config::_DLSOn)
        ticked |= MENU_BIT(DLS);
      if (Config::_clockStyle != Config::NoClock)
      {
        if (Clock::_hidden)
          ticked |= MENU_BIT(Hide);
      }
      else
        disabled |= MENU_BIT(Hide);
#ifndef CFG_RANDOMIZE_BALL_PERCENT
      disabled |= MENU_BIT(Randomize);
#endif
      switch (Config::_clockStyle)
      {
        case Config::NoClock:
          ticked |= MENU_BIT(Ball);
          break;
        case Config::AnalogClock:
          ticked |= MENU_BIT(Analog);
          break;
        case Config::DigitalClock:
          ticked |= MENU_BIT(Digital);
          break;
        case Config::WorkbenchClock:
          ticked |= MENU_BIT(Workbench);
          break;
      }
      Ball::SetDim(); // dim the ball while frozen
      Ball::Redraw();
      int sel = Menu::Select(1, 1, pMenuItems, ticked, disabled);
      bool save = false, drawClock = true;
      Config::ClockStyle oldStyle = Config::_clockStyle;
      switch (sel)
      {
        case Ball:
          Config::_clockStyle = Config::NoClock;
          break;
        case Analog:
          Config::_clockStyle = Config::AnalogClock;
          break;
        case Digital:
          Config::_clockStyle = Config::DigitalClock;
          break;
        case Workbench:
          Config::_clockStyle = Config::WorkbenchClock;
          break;
        case Hide:
          Clock::Hide(!Clock::_hidden);
          drawClock = false;
          break;
        case Set:
          Ball::Redraw();
          Clock::Draw();
          SetTime();
          break;
        case DLS:
          Config::_DLSOn = !Config::_DLSOn;
          Clock::Update();
          save = true;
          break;
        case Randomize:
          randomSeed(rtc.Entropy());
          break;
        case ResetAll:
          Reset();
          Ball::Start();
          Save();
          break;
        case Help:
          ShowHelp();
          break;
      }
      Ball::RestoreColours();
      Ball::Redraw();
      if (oldStyle != Config::_clockStyle)
      {
        Clock::SetStyle(Config::_clockStyle);
        Clock::Hide(false);
        save = true;
      }
      else if (drawClock)
        Clock::Draw();
      if (save)
        Save();
      while (Utility::GetStableTouch(touchX, touchY)) // wait for up
        ;
      return true;
    }
    return false;
  }
#else
  enum MenuItems { Randomize, Help };
  static const char pMenuItems[] PROGMEM = MSTR("Randomize")  MSTR("Help");
  bool Loop()
  {
    CheckPause();
    // check for menu selection
    int touchX, touchY;
    if (Utility::GetStableTouch(touchX, touchY) && touchX < MENU_HOTSPOT_SIZE && touchY < MENU_HOTSPOT_SIZE)
    {
      uint16_t ticked = 0;
      uint16_t disabled = 0;
      Ball::SetDim(); // dim the ball while frozen
      Ball::Redraw();
      int sel = Menu::Select(10, 10, pMenuItems, ticked, disabled);
      switch (sel)
      {
      case Randomize:
        randomSeed(rtc.Entropy());
        break;
      case Help:
        ShowHelp();
        break;
      }
      Ball::RestoreColours();
      Ball::Redraw();
      while (Utility::GetStableTouch(touchX, touchY)) // wait for up
        ;
      return true;
    }
    return false;
  }
#endif

}
