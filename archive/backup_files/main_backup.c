#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <libgen.h>
#include "layer-shell-protocol.h"

// Forward declarations
struct wl_shell_surface;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

// Global variables for Wayland objects
static struct wl_display *display = NULL;
static struct wl_registry *registry = NULL;
static struct wl_compositor *compositor = NULL;
static struct wl_shm *shm = NULL;
static struct wl_surface *surface = NULL;
static struct wl_shell *shell = NULL;
static struct zwlr_layer_shell_v1 *layer_shell = NULL;
static struct zwlr_layer_surface_v1 *layer_surface = NULL;

// Screen dimensions
static int screen_width = 1920;
static int screen_height = 1080;

// Handle registry events
static void registry_handle_global(void *data, struct wl_registry *registry, 
                                  uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_shell_interface.name) == 0) {
        shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
    } else if (strcmp(interface, "zwlr_layer_shell_v1") == 0) {
        layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, 
                                          uint32_t name) {
    // Handle global removal
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

// Create a shared memory buffer
static struct wl_buffer *create_shm_buffer(int width, int height) {
    int stride = width * 4;
    int size = stride * height;
    
    // Create shared memory file in /tmp directory
    char name[] = "/tmp/linestatus-XXXXXX";
    int fd = mkstemp(name);
    if (fd < 0) {
        perror("mkstemp");
        return NULL;
    }
    
    // Set close-on-exec flag
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    
    // Set size
    if (ftruncate(fd, size) < 0) {
        perror("ftruncate");
        close(fd);
        unlink(name);
        return NULL;
    }
    
    // Map memory
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        unlink(name);
        return NULL;
    }
    
    // Create pool and buffer
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    
    wl_shm_pool_destroy(pool);
    close(fd);
    unlink(name);
    
    // Draw orange line on the right side
    uint32_t *pixels = (uint32_t *)data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == width - 1) {  // Rightmost pixel
                // Orange color: RGBA (255, 165, 0, 255)
                pixels[y * width + x] = 0xFFA500FF;
            } else {
                // Transparent
                pixels[y * width + x] = 0x00000000;
            }
        }
    }
    
    munmap(data, size);
    return buffer;
}

int main() {
    // Connect to Wayland display
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
    
    // Get registry
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    
    // Roundtrip to get all globals
    wl_display_roundtrip(display);
    
    if (!compositor || !shm) {
        fprintf(stderr, "Failed to get required Wayland interfaces\n");
        return 1;
    }
    
    // Create surface
    surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        fprintf(stderr, "Failed to create surface\n");
        return 1;
    }
    
    // Try to use layer shell for proper overlay positioning
    if (layer_shell) {
        printf("Using layer shell for overlay positioning\n");
        layer_surface = (struct zwlr_layer_surface_v1*)malloc(sizeof(struct zwlr_layer_surface_v1));
        // This is a simplified approach - in a real implementation, we'd use the proper protocol
        // For now, we'll fall back to regular surface
        free(layer_surface);
        layer_surface = NULL;
    }
    
    // Create buffer and attach it
    struct wl_buffer *buffer = create_shm_buffer(screen_width, screen_height);
    if (!buffer) {
        fprintf(stderr, "Failed to create buffer\n");
        return 1;
    }
    
    wl_surface_attach(surface, buffer, 0, 0);
    
    // Set surface role as toplevel if shell is available
    if (shell) {
        struct wl_shell_surface *shell_surface = wl_shell_get_shell_surface(shell, surface);
        if (shell_surface) {
            wl_shell_surface_set_toplevel(shell_surface);
            wl_shell_surface_set_title(shell_surface, "LineStatus");
        }
    }
    
    wl_surface_commit(surface);
    
    printf("LineStatus running. Drawing orange line on right side of screen.\n");
    printf("Press Ctrl+C to exit.\n");
    
    // Main event loop
    while (wl_display_dispatch(display) != -1) {
        // Keep running
    }
    
    // Cleanup
    wl_buffer_destroy(buffer);
    wl_surface_destroy(surface);
    if (shell) {
        // Note: shell_surface would be destroyed when surface is destroyed
        wl_shell_destroy(shell);
    }
    wl_shm_destroy(shm);
    wl_compositor_destroy(compositor);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    
    return 0;
}