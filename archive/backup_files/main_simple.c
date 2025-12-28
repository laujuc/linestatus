#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>

#define _POSIX_C_SOURCE 200809L

// Global variables
static struct wl_display *display = NULL;
static struct wl_compositor *compositor = NULL;
static struct wl_shm *shm = NULL;
static struct wl_surface *surface = NULL;

// Screen dimensions
static int width = 1920;
static int height = 1080;

// Create SHM buffer
static struct wl_buffer *create_buffer() {
    int stride = width * 4;
    int size = stride * height;
    
    char name[] = "/tmp/linestatus-simple-XXXXXX";
    int fd = mkstemp(name);
    if (fd < 0) {
        perror("mkstemp");
        return NULL;
    }
    
    if (ftruncate(fd, size) < 0) {
        perror("ftruncate");
        close(fd);
        unlink(name);
        return NULL;
    }
    
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        unlink(name);
        return NULL;
    }
    
    // Draw orange line on right side
    uint32_t *pixels = (uint32_t *)data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == width - 1) {  // Rightmost pixel
                pixels[y * width + x] = 0xFFA500FF;  // Orange with full opacity
            } else {
                pixels[y * width + x] = 0x00000000;  // Transparent
            }
        }
    }
    
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    
    wl_shm_pool_destroy(pool);
    munmap(data, size);
    close(fd);
    unlink(name);
    
    return buffer;
}

int main() {
    printf("LineStatus - Simple Wayland Orange Line Demo\n");
    printf("This draws a 1px orange line on the right side of a %dx%d surface\n", width, height);
    
    // Connect to Wayland
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
    
    // Get registry and bind interfaces
    struct wl_registry *registry = wl_display_get_registry(display);
    
    // Simple registry handling - just get compositor and shm
    wl_display_roundtrip(display);
    
    // This is simplified - in a real app we'd properly handle registry events
    // For this demo, we'll assume the interfaces are available
    compositor = wl_display_bind(display, 1, &wl_compositor_interface);
    shm = wl_display_bind(display, 2, &wl_shm_interface);
    
    if (!compositor || !shm) {
        fprintf(stderr, "Failed to get Wayland interfaces\n");
        return 1;
    }
    
    // Create surface
    surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        fprintf(stderr, "Failed to create surface\n");
        return 1;
    }
    
    // Create and attach buffer
    struct wl_buffer *buffer = create_buffer();
    if (!buffer) {
        fprintf(stderr, "Failed to create buffer\n");
        return 1;
    }
    
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);
    
    printf("✅ Surface created and buffer attached\n");
    printf("🎨 Drawing orange line (1px × %dpx) on right side\n", height);
    printf("📏 Position: x=%d (rightmost pixel), y=0 to y=%d\n", width - 1, height - 1);
    printf("🎨 Color: RGB(255, 165, 0) - Orange\n");
    printf("🔄 Running... Press Ctrl+C to exit\n");
    
    // Main loop
    while (wl_display_dispatch(display) != -1) {
        // Keep running
    }
    
    // Cleanup
    wl_buffer_destroy(buffer);
    wl_surface_destroy(surface);
    wl_shm_destroy(shm);
    wl_compositor_destroy(compositor);
    wl_display_disconnect(display);
    
    return 0;
}