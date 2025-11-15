#!/usr/bin/python
import sys
import math
from PIL import Image, ImageDraw

# encode a Amiga-boing-style ball, run-length, PROGMEM source
# two tables, in BallData.h
# see ball_render.py

def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()
    
#  <ball pixels in row>, <pixels to LHS of ball>, <encoding1>, <encoding2> ...
#  encoding is 0xPQ P = run length - 1, Q = palette idx
def Encode(length, colour):
  encoded = ((length - 1) & 0x0F) << 4
  encoded += colour[2] & 0x0F # index is the blue component
  return encoded

image = Image.open("ball_image.png")
background = (0, 0, 0)
# scan for empty rows/cols
firstRow = None
lastRow = image.height - 1
for row in range(image.height):
  empty = True
  for col in range(image.width):
    if image.getpixel((col, row)) != background:
      empty = False
      break
  if not empty:
    if firstRow is None:
      firstRow = row
    lastRow = row
#sys.stdout.write("firstRow " + str(firstRow) + "\n")
#sys.stdout.write("lastRow " + str(lastRow) + "\n")

firstCol = None
lastCol = image.width - 1
for col in range(image.width):
  for row in range(image.height):
    empty = True
    if image.getpixel((col, row)) != background:
      empty = False
      break
  if not empty:
    if firstCol is None:
      firstCol = col
    lastCol = col
#sys.stdout.write("firstCol " + str(firstCol) + "\n")
#sys.stdout.write("lastCol " + str(lastCol) + "\n")
    
    
file = open("BallData.h", "w")
file.write("// (created by ball_encode.py)\n")
file.write("// ball is " + str(lastCol - firstCol + 1) + "x" + str(lastRow - firstRow + 1) + "\n")
file.write("static const uint8_t ballPixels[] PROGMEM =\n{\n")
file.write("// <encoding1>, <encoding2> ... <encodingN>\n")
file.write("//  encoding = 0x<length-1><palette idx>\n")

totalEncodedBytes = 0
rowData = []
for row in range(firstRow, lastRow + 1):
  col = firstCol
  encodedData = []
  # count initial background pixels
  pixel = image.getpixel((col, row))
  backgroundCount = 0
  while pixel == background and col <= lastCol:
    backgroundCount += 1
    col += 1
    if col <= lastCol:
      pixel = image.getpixel((col, row))
  
  # encode ball pixels in row
  totalLength = runLength = 0
  if col < image.width:
    pixel = image.getpixel((col, row))
  while col <= lastCol and pixel != background:
    # find the run
    while pixel == image.getpixel((col, row)) and col <= lastCol:
      col += 1
      runLength += 1
    totalLength += runLength
    while runLength > 0:
      encodedData.append(Encode(runLength, pixel))
      runLength -= 16
    pixel = image.getpixel((col, row))
    runLength = 0
    
  # write the encoded line
  totalEncodedBytes += len(encodedData)
  rowData.append((backgroundCount, totalLength))
  file.write("  ")
  for byte in encodedData:
    file.write(ByteStr(byte) + ", ")
  file.write("\n")
file.write("}; // " + str(totalEncodedBytes) + " bytes\n")


file.write("\n\nstatic const uint8_t ballRows[] PROGMEM =\n{\n")
file.write("// <# pixels to LHS of ball>, <# ball pixels in row>\n")
file.write("//#lhs  #ball\n")
for row in rowData:
  file.write("  " + ByteStr(row[0]) + ", " + ByteStr(row[1]) + ", \n")
file.write("}; // " + str(2*len(rowData)) + " bytes\n")
# insert check
file.write("#if BALL_WIDTH != " + str(lastCol - firstCol + 1) + " || BALL_HEIGHT != " + str(lastRow - firstRow + 1) + "\n")
file.write("#error ball dimensions mismatch!\n")
file.write("#endif\n")
file.close()
sys.stdout.write("Created " + file.name + "\n")
