#pragma once
#include "Module.hpp"
#include "../HyprIPC.hpp"
#include <vector>
#include <mutex>
#include <functional>

class WorkspacesModule : public Module {
public:
    explicit WorkspacesModule(HyprIPC& ipc);

    double draw(cairo_t* cr, double x, double y,
                double maxWidth, double barHeight) override;

    void setChangeCallback(std::function<void()> cb);

private:
    HyprIPC& m_ipc;
    std::vector<Workspace> m_workspaces;
    std::mutex m_mutex;
    std::function<void()> m_changeCb;

    // Visual config
    static constexpr double WS_PILL_W  = 34.0;
    static constexpr double WS_PILL_H  = 24.0;
    static constexpr double WS_PADDING = 4.0;
    static constexpr double WS_RADIUS  = 6.0;
};
