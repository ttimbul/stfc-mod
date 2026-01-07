#pragma once

#include <string_view>

namespace VirtualInputDrain
{
void Tick();
void TypeTextUtf8(std::string_view utf8);
} // namespace VirtualInputDrain
