#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "windows_keyboard.h"

extern int dos_main(int argc, char *argv[]);

// --- Global state ---
static HWND g_hwnd = NULL;
static IMAGE g_imageBuffer = {0};
static int g_currentScale = 2;
static BITMAPINFO g_bitmapInfo = {0};
static unsigned char *g_scaledBuffer = NULL;
static size_t g_scaledBufferSize = 0;

// DOS thread
static HANDLE g_dosThread = NULL;
static DWORD g_dosThreadId = 0;
static volatile BOOL g_dosFinished = FALSE;
static int g_dosResult = 0;

// Blink state
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int g_blinkFrameCounter = 0;

// Command line args for DOS thread
static int g_argc = 0;
static char **g_argv = NULL;

// --- Forward declarations ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializePCCore(void);
void RenderFrame(void);
void ScalePixelBuffer(void);
void SetWindowScale(int scale);
DWORD WINAPI DOSThreadFunction(LPVOID lpParam);
void CleanupDOSThread(void);

/**
 * @brief DOS thread function
 */
DWORD WINAPI DOSThreadFunction(LPVOID lpParam) {
    printf("DOS thread started with %d arguments\n", g_argc);
    
    // Call the DOS main function
    g_dosResult = dos_main(g_argc, g_argv);
    
    printf("DOS thread finished with result: %d\n", g_dosResult);
    
    // Mark as finished
    g_dosFinished = TRUE;
    
    return 0;
}

/**
 * @brief Initialize the PCCORE struct
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
}

/**
 * @brief Scale the pixel buffer using nearest-neighbor
 */
void ScalePixelBuffer(void) {
    if (g_imageBuffer.raw == NULL || g_imageBuffer.width == 0 || g_imageBuffer.height == 0) {
        return;
    }
    
    int srcWidth = g_imageBuffer.width;
    int srcHeight = g_imageBuffer.height;
    int dstWidth = srcWidth * g_currentScale;
    int dstHeight = srcHeight * g_currentScale;
    
    size_t requiredSize = dstWidth * dstHeight * 3;
    
    // Allocate or reallocate buffer if needed
    if (g_scaledBuffer == NULL || g_scaledBufferSize != requiredSize) {
        if (g_scaledBuffer) {
            free(g_scaledBuffer);
        }
        g_scaledBuffer = (unsigned char *)malloc(requiredSize);
        g_scaledBufferSize = requiredSize;
    }
    
    unsigned char *src = g_imageBuffer.raw;
    
    // Fast nearest-neighbor scaling
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / g_currentScale;
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / g_currentScale;
            
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 3;
            
            g_scaledBuffer[dstIndex] = src[srcIndex];
            g_scaledBuffer[dstIndex + 1] = src[srcIndex + 1];
            g_scaledBuffer[dstIndex + 2] = src[srcIndex + 2];
        }
    }
}

/**
 * @brief Render a single frame
 */
void RenderFrame(void) {
    // Store current dimensions before rendering
    const int oldWidth = g_imageBuffer.width;
    const int oldHeight = g_imageBuffer.height;

    // Update pccore.time with system milliseconds
    DWORD ticks = GetTickCount();
    pccore.time = (long long)ticks;

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
        SetWindowScale(g_currentScale);
    }

    // Trigger window redraw
    if (g_hwnd) {
        InvalidateRect(g_hwnd, NULL, FALSE);
    }
}

/**
 * @brief Set window scale
 */
void SetWindowScale(int scale) {
    if (scale < 1 || scale > 4) {
        return;
    }
    
    g_currentScale = scale;
    
    if (g_hwnd && g_imageBuffer.width > 0 && g_imageBuffer.height > 0) {
        int newWidth = g_imageBuffer.width * scale;
        int newHeight = g_imageBuffer.height * scale;
        
        // Calculate window size including borders
        RECT rect = {0, 0, newWidth, newHeight};
        DWORD style = GetWindowLong(g_hwnd, GWL_STYLE);
        DWORD exStyle = GetWindowLong(g_hwnd, GWL_EXSTYLE);
        AdjustWindowRectEx(&rect, style, TRUE, exStyle); // TRUE for menu bar
        
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        
        // Resize window (keeping position)
        RECT currentRect;
        GetWindowRect(g_hwnd, &currentRect);
        SetWindowPos(g_hwnd, NULL, currentRect.left, currentRect.top, 
                     windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
        
        printf("Scale set to %dx (%dx%d)\n", scale, newWidth, newHeight);
    }
}

/**
 * @brief Window procedure
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (g_imageBuffer.width > 0 && g_imageBuffer.height > 0) {
                // Scale the pixel buffer
                ScalePixelBuffer();
                
                if (g_scaledBuffer) {
                    int dstWidth = g_imageBuffer.width * g_currentScale;
                    int dstHeight = g_imageBuffer.height * g_currentScale;
                    
                    // Setup bitmap info if needed
                    if (g_bitmapInfo.bmiHeader.biSize == 0) {
                        g_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        g_bitmapInfo.bmiHeader.biPlanes = 1;
                        g_bitmapInfo.bmiHeader.biBitCount = 24;
                        g_bitmapInfo.bmiHeader.biCompression = BI_RGB;
                    }
                    
                    g_bitmapInfo.bmiHeader.biWidth = dstWidth;
                    g_bitmapInfo.bmiHeader.biHeight = -dstHeight; // Negative for top-down
                    
                    // Disable interpolation for crisp pixels
                    SetStretchBltMode(hdc, COLORONCOLOR);
                    
                    // Draw the bitmap
                    StretchDIBits(hdc,
                        0, 0, dstWidth, dstHeight,
                        0, 0, dstWidth, dstHeight,
                        g_scaledBuffer,
                        &g_bitmapInfo,
                        DIB_RGB_COLORS,
                        SRCCOPY);
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_KEYDOWN: {
            if (lParam & (1 << 30)) { // Check repeat bit
                return 0; // Ignore key repeats
            }
            
            int scancode = get_scancode(wParam, lParam);
            if (scancode != 0) {
                pccore.key = scancode;
                pccore.memory[BDA_KBD_STATUS_1] = get_statuscode();
                printf("Key pressed: 0x%x 0x%x\n", pccore.key, pccore.memory[BDA_KBD_STATUS_1]);
            }
            return 0;
        }
        
        case WM_KEYUP: {
            int scancode = get_scancode(wParam, lParam);
            if (pccore.key == scancode) {
                pccore.key = 0;
                printf("Key released\n");
            }
            pccore.memory[BDA_KBD_STATUS_1] = get_statuscode();
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case 1001: SetWindowScale(1); return 0;
                case 1002: SetWindowScale(2); return 0;
                case 1003: SetWindowScale(3); return 0;
                case 1004: SetWindowScale(4); return 0;
                case 1005: PostQuitMessage(0); return 0;
            }
            break;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief Cleanup DOS thread
 */
void CleanupDOSThread(void) {
    if (g_dosThread) {
        // Wait for thread to finish (with timeout)
        WaitForSingleObject(g_dosThread, 5000);
        CloseHandle(g_dosThread);
        g_dosThread = NULL;
    }
    
    if (g_argv) {
        for (int i = 0; i < g_argc; i++) {
            free(g_argv[i]);
        }
        free(g_argv);
        g_argv = NULL;
    }
}

/**
 * @brief Application entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    // Get command line arguments
    int nArgs;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    
    if (szArglist == NULL) {
        printf("CommandLineToArgvW failed\n");
        return 1;
    }
    
    // Convert to char** for dos_main
    g_argc = nArgs;
    g_argv = (char **)malloc(sizeof(char *) * (g_argc + 1));
    
    for (int i = 0; i < g_argc; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, NULL, 0, NULL, NULL);
        g_argv[i] = (char *)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, g_argv[i], len, NULL, NULL);
    }
    g_argv[g_argc] = NULL;
    
    LocalFree(szArglist);
    
    // Initialize PC Core
    InitializePCCore();
    
    // Register window class
    const char CLASS_NAME[] = "PCCoreWindowClass";
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    
    RegisterClassA(&wc);
    
    // Create menu bar
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hViewMenu = CreateMenu();
    
    AppendMenuA(hFileMenu, MF_STRING, 1005, "E&xit");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
    
    AppendMenuA(hViewMenu, MF_STRING, 1001, "1x Scale\tCtrl+1");
    AppendMenuA(hViewMenu, MF_STRING, 1002, "2x Scale\tCtrl+2");
    AppendMenuA(hViewMenu, MF_STRING, 1003, "3x Scale\tCtrl+3");
    AppendMenuA(hViewMenu, MF_STRING, 1004, "4x Scale\tCtrl+4");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, "&View");
    
    // Calculate initial window size
    int baseWidth = g_imageBuffer.width * g_currentScale;
    int baseHeight = g_imageBuffer.height * g_currentScale;
    
    RECT rect = {0, 0, baseWidth, baseHeight};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&rect, style, TRUE, 0);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    // Create window
    g_hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "PC Core Emulator",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        NULL,
        hMenuBar,
        hInstance,
        NULL
    );
    
    if (g_hwnd == NULL) {
        printf("Failed to create window\n");
        return 1;
    }
    
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    
    // Start DOS thread
    g_dosThread = CreateThread(NULL, 0, DOSThreadFunction, NULL, 0, &g_dosThreadId);
    if (g_dosThread == NULL) {
        printf("Failed to create DOS thread\n");
    } else {
        printf("DOS thread created successfully\n");
    }
    
    // Setup timer for 60 FPS rendering
    SetTimer(g_hwnd, 1, 16, NULL); // ~60 FPS (16ms)
    
    // Main message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_TIMER && msg.wParam == 1) {
            RenderFrame();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    KillTimer(g_hwnd, 1);
    CleanupDOSThread();
    
    if (g_scaledBuffer) {
        free(g_scaledBuffer);
    }
    
    return (int)msg.wParam;
}
