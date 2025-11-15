#pragma once

// Draw "workbench" windows etc
namespace Workbench
{
  const int kFrameTopThickness = 20;  // title bar height
  enum WindowType { Workbench,        // Full-width window with no Close
                    App,              // Window with Close 
                    DitheredFolder,   // App Window with dithered title
                    WorkbenchFast,    // Plain Workbench window for moving
                    AppFast };        // Plain App window for moving

  enum WindowHit { NoHit,             // No touch
                   TitleHit,          // Touch on title (move)
                   SendBehindHit,     // Touch on send behind (hide)
                   BodyHit };         // Touch on body (hide)
  void Init();
  void DrawWindowFrame(int x, int y, int width, int height, WindowType type, int* pClientX = NULL, int* pClientY = NULL, int* pClientW = NULL, int* pClientH = NULL);
  void DrawWindowTitle(int x, int y, const char* pTitle, WindowType type, bool progmem = false);
  WindowHit HitTest(int touchX, int touchY, int x, int y, int w, int h, WindowType type);
  void DrawBallIcon(int x, int y);
};
