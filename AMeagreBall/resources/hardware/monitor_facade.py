#! /usr/bin/env python
'''
Simple monitor-like facade for Uno+touch LCD
a front face (two variations, plain and detailed)
a spacer plate, glued to front plate, with nylon m3 hex standoffs glued facing back
a back clamp plate
spacer plate rests on LCD shield board with 18-pin connector (optional break-away gap)
also rests on USB socket, with a PCB shim (optionally)
small rectangular pads to optionally glue on front face of clamp board to clear soldered pins on underside of Uno
small square pads with hex holes to optionally reinforce glues standoffs
detailed front face deemed too fussy and etched lines almost invisible

3mm White Satin?

NOTE: inksnek.setup() is inksnek.DEVEL, switch to FINAL for cutting

'''

import inkex
import simplestyle, sys, copy
from math import *
from inksnek import *
import operator

# plates:
FRONT_DETAIL, MIDDLE_SPACER, BACK_CLAMP, FRONT_PLAIN = 0,1,2,3

class MyDesign(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)

  def addStandoff(self, group, x, y, plate):
    # add/show hex cutout and m2 hole
    holeStyle = inksnek.ignore_style if plate == 1 else inksnek.cut_style
    hexStyle = inksnek.cut_style if plate == 1 else inksnek.ignore_style
    holeDiameter = 3.0 #if plate == 1 else 3.3 # add some slack? -- no
    inksnek.add_circle(group, x, y, holeDiameter/2.0, holeStyle)
    hex = inksnek.add_group(group, inksnek.translate_group(x, y))
    path = inksnek.path_move_to(inksnek.polar_to_rectangular(self.hex_radius, 30))
    for a in range(6):
        path += inksnek.path_line_to(inksnek.polar_to_rectangular(self.hex_radius, 30 + a*60))
    path += inksnek.path_close()
    inksnek.add_path(hex, path, hexStyle)
      
      
  def addPlate(self, group, x, y, plate):
    # add the given plate
    group = inksnek.add_group(group, inksnek.translate_group(x, y))
    inksnek.add_round_rect(group, 0, 0, self.totalWidth, self.totalHeight, self.surroundRadius, inksnek.cut_style)
    if plate != BACK_CLAMP:
        inksnek.add_rect(group, self.surroundWidth, self.surroundHeightBottom, self.lcdWidth, self.lcdHeight, inksnek.cut_style) # lcd cutour
    if plate != FRONT_DETAIL and plate != FRONT_PLAIN:
        # holes for standoffs 15mm, diameter 5.48 (flat-flat) 6.27mm (corner-corner)
        lowerY = self.surroundHeightBottom - self.surroundHeightTop/2.0 # closer to LCD/Uno, symmetric
        self.addStandoff(group, self.surroundWidth, lowerY, plate)
        self.addStandoff(group, self.totalWidth - self.surroundWidth, lowerY, plate)
        self.addStandoff(group, self.surroundWidth, self.totalHeight - self.surroundHeightTop/2.0, plate)
        self.addStandoff(group, self.totalWidth - self.surroundWidth, self.totalHeight - self.surroundHeightTop/2.0, plate)
    if plate == BACK_CLAMP:
        # notch for power at right-angles
        path = inksnek.path_move_to(0, self.surroundHeightBottom + self.powerOffsetY - self.powerR1 - self.powerR2)
        path += inksnek.path_round_by(+self.powerR2, +self.powerR2, self.powerR2)
        path += inksnek.path_horz_by(self.surroundHeightBottom - self.powerR1 - self.powerR2)
        path += inksnek.path_round_by(+self.powerR1, +self.powerR1, -self.powerR1)
        path += inksnek.path_round_by(-self.powerR1, +self.powerR1, -self.powerR1)
        path += inksnek.path_horz_by(-(self.surroundHeightBottom - self.powerR1 - self.powerR2))
        path += inksnek.path_round_by(-self.powerR2, +self.powerR2, self.powerR2)
        inksnek.add_path(group, path, inksnek.cut_style)
    if plate == MIDDLE_SPACER:
        # gap for connector?
        path = inksnek.path_move_to(self.totalWidth - self.surroundWidth,  self.surroundHeightBottom + self.connectorOffsetY)
        path += inksnek.path_horz_by(self.surroundWidth - self.connectorOffsetX)
        # optional break-away:
        off = 1.0
        on = (self.connectorHeight - 2.0*off)/3.0
        path += inksnek.path_vert_by(on)
        path += inksnek.path_move_by(0.0, off)
        path += inksnek.path_vert_by(on)
        path += inksnek.path_move_by(0.0, off)
        path += inksnek.path_vert_by(on)
        path += inksnek.path_horz_by(-(self.surroundWidth - self.connectorOffsetX))
        inksnek.add_path(group, path, inksnek.cut_style)
        # pads
        w, h = 20.0, 5.0
        for pad in range(4):
            inksnek.add_rect(group, self.surroundWidth, self.surroundHeightBottom + pad*h, w, h, inksnek.cut_style, "TR")
        # standoff reinforcements
        gap = 3.0
        d = 2.0*self.hex_radius + gap
        for pad in range(4):
            g = inksnek.add_group(group, inksnek.translate_group(self.surroundWidth + w + (pad + 1)*d, self.surroundHeightBottom + d/2))
            inksnek.add_rect(g, -d/2.0, -d/2.0, d, d - gap/3.0, inksnek.cut_style, "LT" if pad != 3 else "LTR")
            self.addStandoff(g, 0, 0, MIDDLE_SPACER)
        # gap for USB (breakaway)
        #inksnek.add_rect(group, 0, self.totalHeight - self.surroundHeightTop - self.USBOffsetY, self.USBWidth, -self.USBHeight, inksnek.cut_style, "TRB")
        on = -(self.USBHeight - 2.0*off)/3.0
        off = -off
        path = inksnek.path_move_to(0, self.totalHeight - self.surroundHeightTop - self.USBOffsetY)
        path += inksnek.path_horz_by(self.USBWidth)
        path += inksnek.path_vert_by(on)
        path += inksnek.path_move_by(0.0, off)
        path += inksnek.path_vert_by(on)
        path += inksnek.path_move_by(0.0, off)
        path += inksnek.path_vert_by(on)
        path += inksnek.path_horz_by(-self.USBWidth)
        inksnek.add_path(group, path, inksnek.cut_style)
    if plate == FRONT_DETAIL:
        if self.controls: # "controls" panel
            offsY = self.surroundHeightBottom - self.surroundHeightTop
            offsX = 2.0*offsY
            inksnek.add_line_by(group, 0.0, offsY, self.totalWidth, 0.0, inksnek.etch_style)
            inksnek.add_line_by(group, offsX, 0.0, 0.0, offsY, inksnek.etch_style)
            inksnek.add_line_by(group, self.totalWidth - offsX, 0.0, 0.0, offsY, inksnek.etch_style)
        if self.innerLineOffs != 0.0: # surround/bezel detail
          x0 = self.surroundWidth - self.innerLineOffs
          y0 = self.surroundHeightBottom - self.innerLineOffs
          r0 = self.surroundRadius*(1.0 - x0/self.surroundWidth) # pro-rate radius
          inksnek.add_round_rect(group, x0, y0, self.lcdWidth + 2.0*self.innerLineOffs, self.lcdHeight + 2.0*self.innerLineOffs, r0, inksnek.etch_style)
          
  def effect(self):
    inksnek.setup(self, inksnek.A4, inksnek.ACRYLIC, 3.0, 'mm', inksnek.DEVEL) ## switch to FINAL to cut

    self.controls = True
    self.innerLineOffs = 4.0 ## 0.0 or offset out from LCD 
    self.lcdWidth = 72.0
    self.lcdHeight = 52.0
    self.surroundWidth = 10.0
    self.surroundHeightTop = 10.0
    self.surroundHeightBottom = 15.0 if self.controls else self.surroundHeightTop
    self.surroundRadius = 5.0
    self.totalWidth = self.lcdWidth + 2.0*self.surroundWidth
    self.totalHeight = self.lcdHeight + self.surroundHeightBottom + self.surroundHeightTop
    self.powerOffsetY = 8.0 # from LCD corner to middle of notch
    self.powerR1 = 6.0
    self.powerR2 = 2.0
    self.USBOffsetY = 6.0 # from top of LCD
    self.USBHeight = 15.0
    self.USBWidth = 5.0
    self.hex_radius = 6.27/2.0
     # void for 18-pin connector, from LCD corner
    self.connectorOffsetX = 2.0 # from edge
    self.connectorOffsetY = 12.0
    self.connectorHeight = 15.0
    design = inksnek.add_group(inksnek.top_group, inksnek.translate_group(10, 10))
    self.addPlate(design, 0, 0, FRONT_DETAIL)
    self.addPlate(design, 0, self.totalHeight + 2.0, MIDDLE_SPACER)
    self.addPlate(design, self.totalWidth + 2.0, 0, BACK_CLAMP)
    self.addPlate(design, self.totalWidth + 2.0, self.totalHeight + 2.0, FRONT_PLAIN)
