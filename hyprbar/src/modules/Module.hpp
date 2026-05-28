#pragma once
#include <cairo/cairo.h>
#include <string>

// Every bar module can draw itself onto a Cairo context.
// Modules return their rendered width so the bar can lay them out.
class Module {
public:
    virtual ~Module() = default;

    // Draw the module at (x, y) within barHeight pixels.
    // Returns the width consumed.
    virtual double draw(cairo_t* cr, double x, double y,
                        double maxWidth, double barHeight) = 0;

    // Optional: called periodically (every second) for updates.
    virtual void tick() {}
};
