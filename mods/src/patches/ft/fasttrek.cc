#include "config.h"
#include "errormsg.h"

#include "spud/detour.h"

void InstallVirtualMouseHooks();
void InstallVirtualKeyboardHooks();

void InstallFastTrekExtensions()
{
  if (Config::Get().virtual_input_server_enabled) {
    InstallVirtualMouseHooks();
    InstallVirtualKeyboardHooks();
  }
}
