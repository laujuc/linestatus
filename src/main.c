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
#include <signal.h>
#include <string.h>

// Global variables
static GtkWidget *window = NULL;
static GtkWidget *drawing_area = NULL;
static GtkCssProvider *css_provider = NULL; // Global CSS provider for cleanup
static float current_volume = 0.7f; // Default to 70%
static int socket_fd = -1; // Socket file descriptor
static char socket_path[256] = {0}; // Store socket path for cleanup

// Line color - default is orange (RGB: 255, 165, 0)
static double line_red = 1.0;
static double line_green = 0.647;
static double line_blue = 0.0;

// Debug mode for testing visibility
static int debug_mode = 0;

// RGB values as integers for display
static int line_r = 255;
static int line_g = 165;
static int line_b = 0;

// Socket type for different status indicators
static const char *socket_type = "status";

// Position and orientation settings
static int window_x = -1; // -1 means auto-position (right edge)
static int window_y = 0;  // 0 means top
static const char *orientation = "vertical"; // "vertical" or "horizontal"

// Screen dimensions
static int screen_height = 1080;

// Drawing function for the status line
static void on_draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data) {
    (void)drawing_area; (void)data;
    
    // Clear with transparent background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    
    // Set up for drawing the line with dynamic color
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    // Use black color in debug mode for better visibility
    if (debug_mode) {
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
    } else {
        cairo_set_source_rgb(cr, line_red, line_green, line_blue);
    }
    
    // Draw based on orientation
    if (strcmp(orientation, "vertical") == 0) {
        // Vertical orientation - line grows from bottom
        cairo_set_line_width(cr, width); // Use full window width for line
        int line_height = (int)(height * current_volume);
        int start_y = height - line_height;
        cairo_move_to(cr, width / 2, start_y); // Center in the narrow window
        cairo_line_to(cr, width / 2, height);
    } else {
        // Horizontal orientation - line grows from left
        cairo_set_line_width(cr, height); // Use full window height for line
        int line_width = (int)(width * current_volume);
        cairo_move_to(cr, 0, height / 2); // Center in the narrow window
        cairo_line_to(cr, line_width, height / 2);
    }
    cairo_stroke(cr);
}

// Function to set volume from socket or other source
void set_volume(float volume) {
    // Clamp volume between 0.0 and 1.0
    current_volume = fmax(0.0f, fmin(1.0f, volume));
    
    if (drawing_area) {
        gtk_widget_queue_draw(drawing_area);
    }
    
    printf("🔊 Volume updated to: %.0f%%\n", current_volume * 100);
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
                    
                    // Parse the volume value
                    char *endptr;
                    long volume_percent = strtol(buffer, &endptr, 10);
                    
                    if (endptr != buffer && volume_percent >= 0 && volume_percent <= 100) {
                        set_volume(volume_percent / 100.0f);
                    } else {
                        printf("⚠️  Invalid volume value: %s", buffer);
                    }
                }
                
                close(client_fd);
            }
        }
    }
    
    return G_SOURCE_CONTINUE; // Keep checking
}

// Signal handler for cleanup
static void cleanup_and_exit(int sig) {
    (void)sig; // Unused parameter
    
    printf("\n🧹 Received signal %d, cleaning up...\n", sig);
    
    // Cleanup CSS provider
    if (css_provider != NULL) {
        g_object_unref(css_provider);
        printf("🧹 CSS provider cleaned up\n");
    }
    
    // Cleanup socket
    if (socket_fd >= 0) {
        close(socket_fd);
        if (socket_path[0] != '\0') {
            unlink(socket_path); // Remove socket file
            printf("🗑️  Removed socket: %s\n", socket_path);
        }
    }
    
    printf("👋 Exiting gracefully...\n");
    exit(0);
}

// Function to parse hex color string to RGB components
static void parse_hex_color(const char *hex_str) {
    // Default orange if parsing fails
    line_red = 1.0;
    line_green = 0.647;
    line_blue = 0.0;
    
    if (hex_str == NULL || strlen(hex_str) == 0) {
        return;
    }
    
    // Remove # if present
    const char *color_str = hex_str;
    if (hex_str[0] == '#') {
        color_str = hex_str + 1;
    }
    
    // Parse hex string
    unsigned int r, g, b;
    if (sscanf(color_str, "%02x%02x%02x", &r, &g, &b) == 3) {
        // Convert to 0.0-1.0 range
        line_red = r / 255.0;
        line_green = g / 255.0;
        line_blue = b / 255.0;
        
        // Store integer values for display
        line_r = r;
        line_g = g;
        line_b = b;
    } else {
        printf("⚠️  Invalid color format '%s', using default orange\n", hex_str);
    }
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
                    set_volume(volume_percent / 100.0f);
                } else {
                    printf("⚠️  Invalid volume value: %s", buffer);
                }
            }
        }
    }
    
    return G_SOURCE_CONTINUE; // Keep the timeout running
}



// Activate function - creates the window
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)app; (void)user_data;
    
    // Create the main window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "LineStatus");
    
    // Set window size based on orientation
    if (strcmp(orientation, "vertical") == 0) {
        // Make window wider in debug mode for better visibility
        if (debug_mode) {
            gtk_window_set_default_size(GTK_WINDOW(window), 10, screen_height); // 10px wide, full height
        } else {
            gtk_window_set_default_size(GTK_WINDOW(window), 4, screen_height); // 4px wide, full height
        }
    } else {
        // Make window taller in debug mode for better visibility
        if (debug_mode) {
            gtk_window_set_default_size(GTK_WINDOW(window), screen_height, 10); // Full width, 10px tall
        } else {
            gtk_window_set_default_size(GTK_WINDOW(window), screen_height, 4); // Full width, 4px tall
        }
    }
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    
    // Set up layer shell for proper overlay positioning
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    
    // Set positioning based on user input
    if (window_x == -1) {
        // Auto-position on right edge (default behavior)
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    } else {
        // Custom positioning - use layer shell anchoring with margins
        // For left/top positioning, anchor to left and top edges with margins
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
        
        // Set margins as offsets from the anchored edges
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, window_x);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, window_y);
    }
    
    // No exclusive zone - don't reserve space
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), -1);
    
    // Note: Window is naturally non-interactive since it's very narrow
    // Clicks will pass through to applications behind it
    
    // Apply CSS for transparency
    css_provider = gtk_css_provider_new();
    
    // Try different paths for CSS file
    const char *css_paths[] = {
        "src/style.css",
        "style.css",
        "/home/ljuc/projects/linestatus/src/style.css",
        "/home/ljuc/projects/linestatus/style.css"
    };
    
    gboolean css_loaded = FALSE;
    for (gsize i = 0; i < G_N_ELEMENTS(css_paths); i++) {
        if (g_file_test(css_paths[i], G_FILE_TEST_EXISTS)) {
            gtk_css_provider_load_from_path(css_provider, css_paths[i]);
            css_loaded = TRUE;
            printf("🎨 CSS loaded from: %s\n", css_paths[i]);
            break;
        } else {
            printf("🔍 CSS file not found: %s\n", css_paths[i]);
        }
    }
    
    if (!css_loaded) {
        printf("⚠️  Could not load CSS file, transparency may not work perfectly\n");
    }
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // Create drawing area for custom drawing
    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), on_draw, NULL, NULL);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    
    // Set window child
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    
    // Show the window
    gtk_window_present(GTK_WINDOW(window));
    
    // Create Unix domain socket for status updates
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        snprintf(socket_path, sizeof(socket_path), "%s/linestatus-%s.sock", runtime_dir, socket_type);
        
        socket_fd = create_socket(socket_path);
        if (socket_fd >= 0) {
            printf("Socket created at: %s\n", socket_path);
            g_timeout_add(100, handle_socket, NULL);
        } else {
            printf("Failed to create socket, falling back to stdin\n");
            g_timeout_add(100, check_stdin, NULL);
        }
    } else {
        printf("XDG_RUNTIME_DIR not set, using stdin\n");
        g_timeout_add(100, check_stdin, NULL);
    }
    
    // Set up signal handlers for graceful cleanup
    signal(SIGINT, cleanup_and_exit);  // Ctrl+C
    signal(SIGTERM, cleanup_and_exit); // Termination signal
    
    printf("LineStatus started (type: %s)\n", socket_type);
    if (debug_mode) {
        printf("🐞 DEBUG MODE: Using black line for better visibility\n");
    }
    if (strcmp(orientation, "vertical") == 0) {
        if (debug_mode) {
        printf("Narrow window (10px wide × %dpx tall) - DEBUG MODE\n", screen_height);
    } else {
        printf("Narrow window (4px wide × %dpx tall)\n", screen_height);
    }
        if (!debug_mode) {
            printf("Line height = volume level (RGB: %d, %d, %d)\n", line_r, line_g, line_b);
        } else {
            printf("Line height = volume level (RGB: 0, 0, 0 - BLACK)\n");
        }
    } else {
    if (debug_mode) {
        printf("Wide window (%dpx wide × 10px tall) - DEBUG MODE\n", screen_height);
    } else {
        printf("Wide window (%dpx wide × 4px tall)\n", screen_height);
    }
        if (!debug_mode) {
            printf("Line width = volume level (RGB: %d, %d, %d)\n", line_r, line_g, line_b);
        } else {
            printf("Line width = volume level (RGB: 0, 0, 0 - BLACK)\n");
        }
    }
    printf("Initial volume: %.0f%%\n", current_volume * 100);
    printf("Clicks pass through - no interference\n");
    printf("Send updates to socket: echo 60 > $XDG_RUNTIME_DIR/linestatus-%s.sock\n", socket_type);
}

// Function to remove processed arguments from argv
static void remove_arguments(int *argc, char ***argv, int start_index, int count) {
    for (int i = start_index; i < *argc - count; i++) {
        (*argv)[i] = (*argv)[i + count];
    }
    *argc -= count;
}

int main(int argc, char **argv) {
    // Parse command line arguments for color
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--color") == 0 || strcmp(argv[i], "--line-color") == 0) {
            if (i + 1 < argc) {
                parse_hex_color(argv[i + 1]);
                // Remove both the option and its value from argv
                remove_arguments(&argc, &argv, i, 2);
                // Don't increment i since we removed 2 elements
            } else {
                printf("❌ Error: --color requires a hex color value\n");
                printf("Usage: %s --color RRGGBB\n", argv[0]);
                printf("Example: %s --color FF5733\n", argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--type") == 0) {
            if (i + 1 < argc) {
                socket_type = argv[i + 1];
                // Remove both the option and its value from argv
                remove_arguments(&argc, &argv, i, 2);
                // Don't increment i since we removed 2 elements
            } else {
                printf("❌ Error: --type requires a type name\n");
                printf("Usage: %s --type TYPE\n", argv[0]);
                printf("Example: %s --type volume\n", argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--position") == 0 || strcmp(argv[i], "--pos") == 0) {
            if (i + 1 < argc) {
                // Parse position in format "X,Y" or "x,y"
                char *pos_str = argv[i + 1];
                char *comma = strchr(pos_str, ',');
                if (comma) {
                    *comma = '\0';
                    window_x = atoi(pos_str);
                    window_y = atoi(comma + 1);
                    // Remove both the option and its value from argv
                    remove_arguments(&argc, &argv, i, 2);
                    // Don't increment i since we removed 2 elements
                } else {
                    printf("❌ Error: --position requires format X,Y\n");
                    printf("Usage: %s --position X,Y\n", argv[0]);
                    printf("Example: %s --position 100,200\n", argv[0]);
                    return 1;
                }
            } else {
                printf("❌ Error: --position requires X,Y coordinates\n");
                printf("Usage: %s --position X,Y\n", argv[0]);
                printf("Example: %s --position 100,200\n", argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--orientation") == 0 || strcmp(argv[i], "--orient") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[i + 1], "vertical") == 0 || strcmp(argv[i + 1], "horizontal") == 0) {
                    orientation = argv[i + 1];
                    // Remove both the option and its value from argv
                    remove_arguments(&argc, &argv, i, 2);
                    // Don't increment i since we removed 2 elements
                } else {
                    printf("❌ Error: --orientation must be 'vertical' or 'horizontal'\n");
                    printf("Usage: %s --orientation vertical|horizontal\n", argv[0]);
                    return 1;
                }
            } else {
                printf("❌ Error: --orientation requires vertical or horizontal\n");
                printf("Usage: %s --orientation vertical|horizontal\n", argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
            printf("🐞 Debug mode enabled - using black line for better visibility\n");
            remove_arguments(&argc, &argv, i, 1);
            // Don't increment i since we removed 1 element
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("LineStatus - Wayland Status Indicator\n");
            printf("======================================\n");
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\n");
            printf("Options:\n");
            printf("  --color, --line-color RRGGBB\n");
            printf("                          Set line color in hex format (RRGGBB)\n");
            printf("                          Example: --color FF5733 for orange-red\n");
            printf("  --type TYPE            Set socket type (default: status)\n");
            printf("                          Example: --type volume, --type brightness\n");
            printf("  --position, --pos X,Y  Set window position (default: auto)\n");
            printf("                          Example: --position 100,200\n");
            printf("  --orientation, --orient vertical|horizontal\n");
            printf("                          Set line orientation (default: vertical)\n");
            printf("                          Example: --orientation horizontal\n");
            printf("  --debug                Enable debug mode (black line for visibility)\n");
            printf("  -h, --help             Show this help message\n");
            printf("\n");
            printf("Default: Orange line (RGB: 255, 165, 0), socket type: status\n");
            printf("         Position: auto (right edge), Orientation: vertical\n");
            printf("Example: %s --color 00FF00 --type volume  # Green volume indicator\n", argv[0]);
            printf("         %s --position 100,200 --orientation horizontal\n", argv[0]);
            printf("         %s --color FF0000 --pos 50,50 --orient horizontal\n", argv[0]);
            printf("         %s --debug  # Debug mode with black line\n", argv[0]);
            printf("\n");
            printf("Socket communication:\n");
            printf("  echo 60 > $XDG_RUNTIME_DIR/linestatus-TYPE.sock\n");
            printf("  ./send-status --type TYPE 60\n");
            return 0;
        } else {
            i++; // Move to next argument
        }
    }
    
    printf("LineStatus - Wayland Status Indicator\n");
    printf("======================================\n");
    printf("Minimal status indicator\n");
    if (window_x == -1) {
        printf("Narrow line on right edge\n");
    } else {
        printf("Position: %d,%d\n", window_x, window_y);
    }
    printf("Orientation: %s\n", orientation);
    printf("Uses Unix socket for dynamic updates\n");
    printf("Line color: RGB(%d, %d, %d)\n", line_r, line_g, line_b);
    printf("Socket type: %s\n", socket_type);
    printf("Initial volume: %.0f%%\n\n", current_volume * 100);
    fflush(stdout); // Ensure output is flushed before GTK takes over
    
    // Create GTK application with dynamic ID based on socket type
    char app_id[64];
    snprintf(app_id, sizeof(app_id), "com.linestatus.%s", socket_type);
    GtkApplication *app = gtk_application_new(app_id, 
                                              G_APPLICATION_DEFAULT_FLAGS);
    // g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // Run the application
    printf("🚀 Starting GTK main loop...\n");
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    printf("🏁 GTK main loop exited with status: %d\n", status);
    
    // Cleanup
    if (css_provider != NULL) {
        g_object_unref(css_provider);
    }
    
    if (socket_fd >= 0) {
        close(socket_fd);
        if (socket_path[0] != '\0') {
            unlink(socket_path); // Remove the correct socket file
        }
    }
    g_object_unref(app);
    
    printf("👋 LineStatus Static Volume terminated\n");
    return status;
}
