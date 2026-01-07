#include "virtual_input_drain.h"

#include "config.h"

#include "patches/key.h"
#include "patches/ft/virtual_input_queue.h"
#include "patches/ft/virtual_keyboard.h"
#include "patches/ft/virtual_mouse.h"
#include "patches/ft/virtual_game_commands.h"
#include "patches/ft/virtual_actions.h"

#include "str_utils.h"
#include "patches/modifierkey.h"

#include <il2cpp-api-types.h>

#include "prime/EventSystem.h"
#include "prime/TMP_InputField.h"

#include <il2cpp/il2cpp_helper.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <vector>

namespace
{
struct HeldKey
{
  KeyCode                               key;
  std::chrono::steady_clock::time_point release_at;
};

struct HeldMouse
{
  int                                   button;
  std::chrono::steady_clock::time_point release_at;
};

static std::vector<HeldKey> held_keys;
static std::vector<HeldMouse> held_mice;
static std::chrono::steady_clock::time_point last_mouse_override;

static void TypeTextUtf8_Impl(std::string_view utf8)
{
  if (utf8.empty()) {
    return;
  }

  // Try to insert text into the focused TMP_InputField using the text property directly
  try {
    if (auto eventSystem = EventSystem::current(); eventSystem) {
      if (auto go = eventSystem->currentSelectedGameObject; go) {
        if (auto input = go->GetComponentFastPath2<TMP_InputField>(); input) {
          if (input->isFocused) {
            static auto tmp_input_helper = il2cpp_get_class_helper("Unity.TextMeshPro", "TMPro", "TMP_InputField");

            // Try using set_text property with simple string creation
            static auto SetText = tmp_input_helper.GetMethod<void(TMP_InputField*, Il2CppString*)>("set_text");
            static auto GetText = tmp_input_helper.GetMethod<Il2CppString*(TMP_InputField*)>("get_text");
            static auto MoveToEnd = tmp_input_helper.GetMethod<void(TMP_InputField*, bool)>("MoveTextEnd");
            
            if (SetText && GetText) {
              auto cur = GetText(input);
              std::string current_text = cur ? to_string(cur) : "";
              std::string new_text = current_text + std::string(utf8);
              
              // Use il2cpp_string_new which handles the string properly
              auto s = il2cpp_string_new(new_text.c_str());
              SetText(input, s);
              
              // Move cursor to end of text
              if (MoveToEnd) {
                MoveToEnd(input, false);
              }
              return;
            }
          }
        }
      }
    }
  } catch (...) {
    // fall through
  }

  // Fallback: set system copy buffer
  static auto set_clip =
      il2cpp_resolve_icall_typed<void(Il2CppString*)>("UnityEngine.GUIUtility::set_systemCopyBuffer(System.String)");
  if (set_clip) {
    auto s = il2cpp_string_new(std::string(utf8).c_str());
    set_clip(s);
  }
}

static bool EnqueueVirtualAction(GameFunction fn)
{
  auto action = VirtualActions::FromGameFunction(fn);
  if (!action.has_value()) {
    return false;
  }

  for (const auto& mk : action->keys) {
    if (mk.Key == KeyCode::None) {
      continue;
    }

    std::vector<KeyCode> pressed_mods;
    for (const auto& mod : mk.Modifiers) {
      auto mod_str   = mod.GetParsedValues();
      auto mod_parts = StrSplit(AsciiStrToUpper(mod_str), '-');
      for (const auto& part : mod_parts) {
        auto kc = Key::Parse(part);
        if (kc == KeyCode::None) {
          continue;
        }
        VirtualKeyboard::KeyDown(kc);
        pressed_mods.push_back(kc);
      }
    }

    VirtualKeyboard::KeyDown(mk.Key);
    VirtualKeyboard::KeyUp(mk.Key);

    for (auto it = pressed_mods.rbegin(); it != pressed_mods.rend(); ++it) {
      VirtualKeyboard::KeyUp(*it);
    }
  }

  return true;
}

static void DrainVirtualInputQueue()
{
  const auto now = std::chrono::steady_clock::now();
  bool mouse_released_this_frame = false;

  for (auto it = held_mice.begin(); it != held_mice.end();) {
    if (now >= it->release_at) {
      VirtualMouse::ReleaseButton(it->button);
      it = held_mice.erase(it);
      mouse_released_this_frame = true;
    } else {
      ++it;
    }
  }

  if (mouse_released_this_frame) {
    last_mouse_override = now;
  }

  if (held_mice.empty()) {
    if (last_mouse_override.time_since_epoch().count() != 0) {
      constexpr auto kMouseHoldGrace = std::chrono::milliseconds(150);
      if (now - last_mouse_override >= kMouseHoldGrace) {
        VirtualMouse::ClearPositionOverride();
        last_mouse_override = {};
      }
    }
  }

  for (auto it = held_keys.begin(); it != held_keys.end();) {
    if (now >= it->release_at) {
      VirtualKeyboard::KeyUp(it->key);
      it = held_keys.erase(it);
    } else {
      // Re-assert key down each frame
      VirtualKeyboard::KeyDown(it->key);
      ++it;
    }
  }

  for (;;) {
    auto item = VirtualInputQueue::TryDequeue();
    if (!item.has_value()) {
      break;
    }

    switch (item->type) {
      case VirtualInputQueue::ItemType::GameCommand:
        (void)EnqueueVirtualAction(item->command);
        break;
      case VirtualInputQueue::ItemType::TypeTextUtf8:
        TypeTextUtf8_Impl(item->text);
        break;
      case VirtualInputQueue::ItemType::KeyHold: {
        VirtualKeyboard::KeyDown(item->key);
        held_keys.push_back(HeldKey{item->key, now + std::chrono::milliseconds(item->hold_ms)});
        break;
      }
      case VirtualInputQueue::ItemType::MouseClick: {
        VirtualMouse::SetPosition(item->mouse_x, item->mouse_y);
        VirtualMouse::PressButton(item->mouse_button);
        held_mice.push_back(HeldMouse{item->mouse_button, now + std::chrono::milliseconds(item->hold_ms)});
        break;
      }
      case VirtualInputQueue::ItemType::Deselect: {
        VirtualGameCommands::Deselect();
        break;
      }
    }
  }
}

} // namespace

namespace VirtualInputDrain
{
void Tick()
{
  if (!Config::Get().virtual_input_server_enabled) {
    return;
  }

  DrainVirtualInputQueue();
}

void TypeTextUtf8(std::string_view utf8)
{
  TypeTextUtf8_Impl(utf8);
}

} // namespace VirtualInputDrain
