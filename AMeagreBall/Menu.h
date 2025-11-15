#pragma once

namespace Menu
{
  typedef char* (*LiveItemFunc)(int);
  int Select(int x, int y, const char* pStrings_P, uint16_t ticked, uint16_t disabled, uint16_t labels = 0, LiveItemFunc live = NULL);
  void Erase();
}