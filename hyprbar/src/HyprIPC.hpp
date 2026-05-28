#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

struct Workspace {
    int id;
    std::string name;
    bool active;
    bool hasWindows;
};

struct WindowInfo {
    std::string title;
    std::string wclass;
};

using WorkspacesCallback = std::function<void(std::vector<Workspace>)>;
using WindowCallback     = std::function<void(WindowInfo)>;

class HyprIPC {
public:
    HyprIPC();
    ~HyprIPC();

    // One-shot queries (command socket)
    std::vector<Workspace> getWorkspaces();
    WindowInfo             getActiveWindow();

    // Subscribe to live events (event socket thread)
    void onWorkspaceChange(WorkspacesCallback cb);
    void onWindowChange(WindowCallback cb);
    void startListening();
    void stop();

private:
    std::string      socketPath(const std::string& suffix);
    std::string      sendCommand(const std::string& cmd);
    void             eventLoop();

    std::string      m_instanceSig;
    WorkspacesCallback m_wsCb;
    WindowCallback     m_winCb;
    std::thread      m_thread;
    std::atomic<bool> m_running{false};
    std::mutex       m_cbMutex;
};
