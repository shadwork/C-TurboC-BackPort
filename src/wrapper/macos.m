@import AppKit;

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "macos_keyboard.h"
#include <string.h> // For memset
#include <pthread.h> // For threading
#include <AppKit/NSEvent.h> // For key codes
#include <sys/time.h> // For gettimeofday

extern "C" int dos_main(int argc, char *argv[]);

// --- Forward Declaration
@class PixelRenderView;

// Structure to pass data to DOS thread
typedef struct {
    int argc;
    char **argv;
    int result;
    BOOL finished;
} DOSThreadData;

// --- Global/File-scope state for blinking ---
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int blinkFrameCounter = 0; 

/**
 * @brief AppDelegate
 * Manages the application's lifecycle, window, and render loop.
 */
@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
    PixelRenderView *renderView; 
    
    IMAGE imageBuffer; 
    
    pthread_t dosThread;      
    DOSThreadData *dosData;   
    
    NSInteger currentScale;   // Current integer scale
    
    // Fullscreen state tracking
    BOOL isFullscreen;
    NSRect savedWindowFrame;
    NSInteger savedScale;
}

- (void)setScale:(NSInteger)scale resizeWindow:(BOOL)shouldResize;
- (void)toggleFullscreen:(id)sender;
- (void)createMenuBar;
@end


/**
 * @brief PixelRenderView
 * A custom NSView that draws pixels directly with perfect integer scaling.
 */
@interface PixelRenderView : NSView {
    PCCORE *pccore_ptr; 
    IMAGE *imageBuffer_ptr; 
    NSInteger pixelScale; 
    unsigned char *scaledBuffer; 
    size_t scaledBufferSize; 
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
    
    // Free old buffer to force regeneration
    if (scaledBuffer) {
        free(scaledBuffer);
        scaledBuffer = NULL;
        scaledBufferSize = 0;
    }
    
    [self setNeedsDisplay:YES];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    if (pccore_ptr == NULL || [event isARepeat]) {
        return;
    }
    // Handle Fullscreen Toggle Shortcut (Cmd + F)
    if (([event modifierFlags] & NSEventModifierFlagCommand) && [event keyCode] == 3) { // 3 is 'F'
        [NSApp sendAction:@selector(toggleFullscreen:) to:nil from:self];
        return;
    }
    
    pccore_ptr->key = get_scancode(event);
    pccore_ptr->memory[BDA_KBD_STATUS_1] = get_statuscode(event);
    printf("Key pressed: 0x%x 0x%x\n", pccore_ptr->key, pccore_ptr->memory[BDA_KBD_STATUS_1]);
}

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
    
    NSInteger effectiveScaleY = pixelScale * (int)imageBuffer_ptr->aspect_ratio;
    
    NSInteger dstWidth = srcWidth * pixelScale;
    NSInteger dstHeight = srcHeight * effectiveScaleY;
    
    size_t requiredSize = dstWidth * dstHeight * 3;
    
    if (scaledBuffer == NULL || scaledBufferSize != requiredSize) {
        if (scaledBuffer) {
            free(scaledBuffer);
        }
        scaledBuffer = (unsigned char *)malloc(requiredSize);
        scaledBufferSize = requiredSize;
    }
    
    unsigned char *src = imageBuffer_ptr->raw;
    
    for (NSInteger dstY = 0; dstY < dstHeight; dstY++) {
        NSInteger srcY = dstY / effectiveScaleY;
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
    // 1. Paint the background black (for borders in fullscreen)
    [[NSColor blackColor] set];
    NSRectFill(dirtyRect);

    if (imageBuffer_ptr == NULL || imageBuffer_ptr->width == 0 || imageBuffer_ptr->height == 0) {
        return;
    }
    
    // 2. Scale the pixel buffer
    [self scalePixelBuffer];
    
    if (scaledBuffer == NULL) {
        return;
    }
    
    NSInteger srcWidth = imageBuffer_ptr->width;
    NSInteger srcHeight = imageBuffer_ptr->height;
    
    // Calculate actual dimensions of the image to be drawn
    NSInteger dstWidth = srcWidth * pixelScale;
    NSInteger dstHeight = srcHeight * pixelScale * (int)imageBuffer_ptr->aspect_ratio;
    
    // 3. Create bitmap
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
    
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    
    // 4. Calculate Centering Offsets
    // self.bounds is the size of the view (could be window size or screen size)
    CGFloat viewWidth = self.bounds.size.width;
    CGFloat viewHeight = self.bounds.size.height;
    
    CGFloat xOffset = floor((viewWidth - dstWidth) / 2.0);
    CGFloat yOffset = floor((viewHeight - dstHeight) / 2.0);
    
    // 5. Draw Centered
    NSRect imageRect = NSMakeRect(xOffset, yOffset, dstWidth, dstHeight);
    [rep drawInRect:imageRect];
}

@end


void* dosThreadFunction(void *arg) {
    DOSThreadData *data = (DOSThreadData *)arg;
    printf("DOS thread started with %d arguments\n", data->argc);
    data->result = dos_main(data->argc, data->argv);
    printf("DOS thread finished with result: %d\n", data->result);
    data->finished = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSNotificationCenter defaultCenter] 
            postNotificationName:@"DOSThreadFinished" 
                          object:nil];
    });
    return NULL;
}


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    currentScale = 2;
    isFullscreen = NO;
    
    [self setuppccore];
    [self setupWindow];
    [self createMenuBar];
    [NSApp activateIgnoringOtherApps:YES];
    
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                     target:self
                                   selector:@selector(renderAndUpdate:)
                                   userInfo:nil
                                    repeats:YES];
    
    [[NSNotificationCenter defaultCenter] 
        addObserver:self 
           selector:@selector(dosThreadDidFinish:) 
               name:@"DOSThreadFinished" 
             object:nil];
    
    [self startDOSThread];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

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

- (void)createMenuBar {
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // App Menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // View Menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] initWithTitle:@"View" action:nil keyEquivalent:@""];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    
    NSMenuItem *fullScreenItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Full Screen" action:@selector(toggleFullscreen:) keyEquivalent:@"f"];
    [viewMenu addItem:fullScreenItem];
    [viewMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *scale1x = [[NSMenuItem alloc] initWithTitle:@"1x Scale" action:@selector(scale1x:) keyEquivalent:@"1"];
    NSMenuItem *scale2x = [[NSMenuItem alloc] initWithTitle:@"2x Scale" action:@selector(scale2x:) keyEquivalent:@"2"];
    NSMenuItem *scale3x = [[NSMenuItem alloc] initWithTitle:@"3x Scale" action:@selector(scale3x:) keyEquivalent:@"3"];
    NSMenuItem *scale4x = [[NSMenuItem alloc] initWithTitle:@"4x Scale" action:@selector(scale4x:) keyEquivalent:@"4"];
    
    [scale1x setTarget:self];
    [scale2x setTarget:self];
    [scale3x setTarget:self];
    [scale4x setTarget:self];
    
    [scale2x setState:NSControlStateValueOn];
    
    [viewMenu addItem:scale1x];
    [viewMenu addItem:scale2x];
    [viewMenu addItem:scale3x];
    [viewMenu addItem:scale4x];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    [NSApp setMainMenu:mainMenu];
}

- (void)scale1x:(id)sender { [self setScale:1 resizeWindow:!isFullscreen]; [self updateMenuCheckmarks:sender]; }
- (void)scale2x:(id)sender { [self setScale:2 resizeWindow:!isFullscreen]; [self updateMenuCheckmarks:sender]; }
- (void)scale3x:(id)sender { [self setScale:3 resizeWindow:!isFullscreen]; [self updateMenuCheckmarks:sender]; }
- (void)scale4x:(id)sender { [self setScale:4 resizeWindow:!isFullscreen]; [self updateMenuCheckmarks:sender]; }

- (void)updateMenuCheckmarks:(id)sender {
    NSMenuItem *clickedItem = (NSMenuItem *)sender;
    NSMenu *menu = [clickedItem menu];
    for (NSMenuItem *item in [menu itemArray]) {
        // Don't uncheck the fullscreen toggle
        if ([[item title] isEqualToString:@"Toggle Full Screen"]) continue;
        [item setState:NSControlStateValueOff];
    }
    [clickedItem setState:NSControlStateValueOn];
}

/**
 * @brief Sets the internal pixel scale and optionally resizes the window.
 */
- (void)setScale:(NSInteger)scale resizeWindow:(BOOL)shouldResize {
    currentScale = scale;
    
    if (shouldResize) {
        const CGFloat baseWidth = imageBuffer.width;
        const CGFloat baseHeight = imageBuffer.height;
        
        NSSize newSize = NSMakeSize(baseWidth * scale, 
                                    baseHeight * scale * (int)imageBuffer.aspect_ratio);
        
        NSRect windowFrame = [window frame];
        NSRect contentRect = [window contentRectForFrameRect:windowFrame];
        
        CGFloat widthDiff = newSize.width - contentRect.size.width;
        CGFloat heightDiff = newSize.height - contentRect.size.height;
        
        windowFrame.size.width += widthDiff;
        windowFrame.size.height += heightDiff;
        windowFrame.origin.y -= heightDiff;
        
        [window setFrame:windowFrame display:YES animate:YES];
    }
    
    // Update the render view's scale (this will trigger a redraw)
    [renderView setPixelScale:scale];
    
    printf("Scale set to %ldx\n", (long)scale);
}

/**
 * @brief Toggles between Windowed and Fullscreen mode with integer scaling and black borders.
 */
- (void)toggleFullscreen:(id)sender {
    if (isFullscreen) {
        // --- EXIT Fullscreen ---
        
        // Restore window style
        [window setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable];
        [window setLevel:NSNormalWindowLevel];
        
        // Restore previous scale and frame
        [window setFrame:savedWindowFrame display:YES animate:YES];
        [self setScale:savedScale resizeWindow:NO]; // Don't resize, we just set the frame above
        
        [window setTitle:@"PC Core Emulator"];
        isFullscreen = NO;
        
    } else {
        // --- ENTER Fullscreen ---
        
        // Save current state
        savedWindowFrame = [window frame];
        savedScale = currentScale;
        
        // Get screen dimensions
        NSScreen *screen = [NSScreen mainScreen];
        NSRect screenRect = [screen frame];
        
        // Calculate max integer scale
        CGFloat baseW = imageBuffer.width;
        CGFloat baseH = imageBuffer.height * (int)imageBuffer.aspect_ratio;
        
        if (baseW > 0 && baseH > 0) {
            NSInteger maxScaleX = (NSInteger)(screenRect.size.width / baseW);
            NSInteger maxScaleY = (NSInteger)(screenRect.size.height / baseH);
            
            // Use the smaller of the two to ensure it fits
            NSInteger bestScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
            if (bestScale < 1) bestScale = 1;
            
            // Set to borderless and fill screen
            [window setStyleMask:NSWindowStyleMaskBorderless];
            [window setLevel:NSMainMenuWindowLevel + 1]; // Cover menu bar and dock
            [window setFrame:screenRect display:YES animate:YES];
            
            // Set the scale (PixelRenderView will center it automatically in drawRect)
            [self setScale:bestScale resizeWindow:NO];
            
            isFullscreen = YES;
        }
    }
    
    // Ensure we keep focus for keyboard input
    [window makeFirstResponder:renderView];
}

- (void)startDOSThread {
    dosData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    dosData->finished = NO;
    dosData->result = 0;
    
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    dosData->argc = (int)[arguments count];
    dosData->argv = (char **)malloc(sizeof(char *) * (dosData->argc + 1));
    
    for (int i = 0; i < dosData->argc; i++) {
        NSString *arg = arguments[i];
        const char *cStr = [arg UTF8String];
        dosData->argv[i] = strdup(cStr);
    }
    dosData->argv[dosData->argc] = NULL;
    
    int result = pthread_create(&dosThread, NULL, dosThreadFunction, dosData);
    if (result != 0) {
        NSLog(@"Error creating DOS thread: %d", result);
        free(dosData);
        dosData = NULL;
    }
}

- (void)dosThreadDidFinish:(NSNotification *)notification {
    if (dosData) {
        pthread_join(dosThread, NULL);
        for (int i = 0; i < dosData->argc; i++) free(dosData->argv[i]);
        free(dosData->argv);
        free(dosData);
        dosData = NULL;
    }
}

- (void)setuppccore {
    pccore = (PCCORE*)malloc(sizeof(PCCORE));
    pccore->mode = CGA320x200x2;
    pccore->key = 0;
    pccore->port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01; 
    render(&imageBuffer, pccore);
}

- (void)setupWindow {
    const CGFloat baseWidth = imageBuffer.width;
    const CGFloat baseHeight = imageBuffer.height;

    NSRect contentRect = NSMakeRect(0, 0, 
                                    baseWidth * currentScale, 
                                    baseHeight * currentScale * (int)imageBuffer.aspect_ratio);
    
    NSWindowStyleMask style = NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable;

    window = [[NSWindow alloc] initWithContentRect:contentRect
                                         styleMask:style
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    
    [window setTitle:@"PC Core Emulator"];
    [window center];
    [window setAcceptsMouseMovedEvents:YES];
    [window setLevel:NSNormalWindowLevel];

    renderView = [[PixelRenderView alloc] initWithFrame:contentRect 
                                                 pccore:pccore 
                                            imageBuffer:&imageBuffer 
                                                  scale:currentScale];
    
    [window setContentView:renderView];
    [window makeKeyAndOrderFront:nil];
    [window setInitialFirstResponder:renderView];
    [window makeFirstResponder:renderView];
}

- (void)renderAndUpdate:(NSTimer *)timer {
    const int oldWidth = imageBuffer.width;
    const int oldHeight = imageBuffer.height;

    struct timeval te; 
    gettimeofday(&te, NULL); 
    pccore->time = (long long)te.tv_sec * 1000LL + te.tv_usec / 1000;

    blinkFrameCounter++;
    if (blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
        pccore->blink = 1 - pccore->blink; 
        blinkFrameCounter = 0;
    }

    render(&imageBuffer, pccore);

    // If resolution changed dynamically during execution
    if (imageBuffer.width != oldWidth || imageBuffer.height != oldHeight) {
        if (isFullscreen) {
            // Re-calculate best fit for new resolution while staying full screen
            [self toggleFullscreen:nil]; // Toggle off
            [self toggleFullscreen:nil]; // Toggle on (re-calculates math)
        } else {
            [self setScale:currentScale resizeWindow:YES];
        }
    }

    if (imageBuffer.width > 0 && imageBuffer.height > 0) {
        [renderView setNeedsDisplay:YES];
    }
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}