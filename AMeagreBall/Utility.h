#pragma once

// Variuous utilities
namespace Utility
{
  // Multiple strings in a single PROGMEM const char*
  #undef MSTR
  #define MSTR(_s) _s "\0"
  const char* GetMString(const char* pMultiStr, uint8_t n);

  void Format(uint8_t NN, char* pBuff, char lead, bool NUL = false);
  
  int32_t rsint(int32_t r, int32_t t);
  int32_t rcost(int32_t r, int32_t t);
  
  void FillCircle(int centreX, int centreY, int innerRadius, int thickness);
  
  #define KITE_MAX_ROWS 50 // Should be >= kAnalogClockRadius!
  struct tKite
  {
    int _xOrg = 0xFF;
    int _yOrg = 0xFF;
    byte _rowLHS[KITE_MAX_ROWS];
    byte _rowRHS[KITE_MAX_ROWS];
  };

  void MakeKite(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, tKite& kite);
  void FillKite(tKite& kite, byte colour);
  
  bool GetStableTouch(int& touchX, int& touchY);
}
