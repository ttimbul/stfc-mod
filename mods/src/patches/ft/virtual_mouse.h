#pragma once

#include <prime/KeyCode.h>
#include <prime/Vector3.h>

#include <array>
#include <atomic>
#include <cstdint>

class VirtualMouse
{
public:
  static VirtualMouse& Get();

  // Called once per frame (e.g. from ScreenManager_Update_Hook) to advance edge state.
  static void NextFrame();

  // Set absolute mouse position in Unity screen coordinates (pixels).
  // This does not move the OS cursor.
  static void SetPosition(float x, float y);
  static void ClearPositionOverride();

  // Convenience helpers for common mouse buttons.
  static void PressButton(int button);   // 0=left,1=right,2=middle
  static void ReleaseButton(int button);

  // Queries used by hooks.
  static bool HasPositionOverride();
  static void ReadPosition(Vector3* out);

  static bool GetMouseButton(int button);
  static bool GetMouseButtonDown(int button);
  static bool GetMouseButtonUp(int button);

private:
  VirtualMouse();

  struct ButtonState {
    std::atomic<bool> held{false};
    std::atomic<bool> downEdge{false};
    std::atomic<bool> upEdge{false};
  };

  std::array<ButtonState, 8> buttons_{};

  std::atomic<bool>  has_pos_{false};
  std::atomic<float> x_{0.0f};
  std::atomic<float> y_{0.0f};
};
