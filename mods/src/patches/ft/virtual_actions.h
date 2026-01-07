#pragma once

#include <EASTL/vector.h>

#include <optional>

#include "patches/gamefunctions.h"
#include "patches/mapkey.h"

namespace VirtualActions
{
struct Action
{
  eastl::vector<MapKey> keys;
};

std::optional<Action> FromGameFunction(GameFunction fn);

} // namespace VirtualActions
