// One library:
#include <SoftWire.h> // or SoftwareI2C.h, see RTC.cpp

#include <Arduino.h>
#include "Boing.h"
#include "Ball.h"
#include "Clock.h"
#include "Config.h"
#include "Font.h"
#include "RTC.h"
#include "Workbench.h"


//  A M e a g r e B a l l

// What:
//  A recreation of the look of the Amiga “Boing” bouncing ball demo, running on an Uno with a 320x240 Touch LCD shield.  
//  As a raison d’etre for having the thing as a desk accessory, it can optionally show the time.  I’ve added 3 "windows":
//    * An "analog" clock, based on the Amiga one, but small.
//    * A "digital" clock, not sure this existed but it is conveniently small.
//    * A "workbench" clock. This shows the time digitally on the workbench title bar.
//  All three can be moved around the screen by touching and dragging their title bars.  The workbench clock has limited movement, two heights locked at the bottom of the display.
//  Touching within the "body" of a clock will hide it (as will touching the "background" button on the title bar). Watch the ball!  
//  When hidden, touching anywhere on the screen will show the clock again.
//  Touching at the top-left corner will bring up a menu. The menu lets you
//    * switch between clocks (or no clock)
//    * hide/show the current clock
//    * set/clear daylight saving
//    * re-seed the randomness
//    * set the time
//    * reset the display
//    * view an abbreviated help
//  Most of this is N/A if there is no RTC present (see CFG_HAVE_RTC)
//  
// Why:
//  In August this year I saw a post somewhere (one of many) celebrating 40 years since the Amiga launched. 
//  It included video of "Boing" and I was inspired to try it on an Arduino+LCD, given that I’ve done several similar retro recreations (ElitePetite, Rasteroids, LittleZone, ArDSKYlite).
//  
// How:
//  The LCD interface is in two parts:
//    * define a destination window
//    * send a block of n pixels of a particular colour to that window (it will wrap)
//  The code is all about logic to determine the window locations, and more significantly, to determine the colours to send.  The image is build dynamically, on-the-fly. There is no buffering. It’s all about speed.
//  Extensive use is made of PROGMEM lookups.
//  The ball is a a pair of PROGMEM data structures, created as a header file by a Python script in the resources subdir (ball_encode.py). 
//  This run-length-encodes an image made by another script (ball_render.py). The image script makes a .PNG of the faceted ball, with 8x as many faces on a layer, facets are given one of N colours,
//  Rendering the ball is a matter of scanning rows in the data, accumulating runs of a physical colour (red or white) from runs of logical colours, then sending to the LCD. 
//  Only the background pixels exposed by the ball's movement are redrawn.
//  The background grid is also encoded as PROGMEM data, created by grid_encode.py.  This provides faster repainting.
//  The intent to send a block of colour to the LCD from the ball, or the background rendering logic, passes through additional logic which clips it with respect to the clock window, if present.
//  I'm using this LCD from Jaycar:
//  https://www.jaycar.co.nz/duinotech-arduino-compatible-2-5-inch-colour-lcd-touch-screen-display/p/XC4630
//  LCD.h supports 3 generations of the product, see XC4630_HX8347i and XC4630_UC8230.
//  
// Configuration:
//  Many things can be tweaked by altering defines in Config.h. For example
//    * Colours eg CFG_WORKBENCH_BACKGND
//    * Style of digital time display, eg CFG_DISPLAY_12_HOUR
//    * Make the screen "shudder" a little when the ball bounces off the sides (CFG_SHUDDER)
//    * Double-size text in the Digital clock (CFG_DOUBLE_DIGITAL)
//
// Resources:
//  See the resources subdirectory for various Python scripts etc.
//
// Not:
//  * The ball is a perhaps proportionately a little smaller, to get a better frame rate (~24fps). 
//  * I’ve also introduced some optional randomness in the bounce (see CFG_RANDOMIZE_BALL_PERCENT).
//  * You can’t resize the windows and I’ve made the Analog clock quite small.  
//  * The digital clock is even smaller, and the Workbench hugs the bottom of the display.  
//    The idea is you want to see as much of the ball as possible.
//  * I have taken some liberties, including showing the time on the Workbench title bar, but the wb_12.html
//    link below seems to show something like it.
//  * There is no sound, but optionally the screen shudders a little when the ball bounces off the sides (see CFG_SHUDDER)
//
// Disclaimer:
//  I am not an Amiga guy, but I remember first seeing one in around '85, in my post-grad lab.  It was impressive but I didn't use it a great deal.
//  So I’ve relied on YouTube videos etc as a rough guide. Here are some of the resources I used
//    https://youtu.be/-ga41edXw3A?si=NQKSlD4rARurb9oJ
//    https://www.youtube.com/shorts/IqQUqxjZEBI  (note that the floor of the grid is often shown clipped, I don't know why)
//    https://heckmeck.de/blog/amiga-topaz-1.4/
//    https://taws.ch/WB.html
//    http://amiga.filfre.net/?page_id=5
//    https://www.gregdonner.org/workbench/wb_12.html
//
// Also:
//  If DEBUG is defined in Boing.h the display can be paused (for tear-free photos) by
//  * sending ' ' via serial, same to resume
//  * touching top-right, touch anywhere to resume
//  
// Note:
//  I access the LCD directly via registers, so the code is specific to the Uno and the type(s) of LCD I have access to.  
//  Jaycar has sold at least three XC4630 variations, it should work with any of them by setting the appropriate #define
// Who:
//  Mark Wilson. November 2025


void setup() 
{
#ifdef DEBUG  
  Serial.begin(38400);
  Serial.println("AMeagreBall");  
#endif  
  LCD_INIT();
#ifndef DEBUG  
  Ball::Splash();  
#endif  
  Config::Init();
  Ball::Init();
  Clock::Init();
  Workbench::Init();
#if !defined(DEBUG) && defined(CFG_HAVE_RTC)
  randomSeed(rtc.Entropy());  
#endif  
}

unsigned long ms;
int ctr = 0;

void loop() 
{
#ifdef DEBUG
  // check serial for <space> toggling pause/resume (for tear-free photos)
  static bool paused = false;
  while (Serial.available())
    if (Serial.read() == ' ')
    {
      paused = !paused;
      Serial.println(paused?"pause":"resume");
    }
  if (paused)
    return;
#endif    
  
#ifdef DEBUG  
  if (ctr == 0)
    ms = millis();
#endif    
  Config::Loop();
  Clock::Loop();
  Ball::Step();
#ifdef DEBUG
  // calc frame rate
  ctr++;
  if (ctr == 1000)
  {
    ms = millis() - ms;
    Serial.println(ms/ctr);  // ~40ms
    ctr = 0;
  }
#endif    
}
