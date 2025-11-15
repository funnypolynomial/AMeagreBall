#pragma once
#include <Arduino.h>
#include "Boing.h"

// I use an LCD shield so the only pins of interest are
// SDA and SCL for an RTC, if present.
// The shield uses D2-D9 for data and A0-A4 for control.
// Also A2+D8 for touch-x & A3+D9 for touch-y.
// A5 is unused but not accessible under the shield.
// I use the ICSP header and squeeze the RTC between Uno and shield
// with my I-C-S-P-R-T-C breakout board.  See the resources subdirectory

// Note:
// Alternatively, the SD pins are called out on the shield's 18-pin P2/CON1 connector (there's no 5V):
//                  ~10  12 
//  o  o  o  o  o  o  o  o [o] <-- square
//  o  o  o  o  o  o  o  o  o  
//                   13 ~11 GND

#ifdef LCD_SMALL
// *** Small *** (there is no Large)
// touch screen 
#define PIN_RTC_SDA   12
#define PIN_RTC_SCL   11
#endif
