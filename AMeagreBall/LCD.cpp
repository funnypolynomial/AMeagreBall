#include <Arduino.h>
#include "LCD.h"

// The XC4630d hairball, see below
// touch calibration data => raw values correspond to orientation 1
#define XC4630_TOUCHX0 920
#define XC4630_TOUCHY0 950
#define XC4630_TOUCHX1 120 //170
#define XC4630_TOUCHY1 160 //200

extern int XC4630_width,XC4630_height;
extern byte XC4630_orientation;

extern void XC4630_init();
extern void XC4630_rotate(int n);

extern int XC4630_touchrawx();
extern int XC4630_touchrawy();
extern int XC4630_touchx();
extern int XC4630_touchy();
extern int XC4630_istouch(int x1,int y1,int x2,int y2);

LCD lcd;
void HX8347i_Init(byte rotation);
void UC8230_Init(byte rotation);

#ifdef SERIALIZE
#define SERIALISE_COMMENT(_c) if (_serialise) { Serial.print("; ");Serial.println(_c);}
#define SERIALISE_INIT(_w,_h,_s) if (_serialise) { Serial.print(_w);Serial.print(',');Serial.print(_h);Serial.print(',');Serial.println(_s);}
#define SERIALISE_BEGINFILL(_x,_y,_w,_h) if (_serialise) { Serial.print(_x);Serial.print(',');Serial.print(_y);Serial.print(',');Serial.print(_w);Serial.print(',');Serial.println(_h);}
#define SERIALISE_FILLCOLOUR(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour);}
#define SERIALISE_FILLBYTE(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour?0xFFFF:0x0000);}
#else
#define SERIALISE_COMMENT(_c)
#define SERIALISE_INIT(_w,_h,_s)
#define SERIALISE_BEGINFILL(_x,_y,_w,_h)
#define SERIALISE_FILLCOLOUR(_len,_colour)
#define SERIALISE_FILLBYTE(_len,_colour)
#endif

#ifdef LCD_CHECK_PARMS
#define LCD_MAX_PARM 320UL*240UL
#define DGB_CHECK(_parm, _max) if (_check && (unsigned long)(_parm) > (unsigned long)(_max)) { Serial.println();Serial.print("*** Bad value! "); Serial.println(_parm);  }
#else
#define DGB_CHECK(_parm, _max)
#endif

// Control pins
#define LCD_RD   B00000001  // A0
#define LCD_WR   B00000010  // A1
#define LCD_RS   B00000100  // A2
#define LCD_CD   B00000100  // A2
#define LCD_CS   B00001000  // A3
#define LCD_RST  B00010000  // A4
// Data pins
// D2, D3, D4, D5, D6, D7, D8, D9

/////////// UNO
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
// optimised code, specific to Uno
#define CTRL_PORT PORTC
// Preserves 10,11,12 & 13 on B
#define FastData2(h, l) PORTD = (h); PORTB = (PORTB & B11111100) | (l); ToggleDataWR;

// Preserves 10,11,12 & 13 on B, Keeps CS active low
#define FastCmdByte(c) CTRL_PORT = LCD_RST | LCD_RD; PORTD = c & B11111100; PORTB = (PORTB & B11111100) | (c & B00000011);CTRL_PORT |= LCD_WR; CTRL_PORT |= LCD_RS;
#endif

/////////// MEGA
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define MEGA
//// optimised code, specific to Mega
#define CTRL_PORT PORTF
// the hi/low precomputation is largely moot on Mega
#define FastData2(h, l)                                                                                       \
                        PORTE = (PORTE & 0b11000111) | (((h) & 0b00001100) << 2) | (((h) & 0b00100000) >> 2); \
                        PORTG = (PORTG & 0b11011111) | (((h) & 0b00010000) << 1);                             \
                        PORTH = (PORTH & 0b10000111) | (((h) & 0b11000000) >> 3) | ( (l)               << 5); \
                        ToggleDataWR;
#define FastCmdByte(c)  CTRL_PORT = LCD_RST | LCD_RD;                                                         \
                        PORTE = (PORTE & 0b11000111) | (((c) & 0b00001100) << 2) | (((c) & 0b00100000) >> 2); \
                        PORTG = (PORTG & 0b11011111) | (((c) & 0b00010000) << 1);                             \
                        PORTH = (PORTH & 0b10000111) | (((c) & 0b11000000) >> 3) | (((c) & 0b00000011) << 5); \
                        CTRL_PORT |= LCD_WR; CTRL_PORT |= LCD_RS;
#endif
#define ToggleDataWR CTRL_PORT = B00010101; CTRL_PORT = B00010111; // keeps RST, RS & RD HIGH, CS active low
#define FastData(d) FastData2((d) & B11111100, (d) & B00000011);
#define FastCmd(c) FastCmdByte(c);

#define BMASK         0b00000011
#define DMASK         0b11111100

#ifdef XC4630_UC8230
#ifdef MEGA
#error XC4630_UC8230/MEGA not supported
#else
#undef ToggleDataWR
#undef FastData2
#undef FastCmdByte
#define ToggleDataWR CTRL_PORT &= ~LCD_WR; CTRL_PORT |= LCD_WR;
#define FastData2(h, l) PORTD = (h); PORTB = (PORTB & ~BMASK) | (l); ToggleDataWR;
//#define FastCmdByte(c) CTRL_PORT &= ~LCD_CD; PORTD = c & DMASK; PORTB = (PORTB & ~BMASK) | (c & BMASK);ToggleDataWR;CTRL_PORT |= LCD_CD;
// assuming = is faster than &= or |=
#define FastCmdByte(c) CTRL_PORT = LCD_RD | LCD_WR | LCD_RST; PORTD = c & DMASK; PORTB = (PORTB & ~BMASK) | (c & BMASK);CTRL_PORT = LCD_RD | LCD_RST; CTRL_PORT = LCD_RD | LCD_WR | LCD_RST | LCD_CD; 
#endif
#endif

void LCD::init()
{
  // reset
  byte rotation = 0;
#if XC4630_ROTATION_USB_OCLOCK  == 12
  rotation = 1;
  #define UC8230_MADCTL0x36 0x48  
#elif XC4630_ROTATION_USB_OCLOCK == 9
  rotation = 2;
  #define UC8230_MADCTL0x36 0x28  
#elif XC4630_ROTATION_USB_OCLOCK == 6
  rotation = 3;
  #define UC8230_MADCTL0x36 0x98  
#elif XC4630_ROTATION_USB_OCLOCK == 3
  rotation = 4;
  #define UC8230_MADCTL0x36 0xF8  
#endif  

#ifdef XC4630_HX8347i
  HX8347i_Init(rotation);
  ChipSelect(true);
#elif defined(XC4630_UC8230)
  UC8230_Init(rotation);
  ChipSelect(true);
#else
  delay(250);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  delay(100);
  digitalWrite(A4, HIGH);
  delay(500);

  XC4630_init();
  XC4630_rotate(rotation);
  ChipSelect(true);
#endif

#ifdef XC4630_TOUCH_CALIB
  touchCalib();
#elif defined(XC4630_TOUCH_CHECK)
  touchCheck();
#else
  SERIALISE_COMMENT("*** START");
  SERIALISE_INIT(XC4630_width, XC4630_height, 1);
#endif  
}

void LCD::ChipSelect(bool select)
{
  if (select)
    CTRL_PORT &= ~LCD_CS;
  else
    CTRL_PORT |=  LCD_CS;
}


unsigned long LCD::beginFill(int x, int y, int w, int h)
{
  DGB_CHECK(x, XC4630_WIDTH);DGB_CHECK(y, XC4630_HEIGHT);DGB_CHECK(w, XC4630_WIDTH);DGB_CHECK(h, XC4630_HEIGHT);
  SERIALISE_BEGINFILL(x, y, w, h);
  int x2 = x + w - 1;
  int y2 = y + h - 1;  
  
#ifdef XC4630_HX8347i
  FastCmdByte(0x02);
  FastData(x >> 8);
  FastCmdByte(0x03);
  FastData(x);
  FastCmdByte(0x04);
  FastData(x2 >> 8);
  FastCmdByte(0x05);
  FastData(x2);
  
  FastCmdByte(0x06);
  FastData(y >> 8);
  FastCmdByte(0x07);
  FastData(y);
  FastCmdByte(0x08);
  FastData(y2 >> 8);
  FastCmdByte(0x09);
  FastData(y2);
  FastCmdByte(0x22);  // Write Data to GRAM
#elif defined(XC4630_UC8230)
  FastCmdByte(0x2A);
  FastData(x >> 8);
  FastData(x);
  FastData(x2 >> 8);
  FastData(x2);
  
  FastCmdByte(0x2B);
  FastData(y >> 8);
  FastData(y);
  FastData(y2 >> 8);
  FastData(y2);
  
  FastCmdByte(0x2C); // start writing data...
#else
  // adjust coords
  int tmp = x;
  int tmp2 = x2;
#if XC4630_ROTATION_USB_OCLOCK == 3
  x = XC4630_height - y2 - 1;
  x2 = XC4630_height - y - 1;
  y = tmp;
  y2 = tmp2;
#elif XC4630_ROTATION_USB_OCLOCK == 6
  (void)tmp;
  (void)tmp2;
#elif XC4630_ROTATION_USB_OCLOCK == 9
  x = y;
  x2 = y2;
  y = XC4630_width - tmp2 - 1;
  y2 = XC4630_width - tmp - 1;
#elif XC4630_ROTATION_USB_OCLOCK == 12
  x = XC4630_width - x2 - 1;
  x2 = XC4630_width - tmp - 1;
  tmp = y;
  y = XC4630_height - y2 - 1;
  y2 = XC4630_height - tmp - 1;
#endif  

  FastCmd(0x50);               //set x bounds
  FastData(x >> 8);
  FastData(x);
  FastCmd(0x51);               //set x bounds
  FastData(x2 >> 8);
  FastData(x2);
  FastCmd(0x52);               //set y bounds
  FastData(y >> 8);
  FastData(y);
  FastCmd(0x53);               //set y bounds
  FastData(y2 >> 8);
  FastData(y2);

#if XC4630_ROTATION_USB_OCLOCK == 3 || XC4630_ROTATION_USB_OCLOCK == 12
  x = x2;
#endif  
#if XC4630_ROTATION_USB_OCLOCK == 9 || XC4630_ROTATION_USB_OCLOCK == 12
  y = y2;
#endif
  FastCmd(0x20);               //set x pos
  FastData(x >> 8);
  FastData(x);
  FastCmd(0x21);               //set y pos
  FastData(y >> 8);
  FastData(y);
  
  FastCmd(0x22);  // Write Data to GRAM
#endif

  unsigned long count = w;
  count *= h;
  return count;
}

void LCD::fillColour(unsigned long count, word colour)
{
  DGB_CHECK(count, LCD_MAX_PARM);
  SERIALISE_FILLCOLOUR(count, colour);

  // fill with full 16-bit colour
  byte h1 = (colour >> 8) & B11111100;
  byte l1 = (colour >> 8) & B00000011;
  byte h2 =  colour       & B11111100;
  byte l2 =  colour       & B00000011;
  while (count--)
  {
    FastData2(h1, l1);
    FastData2(h2, l2);
  }
}

void LCD::fillByte(unsigned long count, byte colour)
{
  DGB_CHECK(count, LCD_MAX_PARM);
  SERIALISE_FILLBYTE(count, colour);

  // fill with just one byte, i.e. 0/black or 255/white, or other, for pastels
  PORTD = colour & B11111100;
  PORTB = (PORTB & B11111100) | (colour & B00000011);
  while (count--)
  {
    ToggleDataWR;
    ToggleDataWR;
  }
}

void LCD::OneWhite()
{
  SERIALISE_FILLBYTE(1, 0xFF);

  PORTD = B11111100;
  PORTB = (PORTB & B11111100) | B00000011;
  ToggleDataWR;
  ToggleDataWR;
}

void LCD::OneBlack()
{
  SERIALISE_FILLBYTE(1, 0x00);

  // fill with just one byte, i.e. 0/black or 255/white, or other, for pastels
  PORTD = 0x00;
  PORTB = PORTB & B11111100;
  ToggleDataWR;
  ToggleDataWR;
}

bool LCD::isTouch(int x, int y, int w, int h)
{
  return XC4630_istouch(x, y, x + w, y + h);
}

bool LCD::getTouch(int& x, int& y)
{
  x = XC4630_touchx();
  if (x >= 0)
  {
    y = XC4630_touchy();
    return y >= 0;
  }
  return false;
}

void LCD::touchCalib()
{
  // Writes to Serial, XC4630_TOUCH* #define values to paste at the top of the file
  // Visit each side with the stylus
#ifdef XC4630_TOUCH_CALIB  
  int touchX0 = XC4630_TOUCHX0, touchY0 = XC4630_TOUCHY0, touchX1 = XC4630_TOUCHX1, touchY1 = XC4630_TOUCHY1;
  beginFill(0, 0, XC4630_width, XC4630_height);
  // draw shapes
  for (int row = 0; row < XC4630_height; row++)
  {
    int indent = (row > XC4630_height/2)?XC4630_height - row:row;
    fillColour(indent, 0b0000000000011111);
    fillColour(XC4630_width - 2*indent, (row > XC4630_height/2)?0b1111100000000000:0b111110000011111);
    fillColour(indent, 0b0000011111100000);
  }
  Serial.println("Drag stylus to all edges");
  while (true)
  {
    int x = XC4630_touchrawx();
    int y = XC4630_touchrawy();
    bool update = false;
    if (x > 1000 || y > 1000) 
      continue; // ignore resting values
    if (x >= 0)
    {
      if (x > touchX0)
      {
        touchX0 = x;
        update = true;
      }
      else if (x < touchX1)
      {
        touchX1 = x;
        update = true;
      }    
    }
    if (y >= 0)
    {
      if (y > touchY0)
      {
        touchY0 = y;
        update = true;
      }
      else if (y < touchY1)
      {
        touchY1 = y;
        update = true;
      }    
    }
    if (update)
    {
      Serial.println("--------------------------");
      Serial.print("X0 ");
      Serial.println(touchX0);
      Serial.print("Y0 ");
      Serial.println(touchY0);
      Serial.print("X1 ");
      Serial.println(touchX1);
      Serial.print("Y1 ");
      Serial.println(touchY1);  
    }
  }
#endif  
}

void LCD::touchCheck()
{
  // Tracks the stylus on-screen
  const int tol = 5;
  fillByte(beginFill(0, 0, XC4630_width, XC4630_height), 0x00);
  int prevX = -1, prevY;
  while (true)
  {
    int x, y;
    if (getTouch(x, y))
    {
      if (prevX != -1)
      {
        if (abs(x-prevX) < tol && abs(y-prevY) < tol)
          continue; // avoid flicker
        fillByte(beginFill(prevX, 0, 1, XC4630_height), 0x00);
        fillByte(beginFill(0, prevY, XC4630_width, 1), 0x00);        
      }
      prevX = x;
      prevY = y;
      fillByte(beginFill(prevX, 0, 1, XC4630_height), 0xFF);
      fillByte(beginFill(0, prevY, XC4630_width, 1), 0xFF); 
    }
    else
    {
      fillByte(beginFill(0, 0, XC4630_width, XC4630_height), 0x00);
    }
  }
}


bool scrollLeft = true;
void LCD::setScroll(bool left)
{
  scrollLeft = left;
#ifdef XC4630_HX8347i
  // set the window
  FastCmdByte(0x0E);  // TFA
  FastData(0);
  FastCmdByte(0x0F);
  FastData(0);
  FastCmdByte(0x10);  // VSA
  FastData(XC4630_WIDTH >> 8);
  FastCmdByte(0x11);
  FastData(XC4630_WIDTH);
  FastCmdByte(0x12);  // BFA
  FastData(0);
  FastCmdByte(0x13);
  FastData(0);
  
  FastCmdByte(0x01);  // Display mode control register SCROLL on
  FastData(0x08);
#elif defined(XC4630_UC8230)
  // set the window
  FastCmdByte(0x33);
  // TFA
  FastData(0);
  FastData(0);
  // VSA
  FastData(XC4630_WIDTH >> 8);
  FastData(XC4630_WIDTH);
  // BFA
  FastData(0);
  FastData(0);
#else
  FastCmd(0x61);  // Display mode control register SCROLL on
  FastData(0x00);
  FastData(0x03);
#endif  
}

void LCD::scrollBy(uint16_t cols)
{
  // scrolls the entire landscape screen horizontally by cols
  if (scrollLeft && cols)
    cols = XC4630_WIDTH - cols;
#ifdef XC4630_HX8347i
  FastCmdByte(0x14);  // VSP
  FastData(cols >> 8);
  FastCmdByte(0x15);
  FastData(cols);
#elif defined(XC4630_UC8230)
  FastCmdByte(0x37);
  // VSP
  FastData(cols >> 8);
  FastData(cols);
#else
  FastCmd(0x6A);
  FastData(cols >> 8);
  FastData(cols);
#endif
}

//=================================================================
// **** This initialisation sequence is gleaned from MCUFRIEND_kbv.cpp LCD ID=00x9341 https://github.com/prenticedavid/MCUFRIEND_kbv
static const byte UC8230_initialisation[] PROGMEM = 
{
// len, cmd,   data...
// 0xFF, sleep ms

  1, 0x01,                                // Rst
  0xFF, 150,                              // delay
  1, 0x28,                                // Display Off
  2, 0x3A, 0x55,                          // 565
  4, 0xF6, 0x01, 0x01, 0x00,              // Interface
  4, 0xCF, 0x00, 0x81, 0x30,              // Pwr Ctrl B
  5, 0xED, 0x64, 0x03, 0x12, 0x81,        // Pwr On
  4, 0xE8, 0x85, 0x10, 0x78,              // Drvr Timing A
  6, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,  // Pwr Ctrl A
  2, 0xF7, 0x20,                          // Pump Ratio
  3, 0xEA, 0x00, 0x00,                    // Drvr Timing B
  2, 0xB0, 0x00,                          // RGB
  3, 0xB1, 0x00, 0x1B,                    // Frame Ctrl
  2, 0xB4, 0x00,                          // Inversion Ctrl
  2, 0xC0, 0x21,                          // Pwr Ctrl 1
  2, 0xC1, 0x11,                          // Pwr Ctrl 2
  3, 0xC5, 0x3F, 0x3C,                    // VCOM 1
  2, 0xC7, 0xB5,                          // VCOM 2
  2, 0x36, 0x48,                          // Mem Access
  2, 0xF2, 0x00,                          // Enable 3G
  2, 0x26, 0x01,                          // Gamma...
 16, 0xE0, 0x0F, 0x26, 0x24, 0x0B, 0x0E, 0x09, 0x54, 0xA8, 0x46, 0x0C, 0x17, 0x09, 0x0F, 0x07, 0x00,
 16, 0xE1, 0x00, 0x19, 0x1B, 0x04, 0x10, 0x07, 0x2A, 0x47, 0x39, 0x03, 0x06, 0x06, 0x30, 0x38, 0x0F,
  2, 0x36, UC8230_MADCTL0x36,             // Mem Access Ctrl
  1, 0x11,                                // Wake
  0xFF, 150,                              // delay
  1, 0x29,                                // Display On
};

void UC8230_Init(byte rotation)
{
  DDRC |= LCD_RD | LCD_WR | LCD_CD | LCD_CS | LCD_RST;
  DDRB |=  BMASK; 
  DDRD |=  DMASK;
  CTRL_PORT |= LCD_CS | LCD_RD | LCD_WR | LCD_RST;
  delay(50);
  CTRL_PORT &= ~LCD_RST;
  delay(100);
  CTRL_PORT |=  LCD_RST;
  delay(100);
  CTRL_PORT &= ~LCD_CS;
    
  const byte* init = UC8230_initialisation;
  do
  {
    byte len = pgm_read_byte(init++);
    if (len == 0xFF)
      delay(pgm_read_byte(init++));
    else if (len)
    {
      byte cmd = pgm_read_byte(init++);
      FastCmdByte(cmd);
      while (--len)
      {
        byte dat = pgm_read_byte(init++);
        FastData(dat);
      }
    }
    else
    {
      break;
    }
  } while (true);
  
  // keeps the touch logic happy
  XC4630_orientation = rotation;
  if (rotation % 2)
  {
    XC4630_width = 240;
    XC4630_height = 320;
  }
  else
  {
    XC4630_width = 320;
    XC4630_height = 240;
  }  
}

//=================================================================
// **** This initialisation sequence is gleaned from MCUFRIEND_kbv.cpp LCD ID=0x9595 https://github.com/prenticedavid/MCUFRIEND_kbv
const byte HX8347i_initialisation[] PROGMEM = 
{
    0xEA, 0x00, 
    0xEB, 0x20,
    0xEC, 0x0C, 
    0xED, 0xC4,
    0xE8, 0x38,
    0xE9, 0x10,
    0xF1, 0x01,
    0xF2, 0x10,
    
    0x40, 0x01, 
    0x41, 0x00, 
    0x42, 0x00, 
    0x43, 0x10, 
    0x44, 0x0E, 
    0x45, 0x24, 
    0x46, 0x04, 
    0x47, 0x50, 
    0x48, 0x02, 
    0x49, 0x13, 
    0x4A, 0x19, 
    0x4B, 0x19, 
    0x4C, 0x16, 
    
    0x50, 0x1B, 
    0x51, 0x31, 
    0x52, 0x2F, 
    0x53, 0x3F, 
    0x54, 0x3F, 
    0x55, 0x3E, 
    0x56, 0x2F, 
    0x57, 0x7B, 
    0x58, 0x09, 
    0x59, 0x06, 
    0x5A, 0x06, 
    0x5B, 0x0C, 
    0x5C, 0x1D, 
    0x5D, 0xCC, 
    
    0x1B, 0x1B,
    0x1A, 0x01,
    0x24, 0x2F,
    0x25, 0x57,
    0x23, 0x88,
    0x18, 0x34,
    0x19, 0x01,
    0x01, 0x00,
    0x1F, 0x88,
    0xFF,    5,
    0x1F, 0x80,
    0xFF,    3,
    0x1F, 0x90,
    0xFF,    5,
    0x1F, 0xD0,
    0xFF,    5,
    0x17, 0x05,
    0x36, 0x00,
    0x28, 0x38,
    0xFF,   40,
    0x28, 0x3F,
    0x16, 0x18,
    
    0xFF,    0  // end!
};

void HX8347i_Init(byte rotation)
{
  // outputs
#ifdef MEGA  
  bitSet(DDRE, 4);  // D2
  bitSet(DDRE, 5);  // D3
  bitSet(DDRG, 5);  // D4
  bitSet(DDRE, 3);  // D5
  bitSet(DDRH, 3);  // D6
  bitSet(DDRH, 4);  // D7
  bitSet(DDRH, 5);  // D8
  bitSet(DDRH, 6);  // D9  
 
  DDRF  |= (LCD_CS | LCD_WR | LCD_RD | LCD_RST | LCD_CD);
//  DDRE = DDRE | 0b00111000; ??
//  DDRG = DDRG | 0b00100000;
//  DDRH = DDRH | 0b01111000;
#else
  DDRB  |=  0x03; 
  DDRC  |= (LCD_CS | LCD_WR | LCD_RD | LCD_RST | LCD_CD);
  DDRD  |=  0xFC;
#endif  
  CTRL_PORT |= (LCD_CS | LCD_WR | LCD_RD | LCD_RST); // all idle
  delay(50);
  CTRL_PORT &= ~LCD_RST; // reset
  delay(100);
  CTRL_PORT |=  LCD_RST;  // idle
  delay(100);

  // init sequence
  const byte* pInit = HX8347i_initialisation;
  while (true)
  {
    byte cmd = pgm_read_byte_near(pInit++);
    byte dat = pgm_read_byte_near(pInit++);
    if (cmd == 0xFF)
    {
      if (dat)
        delay(dat);
      else
        break;
    }
    else
    {
      FastCmdByte(cmd);
      FastData(dat);
    }
  }

  // rotation
  FastCmdByte(0x16);
  FastData((0xF8982848UL >> (8*(rotation - 1)) & 0xFF));
  
  // keeps the touch logic happy
  XC4630_orientation = rotation;
  if (rotation % 2)
  {
    XC4630_width = 240;
    XC4630_height = 320;
  }
  else
  {
    XC4630_width = 320;
    XC4630_height = 240;
  }
}

//=================================================================
// **** This code is not mine, it's from Jaycar, warts and all. But it does initialise the LCD ****
// See  https://www.jaycar.co.nz/rfid-keypad
// Simplified and cleaned up. Converted initialisation code, which is the critical bit, from code to data, saved ~1k


//v3 was supplier change IC is UC8230 (SPFD5408 compatible?) Working with all tests

//basic 'library' for Jaycar XC4630 240x320 touchscreen display (v1/2/3 boards)
//include this in your sketch folder, and reference with #include "XC4630d.c"
//microSD card slot can be accessed via Arduino's SD card library (CS=10)
//supports: Mega, Leonaro, Uno with hardware optimizations (Uno fastest)
//supports: all other R3 layout boards via digitalWrite (ie slow)
//colours are 16 bit (binary RRRRRGGGGGGBBBBB)
//functions:
// XC4630_init() - sets up pins/registers on display, sets orientation=1
// XC4630_clear(c) - sets all pixels to colour c (can use names below)
// XC4630_rotate(n) -  sets orientation (top) 1,2,3 or 4
// typical setup code might be:
// XC4630_touchx() returns pixel calibrated touch data, <0 means no touch
// XC4630_touchy() returns pixel calibrated touch data, <0 means no touch
// XC4630_istouch(x1,y1,x2,y2) is area in (x1,y1) to (x2,y2) being touched?

// Low level functions:
// XC4630_touchrawx() returns raw x touch data (>~900 means no touch)
// XC4630_touchrawy() returns raw y touch data (>~900 means no touch)
// XC4630_areaset(x1,y1,x2,y2) -  set rectangular painting area, only used for raw drawing in other functions
// XC4630_data(d) and XC4630_command - low level functions

//global variables to keep track of orientation etc, can be accessed in sketch
int XC4630_width,XC4630_height;
byte XC4630_orientation;

// BUT NOTE the LCD code is optimised for Uno!
//for Uno board
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define XC4630SETUP   "UNO"
#define XC4630RSTOP   DDRC|=16
#define XC4630RST0    PORTC&=239
#define XC4630RST1    PORTC|=16
#define XC4630CSOP    DDRC|=8
#define XC4630CS0     PORTC&=247
#define XC4630CS1     PORTC|=8
#define XC4630RSOP    DDRC|=4
#define XC4630RS0     PORTC&=251
#define XC4630RS1     PORTC|=4
#define XC4630WROP    DDRC|=2
#define XC4630WR0     PORTC&=253
#define XC4630WR1     PORTC|=2
#define XC4630RDOP    DDRC|=1
#define XC4630RD0     PORTC&=254
#define XC4630RD1     PORTC|=1
#define XC4630dataOP  DDRD |= 252;DDRB |= 3;
#define XC4630data(d) PORTD=(PORTD&3)|(d&252); PORTB=(PORTB&252)|(d&3); 
#endif
// Or MEGA
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define XC4630SETUP   "MEGA"
#define XC4630RSTOP   DDRF|= 16
#define XC4630RST0    PORTF&= 239
#define XC4630RST1    PORTF|= 16
#define XC4630CSOP    DDRF|=8
#define XC4630CS0     PORTF&=247
#define XC4630CS1     PORTF|=8
#define XC4630RSOP    DDRF|=4
#define XC4630RS0     PORTF&=251
#define XC4630RS1     PORTF|=4
#define XC4630WROP    DDRF|=2
#define XC4630WR0     PORTF&=253
#define XC4630WR1     PORTF|=2
#define XC4630RDOP    DDRF|=1
#define XC4630RD0     PORTF&=254
#define XC4630RD1     PORTF|=1
#define XC4630dataOP  DDRE=DDRE|56;DDRG=DDRG|32;DDRH=DDRH|120;
#define XC4630data(d) PORTE=(PORTE&199)|((d&12)<<2)|((d&32)>>2);PORTG=(PORTG&223)|((d&16)<<1);PORTH=(PORTH&135)|((d&192)>>3)|((d&3)<<5);
#endif


void XC4630_command(unsigned char d)
{   
  XC4630RS0;        //RS=0 => command
  XC4630data(d);    //data
  XC4630WR0;        //toggle WR
  XC4630WR1;
  XC4630RS1;
}

void XC4630_data(unsigned char d)
{
  XC4630data(d);    //data
  XC4630WR0;        //toggle WR
  XC4630WR1;
}

//"from misc.ws LCD_ID_reader- gives a successful init"
const byte initialisation[] PROGMEM =
{
  229, 128, 0,
  0, 0, 1,
  1, 0, 0,
  2, 7, 0,
  3, 16, 0,//  "0,  3, 0, 48,"
  4, 0, 0,
  8, 2, 2,
  9, 0, 0,
  10, 0, 0,
  12, 0, 0,
  13, 0, 0,
  15, 0, 0,
  16, 0, 0,
  17, 0, 0,
  18, 0, 0,
  19, 0, 0,
  16, 23, 176,
  17, 0, 55,
  18, 1, 56,
  19, 23, 0,
  41, 0, 13,
  32, 0, 0,
  33, 0, 0,
  48, 0, 1,
  49, 6, 6,
  50, 3, 4,
  51, 2, 2,
  52, 2, 2,
  53, 1, 3,
  54, 1, 29,
  55, 4, 4,
  56, 4, 4,
  57, 4, 4,
  60, 7, 0,
  61, 10, 31,
  80, 0, 0,
  81, 0, 239,
  82, 0, 0,
  83, 1, 63,
  96, 39, 0,
  97, 0, 1,
  106, 0, 0,
  144, 0, 16,
  146, 0, 0,
  147, 0, 3,
  149, 1, 1,
  151, 0, 0,
  152, 0, 0,
  7, 0, 33,
  7, 0, 49,
  7, 1, 115,
  255, // end!
};

void XC4630_init(){
  //set variable defaults
  XC4630_width=240;
  XC4630_height=320;
  XC4630_orientation=1;
  XC4630dataOP;
  XC4630RSTOP;
  XC4630CSOP;
  XC4630RSOP;
  XC4630WROP;
  XC4630RDOP;
  XC4630RST1;
  XC4630CS1;
  XC4630RS1;
  XC4630WR1;
  XC4630RD1;
  delay(50); 
  XC4630RST0;
  delay(150);
  XC4630RST1;
  delay(150);
  XC4630CS0;

  const byte* pInit = initialisation;
  while (true)
  {
    byte cmd = pgm_read_byte_near(pInit++);
    if (cmd == 255)
      break;
    XC4630_command(0);
    XC4630_command(cmd);
    XC4630_data(pgm_read_byte_near(pInit++));
    XC4630_data(pgm_read_byte_near(pInit++));
  }
        
  XC4630CS1;
}

void XC4630_rotate(int n){
  XC4630CS0;
  switch(n){
    case 1:
    XC4630_command(0);  XC4630_command(3);XC4630_data(16);XC4630_data(0);    //0>1
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=1;
    break;
    case 2:
    XC4630_command(0);  XC4630_command(3);XC4630_data(16);XC4630_data(24);    //0>1
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=2;
    break;
    case 3:
    XC4630_command(0);  XC4630_command(3);XC4630_data(16);XC4630_data(48);    //0>1
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=3;
    break;
    case 4:
    XC4630_command(0);  XC4630_command(3);XC4630_data(16);XC4630_data(40);    //0>1
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=4;
    break;
  }
  XC4630CS1;
}


/*
 * https://www.instructables.com/id/4-Wire-Touch-Screen-Interfacing-with-Arduino/
          A3 INPUT
          |
          |
Gnd D8----+----A2 +5V
          |
          |
          D9 TriState
*/
int XC4630_touchrawx(){           //raw analog value
  int x;
  // voltage across X-axis
  pinMode(8,OUTPUT);
  digitalWrite(8,LOW);
  pinMode(A2,OUTPUT);
  digitalWrite(A2,HIGH);

  // tri-state
  pinMode(9,INPUT_PULLUP);
  
  // read voltage divider on A3
  pinMode(A3,INPUT);
//  analogRead(A3);                 // discard first result after pinMode change
  
//  delayMicroseconds(30);
  x=analogRead(A3);
  
  // restore
  pinMode(A3,OUTPUT);
  digitalWrite(A3,HIGH);
  pinMode(9,OUTPUT);
//Serial.print("x=");Serial.println(x);  
  return(x);  
}

int XC4630_touchrawy(){           //raw analog value
  int y;
  pinMode(9,OUTPUT);
  pinMode(A3,OUTPUT);
  digitalWrite(9,LOW);            //doesn't matter if this changes
  digitalWrite(A3,HIGH);          //this is normally high between screen commands
  pinMode(A2,INPUT_PULLUP);       //this is normally high between screen commands
  pinMode(8,INPUT_PULLUP);        //doesn't matter if this changes
//  analogRead(A2);                 //discard first result after pinMode change
  
//  delayMicroseconds(30);
  y=analogRead(A2);
  pinMode(A2,OUTPUT);
  digitalWrite(A2,HIGH);          //restore output state from above
  pinMode(8,OUTPUT);
//Serial.print("y=");Serial.println(y);  
  return(y);  
}

int XC4630_touchx(){
  int x,xc;
  xc=-1;      //default in case of invalid orientation
  switch(XC4630_orientation){
    case 1:
    x=XC4630_touchrawx();
    xc=map(x,XC4630_TOUCHX0,XC4630_TOUCHX1,0,XC4630_width-1);
    break;
    case 2:
    x=XC4630_touchrawy();
    xc=map(x,XC4630_TOUCHY0,XC4630_TOUCHY1,0,XC4630_width-1);
    break;
    case 3:
    x=XC4630_touchrawx();
    xc=map(x,XC4630_TOUCHX1,XC4630_TOUCHX0,0,XC4630_width-1);
    break;
    case 4:
    x=XC4630_touchrawy();
    xc=map(x,XC4630_TOUCHY1,XC4630_TOUCHY0,0,XC4630_width-1);
    break;
  }
  if(xc>XC4630_width-1){xc=-1;}         //off screen
  return xc;
}

int XC4630_touchy(){
  int y,yc;
  yc=-1;      //default in case of invalid orientation
  switch(XC4630_orientation){
    case 1:
    y=XC4630_touchrawy();
    yc=map(y,XC4630_TOUCHY0,XC4630_TOUCHY1,0,XC4630_height-1);
    break;
    case 2:
    y=XC4630_touchrawx();
    yc=map(y,XC4630_TOUCHX1,XC4630_TOUCHX0,0,XC4630_height-1);
    break;
    case 3:
    y=XC4630_touchrawy();
    yc=map(y,XC4630_TOUCHY1,XC4630_TOUCHY0,0,XC4630_height-1);
    break;
    case 4:
    y=XC4630_touchrawx();
    yc=map(y,XC4630_TOUCHX0,XC4630_TOUCHX1,0,XC4630_height-1);
    break;
  }
  if(yc>XC4630_height-1){yc=-1;}         //off screen
  return yc;
}

int XC4630_istouch(int x1,int y1,int x2,int y2){    //touch occurring in box?
  int x,y;
  x=XC4630_touchx();
  if(x<x1){return 0;}
  if(x>x2){return 0;}
  y=XC4630_touchy();
  if(y<y1){return 0;}
  if(y>y2){return 0;}
  return 1;
}
