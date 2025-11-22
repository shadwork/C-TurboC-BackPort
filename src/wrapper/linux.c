#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "linux_keyboard.h"

extern int dos_main(int argc, char *argv[]);

// --- Global state ---
static GtkWidget *g_window = NULL;
static GtkWidget *g_drawing_area = NULL;
static IMAGE g_imageBuffer = {0};
static int g_currentScale = 2;
static unsigned char *g_scaledBuffer = NULL;
static size_t g_scaledBufferSize = 0;

// DOS thread
static pthread_t g_dosThread;
static volatile int g_dosFinished = 0;
static int g_dosResult = 0;

// Blink state
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int g_blinkFrameCounter = 0;

// Command line args for DOS thread
static int g_argc = 0;
static char **g_argv = NULL;

// Menu items for checkmarks
static GtkWidget *g_scale_items[4] = {NULL, NULL, NULL, NULL};

// --- Forward declarations ---
void initialize_pccore(void);
void render_frame(void);
void scale_pixel_buffer(void);
void set_window_scale(int scale);
void* dos_thread_function(void* arg);
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);
gboolean on_key_release(GtkWidget *widget, GdkEventKey *event, gpointer data);
gboolean on_timer(gpointer data);
void on_scale_activate(GtkMenuItem *item, gpointer data);
void on_quit_activate(GtkMenuItem *item, gpointer data);

/**
 * @brief DOS thread function
 */
void* dos_thread_function(void* arg) {
    printf("DOS thread started with %d arguments\n", g_argc);
    
    // Call the DOS main function
    g_dosResult = dos_main(g_argc, g_argv);
    
    printf("DOS thread finished with result: %d\n", g_dosResult);
    
    // Mark as finished
    g_dosFinished = 1;
    
    return NULL;
}

/**
 * @brief Initialize the PCCORE struct
 */
void initialize_pccore(void) {
    // Zero out the entire pccore state
    memset(&pccore, 0, sizeof(PCCORE));

    // Set the requested video mode
    pccore.mode = CGA320x200x2;

    // Initialize key to 0 (meaning "no key pressed")
    pccore.key = 0;

    // Set the CGA Color Register (Port 0x3D9)
    pccore.port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01; // 0x31

    // Run one initial render to get image dimensions
    render(&g_imageBuffer, pccore);
}

/**
 * @brief Scale the pixel buffer using nearest-neighbor
 */
void scale_pixel_buffer(void) {
    if (g_imageBuffer.raw == NULL || g_imageBuffer.width == 0 || g_imageBuffer.height == 0) {
        return;
    }
    
    int srcWidth = g_imageBuffer.width;
    int srcHeight = g_imageBuffer.height;
    int dstWidth = srcWidth * g_currentScale;
    int dstHeight = srcHeight * g_currentScale;
    
    size_t requiredSize = dstWidth * dstHeight * 4; // RGBA for Cairo
    
    // Allocate or reallocate buffer if needed
    if (g_scaledBuffer == NULL || g_scaledBufferSize != requiredSize) {
        if (g_scaledBuffer) {
            free(g_scaledBuffer);
        }
        g_scaledBuffer = (unsigned char *)malloc(requiredSize);
        g_scaledBufferSize = requiredSize;
    }
    
    unsigned char *src = g_imageBuffer.raw;
    
    // Fast nearest-neighbor scaling (convert RGB to RGBA)
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / g_currentScale;
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / g_currentScale;
            
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 4;
            
            // Cairo uses BGRA format in native byte order
            g_scaledBuffer[dstIndex + 0] = src[srcIndex + 2]; // B
            g_scaledBuffer[dstIndex + 1] = src[srcIndex + 1]; // G
            g_scaledBuffer[dstIndex + 2] = src[srcIndex + 0]; // R
            g_scaledBuffer[dstIndex + 3] = 255;                // A
        }
    }
}

/**
 * @brief Render a single frame
 */
void render_frame(void) {
    // Store current dimensions before rendering
    const int oldWidth = g_imageBuffer.width;
    const int oldHeight = g_imageBuffer.height;

    // Update pccore.time with system milliseconds
    struct timeval te;
    gettimeofday(&te, NULL);
    pccore.time = (long long)te.tv_sec * 1000LL + te.tv_usec / 1000;

    // Blink implementation
    g_blinkFrameCounter++;
    if (g_blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
        pccore.blink = 1 - pccore.blink;
        g_blinkFrameCounter = 0;
    }

    // Call the render function
    render(&g_imageBuffer, pccore);

    // Check if mode changed
    if (g_imageBuffer.width != oldWidth || g_imageBuffer.height != oldHeight) {
        printf("Detected mode change: %dx%d -> %dx%d\n", 
               oldWidth, oldHeight, g_imageBuffer.width, g_imageBuffer.height);
        set_window_scale(g_currentScale);
    }

    // Trigger window redraw
    if (g_drawing_area) {
        gtk_widget_queue_draw(g_drawing_area);
    }
}

/**
 * @brief Set window scale
 */
void set_window_scale(int scale) {
    if (scale < 1 || scale > 4) {
        return;
    }
    
    g_currentScale = scale;
    
    // Update menu checkmarks
    for (int i = 0; i < 4; i++) {
        if (g_scale_items[i]) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_scale_items[i]), 
                                          (i + 1) == scale);
        }
    }
    
    if (g_window && g_imageBuffer.width > 0 && g_imageBuffer.height > 0) {
        int newWidth = g_imageBuffer.width * scale;
        int newHeight = g_imageBuffer.height * scale;
        
        gtk_window_resize(GTK_WINDOW(g_window), newWidth, newHeight);
        gtk_widget_set_size_request(g_drawing_area, newWidth, newHeight);
        
        printf("Scale set to %dx (%dx%d)\n", scale, newWidth, newHeight);
    }
}

/**
 * @brief Drawing callback
 */
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)widget;
    (void)data;
    
    if (g_imageBuffer.width == 0 || g_imageBuffer.height == 0) {
        return FALSE;
    }
    
    // Scale the pixel buffer
    scale_pixel_buffer();
    
    if (g_scaledBuffer == NULL) {
        return FALSE;
    }
    
    int dstWidth = g_imageBuffer.width * g_currentScale;
    int dstHeight = g_imageBuffer.height * g_currentScale;
    
    // Create Cairo surface from scaled buffer
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        g_scaledBuffer,
        CAIRO_FORMAT_RGB24,
        dstWidth,
        dstHeight,
        dstWidth * 4
    );
    
    // Disable interpolation for crisp pixels
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(surface);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    
    // Draw the surface
    cairo_set_source(cr, pattern);
    cairo_paint(cr);
    
    // Cleanup
    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(surface);
    
    return FALSE;
}

/**
 * @brief Key press callback
 */
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    (void)data;
    
    // Ignore key repeats
    if (event->is_modifier) {
        return FALSE;
    }
    
    int scancode = get_scancode(event);
    if (scancode != 0) {
        pccore.key = scancode;
        pccore.memory[BDA_KBD_STATUS_1] = get_statuscode(event);
        printf("Key pressed: 0x%x 0x%x\n", pccore.key, pccore.memory[BDA_KBD_STATUS_1]);
    }
    
    return FALSE;
}

/**
 * @brief Key release callback
 */
gboolean on_key_release(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    (void)data;
    
    int scancode = get_scancode(event);
    if (pccore.key == scancode) {
        pccore.key = 0;
        printf("Key released\n");
    }
    pccore.memory[BDA_KBD_STATUS_1] = get_statuscode(event);
    
    return FALSE;
}

/**
 * @brief Timer callback for 60 FPS rendering
 */
gboolean on_timer(gpointer data) {
    (void)data;
    render_frame();
    return TRUE; // Continue timer
}

/**
 * @brief Scale menu item callback
 */
void on_scale_activate(GtkMenuItem *item, gpointer data) {
    int scale = GPOINTER_TO_INT(data);
    set_window_scale(scale);
}

/**
 * @brief Quit menu item callback
 */
void on_quit_activate(GtkMenuItem *item, gpointer data) {
    (void)item;
    (void)data;
    gtk_main_quit();
}

/**
 * @brief Create menu bar
 */
GtkWidget* create_menu_bar(void) {
    GtkWidget *menu_bar = gtk_menu_bar_new();
    
    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), NULL);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);
    
    // View menu
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    
    GSList *group = NULL;
    const char *scale_labels[] = {"1x Scale", "2x Scale", "3x Scale", "4x Scale"};
    
    for (int i = 0; i < 4; i++) {
        g_scale_items[i] = gtk_radio_menu_item_new_with_label(group, scale_labels[i]);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(g_scale_items[i]));
        
        g_signal_connect(g_scale_items[i], "activate", 
                        G_CALLBACK(on_scale_activate), 
                        GINT_TO_POINTER(i + 1));
        
        gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), g_scale_items[i]);
        
        // Set 2x as default
        if (i == 1) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_scale_items[i]), TRUE);
        }
    }
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), view_item);
    
    return menu_bar;
}

/**
 * @brief Window destroy callback
 */
void on_window_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    gtk_main_quit();
}

/**
 * @brief Application activation callback
 */
void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    
    // Initialize PC Core
    initialize_pccore();
    
    // Create window
    g_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_window), "PC Core Emulator");
    gtk_window_set_resizable(GTK_WINDOW(g_window), FALSE);
    
    g_signal_connect(g_window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Create vertical box for menu and drawing area
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(g_window), vbox);
    
    // Create menu bar
    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    
    // Create drawing area
    int baseWidth = g_imageBuffer.width * g_currentScale;
    int baseHeight = g_imageBuffer.height * g_currentScale;
    
    g_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_drawing_area, baseWidth, baseHeight);
    
    g_signal_connect(g_drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    
    gtk_box_pack_start(GTK_BOX(vbox), g_drawing_area, TRUE, TRUE, 0);
    
    // Enable keyboard events
    gtk_widget_set_can_focus(g_drawing_area, TRUE);
    gtk_widget_add_events(g_drawing_area, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    
    g_signal_connect(g_window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(g_window, "key-release-event", G_CALLBACK(on_key_release), NULL);
    
    // Show all widgets
    gtk_widget_show_all(g_window);
    
    // Focus the drawing area for keyboard input
    gtk_widget_grab_focus(g_drawing_area);
    
    // Start DOS thread
    int result = pthread_create(&g_dosThread, NULL, dos_thread_function, NULL);
    if (result != 0) {
        fprintf(stderr, "Error creating DOS thread: %d\n", result);
    } else {
        printf("DOS thread created successfully\n");
    }
    
    // Setup timer for 60 FPS rendering (16ms interval)
    g_timeout_add(16, on_timer, NULL);
}

/**
 * @brief Main application entry point
 */
int main(int argc, char *argv[]) {
    // Store command line arguments for DOS thread
    g_argc = argc;
    g_argv = (char **)malloc(sizeof(char *) * (argc + 1));
    for (int i = 0; i < argc; i++) {
        g_argv[i] = strdup(argv[i]);
    }
    g_argv[argc] = NULL;
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.example.pccore", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // Cleanup
    g_object_unref(app);
    
    // Wait for DOS thread to finish
    if (!g_dosFinished) {
        pthread_cancel(g_dosThread);
    }
    pthread_join(g_dosThread, NULL);
    
    // Free command line arguments
    for (int i = 0; i < g_argc; i++) {
        free(g_argv[i]);
    }
    free(g_argv);
    
    // Free scaled buffer
    if (g_scaledBuffer) {
        free(g_scaledBuffer);
    }
    
    printf("DOS execution completed with code: %d\n", g_dosResult);
    
    return status;
}
