/*
 * Minimal stub for xdg_popup_interface.
 * The wlr-layer-shell protocol references xdg_popup in its get_popup request.
 * We never call get_popup, but the generated protocol C file holds a pointer
 * to xdg_popup_interface — so the linker needs the symbol to exist.
 * Providing a stub here avoids pulling in the entire xdg-shell protocol.
 */
#include <wayland-client.h>

const struct wl_interface xdg_popup_interface = {
    "xdg_popup",
    3,           /* version */
    0, NULL,     /* requests */
    0, NULL      /* events */
};
