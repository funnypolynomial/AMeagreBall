#!/usr/bin/python
import sys
from PIL import Image

# extract 8x16 pixel chars from "Kickstart 1.2" image at https://heckmeck.de/blog/amiga-topaz-1.4/ 

def ByteStr(b):
    return "0b" + bin(256 + b)[3:].upper()
    

image = Image.open("font_topaz.png")
fontWidthPixels = 8
fontHeightPixels = 16
char = 32
totalBytes = 0
file = open("FontData.h", "w")
file.write("// (created by font_extract.py)\n")
file.write("// Font is " + str(fontWidthPixels) +"x" + str(fontHeightPixels) + "\n")
file.write("static const uint8_t font[] PROGMEM =\n{\n")
for charRow in range(6):
  for charCol in range(16):
    x, y = charCol*fontWidthPixels, charRow*fontHeightPixels
    if char < 127:
      file.write("  // '" + chr(char) + "'\n")
    else:
      file.write("  // '\\" + hex(char)[1:] + "'\n")
    for row in range(fontHeightPixels):
      byte = 0
      for col in range(fontWidthPixels):
        byte <<= 1
        if image.getpixel((x + col, y + row)) == (0, 0, 0):
          byte |= 1
      file.write("  " + ByteStr(byte) + ",\n")
    char += 1
    totalBytes += fontHeightPixels
file.write("}; // " + str(totalBytes + 1) + " bytes\n")
file.close()
sys.stdout.write("Created " + file.name + "\n")

