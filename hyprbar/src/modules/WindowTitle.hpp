#pragma once
#include "Module.hpp"
#include "../HyprIPC.hpp"
#include <string>
#include <mutex>
#include <functional>
#include <algorithm>

class WindowTitleModule : public Module {
public:
    explicit WindowTitleModule(HyprIPC& ipc) : m_ipc(ipc) {
        auto w = ipc.getActiveWindow();
        m_title = w.title;

        ipc.onWindowChange([this](WindowInfo wi) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_title = wi.title;
            if (m_changeCb) m_changeCb();
        });
    }

    void setChangeCallback(std::function<void()> cb) {
        m_changeCb = std::move(cb);
    }

    double draw(cairo_t* cr, double x, double y,
                double maxWidth, double barHeight) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string label = m_title;

        cairo_select_font_face(cr, "JetBrains Mono",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);

        // Truncate if too long
        cairo_text_extents_t ext;
        while (!label.empty()) {
            cairo_text_extents(cr, label.c_str(), &ext);
            if (ext.width <= maxWidth) break;
            label = label.substr(0, label.size() - 4) + "...";
        }

        double tx = x;
        double ty = y + barHeight / 2.0 + ext.height / 2.0;

        cairo_set_source_rgba(cr, 0.85, 0.85, 0.90, 0.85);
        cairo_move_to(cr, tx, ty);
        cairo_show_text(cr, label.c_str());

        return ext.width;
    }

private:
    HyprIPC& m_ipc;
    std::string m_title;
    std::mutex m_mutex;
    std::function<void()> m_changeCb;
};
