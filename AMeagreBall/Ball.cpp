#include "Boing.h"
#include "Font.h"
#include "Workbench.h"
#include "Ball.h"

// grid is 15x12 squares (plus floor), centered, one square from top
#define GRID_CELL_SIZE  16
#define GRID_COLS       15
#define GRID_ROWS       12

#include "BallData.h"
#include "GridData.h"
namespace Ball
{
  const COLOUR kNAColour = RGB(1, 2, 3); // place-holder colour
  const COLOUR kBallRed = RGB(255, 0, 0);
  const COLOUR kBallWhite = RGB(255, 255, 255);
  // active colours
  COLOUR _ballRed = kBallRed;
  COLOUR _ballWhite = kBallWhite;
  int _shadowWidth = BALL_SHADOW_WIDTH;

  // the ball
  int _prevX = 0, _prevY = 0;
  int _stepX = 1, _stepY = +1; // speed
  int _ballX = (LCD_WIDTH - BALL_WIDTH) / 2, _ballY = BALL_LIMIT_TOP;
  byte _ballStep = 0; // controls rotation

  // clipping
  int _clipMaxRow = 0; // clip below this row, inclusive
  int _clipX0 = 0, _clipX1 = 0, _clipY0 = 0, _clipY1 = 0; // clipping rect, inclusive
  int _clipSendCtr, _clipSkipCtr; // track status of clipping
  bool _clippingRow; // row falls on clipping rect
  bool _clipped;  // there has been clipping on the row
  int _cursorY; // where we are sending pixels to the LCD

  void SetClip(int x, int y, int w, int h)
  {
    // set cipping rectangle
    _clipX0 = x;
    _clipX1 = x + w;
    _clipY0 = y;
    _clipY1 = y + h - 1;
    _clipMaxRow = 0;
  }

  void SetClip(int row)
  {
    // set cipping top row
    _clipMaxRow = row;
    _clipY1 = 0;
  }

  void ClearClip()
  {
    // clear clipping rectangle/row
    _clipMaxRow = _clipY1 = 0;
  }

#ifdef CFG_HAVE_RTC // only clip if there are clock windows
  void StartRow(int x, int y)
  {
    // start filling a row with colour, prepare the clipping counters
    _cursorY = y;
    _clipped = false;
    _clipSendCtr = _clipSkipCtr = 0;
    if (!_clipMaxRow && _clipY1 && _clipY0 <= y && y <= _clipY1)
    {
      if (x <= _clipX0) // there are pixels before the clipbox
        _clipSendCtr = _clipX0 - x;
      if (x < _clipX1) // there are pixels inside the clipbox
      {
        if (_clipX0 < x)
          _clipSkipCtr = _clipX1 - x; // starts inside
        else
          _clipSkipCtr = _clipX1 - _clipX0;  // starts outside, clipbox width
        _clipped = !_clipSendCtr; // may need to move the window
      }
    }
    _clippingRow = _clipSendCtr || _clipSkipCtr;
    LCD_BEGIN_FILL(x, y, LCD_WIDTH - x, 1);
  }

  void SendColour(int n, COLOUR colour)
  {
    // send n pixels of colour, clipped
    if (_clipMaxRow && _cursorY >= _clipMaxRow)
      return;
    if (_clippingRow)
    {
      while (n)
      {
        if (_clipSendCtr) // drawing up to the LHS
        {
          int sendPixels = min(n, _clipSendCtr);
          LCD_FILL_COLOUR(sendPixels, colour);
          _clipSendCtr -= sendPixels;
          n -= sendPixels;
        }
        else if (_clipSkipCtr) // between LHS & RHS
        {
          int skipPixels = min(n, _clipSkipCtr);
          _clipSkipCtr -= skipPixels;
          n -= skipPixels;
          _clipped = true; // may need to move the window to RHS
        }
        else // unclipped past RHS
        {
          if (_clipped) // move window
            LCD_BEGIN_FILL(_clipX1, _cursorY, LCD_WIDTH - _clipX1 - 1, 1);
          _clippingRow = _clipped = false;
          LCD_FILL_COLOUR(n, colour);
          break;
        }
      }
    }
    else
      LCD_FILL_COLOUR(n, colour);
  }
#else
#define StartRow(_x, _y)        LCD_BEGIN_FILL(_x, _y, LCD_WIDTH - _x, 1)
#define SendColour(_n, _colour) LCD_FILL_COLOUR(_n, _colour)
#endif

  void BackgroundRow(int xStart, int y, int len, bool shadow)
  {
    // send len background pixels for the given row y, starting at xStart
    COLOUR background = shadow ? CFG_GRID_SHADOW_BACKGND : CFG_GRID_BACKGND;
    COLOUR line = shadow ? CFG_GRID_SHADOW_LINE : CFG_GRID_LINE;
    if (y < GRID_CELL_SIZE || y > GRID_FLOOR_LAST_ROW)
    {
      // above/below grid
      SendColour(len, background);
      return;
    }

    if (GRID_FLOOR_FIRST_ROW <= y && y <= GRID_FLOOR_LAST_ROW)
    {
      // floor, a little more complicated but less data, and used less?
      const byte* pFloor = gridFloorRows + (y - GRID_FLOOR_FIRST_ROW) * GRID_FLOOR_ROW_BYTES;
      int col = 0;
      while (col < xStart) // find the starting byte
      {
        byte data = pgm_read_byte(pFloor) & 0x7F;
        if (col + data < xStart)
        {
          col += data;
          pFloor++;
        }
        else
          break;
      }
      bool first = true;
      // pFloor is the byte that starts at col
      while (len)
      {
        byte data = pgm_read_byte(pFloor++);
        int dataLen = data & 0x7F;
        int runLen;
        if (first)
          runLen = min(len, (col + dataLen) - xStart);
        else
          runLen = min(len, dataLen);
        SendColour(runLen, (data & 0x80) ? background : line);
        len -= runLen;
        first = false;
      }
      return;
    }

    // hz/vt grid lines LUT is very generous, probably faster than the more compact floor above, could use the same and claw back some program space
#if GRID_CELL_SIZE == 16 // faster?
    const byte* pLUT = (y & (GRID_CELL_SIZE - 1) ? gridVerticalsLUT : gridHorizontalsLUT) + xStart;
#else
    const byte* pLUT = (row % GRID_CELL_SIZE ? gridVerticalsLUT : gridHorizontalsLUT) + startCol;
#endif
    while (len)
    {
      byte lut = pgm_read_byte(pLUT);
      COLOUR colour = background;
      if (lut & LUT_GRIDLINE_FLAG)
      {
        lut &= ~LUT_GRIDLINE_FLAG;
        colour = line;
      }
      if (lut >= len)
      {
        SendColour(len, colour);
        break;
      }
      SendColour(lut, colour);
      pLUT += lut;
      len -= lut;
    }
  }

  void ClearBackground(int x, int y, int w, int h)
  {
    // clear the rectangle to background (grid) pixels
    for (int row = 0; row < h; row++)
    {
      StartRow(x, y + row);
      BackgroundRow(x, y + row, w, false);
    }
  }

  void RepaintBackground()
  {
    // repaint entire background grid
    for (int row = 0; row < LCD_HEIGHT; row++)
    {
      StartRow(0, row);
      BackgroundRow(0, row, LCD_WIDTH, false);
    }
  }

  void Draw(int x, int y, byte step)
  {
    // draw the ball at x, y with rotation given by step
    const uint8_t* pRows = ballRows;
    const uint8_t* pPixels = ballPixels;
    int dY = y - _prevY;

    const uint8_t* pOffsetRows = ballRows;
    int offsetRow = dY;

    if (dY > 0) // moved down, clear top "cap"
      for (int r = 0; r < dY; r++)
      {
        uint8_t bkg = pgm_read_byte(pOffsetRows++);
        uint8_t len = pgm_read_byte(pOffsetRows++);
        StartRow(_prevX + bkg, _prevY + r);
        BackgroundRow(_prevX + bkg, _prevY + r, len + _shadowWidth, false);
      }
    else
      pOffsetRows += 2 * dY;

    for (int row = 0; row < BALL_HEIGHT; row++)
    {
      uint8_t bkg = pgm_read_byte(pRows++); // background pixels
      uint8_t len = pgm_read_byte(pRows++); // ball pixels
      int ballPixels = len;
      int totalPixels = len + _shadowWidth;
      int lhs = x + bkg;

      int clearLHS = 0;
      int clearRHS = 0;
      if (0 <= offsetRow && offsetRow < BALL_HEIGHT)
      {
        // work out exposed sides
        int prevBkg = pgm_read_byte(pOffsetRows++);
        int prevLen = pgm_read_byte(pOffsetRows++);
        int clear = (x + bkg) - (_prevX + prevBkg);
        if (clear > 0)
          clearLHS = clear;
        int prevRHS = _prevX + prevBkg + prevLen + _shadowWidth;
        clear = prevRHS - (lhs + totalPixels);
        if (clear > 0)
          clearRHS = clear;
      }
      else
        pOffsetRows += 2;

      if (clearLHS) // clear pixels exposed on left
      {
        lhs -= clearLHS;
        StartRow(lhs, y + row);
        BackgroundRow(lhs, y + row, clearLHS, false);
        totalPixels += clearLHS;
      }
      else
        StartRow(lhs, y + row);

      // ball pixels
      COLOUR prevColour = kNAColour;
      int totalRun = 0;
      while (len)
      {
        // each byte encodes a run of the same palette value (0..15)
        // accumulate those into longer runs of the same actual colour (red/white)
        uint8_t encoded = pgm_read_byte(pPixels++);
        int run = (encoded >> 4) + 1;
        COLOUR colour = ((encoded + step) & 0x08) ? _ballRed : _ballWhite;
        if (colour == prevColour || prevColour == kNAColour)
          totalRun += run; // extend current run of pixels
        else
        {
          SendColour(totalRun, prevColour); // paint current run of pixels
          totalRun = run; // start a new run
        }
        prevColour = colour;
        len -= run;
      }
      if (totalRun)
        SendColour(totalRun, prevColour); // trailing run of ball pixels

      // shadow
      if (ballPixels >= _shadowWidth)
        BackgroundRow(x + bkg + ballPixels, y + row, _shadowWidth, true);
      else
      {
        BackgroundRow(x + bkg + ballPixels, y + row, _shadowWidth - ballPixels, false); // unshaded part
        BackgroundRow(x + bkg + _shadowWidth, y + row, ballPixels, true);
      }

      if (clearRHS) // clear pixels exposed on right
      {
        BackgroundRow(lhs + totalPixels, y + row, clearRHS, false);
        totalPixels += clearRHS;
      }
      offsetRow++;
    }

    if (dY < 0)  // moved up, clear bottom "cap"
      for (int r = 0; r > dY; r--)
      {
        uint8_t bkg = pgm_read_byte(pOffsetRows++);
        uint8_t len = pgm_read_byte(pOffsetRows++);
        StartRow(_prevX + bkg, _prevY + offsetRow);
        BackgroundRow(_prevX + bkg, _prevY + offsetRow++, len + _shadowWidth, false);
      }
    _prevX = x;
    _prevY = y;
  }

  void Start()
  {
    // start the animation, ball is centred
    _stepX = 1;
    _stepY = +1;
    _ballX = (LCD_WIDTH - BALL_WIDTH) / 2;
    _ballY = BALL_LIMIT_TOP;
    _ballStep = 0;

    _prevX = _ballX;
    _prevY = _ballY;
  }

  int SpeedY(int Y)
  {
    // mimic acceleration
    const int rangeY = (BALL_LIMIT_BOTTOM - BALL_HEIGHT) - BALL_LIMIT_TOP;
    Y -= BALL_LIMIT_TOP;
    if (Y < rangeY / 8)
      return 1;
    else if (Y < rangeY / 4)
      return 2;
    else if (Y < rangeY / 2)
      return 4;
    return   5;
  }

  void Step()
  {
    // advance the animation
    Draw(_ballX, _ballY, _ballStep);
    _ballX += _stepX;
    _ballY += _stepY * SpeedY(_ballY);
    if (_stepY == 0)
      _stepY = +1;
    if (_ballX > (BALL_LIMIT_RIGHT - BALL_WIDTH)) // bounce off RHS
    {
      _ballX = 2 * (BALL_LIMIT_RIGHT - BALL_WIDTH) - _ballX;
      _stepX = -_stepX;
#ifdef CFG_SHUDDER      
      LCD_SCROLL(true, CFG_SHUDDER);
      delay(50);
      LCD_SCROLL(true, 0);
#endif
    }
    else if (_ballX < BALL_LIMIT_LEFT) // bounce off LHS
    {
      _ballX = 2 * BALL_LIMIT_LEFT - _ballX;
      _stepX = -_stepX;
#ifdef CFG_SHUDDER      
      LCD_SCROLL(false, CFG_SHUDDER);
      delay(50);
      LCD_SCROLL(false, 0);
#endif
    }

    if (_ballY < BALL_LIMIT_TOP) // drop from apex
    {
      _ballY = BALL_LIMIT_TOP;
      _stepY = 0; // dwell vs bounce
    }
    else if (_ballY > (BALL_LIMIT_BOTTOM - BALL_HEIGHT)) // bounce off floor
    {
      _ballY = 2 * (BALL_LIMIT_BOTTOM - BALL_HEIGHT) - _ballY;
      _stepY = -_stepY;
#ifdef CFG_RANDOMIZE_BALL_PERCENT
      // introduce some randomenss, skid/kick off floor
      if ((rand() % 100) < CFG_RANDOMIZE_BALL_PERCENT)
      {
        if (_stepX > 0)
          _stepX = (_stepX == +1) ? +2 : +1;
        else
          _stepX = (_stepX == -1) ? -2 : -1;
      }
#endif
    }
    _ballStep += (_stepX > 0) ? +1 : -1;
  }

  void SetDim()
  {
    // set ball colours to dim
    const byte kDim = 150;
    _ballRed = RGB(kDim, 0, 0);
    _ballWhite = RGB(kDim, kDim, kDim);
  }

  void RestoreColours()
  {
    // restore ball colours to red/white
    _ballRed = kBallRed;
    _ballWhite = kBallWhite;
  }

  void Redraw()
  {
    // redraw the current ball position/step
    Draw(_ballX, _ballY, _ballStep);
  }

  void Init()
  {
    // init the ball and draw the whole grid
    RepaintBackground();
    Start();
    ClearClip();
  }

  static const char pTitle[]  PROGMEM = "A Meagre Ball";
  static const char pCredit[] PROGMEM = "Mark Wilson MMXXV";
  static const char pHelp[]   PROGMEM = "Touch top-left corner for menu";

  void Splash()
  {
    // Spinning ball with colours fading in
    LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
    Start();
    _prevX = _ballX = (LCD_WIDTH - BALL_WIDTH) / 2;
    _prevY = _ballY = (LCD_HEIGHT - BALL_HEIGHT) / 2;
    _shadowWidth = 0;
    Workbench::DrawWindowFrame(0, 0, LCD_WIDTH, 0, Workbench::Workbench);
    Workbench::DrawWindowTitle(0, 0, pTitle, Workbench::Workbench, true);
    Font::DrawString((LCD_WIDTH - FONT_CHAR_WIDTH * (int)strlen_P(pCredit)) / 2, LCD_HEIGHT - FONT_CHAR_HEIGHT - 2, pCredit, RGB(255, 255, 255), RGB(0, 0, 0), false, 0, true);
    for (int step = 0; step < 255; step += 2)
    {
      _ballRed = RGB(step, 0, 0);
      _ballWhite = RGB(step, step, step);
      Draw(_ballX, _ballY, _ballStep++);
    }
    Font::DrawString((LCD_WIDTH - FONT_CHAR_WIDTH * (int)strlen_P(pHelp)) / 2, LCD_HEIGHT - FONT_CHAR_HEIGHT - 2, pHelp, RGB(255, 255, 255), RGB(0, 0, 0), false, 0, true);
    RestoreColours();
    for (int step = 0; step < 255; step += 2)
    {
      Draw(_ballX, _ballY, _ballStep--);
    }
    _shadowWidth = BALL_SHADOW_WIDTH;
  }

}
