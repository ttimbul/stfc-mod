#include "config.h"
#include "errormsg.h"

#include <patches/ft/virtual_keyboard.h>
#include <il2cpp/il2cpp_helper.h>
#include <spud/detour.h>

// Hooks Unity input icalls to optionally return virtual keyboard state.

static bool Input_GetKeyInt_Hook(auto original, int key)
{
  if (Config::Get().installFastTrekExtensions && VirtualKeyboard::GetKey((KeyCode)key)) {
    return true;
  }
  return original(key);
}

static bool Input_GetKeyDownInt_Hook(auto original, int key)
{
  if (Config::Get().installFastTrekExtensions && VirtualKeyboard::GetKeyDown((KeyCode)key)) {
    return true;
  }
  return original(key);
}

void InstallVirtualKeyboardHooks()
{
  if (auto fn = il2cpp_resolve_icall_typed<bool(int)>("UnityEngine.Input::GetKeyInt(UnityEngine.KeyCode)"); !fn) {
    ErrorMsg::MissingMethod("UnityEngine.Input", "GetKeyInt");
  } else {
    SPUD_STATIC_DETOUR(fn, Input_GetKeyInt_Hook);
  }

  if (auto fn = il2cpp_resolve_icall_typed<bool(int)>("UnityEngine.Input::GetKeyDownInt(UnityEngine.KeyCode)"); !fn) {
    ErrorMsg::MissingMethod("UnityEngine.Input", "GetKeyDownInt");
  } else {
    SPUD_STATIC_DETOUR(fn, Input_GetKeyDownInt_Hook);
  }
}
