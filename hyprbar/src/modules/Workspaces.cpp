#include "Workspaces.hpp"
#include <cairo/cairo.h>
#include <cmath>

WorkspacesModule::WorkspacesModule(HyprIPC& ipc) : m_ipc(ipc) {
    // Initial fetch
    m_workspaces = m_ipc.getWorkspaces();

    // Subscribe to live updates
    m_ipc.onWorkspaceChange([this](std::vector<Workspace> ws) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_workspaces = std::move(ws);
        }
        if (m_changeCb) m_changeCb();
    });
}

void WorkspacesModule::setChangeCallback(std::function<void()> cb) {
    m_changeCb = std::move(cb);
}

// ── helpers ──────────────────────────────────────────────────────────────────

static void roundedRect(cairo_t* cr, double x, double y,
                        double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI_2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,       M_PI_2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI_2,  M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI,    -M_PI_2);
    cairo_close_path(cr);
}

// ── draw ─────────────────────────────────────────────────────────────────────

double WorkspacesModule::draw(cairo_t* cr, double x, double y,
                               double /*maxWidth*/, double barHeight) {
    std::lock_guard<std::mutex> lock(m_mutex);

    double cx = x + WS_PADDING;
    double py = y + (barHeight - WS_PILL_H) / 2.0;

    for (const auto& ws : m_workspaces) {
        bool active = ws.active;
        bool filled = ws.hasWindows;

        if (active) {
            // Accent pill
            cairo_set_source_rgb(cr, 0.498, 0.761, 0.965); // #7FC2F6
            roundedRect(cr, cx, py, WS_PILL_W, WS_PILL_H, WS_RADIUS);
            cairo_fill(cr);
            // Label – dark
            cairo_set_source_rgb(cr, 0.08, 0.08, 0.12);
        } else if (filled) {
            // Subtle outline for occupied workspaces
            cairo_set_source_rgba(cr, 0.498, 0.761, 0.965, 0.25);
            roundedRect(cr, cx, py, WS_PILL_W, WS_PILL_H, WS_RADIUS);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.498, 0.761, 0.965, 0.6);
            roundedRect(cr, cx + 0.5, py + 0.5, WS_PILL_W - 1, WS_PILL_H - 1, WS_RADIUS - 0.5);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
            // Label – muted
            cairo_set_source_rgb(cr, 0.75, 0.75, 0.80);
        } else {
            // Empty: dim dot
            cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
            roundedRect(cr, cx, py, WS_PILL_W, WS_PILL_H, WS_RADIUS);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.30);
        }

        // Workspace label (name or id)
        cairo_select_font_face(cr, "JetBrains Mono",
                               CAIRO_FONT_SLANT_NORMAL,
                               active ? CAIRO_FONT_WEIGHT_BOLD
                                      : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 11.0);

        cairo_text_extents_t ext;
        std::string label = ws.name;
        cairo_text_extents(cr, label.c_str(), &ext);

        double tx = cx + (WS_PILL_W - ext.width) / 2.0 - ext.x_bearing;
        double ty = py + (WS_PILL_H + ext.height) / 2.0 - ext.height + ext.height / 2.0;
        // Simple vertical center
        ty = py + WS_PILL_H / 2.0 + ext.height / 2.0 - 1.0;

        cairo_move_to(cr, tx, ty);
        cairo_show_text(cr, label.c_str());

        cx += WS_PILL_W + WS_PADDING;
    }

    return cx - x;
}
