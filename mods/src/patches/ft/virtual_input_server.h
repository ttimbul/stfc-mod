#pragma once

#include <cstdint>

// Minimal localhost REST server that enqueues items into VirtualInputQueue.
namespace VirtualInputServer
{
// Starts background listener thread (idempotent). Port 0 => random ephemeral port.
void Start(uint16_t port);

// Returns the bound port after Start(), or 0 if not started/failed.
uint16_t Port();
} // namespace VirtualInputServer
