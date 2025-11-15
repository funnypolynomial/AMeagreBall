#!/usr/bin/python
import sys
import math
from PIL import Image, ImageDraw
# https://pillow.readthedocs.io/en/stable/reference/ImageDraw.html

# make an Amiga-boing-style ball using 16 colour palette
# see ball_encode.py

def rotateX(x, y, rad):
    return x*math.cos(rad) +y*math.sin(rad)
    
def rotateY(x, y, rad):
    return -x*math.sin(rad) +y*math.cos(rad)
    
def node(vectorX, vectorY):
    # return (vectorX, vectorY) rotated by tilt about (midX, midY)
    return (midX + rotateX(vectorX, vectorY, tilt), midY + rotateY(vectorX, vectorY, tilt))

def colour(band, face):
    # return colour with palette index for the given band+face
    component = 128 + 16 *(face % 8)
    if band % 2:
      paletteIdx = 2*subFaces - ((face + subFaces) % (2*subFaces)) - 1
    else:
      paletteIdx = 2*subFaces - (face % (2*subFaces)) - 1
    if (band + face/subFaces) % 2:
        return (component, 0, paletteIdx)
    else:
        return (component, component, 240 + paletteIdx)
        
pngname = "ball_image.png"
size = 95
width, height = size, size
sides = 16
tilt = math.radians(-20)

ball = Image.new("RGB", (width, height), "black")

bands = 8
subFaces = 8 # colours within a face
faces = 7*subFaces
R = size/2.0 - 1
bandAngle = math.radians(180.0/bands) # angle subtended by a band
faceAngle = math.radians(180.0/faces)# angle subtended by a face
midX, midY = width/2.0, height/2.0
canvas = ImageDraw.Draw(ball)
# top-left is (0,0)
for band in range(bands/2):
  bandRLo = R*math.cos(band*bandAngle) # radius at base of band
  bandRHi = R*math.cos((band + 1)*bandAngle) # radius at top of band
  yLo = R*math.sin(band*bandAngle) # base, from equator
  yHi = R*math.sin((band + 1)*bandAngle) # top, from equator
  for face in range(faces):
    x0Lo = bandRLo*math.cos(face*faceAngle)
    x1Lo = bandRLo*math.cos((face + 1)*faceAngle)
    x0Hi = bandRHi*math.cos(face*faceAngle)
    x1Hi = bandRHi*math.cos((face + 1)*faceAngle)
    #  D-<-C  Hi (but upside down!)
    #  :   |
    #  A->-B  Lo
    # --------
    #  0  1
    A = node(x0Lo, yLo)
    B = node(x1Lo, yLo)
    C = node(x1Hi, yHi)
    D = node(x0Hi, yHi)
    canvas.polygon([A, B, C, D], colour(band, face))
    # --------
    #  A->-B  Lo
    #  :   |
    #  D-<-C  Hi
    #  0  1
    A = node(x0Lo, -yLo)
    B = node(x1Lo, -yLo)
    C = node(x1Hi, -yHi)
    D = node(x0Hi, -yHi)
    canvas.polygon([A, B, C, D], colour(band + 1, face))
 
#ball.show()
ball.save(pngname)
sys.stdout.write("Created " + pngname + "\n")