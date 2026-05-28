#include "Bar.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

// ─── Wayland listener tables ─────────────────────────────────────────────────

static const wl_registry_listener s_registryListener = {
    Bar::registryHandleGlobal,
    Bar::registryHandleGlobalRemove,
};

static const zwlr_layer_surface_v1_listener s_layerListener = {
    Bar::layerSurfaceConfigure,
    Bar::layerSurfaceClosed,
};

// ─── Constructor / Destructor ────────────────────────────────────────────────

Bar::Bar() {
    initWayland();

    // Modules
    m_modWorkspaces = std::make_unique<WorkspacesModule>(m_ipc);
    m_modTitle      = std::make_unique<WindowTitleModule>(m_ipc);
    m_modClock      = std::make_unique<ClockModule>();
    m_modSysInfo    = std::make_unique<SysInfoModule>();

    // When workspace or title changes, mark dirty and wake the display
    auto wake = [this]() { m_dirty = true; wl_display_flush(m_display); };
    m_modWorkspaces->setChangeCallback(wake);
    m_modTitle->setChangeCallback(wake);

    // Start Hyprland event listener
    m_ipc.startListening();
}

Bar::~Bar() {
    m_ipc.stop();
    destroyBuffer();
    if (m_layerSurf)  zwlr_layer_surface_v1_destroy(m_layerSurf);
    if (m_surface)    wl_surface_destroy(m_surface);
    if (m_layerShell) zwlr_layer_shell_v1_destroy(m_layerShell);
    if (m_shm)        wl_shm_destroy(m_shm);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry)   wl_registry_destroy(m_registry);
    if (m_display)    wl_display_disconnect(m_display);
}

// ─── initWayland ─────────────────────────────────────────────────────────────

void Bar::initWayland() {
    m_display = wl_display_connect(nullptr);
    if (!m_display) throw std::runtime_error("Cannot connect to Wayland display");

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &s_registryListener, this);
    wl_display_roundtrip(m_display);  // bind globals
    wl_display_roundtrip(m_display);  // process events

    if (!m_compositor) throw std::runtime_error("No wl_compositor");
    if (!m_shm)        throw std::runtime_error("No wl_shm");
    if (!m_layerShell) throw std::runtime_error("No zwlr_layer_shell_v1 — is wlr-layer-shell supported?");

    initLayerSurface();
}

void Bar::initLayerSurface() {
    m_surface  = wl_compositor_create_surface(m_compositor);
    m_layerSurf = zwlr_layer_shell_v1_get_layer_surface(
        m_layerShell, m_surface,
        nullptr,                                   // output: nullptr = compositor picks
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        "hyprbar");

    // Anchor to top, stretch full width
    zwlr_layer_surface_v1_set_anchor(m_layerSurf,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

    zwlr_layer_surface_v1_set_size(m_layerSurf, 0, m_config.height);
    zwlr_layer_surface_v1_set_exclusive_zone(m_layerSurf,
        m_config.height + m_config.margin_top);
    zwlr_layer_surface_v1_set_margin(m_layerSurf,
        m_config.margin_top, 0, 0, 0);
    zwlr_layer_surface_v1_set_keyboard_interactivity(m_layerSurf,
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);

    zwlr_layer_surface_v1_add_listener(m_layerSurf, &s_layerListener, this);

    // Initial commit (no buffer) to trigger configure
    wl_surface_commit(m_surface);
    wl_display_roundtrip(m_display);
}

// ─── Buffer management (SHM) ─────────────────────────────────────────────────

static int createAnonFd(size_t size) {
    int fd = memfd_create("hyprbar-shm", MFD_CLOEXEC);
    if (fd < 0) throw std::runtime_error("memfd_create failed");
    if (ftruncate(fd, (off_t)size) < 0) {
        close(fd); throw std::runtime_error("ftruncate failed");
    }
    return fd;
}

void Bar::createBuffer(int w, int h) {
    destroyBuffer();
    size_t size = (size_t)w * h * 4;
    m_shmFd   = createAnonFd(size);
    m_shmData = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
    if (m_shmData == MAP_FAILED) {
        close(m_shmFd); throw std::runtime_error("mmap failed");
    }
    memset(m_shmData, 0, size);

    m_shmPool = wl_shm_create_pool(m_shm, m_shmFd, (int32_t)size);
    m_buffer  = wl_shm_pool_create_buffer(m_shmPool, 0, w, h,
                                           w * 4, WL_SHM_FORMAT_ARGB8888);
    m_bufWidth  = w;
    m_bufHeight = h;
}

void Bar::destroyBuffer() {
    if (m_buffer)  { wl_buffer_destroy(m_buffer);  m_buffer  = nullptr; }
    if (m_shmPool) { wl_shm_pool_destroy(m_shmPool); m_shmPool = nullptr; }
    if (m_shmData && m_shmData != MAP_FAILED) {
        munmap(m_shmData, (size_t)m_bufWidth * m_bufHeight * 4);
        m_shmData = nullptr;
    }
    if (m_shmFd >= 0) { close(m_shmFd); m_shmFd = -1; }
    m_bufWidth = m_bufHeight = 0;
}

// ─── Rendering ───────────────────────────────────────────────────────────────

static void roundedRect(cairo_t* cr, double x, double y,
                        double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI_2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,       M_PI_2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI_2,  M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI,    -M_PI_2);
    cairo_close_path(cr);
}

void Bar::drawBackground(cairo_t* cr, int w, int h) {
    // Clear to transparent
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    // Draw rounded pill background
    const double margin = 8.0;
    cairo_set_source_rgba(cr,
        m_config.bg_r, m_config.bg_g, m_config.bg_b, m_config.bg_a);
    roundedRect(cr, margin, 0, w - margin * 2, h, m_config.corner_r);
    cairo_fill(cr);

    // Subtle top highlight line
    cairo_set_source_rgba(cr, 1, 1, 1, 0.06);
    cairo_set_line_width(cr, 1.0);
    roundedRect(cr, margin + 0.5, 0.5, w - margin * 2 - 1, h - 1, m_config.corner_r - 0.5);
    cairo_stroke(cr);
}

void Bar::render() {
    if (!m_configured || !m_shmData) return;

    // Create Cairo surface on SHM buffer (BGRA = ARGB8888 in little-endian)
    cairo_surface_t* cs = cairo_image_surface_create_for_data(
        (unsigned char*)m_shmData,
        CAIRO_FORMAT_ARGB32,
        m_bufWidth, m_bufHeight, m_bufWidth * 4);
    cairo_t* cr = cairo_create(cs);

    int w = m_bufWidth;
    int h = m_bufHeight;

    drawBackground(cr, w, h);

    const double PAD   = 20.0;   // horizontal padding inside pill
    const double LEFT  = PAD;
    double rightEdge   = w - PAD;

    // ── Right section: SysInfo + Clock ──────────────────────────────────────
    // Measure first so we can right-align
    // We'll draw right-to-left

    // Clock (rightmost)
    {
        // Probe width by drawing off-screen
        cairo_surface_t* dummy = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* dc = cairo_create(dummy);
        double cw = m_modClock->draw(dc, 0, 0, 9999, h);
        cairo_destroy(dc);
        cairo_surface_destroy(dummy);

        double cx = rightEdge - cw;
        m_modClock->draw(cr, cx, 0, cw + 4, h);
        rightEdge = cx - 12.0;
    }

    // SysInfo
    {
        cairo_surface_t* dummy = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* dc = cairo_create(dummy);
        double sw = m_modSysInfo->draw(dc, 0, 0, 9999, h);
        cairo_destroy(dc);
        cairo_surface_destroy(dummy);

        double sx = rightEdge - sw;
        m_modSysInfo->draw(cr, sx, 0, sw + 4, h);
        rightEdge = sx - 12.0;
    }

    // ── Left section: Workspaces ─────────────────────────────────────────────
    double leftEnd = LEFT;
    leftEnd += m_modWorkspaces->draw(cr, LEFT, 0, (w / 2.0 - PAD), h);

    // ── Centre: Window title ─────────────────────────────────────────────────
    double centreAvail = rightEdge - leftEnd - 20.0;
    if (centreAvail > 40.0) {
        // Probe title width
        cairo_surface_t* dummy = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* dc = cairo_create(dummy);
        double tw = m_modTitle->draw(dc, 0, 0, centreAvail, h);
        cairo_destroy(dc);
        cairo_surface_destroy(dummy);

        double tx = leftEnd + (centreAvail - tw) / 2.0 + (leftEnd - LEFT);
        // Simple centre
        tx = (w - std::min(tw, centreAvail)) / 2.0;
        m_modTitle->draw(cr, tx, 0, centreAvail, h);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);

    // Attach + commit
    wl_surface_attach(m_surface, m_buffer, 0, 0);
    wl_surface_damage_buffer(m_surface, 0, 0, m_bufWidth, m_bufHeight);
    wl_surface_commit(m_surface);
}

// ─── Main loop ───────────────────────────────────────────────────────────────

void Bar::run() {
    using namespace std::chrono;
    auto lastTick = steady_clock::now();

    while (m_running) {
        // Dispatch pending Wayland events (non-blocking)
        while (wl_display_prepare_read(m_display) != 0)
            wl_display_dispatch_pending(m_display);
        wl_display_flush(m_display);

        // Poll with short timeout so we can tick every second
        struct pollfd pfd{ wl_display_get_fd(m_display), POLLIN, 0 };
        poll(&pfd, 1, 100);

        if (pfd.revents & POLLIN)
            wl_display_read_events(m_display);
        else
            wl_display_cancel_read(m_display);

        wl_display_dispatch_pending(m_display);

        // Tick modules every second
        auto now = steady_clock::now();
        if (duration_cast<milliseconds>(now - lastTick).count() >= 1000) {
            m_modClock->tick();
            m_modSysInfo->tick();
            m_dirty    = true;
            lastTick   = now;
        }

        if (m_dirty.exchange(false)) render();
    }
}

void Bar::requestRedraw() {
    m_dirty = true;
}

// ─── Wayland callbacks ───────────────────────────────────────────────────────

void Bar::registryHandleGlobal(void* data, wl_registry* reg,
                                uint32_t name, const char* iface, uint32_t version) {
    Bar* bar = static_cast<Bar*>(data);
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        bar->m_compositor = static_cast<wl_compositor*>(
            wl_registry_bind(reg, name, &wl_compositor_interface,
                             std::min(version, 4u)));
    } else if (strcmp(iface, wl_shm_interface.name) == 0) {
        bar->m_shm = static_cast<wl_shm*>(
            wl_registry_bind(reg, name, &wl_shm_interface, 1));
    } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        bar->m_layerShell = static_cast<zwlr_layer_shell_v1*>(
            wl_registry_bind(reg, name, &zwlr_layer_shell_v1_interface,
                             std::min(version, 4u)));
    }
}

void Bar::registryHandleGlobalRemove(void*, wl_registry*, uint32_t) {}

void Bar::layerSurfaceConfigure(void* data, zwlr_layer_surface_v1* surf,
                                 uint32_t serial, uint32_t w, uint32_t h) {
    Bar* bar = static_cast<Bar*>(data);
    zwlr_layer_surface_v1_ack_configure(surf, serial);

    int bh = (h > 0) ? (int)h : bar->m_config.height;
    if ((int)w != bar->m_bufWidth || bh != bar->m_bufHeight)
        bar->createBuffer((int)w, bh);

    bar->m_configured = true;
    bar->m_dirty      = true;
}

void Bar::layerSurfaceClosed(void* data, zwlr_layer_surface_v1*) {
    static_cast<Bar*>(data)->m_running = false;
}
