#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <cairo.h>

// Global variables
static GtkWidget *window = NULL;
static GtkWidget *drawing_area = NULL;

// Screen dimensions
static int screen_width = 1920;
static int screen_height = 1080;

// Drawing function for the orange line
static void on_draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data) {
    (void)drawing_area; // Unused parameter
    (void)data; // Unused parameter
    
    // Clear with transparent background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    
    // Set up for drawing the orange line
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb(cr, 1.0, 0.647, 0.0); // Orange: RGB(255, 165, 0)
    cairo_set_line_width(cr, 4.0);
    
    // Draw the vertical line on the right side
    cairo_move_to(cr, width - 1, 0);
    cairo_line_to(cr, width - 1, height);
    cairo_stroke(cr);
}

// Activate function - creates the window
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data; // Unused parameter
    
    // Create the main window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "LineStatus");
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
    
    // Note: gtk_layer_set_keyboard_interactivity removed due to linking issues
    // The window will still be properly positioned as an overlay
    
    // Apply CSS for transparency (like echo-meter)
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
    
    // Make drawing area transparent
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    
    // Set window child
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    
    // Show the window
    gtk_window_present(GTK_WINDOW(window));
    
    g_print("✅ LineStatus GTK version started\n");
    g_print("🎨 Drawing orange line on right side of screen\n");
    g_print("📏 Position: x=%d (rightmost pixel), y=0 to y=%d\n", screen_width - 1, screen_height - 1);
    g_print("🎨 Color: RGB(255, 165, 0) - Orange\n");
    g_print("🔄 Using GTK Layer Shell for proper overlay positioning\n");
    g_print("🚦 Press Ctrl+C to exit\n");
}

int main(int argc, char **argv) {
    g_print("LineStatus - GTK Layer Shell Version\n");
    g_print("====================================\n");
    g_print("This version uses GTK with Layer Shell for proper overlay positioning\n");
    g_print("Compatible with Niri and other modern Wayland compositors\n\n");
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.linestatus", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // Run the application
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // Cleanup
    g_object_unref(app);
    
    return status;
}
