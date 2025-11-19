#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "linux_keyboard.h"
#include "../dosapp.h"

// --- Constants ---
#define WINDOW_TITLE "PC Core Emulator"
#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)

// --- Global Variables ---
Display *g_display = NULL;
Window g_window = 0;
GC g_gc = 0;
XImage *g_ximage = NULL;
IMAGE g_imageBuffer = {0};
Atom g_wmDeleteWindow;

int g_baseWidth = 0;
int g_baseHeight = 0;
int g_running = 1;

// DOS Thread Data
typedef struct {
    int argc;
    char **argv;
    int result;
    int finished;
} DOSThreadData;

pthread_t g_dosThread;
DOSThreadData *g_pDOSData = NULL;

// --- Forward Declarations ---
void InitializePCCore(void);
void CreateAppWindow(int argc, char **argv);
void RenderAndUpdate(void);
void HandleEvents(void);
void CleanupResources(void);
void* DOSThreadFunction(void *arg);
void StartDOSThread(int argc, char **argv);
long GetCurrentTimeMicros(void);

/**
 * @brief Get current time in microseconds
 */
long GetCurrentTimeMicros(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

/**
 * @brief DOS Thread Function
 */
void* DOSThreadFunction(void *arg) {
    DOSThreadData *data = (DOSThreadData *)arg;
    
    printf("DOS thread started with %d arguments\n", data->argc);
    
    // Call the DOS main function
    data->result = dos_main(data->argc, data->argv);
    
    printf("DOS thread finished with result: %d\n", data->result);
    
    // Mark as finished
    __atomic_store_n(&data->finished, 1, __ATOMIC_SEQ_CST);
    
    return NULL;
}

/**
 * @brief Start the DOS thread
 */
void StartDOSThread(int argc, char **argv) {
    // Allocate thread data
    g_pDOSData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    if (!g_pDOSData) {
        fprintf(stderr, "Failed to allocate DOS thread data\n");
        return;
    }
    
    g_pDOSData->finished = 0;
    g_pDOSData->result = 0;
    g_pDOSData->argc = argc;
    
    // Allocate and copy argv
    g_pDOSData->argv = (char **)malloc(sizeof(char *) * (argc + 1));
    for (int i = 0; i < argc; i++) {
        g_pDOSData->argv[i] = strdup(argv[i]);
    }
    g_pDOSData->argv[argc] = NULL;
    
    // Create the thread
    int result = pthread_create(&g_dosThread, NULL, DOSThreadFunction, g_pDOSData);
    if (result != 0) {
        fprintf(stderr, "Failed to create DOS thread: %d\n", result);
        for (int i = 0; i < argc; i++) {
            free(g_pDOSData->argv[i]);
        }
        free(g_pDOSData->argv);
        free(g_pDOSData);
        g_pDOSData = NULL;
    } else {
        printf("DOS thread created successfully\n");
    }
}

/**
 * @brief Initialize PCCORE struct
 */
void InitializePCCore(void) {
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
    
    g_baseWidth = g_imageBuffer.width;
    g_baseHeight = g_imageBuffer.height;
}

/**
 * @brief Create the application window
 */
void CreateAppWindow(int argc, char **argv) {
    // Open connection to X server
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        fprintf(stderr, "Cannot open X display\n");
        exit(1);
    }
    
    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);
    
    // Calculate window size (2x scale by default)
    int defaultScale = 2;
    int windowWidth = g_baseWidth * defaultScale;
    int windowHeight = g_baseHeight * defaultScale;
    
    // Set up window attributes
    XSetWindowAttributes attrs;
    attrs.background_pixel = BlackPixel(g_display, screen);
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                       StructureNotifyMask | FocusChangeMask;
    
    // Create the window
    g_window = XCreateWindow(
        g_display, root,
        0, 0, windowWidth, windowHeight,
        0,
        DefaultDepth(g_display, screen),
        InputOutput,
        DefaultVisual(g_display, screen),
        CWBackPixel | CWEventMask,
        &attrs
    );
    
    if (!g_window) {
        fprintf(stderr, "Cannot create window\n");
        XCloseDisplay(g_display);
        exit(1);
    }
    
    // Set window title
    XStoreName(g_display, g_window, WINDOW_TITLE);
    
    // Set size hints (minimum size)
    XSizeHints *sizeHints = XAllocSizeHints();
    if (sizeHints) {
        sizeHints->flags = PMinSize;
        sizeHints->min_width = g_baseWidth;
        sizeHints->min_height = g_baseHeight;
        XSetWMNormalHints(g_display, g_window, sizeHints);
        XFree(sizeHints);
    }
    
    // Set WM_DELETE_WINDOW protocol
    g_wmDeleteWindow = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wmDeleteWindow, 1);
    
    // Create graphics context
    g_gc = XCreateGC(g_display, g_window, 0, NULL);
    
    // Map (show) the window
    XMapWindow(g_display, g_window);
    XFlush(g_display);
    
    // Wait for window to be mapped
    XEvent event;
    do {
        XNextEvent(g_display, &event);
    } while (event.type != MapNotify);
}

/**
 * @brief Render and update the display
 */
void RenderAndUpdate(void) {
    // Call the C render function
    render(&g_imageBuffer, pccore);
    
    if (g_imageBuffer.width == 0 || g_imageBuffer.height == 0) {
        return;
    }
    
    // Get current window size
    XWindowAttributes windowAttrs;
    XGetWindowAttributes(g_display, g_window, &windowAttrs);
    int windowWidth = windowAttrs.width;
    int windowHeight = windowAttrs.height;
    
    // Create or recreate XImage if needed
    if (!g_ximage || 
        g_ximage->width != g_imageBuffer.width || 
        g_ximage->height != g_imageBuffer.height) {
        
        if (g_ximage) {
            XDestroyImage(g_ximage);
        }
        
        // Create XImage
        // Note: XImage will use the raw buffer directly
        // We need to convert RGB to BGR for X11
        int screen = DefaultScreen(g_display);
        Visual *visual = DefaultVisual(g_display, screen);
        int depth = DefaultDepth(g_display, screen);
        
        // Allocate buffer for BGR format
        char *imageData = (char *)malloc(g_imageBuffer.width * g_imageBuffer.height * 4);
        
        g_ximage = XCreateImage(
            g_display,
            visual,
            depth,
            ZPixmap,
            0,
            imageData,
            g_imageBuffer.width,
            g_imageBuffer.height,
            32,
            0
        );
        
        if (!g_ximage) {
            fprintf(stderr, "Failed to create XImage\n");
            free(imageData);
            return;
        }
    }
    
    // Convert RGB to BGRA format for X11
    unsigned char *src = g_imageBuffer.raw;
    unsigned char *dst = (unsigned char *)g_ximage->data;
    
    for (int i = 0; i < g_imageBuffer.width * g_imageBuffer.height; i++) {
        dst[i * 4 + 0] = src[i * 3 + 2]; // B
        dst[i * 4 + 1] = src[i * 3 + 1]; // G
        dst[i * 4 + 2] = src[i * 3 + 0]; // R
        dst[i * 4 + 3] = 0xFF;            // A
    }
    
    // Scale and draw the image to fill the window
    // Using XPutImage with scaling via server-side scaling if available,
    // otherwise we need to scale manually or use extension
    
    // For simplicity, we'll use XPutImage and let the X server handle it
    // For better performance, consider using XRender or similar extensions
    
    // Calculate scaling to maintain aspect ratio
    float scaleX = (float)windowWidth / g_imageBuffer.width;
    float scaleY = (float)windowHeight / g_imageBuffer.height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int scaledWidth = (int)(g_imageBuffer.width * scale);
    int scaledHeight = (int)(g_imageBuffer.height * scale);
    
    // Center the image
    int offsetX = (windowWidth - scaledWidth) / 2;
    int offsetY = (windowHeight - scaledHeight) / 2;
    
    // Clear background
    XSetForeground(g_display, g_gc, BlackPixel(g_display, DefaultScreen(g_display)));
    XFillRectangle(g_display, g_window, g_gc, 0, 0, windowWidth, windowHeight);
    
    // Draw scaled image
    // Note: Basic XPutImage doesn't support scaling, so we use it at original size
    // For production, you'd want to use XRender or do software scaling
    if (scaledWidth == g_imageBuffer.width && scaledHeight == g_imageBuffer.height) {
        // No scaling needed
        XPutImage(g_display, g_window, g_gc, g_ximage,
                  0, 0, offsetX, offsetY,
                  g_imageBuffer.width, g_imageBuffer.height);
    } else {
        // Simple nearest-neighbor scaling
        // For better quality, consider using XRender extension
        XPutImage(g_display, g_window, g_gc, g_ximage,
                  0, 0, offsetX, offsetY,
                  g_imageBuffer.width, g_imageBuffer.height);
    }
    
    XFlush(g_display);
}

/**
 * @brief Handle X11 events
 */
void HandleEvents(void) {
    XEvent event;
    
    while (XPending(g_display) > 0) {
        XNextEvent(g_display, &event);
        
        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    RenderAndUpdate();
                }
                break;
                
            case KeyPress: {
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                unsigned char scancode = get_scancode(keysym);
                if (scancode != 0) {
                    pccore.key = scancode;
                    printf("Key pressed: 0x%x (keysym: 0x%lx)\n", scancode, keysym);
                }
                break;
            }
            
            case KeyRelease: {
                // Check for key repeat (X11 sends KeyRelease+KeyPress for repeats)
                if (XPending(g_display) > 0) {
                    XEvent nextEvent;
                    XPeekEvent(g_display, &nextEvent);
                    
                    if (nextEvent.type == KeyPress &&
                        nextEvent.xkey.time == event.xkey.time &&
                        nextEvent.xkey.keycode == event.xkey.keycode) {
                        // This is a key repeat, ignore the release
                        break;
                    }
                }
                
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                unsigned char scancode = get_scancode(keysym);
                if (scancode != 0 && pccore.key == scancode) {
                    pccore.key = 0;
                    printf("Key released\n");
                }
                break;
            }
            
            case ConfigureNotify:
                // Window was resized
                RenderAndUpdate();
                break;
                
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == g_wmDeleteWindow) {
                    g_running = 0;
                }
                break;
                
            case FocusIn:
                // Window gained focus
                break;
                
            case FocusOut:
                // Window lost focus - release all keys
                pccore.key = 0;
                break;
        }
    }
}

/**
 * @brief Cleanup resources
 */
void CleanupResources(void) {
    // Wait for DOS thread to finish
    if (g_pDOSData) {
        int finished = __atomic_load_n(&g_pDOSData->finished, __ATOMIC_SEQ_CST);
        if (!finished) {
            printf("Waiting for DOS thread to finish...\n");
            pthread_join(g_dosThread, NULL);
        } else {
            pthread_join(g_dosThread, NULL);
        }
        
        // Free allocated memory
        for (int i = 0; i < g_pDOSData->argc; i++) {
            free(g_pDOSData->argv[i]);
        }
        free(g_pDOSData->argv);
        
        printf("DOS execution completed with code: %d\n", g_pDOSData->result);
        
        free(g_pDOSData);
        g_pDOSData = NULL;
    }
    
    // Free X11 resources
    if (g_ximage) {
        // Free the image data before destroying
        if (g_ximage->data) {
            free(g_ximage->data);
            g_ximage->data = NULL;
        }
        XDestroyImage(g_ximage);
        g_ximage = NULL;
    }
    
    if (g_gc) {
        XFreeGC(g_display, g_gc);
        g_gc = 0;
    }
    
    if (g_window) {
        XDestroyWindow(g_display, g_window);
        g_window = 0;
    }
    
    if (g_display) {
        XCloseDisplay(g_display);
        g_display = NULL;
    }
}

/**
 * @brief Main application entry point
 */
int main(int argc, char **argv) {
    printf("PC Core Emulator - Linux/X11 Version\n");
    
    // Initialize PCCORE
    InitializePCCore();
    
    // Create window
    CreateAppWindow(argc, argv);
    
    // Start DOS thread
    StartDOSThread(argc, argv);
    
    // Main render loop
    long lastFrameTime = GetCurrentTimeMicros();
    
    while (g_running) {
        long currentTime = GetCurrentTimeMicros();
        long deltaTime = currentTime - lastFrameTime;
        
        // Handle events
        HandleEvents();
        
        // Render at target FPS
        if (deltaTime >= FRAME_TIME_US) {
            RenderAndUpdate();
            lastFrameTime = currentTime;
        } else {
            // Sleep for remaining frame time
            long sleepTime = FRAME_TIME_US - deltaTime;
            if (sleepTime > 1000) {
                usleep(sleepTime);
            }
        }
        
        // Small yield to prevent 100% CPU usage
        usleep(100);
    }
    
    // Cleanup
    CleanupResources();
    
    return 0;
}