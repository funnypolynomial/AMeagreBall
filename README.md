# AMeagreBall
Just the Amiga Ball bouncing on a humble Uno with a 320x240 Touch LCD shield. Optionally also with a choice of windows **showing the time**.
<img width="1824" height="1368" alt="Ball" src="https://github.com/user-attachments/assets/a0009df7-22f5-4bb6-ba4f-0ddf50cb4aaf" />
[YouTube demo](https://youtu.be/Nls-YfQCngo?si=pPy6MmNiFAVAdqMV)

Some time in August my feed included a story about the Amiga's 40th anniversary, it included the famous bouncing ball demo.  It seemed like an interesting thing to try recreating on an Arduino and LCD. This is the sort of thing I've done a few times: [ElitePetite](https://github.com/funnypolynomial/ElitePetite), [LittleZone](https://github.com/funnypolynomial/LittleZone), [Rasteroids](https://github.com/funnypolynomial/Rasteroids) etc.

This was all about fast rendering, so it's close to the metal.  I access the LCD directly via registers, so the code is specific to the Uno and the type(s) of LCD I have access to.  Jaycar has sold at least three [XC4630](https://www.jaycar.co.nz/duinotech-arduino-compatible-2-5-inch-colour-lcd-touch-screen-display/p/XC4630) variations, it should work with any of them by setting the appropriate #define.  The code determines the pixels to send to the screen, on-the-fly. There is no buffering, even the clipping is on-the-fly.

Extensive use is made of PROGMEM lookups. The ball is a a pair of PROGMEM data structures, created as a header file by a Python script in the resources subdir (ball_encode.py).  This run-length-encodes an image made by another script (ball_render.py). The image script makes a .PNG of the faceted ball, with 8x as many faces on a layer, facets are given one of N colours, Rendering the ball is a matter of scanning rows in the data, accumulating runs of a physical colour (red or white) from runs of logical colours, then sending to the LCD.  Only the background pixels exposed by the ball's movement are redrawn.
The background grid is also encoded as PROGMEM data, created by grid_encode.py.  This provides faster repainting. The intent to send a block of colour to the LCD from the ball, or the background rendering logic, passes through additional logic which clips it with respect to the clock window, if present.

As a *raison d’etre* for having the thing as a desk accessory, it can optionally **show the time**.  I’ve added 3 "windows":
  * An "analog" clock, based on the Amiga one, but small.
  * A "digital" clock, not sure this existed but it is conveniently small.
  * A "workbench" clock. This shows the time digitally on the workbench title bar.

<img width="1824" height="1368" alt="Analog" src="https://github.com/user-attachments/assets/bc942831-43ba-4f30-a2fb-f4fb6f639fe5" />
<img width="1824" height="1368" alt="Digital" src="https://github.com/user-attachments/assets/b859c6b6-cd10-40e5-bf1b-e4a1120127a1" />
<img width="1824" height="1368" alt="Workbench" src="https://github.com/user-attachments/assets/086f7d0e-855f-4ffb-b9d4-992235029b20" />

All three can be moved around the screen by touching and dragging their title bars.  The workbench clock has limited movement, two heights locked at the bottom of the display.
Touching within the "body" of a clock will hide it (as will touching the "background" button on the title bar). *Watch the ball!*
When hidden, touching anywhere on the screen will show the clock again.

Touching at the top-left corner will bring up a menu. The menu lets you
  * switch between clocks (or no clock)
  * hide/show the current clock
  * set/clear daylight saving
  * re-seed the randomness
  * set the time
  * reset the display
  * view an abbreviated help

<img width="320" height="240" alt="Menu (screen capture)" src="https://github.com/user-attachments/assets/5cdfa1d9-51ed-47b3-963a-889eaf11ef3d" />

Most of this is N/A if there is no RTC present (see `CFG_HAVE_RTC`). 

The hardware is simple: a Uno with an LCD shield, the slightly complicated part is the RTC which is squeezed between the two using  the ICSP pins.
I made a simple laser-cut "monitor" facade which clamps over the LCD & Uno.  The Python code for this and other resources are in the `resources` folder.

[Hackaday project](https://hackaday.io/project/204504-ameagreball)

[Flickr Album](https://flic.kr/s/aHBqjCAKys)

[YouTube video with "monitor" facade](https://youtu.be/1_XDpF_mIzo?si=06jhUEVnwRIcUiV9)

Mark  
November 2025
