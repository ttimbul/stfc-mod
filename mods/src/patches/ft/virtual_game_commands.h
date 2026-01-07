#pragma once

#include "patches/gamefunctions.h"

namespace VirtualGameCommands
{
void Execute(GameFunction fn);

// Hide all visible object viewers (PreScan, Mining, Armada, etc.)
// Returns true if any viewers were hidden
bool Deselect();

} // namespace VirtualGameCommands
