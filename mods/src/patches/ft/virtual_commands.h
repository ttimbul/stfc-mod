#pragma once

#include "patches/gamefunctions.h"

#include <optional>
#include <string_view>

class VirtualCommands
{
public:
  // Parses a stable string name to an enum value.
  // Names are lowercase with underscores, e.g. "show_galaxy", "action_primary".
  static std::optional<GameFunction> ParseGameFunction(std::string_view name);
};
