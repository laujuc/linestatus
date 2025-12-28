#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

// Display element structure
typedef struct {
    const char *name;       // Identifier ("volume", "brightness", etc.)
    float value;            // Current value (0.0 - 1.0)
    float x_pos;            // X position (0.0 - 1.0)
    float y_pos;            // Y position (0.0 - 1.0)
    int vertical;           // Orientation (1 = vertical, 0 = horizontal)
    float r, g, b;          // Color
    GtkWidget *window;      // GTK window for this element
    GtkWidget *drawing_area; // Drawing area for this element
} DisplayElement;

// Global variables
static DisplayElement *elements = NULL;
static int num_elements = 0;
static int socket_fd = -1; // Socket file descriptor

// Screen dimensions
static int screen_width = 1920;
static int screen_height = 1080;

// Drawing function for individual display element
static void on_draw_element(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data) {
    (void)drawing_area;
    DisplayElement *element = (DisplayElement *)data;
    
    // Clear with transparent background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    
    // Set up for drawing
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb(cr, element->r, element->g, element->b);
    
    if (element->vertical == 1) {
        // Vertical bar - fills the narrow window
        int bar_height = (int)(height * element->value);
        int start_y = height - bar_height;
        cairo_set_line_width(cr, width); // Use full window width
        cairo_move_to(cr, width / 2, start_y);
        cairo_line_to(cr, width / 2, height);
    } else {
        // Horizontal bar - fills the short window
        int bar_width = (int)(width * element->value);
        cairo_set_line_width(cr, height); // Use full window height
        cairo_move_to(cr, 0, height / 2);
        cairo_line_to(cr, bar_width, height / 2);
    }
    cairo_stroke(cr);
}

// Function to find display element by name
static DisplayElement* find_element(const char *name) {
    for (int i = 0; i < num_elements; i++) {
        if (strcmp(elements[i].name, name) == 0) {
            return &elements[i];
        }
    }
    return NULL;
}

// Function to update display element value
static void update_element_value(const char *name, float value) {
    DisplayElement *element = find_element(name);
    if (element) {
        element->value = fmax(0.0f, fmin(1.0f, value));
        if (element->drawing_area) {
            gtk_widget_queue_draw(element->drawing_area);
        }
        printf("📊 %s updated to: %.0f%%\n", element->name, element->value * 100);
    } else {
        printf("⚠️  Unknown element: %s\n", name);
    }
}

// Function to create a window for a display element
static void create_element_window(DisplayElement *element, GtkApplication *app) {
    // Create window with appropriate size
    int window_width, window_height;
    if (element->vertical) {
        window_width = 4; // 4px wide for vertical bars
        window_height = screen_height;
    } else {
        window_width = screen_width;
        window_height = 4; // 4px tall for horizontal bars
    }
    
    element->window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(element->window), window_width, window_height);
    gtk_window_set_resizable(GTK_WINDOW(element->window), FALSE);
    
    // Set up layer shell for proper overlay positioning
    gtk_layer_init_for_window(GTK_WINDOW(element->window));
    gtk_layer_set_layer(GTK_WINDOW(element->window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    
    // Position the window
    if (element->vertical) {
        // Vertical bar - anchor to right edge, no margin (exactly on edge)
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    } else {
        // Horizontal bar - anchor to bottom edge, no margin (exactly on edge)
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(element->window), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
    }
    
    // No exclusive zone - don't reserve space
    gtk_layer_set_exclusive_zone(GTK_WINDOW(element->window), -1);
    
    // Note: Window is naturally click-through due to small size
    // Clicks will pass through to applications behind it
    
    // Create drawing area
    element->drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(element->drawing_area), on_draw_element, element, NULL);
    gtk_widget_set_hexpand(element->drawing_area, TRUE);
    gtk_widget_set_vexpand(element->drawing_area, TRUE);
    
    // Set window child and show
    gtk_window_set_child(GTK_WINDOW(element->window), element->drawing_area);
    gtk_window_present(GTK_WINDOW(element->window));
}

// Function to add a new display element
static void add_display_element(const char *name, float x, float y, bool vertical, float r, float g, float b, GtkApplication *app) {
    // Reallocate elements array
    DisplayElement *new_elements = realloc(elements, (num_elements + 1) * sizeof(DisplayElement));
    if (!new_elements) {
        printf("⚠️  Failed to allocate memory for new element\n");
        return;
    }
    
    elements = new_elements;
    DisplayElement *element = &elements[num_elements++];
    
    // Initialize element
    element->name = g_strdup(name);
    element->value = 0.5f; // Default to 50%
    element->x_pos = x;
    element->y_pos = y;
    element->vertical = vertical; // 1 for vertical, 0 for horizontal
    element->r = r;
    element->g = g;
    element->b = b;
    element->window = NULL;
    element->drawing_area = NULL;
    
    // Create window for this element
    create_element_window(element, app);
    
    printf("➕ Added %s display at (%.1f, %.1f) - %.1f%%\n", 
           element->name, element->x_pos * 100, element->y_pos * 100, element->value * 100);
}

// Function to create Unix domain socket
static int create_socket(const char *socket_path) {
    struct sockaddr_un addr;
    int fd;
    
    // Remove existing socket if it exists
    unlink(socket_path);
    
    // Create socket
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    
    // Set up socket address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    // Bind socket
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    
    // Listen for connections
    if (listen(fd, 5) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    
    // Set socket permissions
    chmod(socket_path, 0666);
    
    return fd;
}

// Function to handle socket connections
static gboolean handle_socket(gpointer user_data) {
    (void)user_data;
    
    if (socket_fd < 0) {
        return G_SOURCE_CONTINUE;
    }
    
    // Check if there's a connection
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    
    struct timeval timeout = {0, 0}; // Non-blocking check
    if (select(socket_fd + 1, &read_fds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(socket_fd, &read_fds)) {
            // Accept connection
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                // Read data from client
                char buffer[32];
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    
                    // Parse the value - support both simple numbers and key:value format
                    char *endptr;
                    long value_percent = strtol(buffer, &endptr, 10);
                    
                    if (endptr != buffer) {
                        // Simple number format (backward compatible)
                        if (value_percent >= 0 && value_percent <= 100) {
                            update_element_value("volume", value_percent / 100.0f);
                        } else {
                            printf("⚠️  Invalid volume value: %s", buffer);
                        }
                    } else if (strstr(buffer, ":") != NULL) {
                        // Key:value format
                        char *colon = strchr(buffer, ':');
                        if (colon) {
                            *colon = '\0';
                            char *key = buffer;
                            char *value_str = colon + 1;
                            
                            long value = strtol(value_str, &endptr, 10);
                            if (endptr != value_str && value >= 0 && value <= 100) {
                                if (strcmp(key, "volume") == 0) {
                                    update_element_value("volume", value / 100.0f);
                                } else if (strcmp(key, "brightness") == 0) {
                                    update_element_value("brightness", value / 100.0f);
                                } else {
                                    printf("📭 Unknown key received: %s=%ld\n", key, value);
                                }
                            } else {
                                printf("⚠️  Invalid value for key %s: %s", key, value_str);
                            }
                        }
                    } else {
                        printf("⚠️  Invalid format: %s", buffer);
                    }
                }
                
                close(client_fd);
            }
        }
    }
    
    return G_SOURCE_CONTINUE; // Keep checking
}

// Function to read volume from stdin using timeout
static gboolean check_stdin(gpointer user_data) {
    (void)user_data;
    
    // Check if there's data available on stdin
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    
    struct timeval timeout = {0, 0}; // Non-blocking check
    if (select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                // Parse the volume value
                char *endptr;
                long volume_percent = strtol(buffer, &endptr, 10);
                
                if (endptr != buffer && volume_percent >= 0 && volume_percent <= 100) {
                    update_element_value("volume", volume_percent / 100.0f);
                } else {
                    printf("⚠️  Invalid volume value: %s", buffer);
                }
            }
        }
    }
    
    return G_SOURCE_CONTINUE; // Keep the timeout running
}



// Activate function - creates display element windows
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    
    // Initialize display elements - exactly on screen edges
    add_display_element("volume", 1.0f, 0.0f, true, 1.0f, 0.647f, 0.0f, app); // Orange, right edge, vertical
    add_display_element("brightness", 0.0f, 1.0f, false, 0.0f, 0.8f, 1.0f, app); // Blue, bottom edge, horizontal
    
    // Create Unix domain socket for volume updates
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        char socket_path[256];
        snprintf(socket_path, sizeof(socket_path), "%s/linestatus.sock", runtime_dir);
        
        socket_fd = create_socket(socket_path);
        if (socket_fd >= 0) {
            printf("🔌 Socket created at: %s\n", socket_path);
            g_timeout_add(100, handle_socket, NULL);
        } else {
            printf("⚠️  Failed to create socket, falling back to stdin\n");
            g_timeout_add(100, check_stdin, NULL);
        }
    } else {
        printf("⚠️  XDG_RUNTIME_DIR not set, using stdin\n");
        g_timeout_add(100, check_stdin, NULL);
    }
    
    printf("✅ LineStatus Multi Display started\n");
    printf("🪟 Separate narrow windows for each indicator\n");
    printf("📊 Displaying %d elements\n", num_elements);
    printf("🖱️  Clicks pass through - no interference\n");
    printf("📭  Send updates: ./send-volume 60\n");
    printf("📭  Send updates: ./send-volume 80 brightness\n");
}

int main(int argc, char **argv) {
    // Volume updates come from stdin, not command line arguments
    // This allows for simple piping: echo 60 | ./linestatus-static-volume
    
    printf("LineStatus - Multi Display\n");
    printf("==========================\n");
    printf("Modular display system\n");
    printf("Supports multiple indicators\n");
    printf("Uses Unix socket for updates\n");
    printf("Initializing...\n\n");
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.linestatus.staticvolume", 
                                              G_APPLICATION_DEFAULT_FLAGS);
    // g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // Run the application
    printf("🚀 Starting GTK main loop...\n");
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    printf("🏁 GTK main loop exited with status: %d\n", status);
    
    // Cleanup
    if (socket_fd >= 0) {
        close(socket_fd);
        const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
        if (runtime_dir) {
            char socket_path[256];
            snprintf(socket_path, sizeof(socket_path), "%s/linestatus.sock", runtime_dir);
            unlink(socket_path); // Remove socket file
        }
    }
    
    // Destroy display element windows
    if (elements) {
        for (int i = 0; i < num_elements; i++) {
            if (elements[i].window) {
                gtk_window_destroy(GTK_WINDOW(elements[i].window));
            }
            g_free((void *)elements[i].name);
        }
        free(elements);
    }
    
    g_object_unref(app);
    
    printf("👋 LineStatus Static Volume terminated\n");
    return status;
}