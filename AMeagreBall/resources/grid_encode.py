#!/usr/bin/python
import sys
import math
from PIL import Image, ImageDraw
# https://pillow.readthedocs.io/en/stable/reference/ImageDraw.html

# make and encode the back wall and floor of the Amiga Boing grid
# do most of the calculations ahead of time

def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()

def WriteRun(file, length, background):
  bytes = 0
  flag = 128 if background else 0
  while length > 0:
    if length > 127:
      bytes += 1
      file.write(ByteStr(127 + flag) + ", ")
      length -= 127
    else:
      bytes += 1
      file.write(ByteStr(length + flag) + ", ")
      length = 0
  return bytes


# these must match sketch
LUT_GRIDLINE_FLAG = 0x80
LCD_WIDTH      = 320
GRID_CELL_SIZE = 16
GRID_COLS      = 15
GRID_ROWS      = 12  

def EncodeBackWall(file):
  file.write("#define LUT_GRIDLINE_FLAG " + ByteStr(LUT_GRIDLINE_FLAG) + "\n")
  gridVerticalsLUT = [0]*LCD_WIDTH
  gridHorizontalsLUT = [0]*LCD_WIDTH
  # build LUT(s)
  # vertical lines
  LHS = (LCD_WIDTH - (GRID_COLS * GRID_CELL_SIZE + 1))/2
  RHS = LHS + GRID_COLS * GRID_CELL_SIZE + 1
  # leading background
  for col in range(LHS):
    gridVerticalsLUT[col] = LHS - col
  ctr = 0;
  # verticals
  for col in range(LHS, RHS):
    if ctr:
      gridVerticalsLUT[col] = GRID_CELL_SIZE - ctr  # background
    else:
      gridVerticalsLUT[col] = LUT_GRIDLINE_FLAG + 1 # 1 line pixel
    ctr += 1
    if ctr >= GRID_CELL_SIZE:
      ctr = 0
  # trailing background
  for col in range(RHS, LCD_WIDTH):
    gridVerticalsLUT[col] = LCD_WIDTH - col

  # horizontal lines
  # leading background
  for col in range(LHS):
    gridHorizontalsLUT[col] = LHS - col
  # horizontal line
  for col in range(LHS, RHS):
    len = RHS - col;
    if len > (LUT_GRIDLINE_FLAG - 1): # pegged at 127
      len = LUT_GRIDLINE_FLAG - 1
    gridHorizontalsLUT[col] = LUT_GRIDLINE_FLAG + len
  
  # trailing background
  for col in range(RHS, LCD_WIDTH):
    gridHorizontalsLUT[col] = LCD_WIDTH - col
  
  file.write("// LUT[col] is run of pixels starting at col.  If LUT_GRIDLINE_FLAG is set colour is gridline, else background\n")
  file.write("// gridVerticalsLUT is a row with vertical grid lines, gridHorizontalsLUT is a row with horizontal grid lines\n")
  file.write("static const byte gridVerticalsLUT[] PROGMEM = {\n")
  file.write("  ")
  for col in range(LCD_WIDTH):
    if col:
      file.write(",")
    if col and not(col % 64):
      file.write("\n  ")
    file.write(ByteStr(gridVerticalsLUT[col]))
  file.write("\n};  // (" + str(LCD_WIDTH) + " bytes) \n\n")
  
  
  file.write("static const byte gridHorizontalsLUT[] PROGMEM = {\n")
  file.write("  ")
  for col in range(LCD_WIDTH):
    if col:
      file.write(",")
    if col and not (col % 64):
      file.write("\n  ")
    file.write(ByteStr(gridHorizontalsLUT[col]))
  file.write("\n};  // (" + str(LCD_WIDTH) + " bytes) \n\n")


pngName = "grid_image.png"
fileName = "GridData.h"
width, height = 320, 240
dX = 9 # centre it!
dY = 17 #lower it
firstRow = 192 + dY
lastRow = 215 + dY

# -------------- make the floor image
grid = Image.new("RGB", (width, height), "black")
canvas = ImageDraw.Draw(grid)

topX = 48
botX = 20

while topX < 300: # columns
  canvas.line([(topX - dX, firstRow), (botX - dX, lastRow)], "white")
  topX += 16
  botX += 20
# rows
canvas.line([(45 - dX, 194 + dY), (291 - dX, 194 + dY)], "white")
canvas.line([(41 - dX, 197 + dY), (295 - dX, 197 + dY)], "white")
canvas.line([(37 - dX, 201 + dY), (300 - dX, 201 + dY)], "white")
canvas.line([(30 - dX, 207 + dY), (308 - dX, 207 + dY)], "white")
canvas.line([(20 - dX, 215 + dY), (319 - dX, 215 + dY)], "white")
 
#grid.show()
grid.save(pngName)
sys.stdout.write("Created " + pngName + "\n")

file = open(fileName, "w")
file.write("// (created by grid_encode.py)\n")
#encode the back wall
EncodeBackWall(file)

#encode the floor image
#          LHS    grid      RHS
rowBytes = 1 + (15*2 + 1) + 1
file.write("// Encoded grid floor lines. More compact encoding than the back wall.\n")
file.write("#define GRID_FLOOR_FIRST_ROW " + str(firstRow) + "\n")
file.write("#define GRID_FLOOR_LAST_ROW  " + str(lastRow) + "\n")
file.write("#define GRID_FLOOR_ROW_BYTES " + str(rowBytes) + "\n")
file.write("static const uint8_t gridFloorRows[] PROGMEM =\n{\n")
file.write("// <GRID_FLOOR_ROW_BYTES> bytes, 0bPNNNNNNN where P=0/1:foreground/background, N=len. Padded with 0's\n")
background = (0, 0, 0)
for row in range(firstRow, lastRow + 1):
  prevPixel = (255, 0, 0)
  length = 0
  file.write("  ")
  bytesWritten = 0
  for col in range(width):
    pixel = grid.getpixel((col, row))
    if pixel == prevPixel:
        length += 1
    else:
      bytesWritten += WriteRun(file, length, prevPixel == background)
      prevPixel = pixel
      length = 1
  bytesWritten += WriteRun(file, length, prevPixel == background)
  while bytesWritten < rowBytes: # pad row
    file.write(ByteStr(0) + ", ")
    bytesWritten += 1
  file.write("\n")
file.write("};  // (" + str(rowBytes*(lastRow - firstRow + 1)) + " bytes) \n")
# insert check
file.write("#if GRID_CELL_SIZE != " + str(GRID_CELL_SIZE) + " || GRID_COLS != " + str(GRID_COLS) + " || GRID_ROWS != " + str(GRID_ROWS) + "\n")
file.write("#error grid dimensions mismatch!\n")
file.write("#endif\n")

file.close()
sys.stdout.write("Created " + fileName + "\n")
