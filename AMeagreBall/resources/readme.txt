Resources to augment the sketch.
Ball:
ball_render.py creates ball_image.png which ball_encode.py converts to PROGMEM encoded data in BallData.h

Grid:
grid_encode.py converts the grid into PROGMEM encoded data in GridData.h. It creates grid_image.png which is just the floor.

Font:
font_extract.py extracts font data from font_topaz.png and writes it as PROGMEM encoded data in FontData.h

Icons:
icons_encode.py converts the icon*.png files into PROGMEM encoded data in IconData.h

*NOTES*
The script produce header files in the resources subdirectory.  
They need to be copied up one level for inclusion in the sketch.
Python.bat will likely need to be edited to point python.exe

