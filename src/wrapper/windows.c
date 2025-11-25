#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "windows_keyboard.h"

extern int dos_main(int argc, char *argv[]);

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DOSThreadFunction(LPVOID lpParam);

// Structure to pass data to DOS thread
typedef struct {
    int argc;
    char **argv;
    int result;
    BOOL finished;
} DOSThreadData;

// Global state
static HWND g_hwnd = NULL;
static IMAGE imageBuffer;
static HDC g_backBufferDC = NULL;
static HBITMAP g_backBufferBitmap = NULL;
static unsigned char *g_scaledBuffer = NULL;
static size_t g_scaledBufferSize = 0;
static int g_currentScale = 2;
static BOOL g_isFullscreen = FALSE;
static RECT g_savedWindowRect;
static LONG g_savedWindowStyle;
static int g_savedScale = 2;

// Blinking state
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int blinkFrameCounter = 0;

// DOS thread
static HANDLE g_dosThread = NULL;
static DOSThreadData *g_dosData = NULL;

// Window class name
static const char* WINDOW_CLASS_NAME = "PCCoreEmulatorWindow";

/**
 * @brief Scale the pixel buffer using nearest-neighbor
 */
void ScalePixelBuffer(int scale) {
    if (imageBuffer.raw == NULL || imageBuffer.width == 0 || imageBuffer.height == 0) {
        return;
    }
    
    int srcWidth = imageBuffer.width;
    int srcHeight = imageBuffer.height;
    int effectiveScaleY = scale * (int)imageBuffer.aspect_ratio;
    
    int dstWidth = srcWidth * scale;
    int dstHeight = srcHeight * effectiveScaleY;
    
    size_t requiredSize = dstWidth * dstHeight * 4; // RGBA
    
    if (g_scaledBuffer == NULL || g_scaledBufferSize != requiredSize) {
        if (g_scaledBuffer) {
            free(g_scaledBuffer);
        }
        g_scaledBuffer = (unsigned char *)malloc(requiredSize);
        g_scaledBufferSize = requiredSize;
    }
    
    unsigned char *src = imageBuffer.raw;
    
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / effectiveScaleY;
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / scale;
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 4;
            
            // Convert RGB to BGRA for Windows
            g_scaledBuffer[dstIndex + 0] = src[srcIndex + 2]; // B
            g_scaledBuffer[dstIndex + 1] = src[srcIndex + 1]; // G
            g_scaledBuffer[dstIndex + 2] = src[srcIndex + 0]; // R
            g_scaledBuffer[dstIndex + 3] = 255;                // A
        }
    }
}

/**
 * @brief Render the frame to the window
 */
void RenderFrame(HDC hdc, int width, int height) {
    // Fill background with black
    RECT clientRect;
    GetClientRect(g_hwnd, &clientRect);
    FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    if (imageBuffer.raw == NULL || imageBuffer.width == 0 || imageBuffer.height == 0) {
        return;
    }
    
    ScalePixelBuffer(g_currentScale);
    
    if (g_scaledBuffer == NULL) {
        return;
    }
    
    int srcWidth = imageBuffer.width;
    int srcHeight = imageBuffer.height;
    int dstWidth = srcWidth * g_currentScale;
    int dstHeight = srcHeight * g_currentScale * (int)imageBuffer.aspect_ratio;
    
    // Create bitmap info
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = dstWidth;
    bmi.bmiHeader.biHeight = -dstHeight; // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // Calculate centering offsets
    int xOffset = (clientRect.right - dstWidth) / 2;
    int yOffset = (clientRect.bottom - dstHeight) / 2;
    
    // Draw the bitmap
    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(hdc, xOffset, yOffset, dstWidth, dstHeight,
                  0, 0, dstWidth, dstHeight,
                  g_scaledBuffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

/**
 * @brief Set the window scale
 */
void SetScale(int scale, BOOL resizeWindow) {
    g_currentScale = scale;
    
    if (resizeWindow && !g_isFullscreen) {
        int baseWidth = imageBuffer.width;
        int baseHeight = imageBuffer.height;
        
        int newWidth = baseWidth * scale;
        int newHeight = baseHeight * scale * (int)imageBuffer.aspect_ratio;
        
        // Calculate window size including borders
        RECT rect = {0, 0, newWidth, newHeight};
        DWORD style = GetWindowLong(g_hwnd, GWL_STYLE);
        DWORD exStyle = GetWindowLong(g_hwnd, GWL_EXSTYLE);
        AdjustWindowRectEx(&rect, style, TRUE, exStyle);
        
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        
        // Center window
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        int x = (workArea.right - windowWidth) / 2;
        int y = (workArea.bottom - windowHeight) / 2;
        
        SetWindowPos(g_hwnd, NULL, x, y, windowWidth, windowHeight, 
                     SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    
    InvalidateRect(g_hwnd, NULL, FALSE);
    printf("Scale set to %dx\n", scale);
}

/**
 * @brief Toggle fullscreen mode
 */
void ToggleFullscreen(void) {
    if (g_isFullscreen) {
        // Exit fullscreen
        SetWindowLong(g_hwnd, GWL_STYLE, g_savedWindowStyle);
        SetWindowPos(g_hwnd, HWND_NOTOPMOST, 
                     g_savedWindowRect.left, g_savedWindowRect.top,
                     g_savedWindowRect.right - g_savedWindowRect.left,
                     g_savedWindowRect.bottom - g_savedWindowRect.top,
                     SWP_FRAMECHANGED);
        
        g_currentScale = g_savedScale;
        g_isFullscreen = FALSE;
        
    } else {
        // Enter fullscreen
        GetWindowRect(g_hwnd, &g_savedWindowRect);
        g_savedWindowStyle = GetWindowLong(g_hwnd, GWL_STYLE);
        g_savedScale = g_currentScale;
        
        // Get monitor info
        HMONITOR hMonitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {sizeof(MONITORINFO)};
        GetMonitorInfo(hMonitor, &mi);
        
        // Calculate best scale
        int baseW = imageBuffer.width;
        int baseH = imageBuffer.height * (int)imageBuffer.aspect_ratio;
        
        if (baseW > 0 && baseH > 0) {
            int screenWidth = mi.rcMonitor.right - mi.rcMonitor.left;
            int screenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
            
            int maxScaleX = screenWidth / baseW;
            int maxScaleY = screenHeight / baseH;
            int bestScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
            if (bestScale < 1) bestScale = 1;
            
            g_currentScale = bestScale;
            
            // Remove window decorations and maximize
            SetWindowLong(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(g_hwnd, HWND_TOPMOST,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         screenWidth, screenHeight,
                         SWP_FRAMECHANGED);
            
            g_isFullscreen = TRUE;
        }
    }
    
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/**
 * @brief DOS thread function
 */
DWORD WINAPI DOSThreadFunction(LPVOID lpParam) {
    DOSThreadData *data = (DOSThreadData *)lpParam;
    printf("DOS thread started with %d arguments\n", data->argc);
    data->result = dos_main(data->argc, data->argv);
    printf("DOS thread finished with result: %d\n", data->result);
    data->finished = TRUE;
    return 0;
}

/**
 * @brief Window procedure
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 1000/60, NULL); // 60 FPS timer
            return 0;
            
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderFrame(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER:
            if (wParam == 1) {
                // Update time
                struct _timeb tb;
                _ftime(&tb);
                pccore.time = (long long)tb.time * 1000LL + tb.millitm;
                
                // Update blink
                blinkFrameCounter++;
                if (blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
                    pccore.blink = 1 - pccore.blink;
                    blinkFrameCounter = 0;
                }
                
                // Render
                int oldWidth = imageBuffer.width;
                int oldHeight = imageBuffer.height;
                
                render(&imageBuffer, pccore);
                
                // Handle resolution changes
                if (imageBuffer.width != oldWidth || imageBuffer.height != oldHeight) {
                    if (g_isFullscreen) {
                        ToggleFullscreen();
                        ToggleFullscreen();
                    } else {
                        SetScale(g_currentScale, TRUE);
                    }
                }
                
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (wParam == VK_F11 || (wParam == 'F' && (GetAsyncKeyState(VK_CONTROL) & 0x8000))) {
                ToggleFullscreen();
                return 0;
            }
            
            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);
            
            pccore.key = get_scancode(wParam, lParam, keyboardState);
            pccore.memory[BDA_KBD_STATUS_1] = get_statuscode(keyboardState);
            printf("Key pressed: 0x%x 0x%x\n", pccore.key, pccore.memory[BDA_KBD_STATUS_1]);
            return 0;
        }
        
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);
            
            int scancode = get_scancode(wParam, lParam, keyboardState);
            pccore.memory[BDA_KBD_STATUS_1] = get_statuscode(keyboardState);
            
            if (pccore.key == scancode) {
                pccore.key = 0;
                printf("Key released\n");
            }
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case 1001: SetScale(1, !g_isFullscreen); return 0;
                case 1002: SetScale(2, !g_isFullscreen); return 0;
                case 1003: SetScale(3, !g_isFullscreen); return 0;
                case 1004: SetScale(4, !g_isFullscreen); return 0;
                case 1010: ToggleFullscreen(); return 0;
                case 1020: PostQuitMessage(0); return 0;
            }
            break;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief Create the menu bar
 */
void CreateMenuBar(HWND hwnd) {
    HMENU hMenuBar = CreateMenu();
    
    // View Menu
    HMENU hViewMenu = CreateMenu();
    AppendMenu(hViewMenu, MF_STRING, 1010, "Toggle Full Screen\tF11");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, 1001, "1x Scale\tCtrl+1");
    AppendMenu(hViewMenu, MF_STRING | MF_CHECKED, 1002, "2x Scale\tCtrl+2");
    AppendMenu(hViewMenu, MF_STRING, 1003, "3x Scale\tCtrl+3");
    AppendMenu(hViewMenu, MF_STRING, 1004, "4x Scale\tCtrl+4");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, "View");
    
    // File Menu (for Quit)
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, 1020, "Quit\tAlt+F4");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    
    SetMenu(hwnd, hMenuBar);
}

/**
 * @brief Initialize pccore
 */
void SetupPCCore(void) {
    memset(&pccore, 0, sizeof(PCCORE));
    pccore.mode = CGA320x200x2;
    pccore.key = 0;
    pccore.port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01;
    render(&imageBuffer, pccore);
}

/**
 * @brief Start the DOS thread
 */
void StartDOSThread(void) {
    g_dosData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    g_dosData->finished = FALSE;
    g_dosData->result = 0;
    
    // Get command line arguments
    int argc = __argc;
    char **argv = __argv;
    
    g_dosData->argc = argc;
    g_dosData->argv = (char **)malloc(sizeof(char *) * (argc + 1));
    
    for (int i = 0; i < argc; i++) {
        g_dosData->argv[i] = _strdup(argv[i]);
    }
    g_dosData->argv[argc] = NULL;
    
    g_dosThread = CreateThread(NULL, 0, DOSThreadFunction, g_dosData, 0, NULL);
    if (g_dosThread == NULL) {
        printf("Error creating DOS thread\n");
        free(g_dosData);
        g_dosData = NULL;
    }
}

/**
 * @brief WinMain entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Register window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Initialize pccore
    SetupPCCore();
    
    // Calculate initial window size
    int baseWidth = imageBuffer.width;
    int baseHeight = imageBuffer.height;
    int windowWidth = baseWidth * g_currentScale;
    int windowHeight = baseHeight * g_currentScale * (int)imageBuffer.aspect_ratio;
    
    RECT rect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, TRUE, 0);
    
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;
    
    // Center window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // Create window
    g_hwnd = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME,
        "PC Core Emulator",
        WS_OVERLAPPEDWINDOW,
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );
    
    if (g_hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONERROR);
        return 1;
    }
    
    CreateMenuBar(g_hwnd);
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    
    // Start DOS thread
    StartDOSThread();
    
    // Message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if (g_dosThread) {
        WaitForSingleObject(g_dosThread, INFINITE);
        CloseHandle(g_dosThread);
    }
    
    if (g_dosData) {
        for (int i = 0; i < g_dosData->argc; i++) {
            free(g_dosData->argv[i]);
        }
        free(g_dosData->argv);
        free(g_dosData);
    }
    
    if (g_scaledBuffer) {
        free(g_scaledBuffer);
    }
    
    return (int)msg.wParam;
}