#include "Boing.h"

namespace Utility
{
const char* GetMString(const char* pMultiStr, uint8_t n)
{
  // return the nth string from pMultiStr
  while (n-- && pgm_read_byte(pMultiStr))
    pMultiStr += strlen_P(pMultiStr) + 1;
  return pMultiStr;
}

void Format(uint8_t NN, char* pBuff, char lead, bool NUL)
{
  // format NN (0..99) into pBuff, *without* NUL, lead '0' or ' '
  if (NN > 99)
    NN = 99;
  if (NN < 10)
  {
    *pBuff++ = lead;
    *pBuff++ = '0' + NN;
  }
  else
  {
    *pBuff++ = '0' + NN / 10;
    *pBuff++ = '0' + NN % 10;
  }
  if (NUL)
    *pBuff = '\0';
}

// Bhaskara I's sine approximation, r*sin(t) (t in degrees)
int32_t rsint(int32_t r, int32_t t)
{
  //  r*4L*t*(180L-t)/(40500L - t*(180L-t));
  while (t < 0)
  {
    t += 180;
    r = -r;
  }
  while (t > 180)
  {
    t -= 180;
    r = -r;
  }
  int32_t numerator = r * 4L * t * (180L - t);
  int32_t denominator = (40500L - t * (180L - t));
  int32_t result = numerator / denominator;
  // round!
  if (result >= 0)
  {
    if ((numerator % denominator) >= denominator / 2)
      result++;
  }
  else
  {
    numerator = abs(numerator);
    denominator = abs(denominator);
    if ((numerator % denominator) >= denominator / 2)
      result--;
  }
  return result;
}

int32_t rcost(int32_t r, int32_t t)
{
  return rsint(r, 90L - t);
}

#define TOUCH_SAMPLE_COUNT_SHIFT 3 // 8 samples
#define TOUCH_SAMPLE_COUNT (1 << TOUCH_SAMPLE_COUNT_SHIFT)
byte _xSamples[TOUCH_SAMPLE_COUNT];
byte _ySamples[TOUCH_SAMPLE_COUNT];
byte _sampleCount = 0;
byte _sampleIdx = 0;

int Average(byte* pSamples)
{
  // return the average sample, TIMES TWO
  int sum = 0;
  for (int i = 0; i < TOUCH_SAMPLE_COUNT; i++)
    sum += *pSamples++;
  return sum >> (TOUCH_SAMPLE_COUNT_SHIFT - 1);
}

bool GetStableTouch(int& touchX, int& touchY)
{
  // true if there's a touch. averaged position (seems quite noisy)
  if (LCD_GET_TOUCH(touchX, touchY))
  {
    // discard LSB so they fit in a byte (HALVED)
    touchX >>= 1;
    touchY >>= 1;
    if (_sampleCount == TOUCH_SAMPLE_COUNT)
    {
      _xSamples[_sampleIdx] = touchX;
      _ySamples[_sampleIdx] = touchY;
      _sampleIdx++;
      if (_sampleIdx >= TOUCH_SAMPLE_COUNT)
        _sampleIdx = 0;
      touchX = Average(_xSamples);
      touchY = Average(_ySamples);
      return true;
    }
    else
    {
      // just add the samples
      _xSamples[_sampleIdx] = touchX;
      _ySamples[_sampleIdx] = touchY;
      _sampleCount = ++_sampleIdx;
      return false;
    }
  }
  else
  {
    _sampleCount = _sampleIdx = 0;
    return false;
  }
}

#define BLACK 0x00
#define WHITE 0xFF

void CircleRow(int x1, int x2, int thickness, int yC, int dY)
{
  // fill a row @ yC+dY from x1 to x2 start/end black bands of thickness
  // there is a gap in the middle proportional to dY so there's no doubling-up on the fill
  int width = x2 - x1 + 1;
  int w = (width - 2 * abs(dY)) / 2 + 1;
  if (w < thickness)
    w = thickness;
  LCD_BEGIN_FILL(x1, yC + dY, w, 1);
  LCD_FILL_BYTE(thickness, BLACK);
  LCD_FILL_BYTE(w - thickness, WHITE);
  LCD_BEGIN_FILL(x2 - w, yC + dY, w, 1);
  LCD_FILL_BYTE(w - thickness, WHITE);
  LCD_FILL_BYTE(thickness, BLACK);
}

void CircleCol(int xC, int dX, int y1, int y2, int thickness)
{
  // fill a col @ xC+xY from y1 to y2 start/end black bands of thickness
  // there is a gap in the middle proportional to dX so there's no doubling-up on the fill
  int height = y2 - y1 + 1;
  int h = (height - 2 * abs(dX)) / 2 + 1;
  if (h < thickness)
    h = thickness;
  if (h - thickness == 1)
  {
    // reduce white pixel glitch
    thickness++;
    h = thickness;
  }    
  LCD_BEGIN_FILL(xC + dX, y1, 1, h);
  LCD_FILL_BYTE(thickness, BLACK);
  LCD_FILL_BYTE(h - thickness, WHITE);
  LCD_BEGIN_FILL(xC + dX, y2 - h, 1, h);
  LCD_FILL_BYTE(h - thickness, WHITE);
  LCD_FILL_BYTE(thickness, BLACK);
}

void FillCircle(int centreX, int centreY, int innerRadius, int thickness)
{
  // draw a black circle of the thickness and given inner R, filled with white
  int xOuter = innerRadius + thickness - 1;
  int xInner = innerRadius;
  int y = 0;
  int errOuter = 1 - xOuter;
  int errInner = 1 - xInner;

  while (xOuter >= y)
  {
    CircleRow(centreX - xOuter, centreX + xOuter, xOuter - xInner + 1, centreY, +y);
    CircleCol(centreX, -y, centreY - xOuter, centreY + xOuter, xOuter - xInner + 1);
    if (xOuter)
    {
      CircleRow(centreX - xOuter, centreX + xOuter, xOuter - xInner + 1, centreY, -y);
      CircleCol(centreX, +y, centreY - xOuter, centreY + xOuter, xOuter - xInner + 1);
    }

    y++;

    if (errOuter < 0)
      errOuter += 2*y + 1;
    else
    {
      xOuter--;
      errOuter += 2*(y - xOuter + 1);
    }

    if (y > innerRadius)
      xInner = y;
    else
    {
      if (errInner < 0)
        errInner += 2*y + 1;
      else
      {
        xInner--;
        errInner += 2*(y - xInner + 1);
      }
    }
  }
}

// Use Bresenham to virtually stroke the perimeter lines, recording the row-extents. Requires 2 byte buffers up to max vertical span of figure.
// https://en.wikipedia.org/wiki/Kite_(geometry)
void KiteLine(int x0, int y0, int x1, int y1, tKite& kite)
{
  // Draw a virtual line {x0, y0} to {x1, y1} updating kite.
  // Row based, y0 increasing
  int dx, dy;
  int     sx;
  int er, e2;
  if (y0 > y1)
  {
    // ensure y0 <= y1;
    dy = y0; y0 = y1; y1 = dy;
    dx = x0; x0 = x1; x1 = dx;
  }

  dy = y1 - y0;
  dx = (x1 >= x0) ? x0 - x1 : x1 - x0;
  sx = (x0 < x1) ? 1 : -1;
  er = dy + dx;
  byte* pMinX = kite._rowLHS + y0; // min/max (left/right) x values indexed by y value
  byte* pMaxX = kite._rowRHS + y0;

  while (true)
  {
    if (x0 < *pMinX)
      *pMinX = x0;
    if (x0 > *pMaxX)
      *pMaxX = x0;
    //Pixel(x0, y0);
    //LCD_FILL_BYTE(LCD_BEGIN_FILL(kite._x0 + x0, kite._y0 + y0, 1, 1), 0x00);

    if ((x0 == x1) && (y0 == y1))
      break;
    e2 = 2 * er;
    if (e2 >= dx)
    {
      er += dx;
      y0++; // new row
      pMinX++;
      pMaxX++;
    }
    if (e2 <= dy)
    {
      er += dy;
      x0 += sx;
    }
  }
}

void MakeKite(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, tKite& kite)
{
  // Fill-in kite raster info by virtually rendering the lines, recording the spans of each row
  //     0
  //   .   .
  //  1     2
  //   .   .
  //     3
  // normalise to minimums to fit in byte
  int minX = min(x0, min(x1, min(x2, x3)));
  int minY = min(y0, min(y1, min(y2, y3)));
  x0 -= minX;
  y0 -= minY;
  x1 -= minX;
  y1 -= minY;
  x2 -= minX;
  y2 -= minY;
  x3 -= minX;
  y3 -= minY;
  // init the kite
  kite._xOrg = minX;
  kite._yOrg = minY;
  ::memset(kite._rowLHS, 0xFF, KITE_MAX_ROWS);
  ::memset(kite._rowRHS, 0x00, KITE_MAX_ROWS);
  // stroke the lines
  KiteLine(x0, y0, x1, y1, kite);
  KiteLine(x0, y0, x2, y2, kite);
  KiteLine(x3, y3, x2, y2, kite);
  KiteLine(x3, y3, x1, y1, kite);
}

void FillKite(tKite& kite, byte colour)
{
  // Draw the kite, fill each row with colour
  int y = kite._yOrg;
  byte* pMinX = kite._rowLHS;
  byte* pMaxX = kite._rowRHS;
  while (*pMinX != 0xFF)
  {
    LCD_FILL_BYTE(LCD_BEGIN_FILL(kite._xOrg + *pMinX, y, *pMaxX - *pMinX + 1, 1), colour);
    pMinX++;
    pMaxX++;
    y++;
  }
}

}
