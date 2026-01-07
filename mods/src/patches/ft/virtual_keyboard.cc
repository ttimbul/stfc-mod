#include "virtual_keyboard.h"

VirtualKeyboard& VirtualKeyboard::Get()
{
  static VirtualKeyboard vk;
  return vk;
}

VirtualKeyboard::VirtualKeyboard() = default;

void VirtualKeyboard::NextFrame()
{
  auto& vk = Get();
  for (auto& k : vk.keys_) {
    k.downEdge.store(false, std::memory_order_relaxed);
  }
}

void VirtualKeyboard::KeyDown(KeyCode key)
{
  const auto idx = (size_t)key;
  if (idx >= Get().keys_.size()) {
    return;
  }

  auto& k = Get().keys_[idx];
  const auto was_held = k.held.exchange(true, std::memory_order_acq_rel);
  if (!was_held) {
    k.downEdge.store(true, std::memory_order_release);
  }
}

void VirtualKeyboard::KeyUp(KeyCode key)
{
  const auto idx = (size_t)key;
  if (idx >= Get().keys_.size()) {
    return;
  }

  Get().keys_[idx].held.store(false, std::memory_order_release);
}

bool VirtualKeyboard::GetKey(KeyCode key)
{
  const auto idx = (size_t)key;
  if (idx >= Get().keys_.size()) {
    return false;
  }

  return Get().keys_[idx].held.load(std::memory_order_acquire);
}

bool VirtualKeyboard::GetKeyDown(KeyCode key)
{
  const auto idx = (size_t)key;
  if (idx >= Get().keys_.size()) {
    return false;
  }

  return Get().keys_[idx].downEdge.load(std::memory_order_acquire);
}
