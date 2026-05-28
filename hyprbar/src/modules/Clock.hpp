#pragma once
#include "Module.hpp"
#include <string>
#include <ctime>

class ClockModule : public Module {
public:
    ClockModule() { tick(); }

    void tick() override {
        time_t t = time(nullptr);
        struct tm* tm = localtime(&t);
        char buf[64];
        strftime(buf, sizeof(buf), "%a %d %b  %H:%M", tm);
        m_text = buf;
    }

    double draw(cairo_t* cr, double x, double y,
                double /*maxWidth*/, double barHeight) override {
        cairo_select_font_face(cr, "JetBrains Mono",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12.5);

        cairo_text_extents_t ext;
        cairo_text_extents(cr, m_text.c_str(), &ext);

        // Icon + text: clock emoji alternative — a simple circle indicator
        double cx = x + 8.0;
        double cy = y + barHeight / 2.0;

        // Draw small clock icon
        cairo_set_source_rgba(cr, 0.498, 0.761, 0.965, 0.9);
        cairo_arc(cr, cx, cy, 6.5, 0, 2 * M_PI);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
        // Hour hand
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, cx + 2.5, cy - 3.5);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
        // Minute hand
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, cx + 4.5, cy + 1.5);
        cairo_set_line_width(cr, 1.2);
        cairo_stroke(cr);

        double tx = cx + 12.0;
        double ty = y + barHeight / 2.0 + ext.height / 2.0;

        cairo_set_source_rgb(cr, 0.90, 0.90, 0.95);
        cairo_move_to(cr, tx, ty);
        cairo_show_text(cr, m_text.c_str());

        return 12.0 + 12.0 + ext.width + 8.0;
    }

private:
    std::string m_text;
};
