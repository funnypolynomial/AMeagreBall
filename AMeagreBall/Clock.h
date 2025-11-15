#pragma once

// Clock windows
namespace Clock
{
  extern bool _hidden;
  void Init();
  void Loop();
  void SetStyle(Config::ClockStyle style);
  void Draw();
  void Update();
  void Hide(bool hide);
};
