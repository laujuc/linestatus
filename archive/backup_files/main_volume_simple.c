#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <cairo.h>
#include <math.h>

// Global variables
static GtkWidget *window = NULL;
static GtkWidget *drawing_area = NULL;
static float current_volume = 0.7f; // Start at 70%
static gboolean volume_changing = FALSE;
static int volume_direction = 1; // 1 = increasing, -1 = decreasing

// Screen dimensions
static int screen_width = 1920;
static int screen_height = 1080;

// Drawing function for the volume line
static void on_draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data) {
    (void)drawing_area; (void)data;
    
    // Clear with transparent background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    
    // Set up for drawing the orange line
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb(cr, 1.0, 0.647, 0.0); // Orange: RGB(255, 165, 0)
    cairo_set_line_width(cr, 2.0); // Slightly thicker for better visibility
    
    // Calculate line height based on volume (0.0 to 1.0)
    int line_height = (int)(height * current_volume);
    
    // Draw the vertical line on the right side
    // Start from bottom and go up based on volume
    int start_y = height - line_height;
    cairo_move_to(cr, width - 1, start_y);
    cairo_line_to(cr, width - 1, height);
    cairo_stroke(cr);
    
    // Add a small indicator at the top
    if (current_volume > 0.01) {
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0); // Lighter orange for indicator
        cairo_arc(cr, width - 1, start_y, 3, 0, 2 * G_PI);
        cairo_fill(cr);
    }
    
    // Add volume percentage text
    if (current_volume > 0.1) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White text
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12);
        
        char text[32];
        snprintf(text, sizeof(text), "%d%%", (int)(current_volume * 100));
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text, &extents);
        
        // Position text near the indicator
        cairo_move_to(cr, width - 1 - extents.width - 5, start_y + extents.height/2);
        cairo_show_text(cr, text);
    }
}

// Timeout callback for volume animation
static gboolean on_volume_timeout(gpointer user_data) {
    (void)user_data;
    
    if (!volume_changing) {
        // Randomly change volume direction occasionally
        if (g_random_double() < 0.02) { // 2% chance per timeout
            volume_direction = -volume_direction;
        }
    }
    
    // Update volume
    current_volume += 0.005f * volume_direction;
    
    // Keep volume in bounds
    if (current_volume <= 0.1f) {
        current_volume = 0.1f;
        volume_direction = 1;
    } else if (current_volume >= 0.95f) {
        current_volume = 0.95f;
        volume_direction = -1;
    }
    
    // Request redraw
    if (drawing_area) {
        gtk_widget_queue_draw(drawing_area);
    }
    
    return G_SOURCE_CONTINUE; // Keep the timeout running
}

// Activate function - creates the window
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)app; (void)user_data;
    
    // Create the main window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "LineStatus - Volume Visualizer");
    gtk_window_set_default_size(GTK_WINDOW(window), screen_width, screen_height);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    
    // Set up layer shell for proper overlay positioning
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    
    // Anchor to all edges for full screen coverage
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    
    // No exclusive zone - don't reserve space
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), -1);
    
    // Set margins to 0 for full screen
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    
    // Apply CSS for transparency
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "src/style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // Create drawing area for custom drawing
    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), on_draw, NULL, NULL);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    
    // Set window child
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    
    // Add timeout for volume animation (simulates real volume changes)
    g_timeout_add(50, on_volume_timeout, NULL);
    
    // Show the window
    gtk_window_present(GTK_WINDOW(window));
    
    g_print("✅ LineStatus Volume Visualizer started\n");
    g_print("🎵 Simulating volume changes (60%% → 95%% → 10%% → ...)\n");
    g_print("📊 Current volume: %.0f%%\n", current_volume * 100);
    g_print("📏 Line height changes dynamically with volume\n");
    g_print("🎨 Orange line with percentage text on right side\n");
    g_print("💡 This simulates what PipeWire integration would look like\n");
    g_print("🔄 Press Ctrl+C to exit\n");
}

int main(int argc, char **argv) {
    g_print("LineStatus - Volume Visualizer (Simulation)\n");
    g_print("==============================================\n");
    g_print("Dynamic volume visualization demonstration\n");
    g_print("Orange line height changes with simulated volume\n");
    g_print("Shows what PipeWire integration would look like\n\n");
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.linestatus.volume.simple", 
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // Run the application
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // Cleanup
    g_object_unref(app);
    
    g_print("👋 LineStatus Volume Visualizer terminated\n");
    return status;
}