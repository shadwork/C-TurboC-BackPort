@import AppKit;

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "macos_keyboard.h"
#include "../dosapp.h"
#include <string.h> // For memset
#include <pthread.h> // For threading
#include <AppKit/NSEvent.h> // For key codes

// --- Forward Declaration ---
@class PixelRenderView;

// Structure to pass data to DOS thread
typedef struct {
    int argc;
    char **argv;
    int result;
    BOOL finished;
} DOSThreadData;

/**
 * @brief AppDelegate
 * Manages the application's lifecycle, window, and render loop.
 */
@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
    PixelRenderView *renderView; // Custom view that draws pixels directly
    
    IMAGE imageBuffer;  // The image buffer our C code will render into
    
    pthread_t dosThread;      // Thread handle for DOS execution
    DOSThreadData *dosData;   // Data shared with DOS thread
    
    NSInteger currentScale;   // Current integer scale (1, 2, 3, or 4)
}
- (void)setScale:(NSInteger)scale;
- (void)createMenuBar;
@end


/**
 * @brief PixelRenderView
 * A custom NSView that draws pixels directly with perfect integer scaling
 * and captures keyboard input.
 */
@interface PixelRenderView : NSView {
    PCCORE *pccore_ptr; // Pointer to the main pccore struct
    IMAGE *imageBuffer_ptr; // Pointer to the image buffer
    NSInteger pixelScale; // Scale factor for each pixel
    unsigned char *scaledBuffer; // Buffer for scaled pixel data
    size_t scaledBufferSize; // Size of scaled buffer
}
- (id)initWithFrame:(NSRect)frameRect pccore:(PCCORE *)pccore imageBuffer:(IMAGE *)imgBuf scale:(NSInteger)scale;
- (void)setPixelScale:(NSInteger)scale;
- (void)dealloc;
@end

@implementation PixelRenderView

- (id)initWithFrame:(NSRect)frameRect pccore:(PCCORE *)pccore imageBuffer:(IMAGE *)imgBuf scale:(NSInteger)scale {
    self = [super initWithFrame:frameRect];
    if (self) {
        pccore_ptr = pccore;
        imageBuffer_ptr = imgBuf;
        pixelScale = scale;
        scaledBuffer = NULL;
        scaledBufferSize = 0;
    }
    return self;
}

- (void)dealloc {
    if (scaledBuffer) {
        free(scaledBuffer);
        scaledBuffer = NULL;
    }
}

- (void)setPixelScale:(NSInteger)scale {
    pixelScale = scale;
    
    // Free old buffer
    if (scaledBuffer) {
        free(scaledBuffer);
        scaledBuffer = NULL;
        scaledBufferSize = 0;
    }
    
    [self setNeedsDisplay:YES];
}

/**
 * @brief Tell the system we are willing to be the first responder
 */
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
    return YES;
}

/**
 * @brief Handle a key being pressed down.
 */
- (void)keyDown:(NSEvent *)event {
    if (pccore_ptr == NULL || [event isARepeat]) {
        return;
    }
    
    pccore_ptr->key = get_scancode(event);
    pccore_ptr->memory[BDA_KBD_STATUS_1] = get_statuscode(event);
    printf("Key pressed: 0x%x 0x%x\n", pccore_ptr->key, pccore_ptr->memory[BDA_KBD_STATUS_1]);
}

/**
 * @brief Handle a key being released.
 */
- (void)keyUp:(NSEvent *)event {
    if (pccore_ptr == NULL) {
        return;
    }
    
    pccore_ptr->memory[BDA_KBD_STATUS_1] = get_statuscode(event);
    if (pccore_ptr->key == get_scancode(event)) {
        pccore_ptr->key = 0;
        printf("Key released\n");
    }
}

/**
 * @brief Scale the pixel buffer efficiently using nearest-neighbor
 */
- (void)scalePixelBuffer {
    if (imageBuffer_ptr == NULL) {
        return;
    }
    
    NSInteger srcWidth = imageBuffer_ptr->width;
    NSInteger srcHeight = imageBuffer_ptr->height;
    NSInteger dstWidth = srcWidth * pixelScale;
    NSInteger dstHeight = srcHeight * pixelScale;
    
    size_t requiredSize = dstWidth * dstHeight * 3;
    
    // Allocate or reallocate buffer if needed
    if (scaledBuffer == NULL || scaledBufferSize != requiredSize) {
        if (scaledBuffer) {
            free(scaledBuffer);
        }
        scaledBuffer = (unsigned char *)malloc(requiredSize);
        scaledBufferSize = requiredSize;
    }
    
    unsigned char *src = imageBuffer_ptr->raw;
    
    // Fast nearest-neighbor scaling
    for (NSInteger dstY = 0; dstY < dstHeight; dstY++) {
        NSInteger srcY = dstY / pixelScale;
        for (NSInteger dstX = 0; dstX < dstWidth; dstX++) {
            NSInteger srcX = dstX / pixelScale;
            
            NSInteger srcIndex = (srcY * srcWidth + srcX) * 3;
            NSInteger dstIndex = (dstY * dstWidth + dstX) * 3;
            
            scaledBuffer[dstIndex] = src[srcIndex];
            scaledBuffer[dstIndex + 1] = src[srcIndex + 1];
            scaledBuffer[dstIndex + 2] = src[srcIndex + 2];
        }
    }
}

/**
 * @brief Draw the scaled pixels to the screen
 */
- (void)drawRect:(NSRect)dirtyRect {
    if (imageBuffer_ptr == NULL) {
        return;
    }
    
    if (imageBuffer_ptr->width == 0 || imageBuffer_ptr->height == 0) {
        return;
    }
    
    // Scale the pixel buffer
    [self scalePixelBuffer];
    
    if (scaledBuffer == NULL) {
        return;
    }
    
    NSInteger srcWidth = imageBuffer_ptr->width;
    NSInteger srcHeight = imageBuffer_ptr->height;
    NSInteger dstWidth = srcWidth * pixelScale;
    NSInteger dstHeight = srcHeight * pixelScale;
    
    // Create bitmap representation from scaled buffer
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:&scaledBuffer
                      pixelsWide:dstWidth
                      pixelsHigh:dstHeight
                   bitsPerSample:8
                 samplesPerPixel:3
                        hasAlpha:NO
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:dstWidth * 3
                    bitsPerPixel:24];
    
    if (rep == nil) {
        return;
    }
    
    // Get the current graphics context
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    
    // Disable interpolation for crisp rendering
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    
    // Draw the bitmap
    NSRect imageRect = NSMakeRect(0, 0, dstWidth, dstHeight);
    [rep drawInRect:imageRect];
}

@end


/**
 * @brief Thread function that runs dos_main
 */
void* dosThreadFunction(void *arg) {
    DOSThreadData *data = (DOSThreadData *)arg;
    
    printf("DOS thread started with %d arguments\n", data->argc);
    
    // Call the DOS main function
    data->result = dos_main(data->argc, data->argv);
    
    printf("DOS thread finished with result: %d\n", data->result);
    
    // Mark as finished
    data->finished = YES;
    
    // Post notification to main thread that DOS has finished
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSNotificationCenter defaultCenter] 
            postNotificationName:@"DOSThreadFinished" 
                          object:nil];
    });
    
    return NULL;
}


/**
 * @brief AppDelegate Implementation
 */
@implementation AppDelegate

/**
 * Called when the application has finished launching.
 */
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Set default scale
    currentScale = 2;
    
    // 1. Initialize the C pccore state
    [self setuppccore];

    // 2. Create the macOS window and views
    [self setupWindow];
    
    // 3. Create menu bar with scale options
    [self createMenuBar];

    // 4. Activate and focus the application
    [NSApp activateIgnoringOtherApps:YES];
    
    // 5. Start the render loop (aiming for 60 FPS)
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                     target:self
                                   selector:@selector(renderAndUpdate:)
                                   userInfo:nil
                                    repeats:YES];
    
    // 6. Register for DOS thread completion notification
    [[NSNotificationCenter defaultCenter] 
        addObserver:self 
           selector:@selector(dosThreadDidFinish:) 
               name:@"DOSThreadFinished" 
             object:nil];
    
    // 7. Start the DOS thread (after window is ready)
    [self startDOSThread];
}

/**
 * Allow the app to terminate when the last window is closed.
 */
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

/**
 * Called when application is about to terminate
 */
- (void)applicationWillTerminate:(NSNotification *)notification {
    
    int killStatus = pthread_kill(dosThread, SIGUSR1);
    if (killStatus != 0) {
        printf("Error sending signal to thread: %d\n", killStatus);
    }

    if (dosData) {
        free(dosData);
        dosData = NULL;
    }
}

/**
 * @brief Creates the menu bar with View > Scale options
 */
- (void)createMenuBar {
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application Menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // View Menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] initWithTitle:@"View" action:nil keyEquivalent:@""];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    
    // Scale submenu items
    NSMenuItem *scale1x = [[NSMenuItem alloc] initWithTitle:@"1x Scale" action:@selector(scale1x:) keyEquivalent:@"1"];
    NSMenuItem *scale2x = [[NSMenuItem alloc] initWithTitle:@"2x Scale" action:@selector(scale2x:) keyEquivalent:@"2"];
    NSMenuItem *scale3x = [[NSMenuItem alloc] initWithTitle:@"3x Scale" action:@selector(scale3x:) keyEquivalent:@"3"];
    NSMenuItem *scale4x = [[NSMenuItem alloc] initWithTitle:@"4x Scale" action:@selector(scale4x:) keyEquivalent:@"4"];
    
    [scale1x setTarget:self];
    [scale2x setTarget:self];
    [scale3x setTarget:self];
    [scale4x setTarget:self];
    
    // Mark 2x as default
    [scale2x setState:NSControlStateValueOn];
    
    [viewMenu addItem:scale1x];
    [viewMenu addItem:scale2x];
    [viewMenu addItem:scale3x];
    [viewMenu addItem:scale4x];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    [NSApp setMainMenu:mainMenu];
}

/**
 * @brief Scale menu action handlers
 */
- (void)scale1x:(id)sender {
    [self setScale:1];
    [self updateMenuCheckmarks:sender];
}

- (void)scale2x:(id)sender {
    [self setScale:2];
    [self updateMenuCheckmarks:sender];
}

- (void)scale3x:(id)sender {
    [self setScale:3];
    [self updateMenuCheckmarks:sender];
}

- (void)scale4x:(id)sender {
    [self setScale:4];
    [self updateMenuCheckmarks:sender];
}

/**
 * @brief Update menu checkmarks
 */
- (void)updateMenuCheckmarks:(id)sender {
    NSMenuItem *clickedItem = (NSMenuItem *)sender;
    NSMenu *menu = [clickedItem menu];
    
    for (NSMenuItem *item in [menu itemArray]) {
        [item setState:NSControlStateValueOff];
    }
    
    [clickedItem setState:NSControlStateValueOn];
}

/**
 * @brief Set the window scale
 */
- (void)setScale:(NSInteger)scale {
    currentScale = scale;
    
    const CGFloat baseWidth = imageBuffer.width;
    const CGFloat baseHeight = imageBuffer.height;
    
    // Calculate new content size
    NSSize newSize = NSMakeSize(baseWidth * scale, baseHeight * scale);
    
    // Get current window frame
    NSRect windowFrame = [window frame];
    NSRect contentRect = [window contentRectForFrameRect:windowFrame];
    
    // Calculate the difference in size
    CGFloat widthDiff = newSize.width - contentRect.size.width;
    CGFloat heightDiff = newSize.height - contentRect.size.height;
    
    // Adjust window frame (keep top-left corner in place)
    windowFrame.size.width += widthDiff;
    windowFrame.size.height += heightDiff;
    windowFrame.origin.y -= heightDiff; // Adjust Y to keep top in place
    
    // Set the new frame
    [window setFrame:windowFrame display:YES animate:YES];
    
    // Update the render view's scale
    [renderView setPixelScale:scale];
    
    printf("Scale set to %ldx (%0.fx%0.f)\n", (long)scale, newSize.width, newSize.height);
}

/**
 * @brief Starts the DOS thread with command line arguments
 */
- (void)startDOSThread {
    // Allocate thread data
    dosData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    dosData->finished = NO;
    dosData->result = 0;
    
    // Get command line arguments from NSProcessInfo
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    dosData->argc = (int)[arguments count];
    
    // Allocate argv array
    dosData->argv = (char **)malloc(sizeof(char *) * (dosData->argc + 1));
    
    // Copy arguments
    for (int i = 0; i < dosData->argc; i++) {
        NSString *arg = arguments[i];
        const char *cStr = [arg UTF8String];
        dosData->argv[i] = strdup(cStr);
    }
    dosData->argv[dosData->argc] = NULL; // NULL terminate
    
    // Create the thread
    int result = pthread_create(&dosThread, NULL, dosThreadFunction, dosData);
    if (result != 0) {
        NSLog(@"Error creating DOS thread: %d", result);
        free(dosData);
        dosData = NULL;
    } else {
        printf("DOS thread created successfully\n");
    }
}

/**
 * @brief Called when DOS thread finishes (on main thread)
 */
- (void)dosThreadDidFinish:(NSNotification *)notification {
    printf("DOS thread completion notification received\n");
    
    if (dosData) {
        printf("DOS main returned: %d\n", dosData->result);
        
        // Wait for thread to fully complete
        pthread_join(dosThread, NULL);
        
        // Free allocated argument strings
        for (int i = 0; i < dosData->argc; i++) {
            free(dosData->argv[i]);
        }
        free(dosData->argv);
        
        // Store result for later if needed
        int finalResult = dosData->result;
        
        free(dosData);
        dosData = NULL;
        
        NSLog(@"DOS execution completed with code: %d", finalResult);
    }
}

/**
 * @brief Initializes the PCCORE struct with test data.
 */
- (void)setuppccore {
    // Zero out the entire pccore state
    memset(&pccore, 0, sizeof(PCCORE));

    // Set the requested video mode
    pccore.mode = CGA320x200x2;

    // Initialize key to 0 (meaning "no key pressed")
    pccore.key = 0;

    // Set the CGA Color Register (Port 0x3D9)
    pccore.port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01; // 0x31

    // Run one initial render to get image dimensions
    render(&imageBuffer, pccore);
}

/**
 * @brief Creates the main application window and view.
 */
- (void)setupWindow {
    // Get base dimensions from the initial render
    const CGFloat baseWidth = imageBuffer.width;
    const CGFloat baseHeight = imageBuffer.height;

    // Create the window rect at default scale (2x)
    NSRect contentRect = NSMakeRect(0, 0, baseWidth * currentScale, baseHeight * currentScale);
    
    // Create a NON-resizable window for pixel-perfect rendering
    NSWindowStyleMask style = NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable;
    // Note: NSWindowStyleMaskResizable is NOT included

    window = [[NSWindow alloc] initWithContentRect:contentRect
                                         styleMask:style
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    
    [window setTitle:@"PC Core Emulator"];
    [window center];
    
    // Configure window to accept key events
    [window setAcceptsMouseMovedEvents:YES];
    [window setLevel:NSNormalWindowLevel];

    // Create our custom pixel render view
    renderView = [[PixelRenderView alloc] initWithFrame:contentRect 
                                                 pccore:&pccore 
                                            imageBuffer:&imageBuffer 
                                                  scale:currentScale];
    
    // Set the renderView as the window's content view
    [window setContentView:renderView];
    
    // Show the window
    [window makeKeyAndOrderFront:nil];
    
    // Set initial first responder
    [window setInitialFirstResponder:renderView];
    
    // Force the view to become first responder
    [window makeFirstResponder:renderView];
    
    // Ensure window is key window
    if (![window isKeyWindow]) {
        [window makeKeyWindow];
    }
}

/**
 * @brief This is our main render loop, called by the NSTimer.
 */
- (void)renderAndUpdate:(NSTimer *)timer {
    // Call your C render function
    render(&imageBuffer, pccore);

    // Safety check
    if (imageBuffer.width == 0 || imageBuffer.height == 0) {
        return;
    }

    // Mark the view as needing display
    [renderView setNeedsDisplay:YES];
}

@end


/**
 * @brief Main application entry point.
 */
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        // Set activation policy to regular app (ensures it appears in Dock)
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        // Create and set the application delegate
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        
        // Run the application
        [app run];
    }
    return 0;
}