#include "virtual_pan.h"

#include "config.h"
#include "patches/ft/virtual_keyboard.h"

#include <prime/NavigationPan.h>

namespace VirtualPan
{
bool HandleVirtualPan(void* nav_pan_ptr)
{
  if (!Config::Get().installFastTrekExtensions) {
    return false;
  }

  auto _this = static_cast<NavigationPan*>(nav_pan_ptr);

  float vx = 0.0f, vy = 0.0f;
  const float pan_speed = 10.0f * Config::Get().system_pan_momentum_falloff;

  // Camera movement is inverted from key direction
  if (VirtualKeyboard::GetKey(KeyCode::D) || VirtualKeyboard::GetKey(KeyCode::RightArrow)) {
    vx -= pan_speed;
  }
  if (VirtualKeyboard::GetKey(KeyCode::A) || VirtualKeyboard::GetKey(KeyCode::LeftArrow)) {
    vx += pan_speed;
  }
  if (VirtualKeyboard::GetKey(KeyCode::W) || VirtualKeyboard::GetKey(KeyCode::UpArrow)) {
    vy -= pan_speed;
  }
  if (VirtualKeyboard::GetKey(KeyCode::S) || VirtualKeyboard::GetKey(KeyCode::DownArrow)) {
    vy += pan_speed;
  }

  if (vx != 0.0f || vy != 0.0f) {
    _this->MoveCamera(vec2{vx, vy}, false);
    return true;
  }

  return false;
}

} // namespace VirtualPan
