#pragma once

namespace VirtualPan
{
// Called from NavigationPan::LateUpdate hook to handle virtual keyboard panning
// Returns true if virtual panning was handled, false to continue with normal processing
bool HandleVirtualPan(void* navigation_pan);

} // namespace VirtualPan
