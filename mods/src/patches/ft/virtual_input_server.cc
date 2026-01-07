#include "virtual_input_server.h"

#include "patches/key.h"
#include "patches/ft/virtual_input_queue.h"
#include "patches/ft/virtual_game_commands.h"

#include <il2cpp/il2cpp_helper.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <atomic>
#include <charconv>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{
std::once_flag        g_start_once;
std::atomic<uint16_t> g_port{0};

#if _WIN32
struct WsaGuard {
  WsaGuard()
  {
    WSADATA wsaData;
    ok = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
  }
  ~WsaGuard()
  {
    if (ok) {
      WSACleanup();
    }
  }
  bool ok{false};

};
#endif

static void close_socket(int s)
{
#if _WIN32
  closesocket((SOCKET)s);
#else
  ::close(s);
#endif
}

static bool send_all(int s, std::string_view data)
{
  const char* p   = data.data();
  size_t      len = data.size();
  while (len > 0) {
#if _WIN32
    int n = ::send((SOCKET)s, p, (int)len, 0);
#else
    ssize_t n = ::send(s, p, len, 0);
#endif
    if (n <= 0) {
      return false;
    }
    p += n;
    len -= (size_t)n;
  }
  return true;
}

static std::string_view ltrim(std::string_view s)
{
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
    s.remove_prefix(1);
  }
  return s;
}

static std::string_view rtrim(std::string_view s)
{
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
    s.remove_suffix(1);
  }
  return s;
}

static bool ieq(std::string_view a, std::string_view b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z') {
      ca = (char)(ca - 'A' + 'a');
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = (char)(cb - 'A' + 'a');
    }
    if (ca != cb) {
      return false;
    }
  }
  return true;
}

struct HttpRequest
{
  std::string method;
  std::string path;
  size_t      content_length{0};
  std::string body;
};

static std::optional<HttpRequest> parse_http_request(std::string& buf)
{
  auto header_end = buf.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    return std::nullopt;
  }

  std::string_view header_view(buf.data(), header_end);
  auto             first_eol = header_view.find("\r\n");
  if (first_eol == std::string_view::npos) {
    return std::nullopt;
  }

  std::string_view request_line = header_view.substr(0, first_eol);
  // METHOD SP PATH SP HTTP/1.1
  auto sp1 = request_line.find(' ');
  if (sp1 == std::string_view::npos) {
    return std::nullopt;
  }
  auto sp2 = request_line.find(' ', sp1 + 1);
  if (sp2 == std::string_view::npos) {
    return std::nullopt;
  }

  HttpRequest req;
  req.method = std::string(request_line.substr(0, sp1));
  req.path   = std::string(request_line.substr(sp1 + 1, sp2 - (sp1 + 1)));

  // headers
  size_t content_len = 0;
  std::string_view headers = header_view.substr(first_eol + 2);
  while (!headers.empty()) {
    auto eol = headers.find("\r\n");
    auto line = (eol == std::string_view::npos) ? headers : headers.substr(0, eol);
    headers = (eol == std::string_view::npos) ? std::string_view{} : headers.substr(eol + 2);

    auto colon = line.find(':');
    if (colon == std::string_view::npos) {
      continue;
    }
    auto name  = rtrim(line.substr(0, colon));
    auto value = ltrim(line.substr(colon + 1));

    if (ieq(name, "Content-Length")) {
      unsigned long v = 0;
      try {
        v = std::stoul(std::string(value));
      } catch (...) {
        v = 0;
      }
      content_len = (size_t)v;
    }
  }

  const size_t body_start = header_end + 4;
  if (buf.size() < body_start + content_len) {
    return std::nullopt;
  }

  req.content_length = content_len;
  req.body.assign(buf.data() + body_start, content_len);

  // consume parsed request from buffer
  buf.erase(0, body_start + content_len);
  return req;
}

static void send_json(int client, int status, const nlohmann::json& payload)
{
  const auto body = payload.dump();
  std::string resp;
  resp.reserve(256 + body.size());

  std::string_view status_text = "OK";
  if (status == 400) status_text = "Bad Request";
  if (status == 404) status_text = "Not Found";
  if (status == 405) status_text = "Method Not Allowed";

  resp += "HTTP/1.1 ";
  resp += std::to_string(status);
  resp += " ";
  resp += status_text;
  resp += "\r\n";
  resp += "Content-Type: application/json\r\n";
  resp += "Connection: close\r\n";
  resp += "Content-Length: ";
  resp += std::to_string(body.size());
  resp += "\r\n\r\n";
  resp += body;

  (void)send_all(client, resp);
}

static std::optional<KeyCode> parse_keycode(std::string_view name)
{
  auto kc = Key::Parse(name);
  if (kc == KeyCode::None) {
    return std::nullopt;
  }
  return kc;
}

// Get screen height for Y coordinate conversion (top-to-bottom -> bottom-to-top)
static float get_screen_height()
{
  static auto get_height = il2cpp_resolve_icall_typed<int()>("UnityEngine.Screen::get_height()");
  if (get_height) {
    return static_cast<float>(get_height());
  }
  return 1080.0f; // fallback
}

static void handle_request(int client, const HttpRequest& req)
{
  try {
    if (req.method == "GET" && req.path == "/health") {
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    if (req.path == "/cmd") {
      if (req.method != "POST") {
        send_json(client, 405, nlohmann::json{{"ok", false}, {"error", "method not allowed"}});
        return;
      }
      auto j = nlohmann::json::parse(req.body.empty() ? "{}" : req.body, nullptr, false);
      if (j.is_discarded()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "invalid json"}});
        return;
      }
      auto name = j.value("name", std::string{});
      if (name.empty()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "missing name"}});
        return;
      }
      if (!VirtualInputQueue::EnqueueCommandByName(name)) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "unknown command"}});
        return;
      }
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    if (req.path == "/text") {
      if (req.method != "POST") {
        send_json(client, 405, nlohmann::json{{"ok", false}, {"error", "method not allowed"}});
        return;
      }
      auto j = nlohmann::json::parse(req.body.empty() ? "{}" : req.body, nullptr, false);
      if (j.is_discarded()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "invalid json"}});
        return;
      }
      auto text = j.value("text", std::string{});
      VirtualInputQueue::EnqueueTextUtf8(text);
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    if (req.path == "/keyhold") {
      if (req.method != "POST") {
        send_json(client, 405, nlohmann::json{{"ok", false}, {"error", "method not allowed"}});
        return;
      }
      auto j = nlohmann::json::parse(req.body.empty() ? "{}" : req.body, nullptr, false);
      if (j.is_discarded()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "invalid json"}});
        return;
      }
      auto key_name = j.value("key", std::string{});
      auto ms       = j.value("ms", 0);
      if (key_name.empty()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "missing key"}});
        return;
      }
      if (ms <= 0) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "missing/invalid ms"}});
        return;
      }
      auto key = parse_keycode(key_name);
      if (!key.has_value()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "unknown key"}});
        return;
      }
      VirtualInputQueue::EnqueueKeyHold(*key, (uint32_t)ms);
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    if (req.path == "/click") {
      if (req.method != "POST") {
        send_json(client, 405, nlohmann::json{{"ok", false}, {"error", "method not allowed"}});
        return;
      }
      auto j = nlohmann::json::parse(req.body.empty() ? "{}" : req.body, nullptr, false);
      if (j.is_discarded()) {
        send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "invalid json"}});
        return;
      }
      auto x      = j.value("x", 0.0f);
      auto y      = j.value("y", 0.0f);
      auto button = j.value("button", 0);
      auto ms     = j.value("ms", 50);
      if (ms <= 0) {
        ms = 50;
      }
      
      // Convert Y from top-to-bottom (screen coords) to bottom-to-top (Unity coords)
      float screen_height = get_screen_height();
      float unity_y = screen_height - y;
      
      VirtualInputQueue::EnqueueMouseClick(x, unity_y, button, (uint32_t)ms);
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    if (req.path == "/deselect") {
      if (req.method != "POST") {
        send_json(client, 405, nlohmann::json{{"ok", false}, {"error", "method not allowed"}});
        return;
      }
      // Queue the deselect to happen on the main thread during the next tick
      VirtualInputQueue::EnqueueDeselect();
      send_json(client, 200, nlohmann::json{{"ok", true}});
      return;
    }

    send_json(client, 404, nlohmann::json{{"ok", false}, {"error", "not found"}});
  } catch (std::exception& ex) {
    send_json(client, 400, nlohmann::json{{"ok", false}, {"error", "bad request"}, {"message", ex.what()}});
  }
}

static void server_thread(uint16_t port)
{
#if _WIN32
  WsaGuard wsa;
  if (!wsa.ok) {
    spdlog::error("VirtualInputServer: WSAStartup failed");
    return;
  }
#endif

  int listen_fd = (int)::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    spdlog::error("VirtualInputServer: socket() failed");
    return;
  }

  int yes = 1;
#if _WIN32
  setsockopt((SOCKET)listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port        = htons(port);

  if (::bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
    spdlog::error("VirtualInputServer: bind() failed");
    close_socket(listen_fd);
    return;
  }

  // Read actual port if 0
  sockaddr_in bound{};
#if _WIN32
  int bound_len = (int)sizeof(bound);
#else
  socklen_t bound_len = sizeof(bound);
#endif
  if (::getsockname(listen_fd, (sockaddr*)&bound, &bound_len) == 0) {
    g_port.store(ntohs(bound.sin_port));
  }

  if (::listen(listen_fd, 16) != 0) {
    spdlog::error("VirtualInputServer: listen() failed");
    close_socket(listen_fd);
    return;
  }

  spdlog::info("VirtualInputServer REST listening on http://127.0.0.1:{}", (int)g_port.load());

  for (;;) {
#if _WIN32
    SOCKET client = ::accept((SOCKET)listen_fd, nullptr, nullptr);
    if (client == INVALID_SOCKET) {
      break;
    }
#else
    int client = ::accept(listen_fd, nullptr, nullptr);
    if (client < 0) {
      break;
    }
#endif

    std::string buf;
    buf.reserve(8192);
    std::vector<char> tmp(2048);

    // Read until we can parse exactly one request then close.
    for (;;) {
#if _WIN32
      int n = ::recv(client, tmp.data(), (int)tmp.size(), 0);
#else
      auto n = ::recv(client, tmp.data(), tmp.size(), 0);
#endif
      if (n <= 0) {
        break;
      }

      buf.append(tmp.data(), (size_t)n);

      auto req = parse_http_request(buf);
      if (req.has_value()) {
        handle_request((int)client, *req);
        break;
      }

      // prevent unbounded growth
      if (buf.size() > 1024 * 1024) {
        send_json((int)client, 400, nlohmann::json{{"ok", false}, {"error", "request too large"}});
        break;
      }
    }

    close_socket((int)client);
  }

  close_socket(listen_fd);
}

} // namespace

namespace VirtualInputServer
{
void Start(uint16_t port)
{
  std::call_once(g_start_once, [port] {
    std::thread(server_thread, port).detach();
  });
}

uint16_t Port()
{
  return g_port.load();
}

} // namespace VirtualInputServer
