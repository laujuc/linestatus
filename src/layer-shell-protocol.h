/*
 * This is a minimal layer shell protocol header based on the standard
 * zwlr_layer_shell_v1 protocol. This allows us to create proper overlays.
 */

#ifndef LAYER_SHELL_PROTOCOL_H
#define LAYER_SHELL_PROTOCOL_H

#include <wayland-client.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Layer shell interface */
extern const struct wl_interface zwlr_layer_shell_v1_interface;

/* Layer surface interface */
extern const struct wl_interface zwlr_layer_surface_v1_interface;

/* Enums for layer shell */
enum zwlr_layer_shell_v1_layer {
    ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND = 0,
    ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM = 1,
    ZWLR_LAYER_SHELL_V1_LAYER_TOP = 2,
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY = 3,
};

enum zwlr_layer_surface_v1_anchor {
    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP = 1,
    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM = 2,
    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT = 4,
    ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT = 8,
};

/* Minimal struct definitions */
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

#ifdef __cplusplus
}
#endif

#endif /* LAYER_SHELL_PROTOCOL_H */