#pragma once

#include "patches/gamefunctions.h"

#include <prime/KeyCode.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Thread-safe queue used by the virtual input server.
// Drained on the main thread once per frame.
class VirtualInputQueue
{
public:
  enum class ItemType {
    GameCommand,
    TypeTextUtf8,
    KeyHold,
    MouseClick,
    Deselect,
  };

  struct Item {
    ItemType type;

    // GameCommand
    GameFunction command;

    // TypeTextUtf8
    std::string text;

    // KeyHold
    KeyCode  key;
    uint32_t hold_ms;

    // MouseClick
    float    mouse_x;
    float    mouse_y;
    int32_t  mouse_button;
  };

  static bool EnqueueCommandByName(std::string_view name);
  static void EnqueueCommand(GameFunction fn);

  static void EnqueueTextUtf8(std::string_view utf8);

  // Press key now, release after hold_ms.
  static void EnqueueKeyHold(KeyCode key, uint32_t hold_ms);

  // Click mouse button at position (x, y) for hold_ms duration.
  static void EnqueueMouseClick(float x, float y, int32_t button, uint32_t hold_ms);

  // Hide all visible object viewers
  static void EnqueueDeselect();

  // Called on main thread.
  static std::optional<Item> TryDequeue();
};
