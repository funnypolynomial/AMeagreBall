#pragma once

// Configuration

// see also BALL_LIMIT_* in Amiga_Small.h

// Undefine for no RTC attached (and therefore just the ball)
#define CFG_HAVE_RTC

// Time display
#define CFG_DISPLAY_12_HOUR
#define CFG_DISPLAY_AM_PM
// If defined, double the size of the text in the Digital clock 
//#define CFG_DOUBLE_DIGITAL

// If defined, percentage of bounces off floor that have a small skid/kick
#define CFG_RANDOMIZE_BALL_PERCENT  25

// Colours
#define CFG_WORKBENCH_BACKGND       RGB(0, 85, 170)     // Blue
#define CFG_WORKBENCH_FOREGND       RGB(255, 255, 255)  // White
#define CFG_WORKBENCH_FOREGND_BYTE  255                 // Fast white
// Menu items/selected items/disabled items/live items
#define CFG_MENU_ITEM_FOREGND       RGB(0, 0, 0)
#define CFG_MENU_ITEM_BACKGND       RGB(200, 200, 200)
#define CFG_MENU_SEL_FOREGND        CFG_WORKBENCH_FOREGND
#define CFG_MENU_SEL_BACKGND        CFG_WORKBENCH_BACKGND
#define CFG_MENU_GHOST_FOREGND      RGB(225, 225, 225)
#define CFG_MENU_GHOST_BACKGND      CFG_MENU_ITEM_BACKGND
#define CFG_MENU_LIVE_FOREGND       RGB(0, 0, 0)
#define CFG_MENU_LIVE_BACKGND       RGB(255, 255, 255)
#define CFG_MENU_FRAME              RGB(0, 0, 0)
// Grid
#define CFG_GRID_BACKGND            RGB(160, 160, 160)
#define CFG_GRID_SHADOW_BACKGND     RGB(90, 90, 90)
#define CFG_GRID_LINE               RGB(150, 40, 150)
#define CFG_GRID_SHADOW_LINE        RGB(80, 10, 80)

#define CFG_ICON_RED                RGB(255, 136, 0) // seems pale but accurate?

#define CFG_MENU_TICK               FONT_TICK_CHAR

// If defined, screen shudders left/right, by the number of pixels, when the ball bounces off the sides
// A poor substitute for sound
//#define CFG_SHUDDER 2

namespace Config
{
  enum ClockStyle { NoClock, AnalogClock, DigitalClock, WorkbenchClock };

  void Init();
  void Reset();
  void Load();
  void Save();
  bool Loop();

  extern ClockStyle _clockStyle;  // Type of clock displayed
  extern bool       _DLSOn;       // DLS on (+1hour)
  // clock window positions
  extern int        _DigitalX;
  extern int        _DigitalY;
  extern int        _AnalogX;
  extern int        _AnalogY;
  extern bool       _WorkbenchMimimized;
}
