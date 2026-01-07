#pragma once

#include <prime/KeyCode.h>

#include <array>
#include <atomic>
#include <cstddef>

class VirtualKeyboard
{
public:
  static VirtualKeyboard& Get();

  // Called once per frame to clear edge state.
  static void NextFrame();

  // Event-like API (suitable for a server to call)
  static void KeyDown(KeyCode key);
  static void KeyUp(KeyCode key);

  // Queries used by Key helpers.
  static bool GetKey(KeyCode key);
  static bool GetKeyDown(KeyCode key);

private:
  VirtualKeyboard();

  struct KeyState {
    std::atomic<bool> held{false};
    std::atomic<bool> downEdge{false};
  };

  std::array<KeyState, (size_t)KeyCode::Max> keys_{};
};
