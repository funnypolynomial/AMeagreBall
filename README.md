# AMeagreBall
Just the Amiga Ball bouncing on a humble Uno with a 320x240 Touch LCD shield. Optionally also with a choice of windows showing the time.

Some time in August my feed included a story about the Amiga's 40th anniversary, it included the famous bouncing ball demo.  It seemed like an interesting thing to try recreating on an Arduino and LCD. This is the sort of thing I've done a few times: [ElitePetite](https://github.com/funnypolynomial/ElitePetite), [LittleZone](https://github.com/funnypolynomial/LittleZone), [Rasteroids](https://github.com/funnypolynomial/Rasteroids) etc.

This was all about fast rendering, so it's close to the metal.  I access the LCD directly via registers, so the code is specific to the Uno and the type(s) of LCD I have access to.  Jaycar has sold at least three [XC4630](https://www.jaycar.co.nz/duinotech-arduino-compatible-2-5-inch-colour-lcd-touch-screen-display/p/XC4630) variations, it should work with any of them by setting the appropriate #define.  Extensive logic determines the pixels to send to the screen, on-the-fly. There is no buffering, even the clipping is on-thefly.

more TBD...
Mark
November 2025
