#include "Boing.h"
#include "Font.h"
#include "Ball.h"
#include "Workbench.h"
#include "RTC.h"
#include "Clock.h"

namespace Clock
{
  #define ANALOG_CLOCK_RADIUS 50
  bool _hidden = false; // is the clock temporarily hidden
#ifndef CFG_HAVE_RTC
  void Init() {}
  void Loop() {}
  void SetStyle(Config::ClockStyle ) {}
  void Draw() {}
  void Update() {}
  void Hide(bool ) {}
#else
  // current clock window:
  int _clockX;
  int _clockY;
  int _clockW;
  int _clockH;
  const int kAnalogClockSize = 131;
  const int kAnalogClockRadius = ANALOG_CLOCK_RADIUS;
  const int kAnalogClockThickness = 2;
#ifdef CFG_DOUBLE_DIGITAL
  const int kDigitalClockWidth = 155;
  const int kDigitalClockHeight = 65;
#else
  const int kDigitalClockWidth = 131;
  const int kDigitalClockHeight = 50;
#endif
  const int kWorkbenchClockW = LCD_WIDTH;
  const int kWorkbenchClockHMax = 55;
  const int kWorkbenchClockHMin = Workbench::kFrameTopThickness;
  const int kWorkbenchClockX = 0;
  const int kWorkbenchClockYMax = LCD_HEIGHT - kWorkbenchClockHMax;
  const int kWorkbenchClockYMin = LCD_HEIGHT - Workbench::kFrameTopThickness;
  int _ClientX, _ClientY, _ClientW, _ClientH;
  byte _CurrentMinute = 0xFF;
  Config::ClockStyle _clockStyle = Config::NoClock;
  static const char pClock[] PROGMEM = "Clock";
  static const char pDemos[] PROGMEM = "Demos";

  void ReadTime()
  {
    // read the time and adjust for DLS
    rtc.ReadTime();
    if (Config::_DLSOn)
    {
      rtc.m_Hour24++;
      if (rtc.m_Hour24 == 24)
        rtc.m_Hour24 = 0;
    }
  }

  char* FormatTime(char* pBuff)
  {
    // build the time string into pBuff
    ReadTime();
    char suffix;
    // Format the hour "HH", no NUL
    suffix = 0;
    uint8_t h = rtc.m_Hour24;
#ifdef CFG_DISPLAY_12_HOUR
#ifdef CFG_DISPLAY_AM_PM
    suffix = 'A';
#endif
    if (h >= 12)
    {
#ifdef CFG_DISPLAY_AM_PM
      suffix = 'P';
#endif
      if (h > 12)
        h -= 12;
    }
    if (h == 0)
      h = 12;
#endif    
#ifdef CFG_DISPLAY_12_HOUR
    Utility::Format(h, pBuff, ' ');
#else  
    Utility::Format(h, pBuff, '0');
#endif      

    pBuff[2] = ':';
    Utility::Format(rtc.m_Minute, pBuff + 3, '0');
    if (suffix)
    {
      pBuff[5] = suffix;
      pBuff[6] = 'M';
      pBuff[7] = 0;
    }
    else
      pBuff[5] = 0;
    return pBuff;
  }

#if KITE_MAX_ROWS < ANALOG_CLOCK_RADIUS 
#error  KITE_MAX_ROWS cannot be less than ANALOG_CLOCK_RADIUS
#endif

#define BLACK 0x00
#define WHITE 0xFF

  struct tHand
  {
    // "kite" corners
    int _x0, _y0; // tip
    int _x1, _y1, _x2, _y2; // shoulders
    int _x3, _y3; // base
    Utility::tKite _kite;
  };

  void DrawHourTick(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3)
  {
    // Make & fill a kite
    //     0
    //   .   .
    //  1     2
    //   .   .
    //     3
    Utility::tKite kite;
    Utility::MakeKite(x0, y0, x1, y1, x2, y2, x3, y3, kite);
    Utility::FillKite(kite, BLACK);
  }

  tHand _minuteHand, _hourHand;

  void DrawAnalogFace()
  {
    // The fixed part of the clock face -- hour ticks
    // The window is small so the minute ticks (shor lines) are omitted
    int midX = _ClientX + _ClientW / 2;
    int midY = _ClientY + _ClientH / 2;
    Utility::FillCircle(midX, midY, kAnalogClockRadius, kAnalogClockThickness);
    // draw hour marks
    for (int angle = 0; angle <= 90; angle += 30)
    {
      int32_t markR0 = kAnalogClockRadius - 3;
      int32_t markR1 = markR0 - 7;
      // angle is clockwise for 0 degrees (12 O'Clock)
      int markMidR = (markR0 + markR1)/2;
      int dAngle = 4;
      int x0 = Utility::rsint(markR0, angle);
      int y0 = Utility::rcost(markR0, angle);
      int x1 = Utility::rsint(markMidR, angle + dAngle);
      int y1 = Utility::rcost(markMidR, angle + dAngle);
      int x2 = Utility::rsint(markMidR, angle - dAngle);
      int y2 = Utility::rcost(markMidR, angle - dAngle);
      int x3 = Utility::rsint(markR1, angle);
      int y3 = Utility::rcost(markR1, angle);
      DrawHourTick(midX + x0, midY - y0, midX + x1, midY - y1, midX + x2, midY - y2, midX + x3, midY - y3);
      DrawHourTick(midX - x0, midY - y0, midX - x1, midY - y1, midX - x2, midY - y2, midX - x3, midY - y3);
      if (angle != 90)
      {
        DrawHourTick(midX + x0, midY + y0, midX + x1, midY + y1, midX + x2, midY + y2, midX + x3, midY + y3);
        DrawHourTick(midX - x0, midY + y0, midX - x1, midY + y1, midX - x2, midY + y2, midX - x3, midY + y3);
      }
    }

    _minuteHand._x3 = _hourHand._x3 = midX;
    _minuteHand._y3 = _hourHand._y3 = midY;
  }

  void DrawAnalogWindow()
  {
    // Draw the analog clock windo wna fixed graphics
    Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::App,
      &_ClientX, &_ClientY, &_ClientW, &_ClientH);
    Workbench::DrawWindowTitle(_clockX, _clockY, pClock, Workbench::App, true);
    Ball::SetClip(_clockX, _clockY, _clockW, _clockH);
    DrawAnalogFace();
  }

  void UpdateHands(int minuteAngle, int hourAngle)
  {
    // draw the hands at the angles, in the colour, erasing previous hands if required
    int midX = _ClientX + _ClientW / 2;
    int midY = _ClientY + _ClientH / 2;
    bool eraseMinute = true;
    bool drawHour = true, eraseHour = true;
    const int32_t kMinuteHandRadius = kAnalogClockRadius - 12;
    const int32_t kHourHandRadius = 2 * kMinuteHandRadius / 3;
    const int kMinuteHandHalfAngle = 5;
    const int kHourHandHalfAngle = 10;
    const int kMinuteHandShoulderRadius = 3 * kMinuteHandRadius / 4;
    const int kHourHandShoulderRadius = 3 * kHourHandRadius / 4;

    int hourTipX = midX + Utility::rsint(kHourHandRadius, hourAngle);
    int hourTipY = midY - Utility::rcost(kHourHandRadius, hourAngle);
    if (_CurrentMinute == 0xFF)
    {
      // first time
      drawHour = true;
      eraseHour = eraseMinute = false;
    }
    else
    {
      eraseHour = drawHour = (hourTipX != _hourHand._x0) || (hourTipY != _hourHand._y0); // moved enough , is this ever false?
      if (!eraseHour)
      {
        // not updating hour, but do hands overlap?
        int dAngle = abs(minuteAngle - hourAngle);
        if (dAngle >= 180) 
          dAngle = 360 - dAngle;
        if (dAngle <= (kHourHandHalfAngle + kMinuteHandHalfAngle + 3))
          eraseHour = drawHour = true;
      }
    }
    // minute
    if (eraseMinute)
      Utility::FillKite(_minuteHand._kite, WHITE);
    if (eraseHour)
      Utility::FillKite(_hourHand._kite, WHITE);
    _minuteHand._x0 = midX + Utility::rsint(kMinuteHandRadius, minuteAngle);
    _minuteHand._y0 = midY - Utility::rcost(kMinuteHandRadius, minuteAngle);
    _minuteHand._x1 = midX + Utility::rsint(kMinuteHandShoulderRadius, minuteAngle + kMinuteHandHalfAngle);
    _minuteHand._y1 = midY - Utility::rcost(kMinuteHandShoulderRadius, minuteAngle + kMinuteHandHalfAngle);
    _minuteHand._x2 = midX + Utility::rsint(kMinuteHandShoulderRadius, minuteAngle - kMinuteHandHalfAngle);
    _minuteHand._y2 = midY - Utility::rcost(kMinuteHandShoulderRadius, minuteAngle - kMinuteHandHalfAngle);
    Utility::MakeKite(_minuteHand._x0, _minuteHand._y0, _minuteHand._x1, _minuteHand._y1, _minuteHand._x2, _minuteHand._y2, _minuteHand._x3, _minuteHand._y3, _minuteHand._kite);
    Utility::FillKite(_minuteHand._kite, BLACK);

    if (drawHour)
    {
      _hourHand._x0 = hourTipX;
      _hourHand._y0 = hourTipY;
      _hourHand._x1 = midX + Utility::rsint(kHourHandShoulderRadius, hourAngle + kHourHandHalfAngle);
      _hourHand._y1 = midY - Utility::rcost(kHourHandShoulderRadius, hourAngle + kHourHandHalfAngle);
      _hourHand._x2 = midX + Utility::rsint(kHourHandShoulderRadius, hourAngle - kHourHandHalfAngle);
      _hourHand._y2 = midY - Utility::rcost(kHourHandShoulderRadius, hourAngle - kHourHandHalfAngle);
      Utility::MakeKite(_hourHand._x0, _hourHand._y0, _hourHand._x1, _hourHand._y1, _hourHand._x2, _hourHand._y2, _hourHand._x3, _hourHand._y3, _hourHand._kite);
      Utility::FillKite(_hourHand._kite, BLACK);
    }
  }

  void UpdateAnalogWindow()
  {
    // draw the hands at the current time
    ReadTime();
    int minuteAngle = rtc.m_Minute * 6;
    int hour = rtc.m_Hour24;
    if (hour > 12)
      hour -= 12;
    int hourAngle = hour * 30 + (rtc.m_Minute / 2);
    UpdateHands(minuteAngle, hourAngle);
  }


  void DrawDigitalWindow()
  {
    // Draw digital clock window
    Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::App,
      &_ClientX, &_ClientY, &_ClientW, &_ClientH);
    Workbench::DrawWindowTitle(_clockX, _clockY, pClock, Workbench::App, true);
    Ball::SetClip(_clockX, _clockY, _clockW, _clockH);
  }

  void UpdateDigitalWindow()
  {
    // Update time on digital clock window
#ifdef CFG_DOUBLE_DIGITAL
    Font::_scaleFactor = 2;
#endif
    char pBuff[10];
    const char* pTime = FormatTime(pBuff);
    Font::DrawString(_ClientX + (_ClientW - Font::_scaleFactor*FONT_CHAR_WIDTH * (int)strlen(pTime)) / 2,
      _ClientY + (_ClientH - Font::_scaleFactor * FONT_CHAR_BASELINE) / 2,
      pTime,
      CFG_WORKBENCH_FOREGND, CFG_WORKBENCH_BACKGND);
    Font::_scaleFactor = 1;
  }

  void DrawWorkbenchWindow()
  {
    // Draw workbench clock window
    if (Config::_WorkbenchMimimized)
    {
      Workbench::DrawWindowFrame(kWorkbenchClockX, kWorkbenchClockYMin, kWorkbenchClockW, kWorkbenchClockHMin, Workbench::Workbench);
      Ball::SetClip(kWorkbenchClockYMin);
    }
    else
    {
      Workbench::DrawWindowFrame(kWorkbenchClockX, kWorkbenchClockYMax, kWorkbenchClockW, kWorkbenchClockHMax, Workbench::Workbench);
      int offset = 30;
      Workbench::DrawWindowFrame(8, LCD_HEIGHT - offset, 180, offset, Workbench::DitheredFolder);
      // folder & icon inside Workbench
      Workbench::DrawWindowTitle(8, LCD_HEIGHT - offset, pDemos, Workbench::DitheredFolder, true);
      Workbench::DrawBallIcon(LCD_WIDTH - 50, LCD_HEIGHT - offset);
      Ball::SetClip(kWorkbenchClockYMax);
    }
  }

  void UpdateWorkbenchWindow()
  {
    // Update time on workbench clock window
    char pBuff[10];
    char buffer[20];
    strcpy_P(buffer, pClock);
    strcat(buffer, " ");
    strcat(buffer, FormatTime(pBuff));
    Workbench::DrawWindowTitle(kWorkbenchClockX, Config::_WorkbenchMimimized?kWorkbenchClockYMin: kWorkbenchClockYMax, buffer, Workbench::Workbench);
  }

  void Erase()
  {
    // erase the current clock
    if (_clockStyle != Config::NoClock)
    {
      Ball::ClearClip();
      Ball::ClearBackground(_clockX, _clockY, _clockW, _clockH);
    }
  }

  void Erase(int dX, int dY)
  {
    // partial erasure of the current clock, just what's exposed moving (dX, dY)
    if (_clockStyle != Config::NoClock)
    {
      Ball::ClearClip();
      if (dX > 0)
        Ball::ClearBackground(_clockX, _clockY, dX, _clockH);
      else if (dX < 0)
        Ball::ClearBackground(_clockX + _clockW + dX, _clockY, -dX, _clockH);

      if (dY > 0)
      {
        if (dX >= 0)
          Ball::ClearBackground(_clockX + dX, _clockY, _clockW - dX, dY);
        else
          Ball::ClearBackground(_clockX, _clockY, _clockW + dX, dY);
      }
      else if (dY < 0)
      {
        if (dX >= 0)
          Ball::ClearBackground(_clockX + dX, _clockY + _clockH + dY, _clockW - dX, -dY);
        else
          Ball::ClearBackground(_clockX, _clockY + _clockH + dY, _clockW + dX, -dY);
      }
    }
  }

  const int kMoveTolerance = 5;
#ifdef CFG_SHUDDER
  const int kSideMargin = CFG_SHUDDER;
#else
  const int kSideMargin = 0;
#endif
  void MoveApp(int startX, int startY)
  {
    // Drag digital/analog clock window
    int touchX, touchY;
    int oldX = _clockX, oldY = _clockY;
    Ball::SetDim(); // dim the ball while moving
    Ball::Redraw();
    Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::AppFast);
    while (Utility::GetStableTouch(touchX, touchY))
    {
      int dX = touchX - startX;
      int dY = touchY - startY;
      if (abs(dX) > kMoveTolerance || abs(dY) > kMoveTolerance)
      {
        int newX = oldX + dX;
        if (newX < kSideMargin)
          newX = kSideMargin;
        if (newX > (LCD_WIDTH - _clockW - kSideMargin))
          newX = (LCD_WIDTH - _clockW - kSideMargin);
        int newY = oldY + dY;
        if (newY < 0)
          newY = 0;
        if (newY > (LCD_HEIGHT - _clockH))
          newY = (LCD_HEIGHT - _clockH);
        if (newX != _clockX || newY != _clockY)
        {
          Erase(dX, dY);
          oldX = _clockX = newX;
          oldY = _clockY = newY;
          startX = touchX;
          startY = touchY;
          Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::AppFast);
          Ball::SetClip(_clockX, _clockY, _clockW, _clockH);
          Ball::Redraw();
        }
      }
    }
    Draw();
    Ball::RestoreColours();
    Ball::Redraw();
    // update config
    if (_clockStyle == Config::AnalogClock)
    {
      Config::_AnalogX = _clockX;
      Config::_AnalogY = _clockY;
    }
    else
    {
      Config::_DigitalX = _clockX;
      Config::_DigitalY = _clockY;
    }
    Config::Save();
  }

  void MoveWorkbench(int , int startY)
  {
    // Drag the workbench window up/down (two sizes)
    int touchX, touchY;
    int oldY = _clockY;
    Ball::SetDim(); // dim the ball while moving
    Ball::Redraw();
    bool Minimised = Config::_WorkbenchMimimized;
    Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::WorkbenchFast);
    while (Utility::GetStableTouch(touchX, touchY))
    {
      int dY = touchY - startY;
      if (abs(dY) > kMoveTolerance)
      {
        int newY = oldY + dY;
        if (newY < 0)
          newY = 0;
        if (newY > LCD_HEIGHT)
          newY = LCD_HEIGHT;
        if (newY != _clockY)
        {
          oldY = newY;
          startY = touchY;
          Minimised = (newY >= (kWorkbenchClockYMax + kWorkbenchClockYMin) / 2);
          int newY = Minimised ? kWorkbenchClockYMin : kWorkbenchClockYMax;
          dY = newY - _clockY;
          if (dY > 0)
            Erase(0, dY);
          _clockY = newY;
          _clockH = Minimised ? kWorkbenchClockHMin : kWorkbenchClockHMax;
          Workbench::DrawWindowFrame(_clockX, _clockY, _clockW, _clockH, Workbench::WorkbenchFast);
          Ball::SetClip(_clockY);
          Ball::Redraw();
        }
      }
    }
    // update config
    Config::_WorkbenchMimimized = Minimised;
    Config::Save();
    Draw();
    Ball::RestoreColours();
    Ball::Redraw();
  }


  void Init()
  {
    rtc.Setup();
    SetStyle(Config::_clockStyle);
  }

  void Loop()
  {
    // update the clock
    if (_clockStyle != Config::NoClock)
    {
      if (_CurrentMinute != rtc.ReadMinute())
      {
        ReadTime();
        Update();
      }
      int touchX, touchY;
      if (Utility::GetStableTouch(touchX, touchY))
      {
        if (_hidden)
        {
          Hide(false);
          while (Utility::GetStableTouch(touchX, touchY)) // wait for up
            ;
        }
        else
        {
          Workbench::WindowHit hit = Workbench::HitTest(touchX, touchY, _clockX, _clockY, _clockW, _clockH, Workbench::App);
          if (hit == Workbench::BodyHit || hit == Workbench::SendBehindHit)
          {
            Hide(true);
            Ball::Redraw();
            while (Utility::GetStableTouch(touchX, touchY)) // wait for up
              ;
          }
          else if (hit == Workbench::TitleHit)
          {
            if (_clockStyle == Config::WorkbenchClock)
              MoveWorkbench(touchX, touchY);
            else
              MoveApp(touchX, touchY);
          }
        }
      }
    }
  }

  void SetStyle(Config::ClockStyle style)
  {
    // set the clock style
    // clears old, draws new
    if (_clockStyle != style)
    {
      if (_clockStyle != Config::NoClock)
      {
        Erase();
      }
      _clockStyle = style;
      switch (_clockStyle)
      {
      case Config::AnalogClock:
        _clockX = Config::_AnalogX;
        _clockY = Config::_AnalogY;
        _clockW = _clockH = kAnalogClockSize;
        break;
      case Config::DigitalClock:
        _clockX = Config::_DigitalX;
        _clockY = Config::_DigitalY;
        _clockW = kDigitalClockWidth;
        _clockH = kDigitalClockHeight;
        break;
      case Config::WorkbenchClock:
        _clockX = kWorkbenchClockX;
        _clockY = Config::_WorkbenchMimimized?kWorkbenchClockYMin: kWorkbenchClockYMax;
        _clockW = kWorkbenchClockW;
        _clockH = Config::_WorkbenchMimimized ? kWorkbenchClockHMin: kWorkbenchClockHMax;
        break;
      default:
        return;
      }

      if (_clockStyle != Config::NoClock)
        Draw();
    }
  }

  void Draw()
  {
    // draw the current clock's window
    if (_hidden) return;
    switch (_clockStyle)
    {
    case Config::AnalogClock:
      DrawAnalogWindow();
      break;
    case Config::DigitalClock:
      DrawDigitalWindow();
      break;
    case Config::WorkbenchClock:
      DrawWorkbenchWindow();
      break;
    default:
      return;
    }
    _CurrentMinute = 0xFF;
  }

  void Update()
  {
    // draw the current clock time display
    if (_hidden) return;
    switch (_clockStyle)
    {
    case Config::AnalogClock:
      UpdateAnalogWindow();
      break;
    case Config::DigitalClock:
      UpdateDigitalWindow();
      break;
    case Config::WorkbenchClock:
      UpdateWorkbenchWindow();
      break;
    default:
      return;
    }
    _CurrentMinute = rtc.m_Minute;
  }

  void Hide(bool hide)
  {
    // hide/show current clock
    if (_hidden != hide)
    {
      _hidden = hide;
      if (_hidden)
        Erase();
      else
      {
        Draw();
        Update();
      }
    }
  }
#endif
}
