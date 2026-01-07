#include "virtual_mouse.h"

VirtualMouse& VirtualMouse::Get()
{
  static VirtualMouse vm;
  return vm;
}

VirtualMouse::VirtualMouse() = default;

void VirtualMouse::NextFrame()
{
  auto& vm = Get();
  for (auto& b : vm.buttons_) {
    b.downEdge.store(false, std::memory_order_relaxed);
    b.upEdge.store(false, std::memory_order_relaxed);
  }
}

void VirtualMouse::SetPosition(float x, float y)
{
  auto& vm = Get();
  vm.x_.store(x, std::memory_order_relaxed);
  vm.y_.store(y, std::memory_order_relaxed);
  vm.has_pos_.store(true, std::memory_order_release);
}

void VirtualMouse::PressButton(int button)
{
  if (button < 0 || button >= (int)Get().buttons_.size()) {
    return;
  }

  auto& b = Get().buttons_[(size_t)button];
  const auto was_held = b.held.exchange(true, std::memory_order_acq_rel);
  if (!was_held) {
    b.downEdge.store(true, std::memory_order_release);
  }
}

void VirtualMouse::ReleaseButton(int button)
{
  if (button < 0 || button >= (int)Get().buttons_.size()) {
    return;
  }

  auto& b = Get().buttons_[(size_t)button];
  const auto was_held = b.held.exchange(false, std::memory_order_acq_rel);
  if (was_held) {
    b.upEdge.store(true, std::memory_order_release);
  }
}

bool VirtualMouse::HasPositionOverride()
{
  return Get().has_pos_.load(std::memory_order_acquire);
}

void VirtualMouse::ReadPosition(Vector3* out)
{
  if (!out) {
    return;
  }

  auto& vm = Get();
  out->x   = vm.x_.load(std::memory_order_relaxed);
  out->y   = vm.y_.load(std::memory_order_relaxed);
  out->z   = 0.0f;
}

bool VirtualMouse::GetMouseButton(int button)
{
  if (button < 0 || button >= (int)Get().buttons_.size()) {
    return false;
  }

  return Get().buttons_[(size_t)button].held.load(std::memory_order_acquire);
}

bool VirtualMouse::GetMouseButtonDown(int button)
{
  if (button < 0 || button >= (int)Get().buttons_.size()) {
    return false;
  }

  return Get().buttons_[(size_t)button].downEdge.load(std::memory_order_acquire);
}

bool VirtualMouse::GetMouseButtonUp(int button)
{
  if (button < 0 || button >= (int)Get().buttons_.size()) {
    return false;
  }

  return Get().buttons_[(size_t)button].upEdge.load(std::memory_order_acquire);
}

void VirtualMouse::ClearPositionOverride()
{
  auto& vm = Get();
  vm.has_pos_.store(false, std::memory_order_release);
}
