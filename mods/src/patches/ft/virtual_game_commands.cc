#include "virtual_game_commands.h"

#include "config.h"

#include "prime/AllianceStarbaseObjectViewerWidget.h"
#include "prime/ArmadaObjectViewerWidget.h"
#include "prime/BookmarksManager.h"
#include "prime/CelestialObjectViewerWidget.h"
#include "prime/ChatManager.h"
#include "prime/EmbassyObjectViewer.h"
#include "prime/FullScreenChatViewController.h"
#include "prime/HousingObjectViewerWidget.h"
#include "prime/Hub.h"
#include "prime/MiningObjectViewerWidget.h"
#include "prime/MissionsObjectViewerWidget.h"
#include "prime/NavigationSectionManager.h"
#include "prime/PreScanTargetWidget.h"

#include <il2cpp/il2cpp_helper.h>

#if _WIN32
#include <Windows.h>
#endif

namespace
{
void GotoSection(SectionID sectionID)
{
  Hub::get_SectionManager()->TriggerSectionChange(sectionID, nullptr, false, false, true);
}

void ChangeNavigationSection(SectionID sectionID)
{
  const auto section_data = Hub::get_SectionManager()->_sectionStorage->GetState(sectionID);

  if (section_data) {
    Hub::get_SectionManager()->TriggerSectionChange(sectionID, section_data, false, false, true);
  } else {
    NavigationSectionManager::ChangeNavigationSection(sectionID);
  }
}

template <typename T> 
bool TryHideViewersOfType()
{
  const auto objects = ObjectFinder<T>::GetAll();
  bool didHide = false;
  for (auto widget : objects) {
    if (!widget) {
      continue;
    }
    auto visibility_controller = widget->_visibilityController;
    if (!visibility_controller) {
      continue;
    }
    const auto visible = (visibility_controller->_state == VisibilityState::Visible
                          || visibility_controller->_state == VisibilityState::Show);
    if (visible) {
      widget->HideAllViewers();
      didHide = true;
    }
  }
  return didHide;
}

} // namespace

namespace VirtualGameCommands
{
bool Deselect()
{
  bool didHide = false;
  didHide = TryHideViewersOfType<AllianceStarbaseObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<ArmadaObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<CelestialObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<EmbassyObjectViewer>() || didHide;
  didHide = TryHideViewersOfType<HousingObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<MiningObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<MissionsObjectViewerWidget>() || didHide;
  didHide = TryHideViewersOfType<PreScanTargetWidget>() || didHide;
  return didHide;
}

void Execute(GameFunction fn)
{
  auto       config    = &Config::Get();
  const auto is_in_chat = Hub::IsInChat();

#if _WIN32
  if (fn == GameFunction::Quit) {
    TerminateProcess(GetCurrentProcess(), 1);
    return;
  }
#endif

  if (fn == GameFunction::DisableHotKeys) {
    config->hotkeys_enabled = false;
    return;
  }
  if (fn == GameFunction::EnableHotKeys) {
    config->hotkeys_enabled = true;
    return;
  }

  if (!is_in_chat) {
    switch (fn) {
      case GameFunction::ShowQTrials:
        return GotoSection(SectionID::ChallengeSelection);
      case GameFunction::ShowBookmarks: {
        if (auto bookmark_manager = BookmarksManager::Instance(); bookmark_manager) {
          return bookmark_manager->ViewBookmarks();
        }
        return GotoSection(SectionID::Bookmarks_Main);
      }
      case GameFunction::ShowLookup:
        return GotoSection(SectionID::Bookmarks_Search_Coordinates);
      case GameFunction::ShowRefinery:
        return GotoSection(SectionID::Shop_Refining_List);
      case GameFunction::ShowFactions:
        return GotoSection(SectionID::Shop_MainFactions);
      case GameFunction::ShoWStationExterior:
        return GotoSection(SectionID::Starbase_Exterior);
      case GameFunction::ShowGalaxy:
        return ChangeNavigationSection(SectionID::Navigation_Galaxy);
      case GameFunction::ShowStationInterior:
        return GotoSection(SectionID::Starbase_Interior);
      case GameFunction::ShowSystem:
        return ChangeNavigationSection(SectionID::Navigation_System);
      case GameFunction::ShowArtifacts:
        return GotoSection(SectionID::ArtifactHall_Inventory);
      case GameFunction::ShowInventory:
        return GotoSection(SectionID::InventoryList);
      case GameFunction::ShowMissions:
        return GotoSection(SectionID::Missions_AcceptedList);
      case GameFunction::ShowResearch:
        return GotoSection(SectionID::Research_LandingPage);
      case GameFunction::ShowScrapYard:
        return GotoSection(SectionID::ShipScrapping_List);
      case GameFunction::ShowOfficers:
        return GotoSection(SectionID::OfficerInventory);
      case GameFunction::ShowCommander:
        return GotoSection(SectionID::FleetCommander_Management);
      case GameFunction::ShowAwayTeam:
        return GotoSection(SectionID::Missions_AwayTeamsList);
      case GameFunction::ShowEvents:
        return GotoSection(SectionID::Tournament_Group_Selection);
      case GameFunction::ShowExoComp:
        return GotoSection(SectionID::Consumables);
      case GameFunction::ShowDaily:
        return GotoSection(SectionID::Missions_DailyGoals);
      case GameFunction::ShowGifts:
        return GotoSection(SectionID::Shop_List);
      case GameFunction::ShowAlliance:
        return GotoSection(SectionID::Alliance_Main);
      case GameFunction::ShowAllianceHelp:
        return GotoSection(SectionID::Alliance_Help);
      case GameFunction::ShowAllianceArmada:
        return GotoSection(SectionID::Alliance_Armadas);
      case GameFunction::ShowSettings:
        return GotoSection(SectionID::GameSettings);
      default:
        break;
    }

    switch (fn) {
      case GameFunction::ToggleQueue:
        config->queue_enabled = !config->queue_enabled;
        return;
      case GameFunction::TogglePreviewLocate:
        config->disable_preview_locate = !config->disable_preview_locate;
        return;
      case GameFunction::TogglePreviewRecall:
        config->disable_preview_recall = !config->disable_preview_recall;
        return;
      case GameFunction::ToggleCargoDefault:
        config->show_cargo_default = !config->show_cargo_default;
        return;
      case GameFunction::ToggleCargoPlayer:
        config->show_player_cargo = !config->show_player_cargo;
        return;
      case GameFunction::ToggleCargoStation:
        config->show_station_cargo = !config->show_station_cargo;
        return;
      case GameFunction::ToggleCargoHostile:
        config->show_hostile_cargo = !config->show_hostile_cargo;
        return;
      case GameFunction::ToggleCargoArmada:
        config->show_armada_cargo = !config->show_armada_cargo;
        return;
      default:
        break;
    }

    if (fn == GameFunction::ShowChat || fn == GameFunction::ShowChatSide1 || fn == GameFunction::ShowChatSide2) {
      if (auto chat_manager = ChatManager::Instance(); chat_manager) {
        if (chat_manager->IsSideChatOpen) {
          if (auto view_controller = ObjectFinder<FullScreenChatViewController>::Get(); view_controller) {
            if (auto message_list = view_controller->_messageList; message_list) {
              if (auto message_field = message_list->_inputField; message_field) {
                return message_field->ActivateInputField();
              }
            }
          }
        } else if (fn != GameFunction::ShowChat) {
          return chat_manager->OpenChannel(ChatChannelCategory::Alliance, ChatViewMode::Side);
        } else {
          return chat_manager->OpenChannel(ChatChannelCategory::Alliance, ChatViewMode::Fullscreen);
        }
      }
    }
  } else {
    if (auto chat_manager = ChatManager::Instance(); chat_manager) {
      if (fn == GameFunction::SelectChatGlobal) {
        return chat_manager->OpenChannel(ChatChannelCategory::Global);
      } else if (fn == GameFunction::SelectChatAlliance) {
        return chat_manager->OpenChannel(ChatChannelCategory::Alliance);
      } else if (fn == GameFunction::SelectChatPrivate) {
        return chat_manager->OpenChannel(ChatChannelCategory::Private);
      }
    }
  }
}

} // namespace VirtualGameCommands
