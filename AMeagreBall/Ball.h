#pragma once

// The Ball animation and logic
namespace Ball
{
  #define BALL_WIDTH  93 // must match BallData.h
  #define BALL_HEIGHT 93
  void Init();
  void Splash();
  void Start();
  void Step();
  void SetDim();
  void RestoreColours();
  void Redraw();
  void RepaintBackground();
  void ClearBackground(int x, int y, int w, int h);
  void SetClip(int x, int y, int w, int h);
  void SetClip(int row);
  void ClearClip();
}
