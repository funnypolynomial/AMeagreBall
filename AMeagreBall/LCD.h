#pragma once

// If defined, use later revision of Jaycar module, anti-static bag, otherwise blister pack
#define XC4630_HX8347i

// If defined use EVEN LATER revision of Jaycar module, in an anti-static bag, in a box 
//#define XC4630_UC8230

// LCD oriented with USB 12, 3, 6, or 9 O'Clock position
#define XC4630_ROTATION_USB_OCLOCK 9

#if XC4630_ROTATION_USB_OCLOCK == 9 || XC4630_ROTATION_USB_OCLOCK == 3
#define XC4630_WIDTH  320
#define XC4630_HEIGHT 240
#else
#define XC4630_WIDTH  240
#define XC4630_HEIGHT 320
#endif  

// If defined initialisation runs touch calibration routine, DEBUG needs to be defined too, see LCD::TouchCalib()
//#define XC4630_TOUCH_CALIB

// If defined initialisation runs touch check routine, cross-hairs drawn at stylus, see LCD::TouchCheck()
//#define XC4630_TOUCH_CHECK

// optionally dump graphics cmds to serial:
//#define SERIALIZE
#ifdef SERIALIZE
#define SERIALISE_ON(_on) lcd_320._serialise=_on;
#else
#define SERIALISE_ON(_on)
#endif

// optionally verify parameters
//#define LCD_CHECK_PARMS

#define COLOUR word
#define RGB(_r, _g, _b) (word)((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8)

class LCD
{
  public:
     void init();
     void ChipSelect(bool select);

     unsigned long beginFill(int x, int y,int w,int h);
     void fillColour(unsigned long count, word colour);
     void fillByte(unsigned long count, byte colour);
     void OneWhite();
     void OneBlack();

     bool isTouch(int x, int y, int w,int h);
     bool getTouch(int& x, int& y);
     void touchCalib();
     void touchCheck();

     void setScroll(bool left);
     void scrollBy(uint16_t cols);
     
#ifdef SERIALIZE
     bool _serialise = false;
#endif
#ifdef LCD_CHECK_PARMS
     bool _check = true;
#endif     
};

extern LCD lcd;
