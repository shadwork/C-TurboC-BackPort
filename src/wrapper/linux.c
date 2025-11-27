#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <unistd.h> 

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "linux_keyboard.h"

extern int dos_main(int argc, char *argv[]);

// Global state
static Display *display = NULL;
static Window window;
static GC gc;
static XImage *ximage = NULL;
static IMAGE imageBuffer;
static int currentScale = 2;
static int isFullscreen = 0;

// Double buffering
static Pixmap backBuffer = 0;
static GC backBufferGC;
static int bufferWidth = 0;
static int bufferHeight = 0;

// Window state for fullscreen toggle
static int savedX, savedY, savedWidth, savedHeight;
static int savedScale;

// Blinking state
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int blinkFrameCounter = 0;

// DOS thread state
typedef struct {
    int argc;
    char **argv;
    int result;
    int finished;
} DOSThreadData;

static pthread_t dosThread;
static DOSThreadData *dosData = NULL;
static volatile int running = 1;

// Forward declarations
void renderFrame();
void setScale(int scale, int resizeWindow);
void toggleFullscreen();
void handleKeyPress(XKeyEvent *event);
void handleKeyRelease(XKeyEvent *event);
void createBackBuffer(int width, int height);
void destroyBackBuffer();

void* dosThreadFunction(void *arg) {
    DOSThreadData *data = (DOSThreadData *)arg;
    printf("DOS thread started with %d arguments\n", data->argc);
    data->result = dos_main(data->argc, data->argv);
    printf("DOS thread finished with result: %d\n", data->result);
    data->finished = 1;
    return NULL;
}

void setupPCCore() {
    pccore = (PCCORE*)malloc(sizeof(PCCORE));
    pccore->mode = CGA320x200x2;
    pccore->key = 0;
    pccore->port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01;
    render(&imageBuffer, pccore);
}

void createBackBuffer(int width, int height) {
    if (backBuffer && (width != bufferWidth || height != bufferHeight)) {
        destroyBackBuffer();
    }
    
    if (!backBuffer) {
        int screen = DefaultScreen(display);
        int depth = DefaultDepth(display, screen);
        
        backBuffer = XCreatePixmap(display, window, width, height, depth);
        backBufferGC = XCreateGC(display, backBuffer, 0, NULL);
        bufferWidth = width;
        bufferHeight = height;
        
        // Clear to black
        XSetForeground(display, backBufferGC, BlackPixel(display, screen));
        XFillRectangle(display, backBuffer, backBufferGC, 0, 0, width, height);
    }
}

void destroyBackBuffer() {
    if (backBuffer) {
        XFreePixmap(display, backBuffer);
        XFreeGC(display, backBufferGC);
        backBuffer = 0;
        bufferWidth = 0;
        bufferHeight = 0;
    }
}

void createWindow() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open X display\n");
        exit(1);
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    
    int baseWidth = imageBuffer.width;
    int baseHeight = imageBuffer.height;
    int winWidth = baseWidth * currentScale;
    int winHeight = baseHeight * currentScale * (int)imageBuffer.aspect_ratio;

    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                       StructureNotifyMask | FocusChangeMask;
    attrs.background_pixel = BlackPixel(display, screen);
    attrs.backing_store = Always; // Request backing store for smoother updates

    window = XCreateWindow(display, root, 0, 0, winWidth, winHeight, 0,
                          CopyFromParent, InputOutput, CopyFromParent,
                          CWEventMask | CWBackPixel | CWBackingStore, &attrs);

    XStoreName(display, window, "PC Core Emulator");
    
    // Set WM hints
    XSizeHints *sizeHints = XAllocSizeHints();
    sizeHints->flags = PMinSize | PMaxSize;
    sizeHints->min_width = baseWidth;
    sizeHints->min_height = baseHeight * (int)imageBuffer.aspect_ratio;
    sizeHints->max_width = baseWidth * 10;
    sizeHints->max_height = baseHeight * 10 * (int)imageBuffer.aspect_ratio;
    XSetWMNormalHints(display, window, sizeHints);
    XFree(sizeHints);

    // Handle window close
    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    gc = XCreateGC(display, window, 0, NULL);
    
    XMapWindow(display, window);
    XFlush(display);
    
    // Create initial back buffer
    createBackBuffer(winWidth, winHeight);
}

void scaleAndRender(unsigned char *scaledBuffer, int dstWidth, int dstHeight) {
    if (!imageBuffer.raw) return;

    int srcWidth = imageBuffer.width;
    int srcHeight = imageBuffer.height;
    int effectiveScaleY = currentScale * (int)imageBuffer.aspect_ratio;

    // Nearest-neighbor scaling
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / effectiveScaleY;
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / currentScale;
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 4;
            
            scaledBuffer[dstIndex + 0] = imageBuffer.raw[srcIndex + 2]; // B
            scaledBuffer[dstIndex + 1] = imageBuffer.raw[srcIndex + 1]; // G
            scaledBuffer[dstIndex + 2] = imageBuffer.raw[srcIndex + 0]; // R
            scaledBuffer[dstIndex + 3] = 0; // Padding
        }
    }
}

void renderFrame() {
    if (!display || imageBuffer.width == 0 || imageBuffer.height == 0) {
        return;
    }

    // Get window size
    XWindowAttributes xwa;
    XGetWindowAttributes(display, window, &xwa);
    
    // Create or resize back buffer if needed
    createBackBuffer(xwa.width, xwa.height);

    int srcWidth = imageBuffer.width;
    int srcHeight = imageBuffer.height;
    int dstWidth = srcWidth * currentScale;
    int dstHeight = srcHeight * currentScale * (int)imageBuffer.aspect_ratio;

    // Allocate scaled buffer
    unsigned char *scaledBuffer = (unsigned char *)malloc(dstWidth * dstHeight * 4);
    if (!scaledBuffer) return;

    scaleAndRender(scaledBuffer, dstWidth, dstHeight);

    // Create XImage
    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);

    if (ximage) {
        ximage->data = (char *)scaledBuffer;
        ximage->width = dstWidth;
        ximage->height = dstHeight;
        ximage->bytes_per_line = dstWidth * 4;
    } else {
        ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
                             (char *)scaledBuffer, dstWidth, dstHeight, 32, dstWidth * 4);
    }

    if (ximage && backBuffer) {
        // Calculate centering offsets
        int xOffset = (xwa.width - dstWidth) / 2;
        int yOffset = (xwa.height - dstHeight) / 2;
        
        // Draw to back buffer (offscreen)
        XSetForeground(display, backBufferGC, BlackPixel(display, screen));
        XFillRectangle(display, backBuffer, backBufferGC, 0, 0, xwa.width, xwa.height);
        XPutImage(display, backBuffer, backBufferGC, ximage, 0, 0, xOffset, yOffset, dstWidth, dstHeight);
        
        // Copy back buffer to window (single operation - no tearing)
        XCopyArea(display, backBuffer, window, gc, 0, 0, xwa.width, xwa.height, 0, 0);
        
        // Sync to ensure the frame is displayed
        XSync(display, False);
    }

    // Note: We don't free scaledBuffer here as it's managed by ximage
}

void setScale(int scale, int resizeWindow) {
    currentScale = scale;
    
    if (resizeWindow && !isFullscreen) {
        int baseWidth = imageBuffer.width;
        int baseHeight = imageBuffer.height;
        int newWidth = baseWidth * scale;
        int newHeight = baseHeight * scale * (int)imageBuffer.aspect_ratio;
        
        XResizeWindow(display, window, newWidth, newHeight);
        XFlush(display);
    }
    
    printf("Scale set to %dx\n", scale);
}

void toggleFullscreen() {
    int screen = DefaultScreen(display);
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    
    if (isFullscreen) {
        // Exit fullscreen
        XEvent xev;
        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = window;
        xev.xclient.message_type = wmState;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
        xev.xclient.data.l[1] = wmFullscreen;
        
        XSendEvent(display, RootWindow(display, screen), False,
                   SubstructureNotifyMask | SubstructureRedirectMask, &xev);
        
        // Restore window size
        XMoveResizeWindow(display, window, savedX, savedY, savedWidth, savedHeight);
        setScale(savedScale, 0);
        
        isFullscreen = 0;
    } else {
        // Save current state
        XWindowAttributes xwa;
        XGetWindowAttributes(display, window, &xwa);
        savedX = xwa.x;
        savedY = xwa.y;
        savedWidth = xwa.width;
        savedHeight = xwa.height;
        savedScale = currentScale;
        
        // Calculate best scale for fullscreen
        Screen *screenInfo = ScreenOfDisplay(display, screen);
        int screenWidth = WidthOfScreen(screenInfo);
        int screenHeight = HeightOfScreen(screenInfo);
        
        int baseW = imageBuffer.width;
        int baseH = imageBuffer.height * (int)imageBuffer.aspect_ratio;
        
        if (baseW > 0 && baseH > 0) {
            int maxScaleX = screenWidth / baseW;
            int maxScaleY = screenHeight / baseH;
            int bestScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
            if (bestScale < 1) bestScale = 1;
            
            setScale(bestScale, 0);
        }
        
        // Enter fullscreen
        XEvent xev;
        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = window;
        xev.xclient.message_type = wmState;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
        xev.xclient.data.l[1] = wmFullscreen;
        
        XSendEvent(display, RootWindow(display, screen), False,
                   SubstructureNotifyMask | SubstructureRedirectMask, &xev);
        
        isFullscreen = 1;
    }
    
    XFlush(display);
}

void handleKeyPress(XKeyEvent *event) {
    if (!pccore) return;
    
    KeySym keysym = XLookupKeysym(event, 0);
    
    // Check for Ctrl+F (fullscreen toggle)
    if ((event->state & ControlMask) && keysym == XK_f) {
        toggleFullscreen();
        return;
    }
    
    // Check for scale shortcuts (Ctrl+1/2/3/4)
    if (event->state & ControlMask) {
        switch (keysym) {
            case XK_1: setScale(1, !isFullscreen); return;
            case XK_2: setScale(2, !isFullscreen); return;
            case XK_3: setScale(3, !isFullscreen); return;
            case XK_4: setScale(4, !isFullscreen); return;
        }
    }
    
    pccore->key = get_scancode(keysym, event->state);
    pccore->memory[BDA_KBD_STATUS_1] = get_statuscode(event->state);
    printf("Key pressed: 0x%x 0x%x\n", pccore->key, pccore->memory[BDA_KBD_STATUS_1]);
}

void handleKeyRelease(XKeyEvent *event) {
    if (!pccore) return;
    
    KeySym keysym = XLookupKeysym(event, 0);
    pccore->memory[BDA_KBD_STATUS_1] = get_statuscode(event->state);
    
    if (pccore->key == get_scancode(keysym, event->state)) {
        pccore->key = 0;
        printf("Key released\n");
    }
}

void cleanup() {
    running = 0;
    
    if (dosData) {
        pthread_kill(dosThread, SIGUSR1);
        pthread_join(dosThread, NULL);
        
        for (int i = 0; i < dosData->argc; i++) {
            free(dosData->argv[i]);
        }
        free(dosData->argv);
        free(dosData);
        dosData = NULL;
    }
    
    destroyBackBuffer();
    
    if (ximage) {
        if (ximage->data) {
            free(ximage->data);
            ximage->data = NULL;
        }
        XDestroyImage(ximage);
        ximage = NULL;
    }
    
    if (gc) XFreeGC(display, gc);
    if (window) XDestroyWindow(display, window);
    if (display) XCloseDisplay(display);
    if (pccore) free(pccore);
}

void signalHandler(int sig) {
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    setupPCCore();
    createWindow();
    
    // Start DOS thread
    dosData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    dosData->finished = 0;
    dosData->result = 0;
    dosData->argc = argc;
    dosData->argv = (char **)malloc(sizeof(char *) * (argc + 1));
    
    for (int i = 0; i < argc; i++) {
        dosData->argv[i] = strdup(argv[i]);
    }
    dosData->argv[argc] = NULL;
    
    if (pthread_create(&dosThread, NULL, dosThreadFunction, dosData) != 0) {
        fprintf(stderr, "Error creating DOS thread\n");
        cleanup();
        return 1;
    }
    
    // Main event loop
    struct timeval lastFrame, currentFrame;
    gettimeofday(&lastFrame, NULL);
    const long frameTime = 1000000 / 60; // 60 FPS in microseconds
    
    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    
    while (running) {
        // Process X events
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            
            switch (event.type) {
                case Expose:
                    renderFrame();
                    break;
                    
                case KeyPress:
                    handleKeyPress(&event.xkey);
                    break;
                    
                case KeyRelease:
                    handleKeyRelease(&event.xkey);
                    break;
                    
                case ClientMessage:
                    if (event.xclient.data.l[0] == wmDeleteWindow) {
                        running = 0;
                    }
                    break;
                    
                case ConfigureNotify:
                    // Window resized
                    renderFrame();
                    break;
            }
        }
        
        // Update and render at 60 FPS
        gettimeofday(&currentFrame, NULL);
        long elapsed = (currentFrame.tv_sec - lastFrame.tv_sec) * 1000000 +
                      (currentFrame.tv_usec - lastFrame.tv_usec);
        
        if (elapsed >= frameTime) {
            int oldWidth = imageBuffer.width;
            int oldHeight = imageBuffer.height;
            
            // Update time
            pccore->time = (long long)currentFrame.tv_sec * 1000LL + 
                          currentFrame.tv_usec / 1000;
            
            // Update blink
            blinkFrameCounter++;
            if (blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
                pccore->blink = 1 - pccore->blink;
                blinkFrameCounter = 0;
            }
            
            render(&imageBuffer, pccore);
            
            // Handle resolution changes
            if (imageBuffer.width != oldWidth || imageBuffer.height != oldHeight) {
                if (isFullscreen) {
                    toggleFullscreen(); // Exit
                    toggleFullscreen(); // Re-enter with new resolution
                } else {
                    setScale(currentScale, 1);
                }
            }
            
            renderFrame();
            lastFrame = currentFrame;
        }
        
        // Small sleep to prevent busy waiting
        usleep(1000); // 1ms
        
        // Check if DOS thread finished
        if (dosData && dosData->finished) {
            running = 0;
        }
    }
    
    cleanup();
    return 0;
}