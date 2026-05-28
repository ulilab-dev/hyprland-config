#pragma once

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <cairo/cairo.h>
#include <memory>
#include <atomic>
#include <functional>

#include "HyprIPC.hpp"
#include "modules/Workspaces.hpp"
#include "modules/WindowTitle.hpp"
#include "modules/Clock.hpp"
#include "modules/SysInfo.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Config – tweak these to customise the bar
// ────────────────────────────────────────────────────────────────────────────
struct BarConfig {
    int    height     = 32;      // bar height in px
    int    margin_top = 6;       // gap from screen edge
    double bg_r       = 0.078;   // background colour R
    double bg_g       = 0.082;   //                   G
    double bg_b       = 0.122;   //                   B
    double bg_a       = 0.92;    // opacity (requires compositor compositor blending)
    double corner_r   = 10.0;    // corner radius
};

// ────────────────────────────────────────────────────────────────────────────

class Bar {
public:
    Bar();
    ~Bar();

    void run();          // blocks until the bar is closed
    void requestRedraw();

private:
    // ── Wayland setup ──────────────────────────────────────────────────────
    void initWayland();
    void initLayerSurface();
    void createBuffer(int width, int height);
    void destroyBuffer();

    // ── Rendering ──────────────────────────────────────────────────────────
    void render();
    void drawBackground(cairo_t* cr, int w, int h);

    // ── Wayland global callbacks (must be public for listener table) ──────────
public:
    static void registryHandleGlobal(void*, wl_registry*, uint32_t,
                                     const char*, uint32_t);
    static void registryHandleGlobalRemove(void*, wl_registry*, uint32_t);

    static void layerSurfaceConfigure(void*, zwlr_layer_surface_v1*,
                                      uint32_t serial, uint32_t w, uint32_t h);
    static void layerSurfaceClosed(void*, zwlr_layer_surface_v1*);

private:

    // ── Wayland objects ────────────────────────────────────────────────────
    wl_display*            m_display     = nullptr;
    wl_registry*           m_registry    = nullptr;
    wl_compositor*         m_compositor  = nullptr;
    wl_shm*                m_shm         = nullptr;
    zwlr_layer_shell_v1*   m_layerShell  = nullptr;

    wl_surface*            m_surface     = nullptr;
    zwlr_layer_surface_v1* m_layerSurf   = nullptr;

    // ── SHM buffer ────────────────────────────────────────────────────────
    wl_shm_pool*           m_shmPool     = nullptr;
    wl_buffer*             m_buffer      = nullptr;
    void*                  m_shmData     = nullptr;
    int                    m_shmFd       = -1;
    int                    m_bufWidth    = 0;
    int                    m_bufHeight   = 0;

    // ── Bar state ─────────────────────────────────────────────────────────
    BarConfig              m_config;
    bool                   m_configured  = false;
    std::atomic<bool>      m_running{true};
    std::atomic<bool>      m_dirty{true};

    // ── IPC + modules ─────────────────────────────────────────────────────
    HyprIPC                        m_ipc;
    std::unique_ptr<WorkspacesModule>  m_modWorkspaces;
    std::unique_ptr<WindowTitleModule> m_modTitle;
    std::unique_ptr<ClockModule>       m_modClock;
    std::unique_ptr<SysInfoModule>     m_modSysInfo;
};
