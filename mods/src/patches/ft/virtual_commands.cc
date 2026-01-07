#include "virtual_commands.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

static std::string normalize(std::string_view s)
{
  std::string out;
  out.reserve(s.size());

  for (const char c : s) {
    if (c == '-' || c == ' ') {
      out.push_back('_');
    } else {
      out.push_back((char)std::tolower((unsigned char)c));
    }
  }

  return out;
}

std::optional<GameFunction> VirtualCommands::ParseGameFunction(std::string_view name)
{
  static const std::unordered_map<std::string, GameFunction> map = {
      {"move_left", GameFunction::MoveLeft},
      {"move_right", GameFunction::MoveRight},
      {"move_up", GameFunction::MoveUp},
      {"move_down", GameFunction::MoveDown},

      {"select_chatalliance", GameFunction::SelectChatAlliance},
      {"select_chatglobal", GameFunction::SelectChatGlobal},
      {"select_chatprivate", GameFunction::SelectChatPrivate},

      {"select_ship1", GameFunction::SelectShip1},
      {"select_ship2", GameFunction::SelectShip2},
      {"select_ship3", GameFunction::SelectShip3},
      {"select_ship4", GameFunction::SelectShip4},
      {"select_ship5", GameFunction::SelectShip5},
      {"select_ship6", GameFunction::SelectShip6},
      {"select_ship7", GameFunction::SelectShip7},
      {"select_ship8", GameFunction::SelectShip8},
      {"select_current", GameFunction::SelectCurrent},

      {"show_alliance", GameFunction::ShowAlliance},
      {"show_alliance_armada", GameFunction::ShowAllianceArmada},
      {"show_alliance_help", GameFunction::ShowAllianceHelp},
      {"show_artifacts", GameFunction::ShowArtifacts},
      {"show_officers", GameFunction::ShowOfficers},
      {"show_commander", GameFunction::ShowCommander},
      {"show_refinery", GameFunction::ShowRefinery},
      {"show_qtrials", GameFunction::ShowQTrials},
      {"show_bookmarks", GameFunction::ShowBookmarks},
      {"show_lookup", GameFunction::ShowLookup},
      {"show_exocomp", GameFunction::ShowExoComp},
      {"show_factions", GameFunction::ShowFactions},
      {"show_gifts", GameFunction::ShowGifts},
      {"show_daily", GameFunction::ShowDaily},
      {"show_awayteam", GameFunction::ShowAwayTeam},
      {"show_missions", GameFunction::ShowMissions},
      {"show_research", GameFunction::ShowResearch},
      {"show_scrapyard", GameFunction::ShowScrapYard},
      {"show_ships", GameFunction::ShowShips},
      {"show_inventory", GameFunction::ShowInventory},
      {"show_stationinterior", GameFunction::ShowStationInterior},
      {"show_stationexterior", GameFunction::ShoWStationExterior},
      {"show_galaxy", GameFunction::ShowGalaxy},
      {"show_system", GameFunction::ShowSystem},
      {"show_chat", GameFunction::ShowChat},
      {"show_chatside1", GameFunction::ShowChatSide1},
      {"show_chatside2", GameFunction::ShowChatSide2},
      {"show_events", GameFunction::ShowEvents},
      {"show_settings", GameFunction::ShowSettings},

      {"zoom_preset1", GameFunction::ZoomPreset1},
      {"zoom_preset2", GameFunction::ZoomPreset2},
      {"zoom_preset3", GameFunction::ZoomPreset3},
      {"zoom_preset4", GameFunction::ZoomPreset4},
      {"zoom_preset5", GameFunction::ZoomPreset5},
      {"zoom_in", GameFunction::ZoomIn},
      {"zoom_out", GameFunction::ZoomOut},
      {"zoom_min", GameFunction::ZoomMin},
      {"zoom_max", GameFunction::ZoomMax},
      {"zoom_reset", GameFunction::ZoomReset},

      {"ui_scaleup", GameFunction::UiScaleUp},
      {"ui_scaledown", GameFunction::UiScaleDown},
      {"ui_scaleviewerup", GameFunction::UiViewerScaleUp},
      {"ui_scaleviewerdown", GameFunction::UiViewerScaleDown},

      {"action_primary", GameFunction::ActionPrimary},
      {"action_secondary", GameFunction::ActionSecondary},
      {"action_queue", GameFunction::ActionQueue},
      {"action_queue_clear", GameFunction::ActionQueueClear},
      {"action_view", GameFunction::ActionView},
      {"action_recall", GameFunction::ActionRecall},
      {"action_recall_cancel", GameFunction::ActionRecallCancel},
      {"action_repair", GameFunction::ActionRepair},

      {"set_zoom_preset1", GameFunction::SetZoomPreset1},
      {"set_zoom_preset2", GameFunction::SetZoomPreset2},
      {"set_zoom_preset3", GameFunction::SetZoomPreset3},
      {"set_zoom_preset4", GameFunction::SetZoomPreset4},
      {"set_zoom_preset5", GameFunction::SetZoomPreset5},
      {"set_zoom_default", GameFunction::SetZoomDefault},

      {"disable_hotkeys", GameFunction::DisableHotKeys},
      {"enable_hotkeys", GameFunction::EnableHotKeys},

      {"toggle_queue", GameFunction::ToggleQueue},
      {"toggle_preview_locate", GameFunction::TogglePreviewLocate},
      {"toggle_preview_recall", GameFunction::TogglePreviewRecall},
      {"toggle_cargo_default", GameFunction::ToggleCargoDefault},
      {"toggle_cargo_player", GameFunction::ToggleCargoPlayer},
      {"toggle_cargo_station", GameFunction::ToggleCargoStation},
      {"toggle_cargo_hostile", GameFunction::ToggleCargoHostile},
      {"toggle_cargo_armada", GameFunction::ToggleCargoArmada},

      {"log_debug", GameFunction::LogLevelDebug},
      {"log_info", GameFunction::LogLevelInfo},
      {"log_trace", GameFunction::LogLevelTrace},
      {"log_error", GameFunction::LogLevelError},
      {"log_warn", GameFunction::LogLevelWarn},
      {"log_off", GameFunction::LogLevelOff},

      {"quit", GameFunction::Quit},
  };

  const auto key = normalize(name);
  if (const auto it = map.find(key); it != map.end()) {
    return it->second;
  }

  return std::nullopt;
}
