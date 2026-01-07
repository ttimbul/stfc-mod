#include "virtual_actions.h"

#include "str_utils.h"

namespace VirtualActions
{
std::optional<Action> FromGameFunction(GameFunction fn)
{
  // Default behavior: reuse the currently configured shortcuts for the function.
  // Note: MapKey stores modifier(s) + main key.
  Action out;
  // MapKey::mappedKeys is private; use the public shortcut string to re-parse.
  // If no shortcut is configured, return nullopt.
  const auto shortcuts = MapKey::GetShortcuts(fn);
  if (shortcuts.empty()) {
    return std::nullopt;
  }

  auto parts = StrSplit(AsciiStrToUpper(shortcuts), '|');
  for (auto p : parts) {
    auto mk = MapKey::Parse(p);
    if (mk.Key != KeyCode::None) {
      out.keys.push_back(mk);
    }
  }

  if (out.keys.empty()) {
    return std::nullopt;
  }

  return out;
}
} // namespace VirtualActions
