#include "config.h"
#include "errormsg.h"

#include <patches/ft/virtual_mouse.h>
#include <patches/ft/virtual_input_server.h>

#include <il2cpp/il2cpp_helper.h>

#include <spud/detour.h>

// Hooks Unity input icalls to optionally return virtual mouse state.
// This is intended to support remote click injection without requiring OS focus.

static bool Input_GetMouseButton_Hook(auto original, int button)
{
  if (Config::Get().virtual_input_server_enabled && VirtualMouse::GetMouseButton(button)) {
    return true;
  }

  return original(button);
}

static bool Input_GetMouseButtonDown_Hook(auto original, int button)
{
  if (Config::Get().virtual_input_server_enabled && VirtualMouse::GetMouseButtonDown(button)) {
    return true;
  }

  return original(button);
}

static bool Input_GetMouseButtonUp_Hook(auto original, int button)
{
  if (Config::Get().virtual_input_server_enabled && VirtualMouse::GetMouseButtonUp(button)) {
    return true;
  }

  return original(button);
}

static void Input_get_mousePosition_Injected_Hook(auto original, Vector3* out)
{
  if (Config::Get().virtual_input_server_enabled && VirtualMouse::HasPositionOverride()) {
    VirtualMouse::ReadPosition(out);
    return;
  }

  return original(out);
}

void InstallVirtualMouseHooks()
{
  if (Config::Get().virtual_input_server_enabled) {
    VirtualInputServer::Start((uint16_t)Config::Get().virtual_input_server_port);
  }

  // Mouse buttons
  if (auto fn = il2cpp_resolve_icall_typed<bool(int)>("UnityEngine.Input::GetMouseButton(System.Int32)"); !fn) {
    ErrorMsg::MissingMethod("UnityEngine.Input", "GetMouseButton");
  } else {
    SPUD_STATIC_DETOUR(fn, Input_GetMouseButton_Hook);
  }

  if (auto fn = il2cpp_resolve_icall_typed<bool(int)>("UnityEngine.Input::GetMouseButtonDown(System.Int32)"); !fn) {
    // Not all backends expose this icall; ignore if missing.
  } else {
    SPUD_STATIC_DETOUR(fn, Input_GetMouseButtonDown_Hook);
  }

  if (auto fn = il2cpp_resolve_icall_typed<bool(int)>("UnityEngine.Input::GetMouseButtonUp(System.Int32)"); !fn) {
    // Not all backends expose this icall; ignore if missing.
  } else {
    SPUD_STATIC_DETOUR(fn, Input_GetMouseButtonUp_Hook);
  }

  // Mouse position
  if (auto fn = il2cpp_resolve_icall_typed<void(Vector3*)>(
          "UnityEngine.Input::get_mousePosition_Injected(UnityEngine.Vector3&)");
      !fn) {
    ErrorMsg::MissingMethod("UnityEngine.Input", "get_mousePosition_Injected");
  } else {
    SPUD_STATIC_DETOUR(fn, Input_get_mousePosition_Injected_Hook);
  }
}
