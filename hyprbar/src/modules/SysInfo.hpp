#pragma once
#include "Module.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>

// Reads /proc/stat for CPU and /proc/meminfo for RAM.
class SysInfoModule : public Module {
public:
    SysInfoModule() { tick(); }

    void tick() override {
        m_cpu = readCPU();
        m_ram = readRAM();
    }

    double draw(cairo_t* cr, double x, double y,
                double /*maxWidth*/, double barHeight) override {
        double cx = x + 6.0;

        // CPU
        cx += drawStat(cr, cx, y, barHeight, "CPU", m_cpu,
                       m_cpu > 80.0 ? 0xFF3B30 : m_cpu > 50.0 ? 0xFF9F0A : 0x32D74B);
        cx += 10.0;

        // RAM
        cx += drawStat(cr, cx, y, barHeight, "RAM", m_ram,
                       m_ram > 85.0 ? 0xFF3B30 : m_ram > 65.0 ? 0xFF9F0A : 0x7FC2F6);
        cx += 6.0;

        return cx - x;
    }

private:
    double m_cpu{0}, m_ram{0};

    static void hexToRGB(uint32_t hex, double& r, double& g, double& b) {
        r = ((hex >> 16) & 0xFF) / 255.0;
        g = ((hex >> 8)  & 0xFF) / 255.0;
        b = (hex & 0xFF)         / 255.0;
    }

    double drawStat(cairo_t* cr, double x, double y, double barH,
                    const char* label, double pct, uint32_t color) {
        double r, g, b;
        hexToRGB(color, r, g, b);

        constexpr double BAR_W = 42.0;
        constexpr double BAR_H = 5.0;
        constexpr double RADIUS = 2.5;

        // Label
        cairo_select_font_face(cr, "JetBrains Mono",
                               CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10.0);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, label, &ext);

        double ty = y + barH / 2.0 - 3.5;

        cairo_set_source_rgba(cr, 0.7, 0.7, 0.75, 0.9);
        cairo_move_to(cr, x, ty + ext.height);
        cairo_show_text(cr, label);

        double bx = x + ext.width + 5.0;
        double by = y + barH / 2.0 - BAR_H / 2.0;

        // Background track
        cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
        cairo_new_sub_path(cr);
        cairo_arc(cr, bx + RADIUS,          by + RADIUS,          RADIUS, M_PI,   -M_PI_2);
        cairo_arc(cr, bx + BAR_W - RADIUS,  by + RADIUS,          RADIUS, -M_PI_2, 0);
        cairo_arc(cr, bx + BAR_W - RADIUS,  by + BAR_H - RADIUS,  RADIUS, 0,       M_PI_2);
        cairo_arc(cr, bx + RADIUS,          by + BAR_H - RADIUS,  RADIUS, M_PI_2,  M_PI);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Filled portion
        double fillW = std::max(RADIUS * 2, BAR_W * pct / 100.0);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_new_sub_path(cr);
        cairo_arc(cr, bx + RADIUS,           by + RADIUS,         RADIUS, M_PI,    -M_PI_2);
        cairo_arc(cr, bx + fillW - RADIUS,   by + RADIUS,         RADIUS, -M_PI_2,  0);
        cairo_arc(cr, bx + fillW - RADIUS,   by + BAR_H - RADIUS, RADIUS, 0,        M_PI_2);
        cairo_arc(cr, bx + RADIUS,           by + BAR_H - RADIUS, RADIUS, M_PI_2,   M_PI);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Percentage text
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", pct);
        cairo_text_extents_t pe;
        cairo_text_extents(cr, pctBuf, &pe);
        cairo_set_source_rgba(cr, 0.8, 0.8, 0.85, 0.9);
        cairo_move_to(cr, bx + BAR_W + 4.0, ty + ext.height);
        cairo_show_text(cr, pctBuf);

        return ext.width + 5.0 + BAR_W + 4.0 + pe.width;
    }

    // ── /proc/stat CPU ──────────────────────────────────────────────────────
    struct CPUStat { long long idle, total; };

    static CPUStat readCPUStat() {
        std::ifstream f("/proc/stat");
        std::string line;
        std::getline(f, line);
        std::istringstream ss(line);
        std::string cpu;
        long long user, nice, sys, idle, iowait, irq, softirq, steal;
        ss >> cpu >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal;
        long long total = user + nice + sys + idle + iowait + irq + softirq + steal;
        return { idle + iowait, total };
    }

    double readCPU() {
        static CPUStat prev{};
        CPUStat cur = readCPUStat();
        long long dTotal = cur.total - prev.total;
        long long dIdle  = cur.idle  - prev.idle;
        double usage = (dTotal > 0) ? 100.0 * (1.0 - (double)dIdle / dTotal) : 0.0;
        prev = cur;
        return std::max(0.0, std::min(100.0, usage));
    }

    // ── /proc/meminfo RAM ───────────────────────────────────────────────────
    double readRAM() {
        std::ifstream f("/proc/meminfo");
        if (!f) return 0;
        long long total = 0, avail = 0;
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                sscanf(line.c_str() + 9, "%lld", &total);
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                sscanf(line.c_str() + 13, "%lld", &avail);
            }
            if (total && avail) break;
        }
        if (total == 0) return 0;
        return 100.0 * (1.0 - (double)avail / total);
    }
};
