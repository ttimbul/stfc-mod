#include "virtual_input_queue.h"

#include "patches/ft/virtual_commands.h"

#include <mutex>
#include <queue>

static std::mutex                       g_mu;
static std::queue<VirtualInputQueue::Item> g_q;

bool VirtualInputQueue::EnqueueCommandByName(std::string_view name)
{
  auto parsed = VirtualCommands::ParseGameFunction(name);
  if (!parsed.has_value()) {
    return false;
  }

  EnqueueCommand(*parsed);
  return true;
}

void VirtualInputQueue::EnqueueCommand(GameFunction fn)
{
  std::scoped_lock lk{g_mu};
  g_q.push(Item{.type = ItemType::GameCommand, .command = fn});
}

void VirtualInputQueue::EnqueueTextUtf8(std::string_view utf8)
{
  std::scoped_lock lk{g_mu};
  Item it{.type = ItemType::TypeTextUtf8};
  it.text = std::string(utf8);
  g_q.push(std::move(it));
}

void VirtualInputQueue::EnqueueKeyHold(KeyCode key, uint32_t hold_ms)
{
  std::scoped_lock lk{g_mu};
  g_q.push(Item{.type = ItemType::KeyHold, .key = key, .hold_ms = hold_ms});
}

void VirtualInputQueue::EnqueueMouseClick(float x, float y, int32_t button, uint32_t hold_ms)
{
  std::scoped_lock lk{g_mu};
  g_q.push(Item{.type = ItemType::MouseClick, .hold_ms = hold_ms, .mouse_x = x, .mouse_y = y, .mouse_button = button});
}

void VirtualInputQueue::EnqueueDeselect()
{
  std::scoped_lock lk{g_mu};
  g_q.push(Item{.type = ItemType::Deselect});
}

std::optional<VirtualInputQueue::Item> VirtualInputQueue::TryDequeue()
{
  std::scoped_lock lk{g_mu};
  if (g_q.empty()) {
    return std::nullopt;
  }
  auto it = std::move(g_q.front());
  g_q.pop();
  return it;
}
