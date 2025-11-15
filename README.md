# AMeagreBall
Just the Amiga Ball bouncing on a humble Uno with a 320x240 Touch LCD shield. Optionally also with a choice of windows showing the time.
![Ball](https://live.staticflickr.com/65535/54924500758_ec3307ba4f_b.jpg)
Some time in August my feed included a story about the Amiga's 40th anniversary, it included the famous bouncing ball demo.  It seemed like an interesting thing to try recreating on an Arduino and LCD. This is the sort of thing I've done a few times: [ElitePetite](https://github.com/funnypolynomial/ElitePetite), [LittleZone](https://github.com/funnypolynomial/LittleZone), [Rasteroids](https://github.com/funnypolynomial/Rasteroids) etc.

This was all about fast rendering, so it's close to the metal.  I access the LCD directly via registers, so the code is specific to the Uno and the type(s) of LCD I have access to.  Jaycar has sold at least three [XC4630](https://www.jaycar.co.nz/duinotech-arduino-compatible-2-5-inch-colour-lcd-touch-screen-display/p/XC4630) variations, it should work with any of them by setting the appropriate #define.  Extensive logic determines the pixels to send to the screen, on-the-fly. There is no buffering, even the clipping is on-the-fly.

As a *raison d’etre* for having the thing as a desk accessory, it can optionally **show the time**.  I’ve added 3 "windows":
  * An "analog" clock, based on the Amiga one, but small.
  * A "digital" clock, not sure this existed but it is conveniently small.
  * A "workbench" clock. This shows the time digitally on the workbench title bar.

![Analog](https://live.staticflickr.com/65535/54923433302_0d64433221_o.png)
![Digital](https://live.staticflickr.com/65535/54923433277_2dfbf86d46_o.png)
![Workbench](https://live.staticflickr.com/65535/54924304101_96f1bfc135_o.png)

All three can be moved around the screen by touching and dragging their title bars.  The workbench clock has limited movement, two heights locked at the bottom of the display.
Touching within the "body" of a clock will hide it (as will touching the "background" button on the title bar). Watch the ball!  
When hidden, touching anywhere on the screen will show the clock again.

Touching at the top-left corner will bring up a menu. The menu lets you
  * switch between clocks (or no clock)
  * hide/show the current clock
  * set/clear daylight saving
  * re-seed the randomness
  * set the time
  * reset the display
  * view an abbreviated help

![Menu](https://live.staticflickr.com/65535/54924606770_b7c78684cf_o.png)

Most of this is N/A if there is no RTC present (see `CFG_HAVE_RTC`). I made a simple laser-cut "monitor facade which clamps over the LCD & Uno.  The Python code for this and other resources are in the `resources` folder.

[Hackaday project](https://hackaday.io/project/204504-ameagreball)

[Flickr Album](https://flic.kr/s/aHBqjCAKys)

[YouTube demo](https://youtu.be/Nls-YfQCngo?si=pPy6MmNiFAVAdqMV)

[YouTube video with "monitor" facade](https://youtu.be/1_XDpF_mIzo?si=06jhUEVnwRIcUiV9)

Mark  
November 2025
