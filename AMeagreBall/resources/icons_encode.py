#!/usr/bin/python
import sys
import os
from PIL import Image, ImageColor

# encode Workbench icons/widgets, run-length, PROGMEM source
#  <width>,<height>
#  0bPPLLLLLL PP=palette index, LLLLLL= run length
#  ends with a 0

def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()

def Encode(pixel, rgbPalette, len):
    idx = rgbPalette.index(pixel)
    return ByteStr((idx << 6) + len)
    
def EncodeIcon(file, png):
  image = Image.open(png)
  arrayName = os.path.splitext(os.path.basename(png))[0] + "Icon"
  bytes = 3
  file.write("static const uint8_t " + arrayName + "[] PROGMEM =\n{\n")
  file.write("  " + str(image.width) + ", " + str(image.height) + ",\n")
  for row in range(image.height):
    prevPixel = (128, 128, 128)
    length = 0
    file.write("  ")
    for col in range(image.width):
      pixel = image.getpixel((col, row))
      if pixel == prevPixel:
          length += 1
      else: 
        if length:
          bytes += 1
          file.write(Encode(prevPixel, rgbs, length) + ", ")
        prevPixel = pixel
        length = 1
    if length:
      bytes += 1
      file.write(Encode(prevPixel, rgbs, length) + ", ")
    file.write("\n")
  file.write("  0\n")
  file.write("}; // " + str(bytes) + " bytes\n\n")


file = open("IconData.h", "w")
file.write("// (created by icons_encode.py)\n")
file.write("#define ICON_PALETTE_IDX(_b) (_b >> 6)    // palette idx (2 bits)\n")
file.write("#define ICON_RUN_LEN(_b) (_b & 0x3F)  // run length (6 bits)\n")
file.write("// width, height, image data (0bppnnnnnn)...\n")
palette = ["black", "blue", "white", "red"]
rgbs = []
for pal in palette:
  rgbs.append(ImageColor.getrgb(pal))
file.write("// palette: " + str(palette) + "\n")
file.write("\n")
EncodeIcon(file, "icon_close.png")
EncodeIcon(file, "icon_backFront.png")
EncodeIcon(file, "icon_size.png")
EncodeIcon(file, "icon_upArrow.png")
EncodeIcon(file, "icon_boing.png")
file.close()