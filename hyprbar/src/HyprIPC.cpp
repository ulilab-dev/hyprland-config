#include "HyprIPC.hpp"

#include <algorithm>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iostream>

// ─── tiny JSON helpers (no dependency) ──────────────────────────────────────

static std::string jsonField(const std::string& json, const std::string& key) {
    // Extracts the first value for "key": <value>
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return {};
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return {};

    if (json[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\') ++pos;
            val += json[pos++];
        }
        return val;
    } else {
        // number or bool
        std::string val;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']')
            val += json[pos++];
        // trim whitespace
        while (!val.empty() && (val.back() == ' ' || val.back() == '\n')) val.pop_back();
        return val;
    }
}

// Split a JSON array of objects
static std::vector<std::string> jsonSplitObjects(const std::string& json) {
    std::vector<std::string> result;
    int depth = 0;
    std::string cur;
    bool inStr = false;
    for (size_t i = 0; i < json.size(); ++i) {
        char c = json[i];
        if (c == '"' && (i == 0 || json[i-1] != '\\')) inStr = !inStr;
        if (!inStr) {
            if (c == '{') { depth++; }
            if (depth > 0) cur += c;
            if (c == '}') {
                depth--;
                if (depth == 0) { result.push_back(cur); cur.clear(); }
            }
        } else {
            if (depth > 0) cur += c;
        }
    }
    return result;
}

// ─── HyprIPC ────────────────────────────────────────────────────────────────

HyprIPC::HyprIPC() {
    const char* sig = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!sig) {
        std::cerr << "[HyprIPC] HYPRLAND_INSTANCE_SIGNATURE not set\n";
        m_instanceSig = "";
    } else {
        m_instanceSig = sig;
    }
}

HyprIPC::~HyprIPC() {
    stop();
}

std::string HyprIPC::socketPath(const std::string& suffix) {
    // XDG_RUNTIME_DIR first, fallback to /tmp/hypr
    const char* xdg = getenv("XDG_RUNTIME_DIR");
    if (xdg) {
        return std::string(xdg) + "/hypr/" + m_instanceSig + suffix;
    }
    return "/tmp/hypr/" + m_instanceSig + suffix;
}

std::string HyprIPC::sendCommand(const std::string& cmd) {
    if (m_instanceSig.empty()) return {};

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return {};

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::string path = socketPath("/.socket.sock");
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return {};
    }

    std::string msg = "j/" + cmd;
    send(fd, msg.c_str(), msg.size(), 0);

    std::string result;
    char buf[4096];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = '\0';
        result += buf;
    }
    close(fd);
    return result;
}

std::vector<Workspace> HyprIPC::getWorkspaces() {
    std::string raw    = sendCommand("workspaces");
    std::string active = sendCommand("activeworkspace");
    int activeId = 0;
    try { activeId = std::stoi(jsonField(active, "id")); } catch(...) {}

    std::vector<Workspace> ws;
    for (auto& obj : jsonSplitObjects(raw)) {
        Workspace w;
        try { w.id = std::stoi(jsonField(obj, "id")); } catch(...) { continue; }
        w.name      = jsonField(obj, "name");
        if (w.name.empty()) w.name = std::to_string(w.id);
        w.active    = (w.id == activeId);
        std::string wins = jsonField(obj, "windows");
        w.hasWindows = (!wins.empty() && wins != "0");
        ws.push_back(w);
    }
    // Sort by ID
    std::sort(ws.begin(), ws.end(), [](auto& a, auto& b){ return a.id < b.id; });
    return ws;
}

WindowInfo HyprIPC::getActiveWindow() {
    std::string raw = sendCommand("activewindow");
    WindowInfo w;
    w.title  = jsonField(raw, "title");
    w.wclass = jsonField(raw, "class");
    return w;
}

void HyprIPC::onWorkspaceChange(WorkspacesCallback cb) {
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_wsCb = std::move(cb);
}

void HyprIPC::onWindowChange(WindowCallback cb) {
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_winCb = std::move(cb);
}

void HyprIPC::startListening() {
    m_running = true;
    m_thread = std::thread(&HyprIPC::eventLoop, this);
}

void HyprIPC::stop() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void HyprIPC::eventLoop() {
    if (m_instanceSig.empty()) return;

    std::string path = socketPath("/.socket2.sock");
    int fd = -1;

    auto reconnect = [&]() -> bool {
        if (fd >= 0) close(fd);
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) return false;
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
        if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(fd); fd = -1; return false;
        }
        return true;
    };

    if (!reconnect()) return;

    char buf[4096];
    std::string partial;

    while (m_running) {
        ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            reconnect();
            continue;
        }
        buf[n] = '\0';
        partial += buf;

        // Events are newline-separated
        size_t pos;
        while ((pos = partial.find('\n')) != std::string::npos) {
            std::string line = partial.substr(0, pos);
            partial = partial.substr(pos + 1);

            // Parse: "eventname>>data"
            auto sep = line.find(">>");
            if (sep == std::string::npos) continue;
            std::string event = line.substr(0, sep);
            std::string data  = line.substr(sep + 2);

            std::lock_guard<std::mutex> lock(m_cbMutex);

            if ((event == "workspace" || event == "focusedmon" ||
                 event == "createworkspace" || event == "destroyworkspace" ||
                 event == "moveworkspace") && m_wsCb) {
                // Refresh workspace list
                auto ws = getWorkspaces();
                m_wsCb(ws);
            }
            if ((event == "activewindow" || event == "activewindowv2") && m_winCb) {
                // data is "class,title"
                WindowInfo wi;
                auto comma = data.find(',');
                if (comma != std::string::npos) {
                    wi.wclass = data.substr(0, comma);
                    wi.title  = data.substr(comma + 1);
                }
                m_winCb(wi);
            }
        }
    }
    if (fd >= 0) close(fd);
}
