#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "windows_keyboard.h"

// Forward declarations
extern int dos_main(int argc, char *argv[]);
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
extern PCCORE* pccore;
IMAGE *imageBuffer;
static HDC g_hdcMem = NULL;
static HBITMAP g_hBitmap = NULL;
static unsigned char *g_bitmapBits = NULL;
static int g_bitmapWidth = 0;
static int g_bitmapHeight = 0;

// Scaling and fullscreen state
static int currentScale = 2;
static BOOL isFullscreen = FALSE;
static WINDOWPLACEMENT savedWindowPlacement = {sizeof(WINDOWPLACEMENT)};
static int savedScale = 2;

// Standard window style (No Resize Border, No Maximize button)
static const DWORD WINDOW_STYLE_WINDOWED = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

// Blinking state
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int blinkFrameCounter = 0;

// DOS thread
static HANDLE dosThread = NULL;
static DOSThreadData *dosData = NULL;

// Menu item IDs
static HMENU g_hScaleMenu = NULL;

// Function to create a DIB section for rendering (The Emulator's internal buffer)
static void CreateRenderBuffer(int width, int height) {
    if (g_hdcMem) {
        if (g_hBitmap) {
            DeleteObject(g_hBitmap);
            g_hBitmap = NULL;
        }
        DeleteDC(g_hdcMem);
        g_hdcMem = NULL;
    }
    
    HDC hdcScreen = GetDC(NULL);
    g_hdcMem = CreateCompatibleDC(hdcScreen);
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    g_hBitmap = CreateDIBSection(g_hdcMem, &bmi, DIB_RGB_COLORS, 
                                 (void**)&g_bitmapBits, NULL, 0);
    SelectObject(g_hdcMem, g_hBitmap);
    
    g_bitmapWidth = width;
    g_bitmapHeight = height;
    
    ReleaseDC(NULL, hdcScreen);
}

// Scale the image buffer to the target size
static void ScaleImageBuffer(void) {
    if (imageBuffer->raw == NULL || imageBuffer->width == 0 || imageBuffer->height == 0) {
        return;
    }
    
    int srcWidth = imageBuffer->width;
    int srcHeight = imageBuffer->height;
    int effectiveScaleY = currentScale * (int)imageBuffer->aspect_ratio;
    int dstWidth = srcWidth * currentScale;
    int dstHeight = srcHeight * effectiveScaleY;
    
    // Create buffer if needed
    if (g_bitmapWidth != dstWidth || g_bitmapHeight != dstHeight) {
        CreateRenderBuffer(dstWidth, dstHeight);
    }
    
    if (g_bitmapBits == NULL) {
        return;
    }
    
    unsigned char *src = imageBuffer->raw;
    
    // Nearest-neighbor scaling
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / effectiveScaleY;
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / currentScale;
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 3;
            
            // BGR format for Windows
            g_bitmapBits[dstIndex + 0] = src[srcIndex + 2]; // B
            g_bitmapBits[dstIndex + 1] = src[srcIndex + 1]; // G
            g_bitmapBits[dstIndex + 2] = src[srcIndex + 0]; // R
        }
    }
}

// --- Double Buffered Paint Function ---
static void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    // Get the size of the window client area
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int viewWidth = clientRect.right - clientRect.left;
    int viewHeight = clientRect.bottom - clientRect.top;

    if (viewWidth <= 0 || viewHeight <= 0) {
        EndPaint(hwnd, &ps);
        return;
    }

    // 1. Create an off-screen DC (Double Buffer) for composition
    HDC hdcDoubleBuffer = CreateCompatibleDC(hdc);
    HBITMAP hbmDoubleBuffer = CreateCompatibleBitmap(hdc, viewWidth, viewHeight);
    HBITMAP hbmOldDoubleBuffer = (HBITMAP)SelectObject(hdcDoubleBuffer, hbmDoubleBuffer);

    // 2. Fill the Double Buffer with Black (Background)
    FillRect(hdcDoubleBuffer, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // 3. Draw the Emulator Image onto the Double Buffer (if available)
    if (g_hdcMem && g_hBitmap && g_bitmapWidth > 0 && g_bitmapHeight > 0) {
        // Calculate centering offsets
        int xOffset = (viewWidth - g_bitmapWidth) / 2;
        int yOffset = (viewHeight - g_bitmapHeight) / 2;
        
        // Set stretch mode
        SetStretchBltMode(hdcDoubleBuffer, COLORONCOLOR);
        
        // Blit from Emulator Buffer (g_hdcMem) -> Double Buffer (hdcDoubleBuffer)
        BitBlt(hdcDoubleBuffer, xOffset, yOffset, g_bitmapWidth, g_bitmapHeight, 
               g_hdcMem, 0, 0, SRCCOPY);
    }
    
    // 4. Flip: Copy the entire Double Buffer to the Screen (hdc) in one operation
    BitBlt(hdc, 0, 0, viewWidth, viewHeight, hdcDoubleBuffer, 0, 0, SRCCOPY);

    // 5. Cleanup
    SelectObject(hdcDoubleBuffer, hbmOldDoubleBuffer);
    DeleteObject(hbmDoubleBuffer);
    DeleteDC(hdcDoubleBuffer);
    
    EndPaint(hwnd, &ps);
}

// Update menu checkmarks
static void UpdateMenuCheckmarks(int selectedScale) {
    if (g_hScaleMenu) {
        CheckMenuItem(g_hScaleMenu, 1001, MF_UNCHECKED);
        CheckMenuItem(g_hScaleMenu, 1002, MF_UNCHECKED);
        CheckMenuItem(g_hScaleMenu, 1003, MF_UNCHECKED);
        CheckMenuItem(g_hScaleMenu, 1004, MF_UNCHECKED);
        CheckMenuItem(g_hScaleMenu, 1000 + selectedScale, MF_CHECKED);
    }
}

// Set scale and optionally resize window
static void SetScale(int scale, BOOL resizeWindow) {
    currentScale = scale;
    UpdateMenuCheckmarks(scale);
    
    if (resizeWindow && !isFullscreen && g_hwnd) {
        int baseWidth = imageBuffer->width;
        int baseHeight = imageBuffer->height;
        
        if (baseWidth > 0 && baseHeight > 0) {
            int newWidth = baseWidth * scale;
            int newHeight = baseHeight * scale * (int)imageBuffer->aspect_ratio;
            
            RECT rect = {0, 0, newWidth, newHeight};
            // Use the non-resizable style for calculation
            AdjustWindowRect(&rect, WINDOW_STYLE_WINDOWED, TRUE);
            
            SetWindowPos(g_hwnd, NULL, 0, 0, 
                        rect.right - rect.left, 
                        rect.bottom - rect.top,
                        SWP_NOMOVE | SWP_NOZORDER);
        }
    }
    
    if (g_hwnd) {
        InvalidateRect(g_hwnd, NULL, FALSE);
    }
    printf("Scale set to %dx\n", scale);
}

// Toggle fullscreen
static void ToggleFullscreen(void) {
    if (!g_hwnd) return;
    
    if (isFullscreen) {
        // Exit fullscreen
        SetWindowLong(g_hwnd, GWL_STYLE, WINDOW_STYLE_WINDOWED | WS_VISIBLE);
        SetWindowPlacement(g_hwnd, &savedWindowPlacement);
        SetScale(savedScale, FALSE);
        isFullscreen = FALSE;
    } else {
        // Enter fullscreen
        savedScale = currentScale;
        GetWindowPlacement(g_hwnd, &savedWindowPlacement);
        
        HMONITOR hMonitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {sizeof(MONITORINFO)};
        GetMonitorInfo(hMonitor, &mi);
        
        int screenWidth = mi.rcMonitor.right - mi.rcMonitor.left;
        int screenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
        
        int baseW = imageBuffer->width;
        int baseH = imageBuffer->height * (int)imageBuffer->aspect_ratio;
        
        if (baseW > 0 && baseH > 0) {
            int maxScaleX = screenWidth / baseW;
            int maxScaleY = screenHeight / baseH;
            int bestScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
            if (bestScale < 1) bestScale = 1;
            
            SetWindowLong(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(g_hwnd, HWND_TOP, 
                        mi.rcMonitor.left, mi.rcMonitor.top,
                        screenWidth, screenHeight,
                        SWP_FRAMECHANGED);
            
            SetScale(bestScale, FALSE);
            isFullscreen = TRUE;
        }
    }
}

// Render and update function (called by timer)
static void RenderAndUpdate(void) {
    int oldWidth = imageBuffer->width;
    int oldHeight = imageBuffer->height;
    
    // Update time in milliseconds
    DWORD currentTime = GetTickCount();
    pccore->time = (long long)currentTime;
    
    // Update blink
    blinkFrameCounter++;
    if (blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
        pccore->blink = 1 - pccore->blink;
        blinkFrameCounter = 0;
    }
    
    // Render
    render(imageBuffer, pccore);

    // Check if resolution changed
    if (imageBuffer->width != oldWidth || imageBuffer->height != oldHeight) {
        if (isFullscreen) {
            ToggleFullscreen();
            ToggleFullscreen();
        } else {
            SetScale(currentScale, TRUE);
        }
    }
    
    if (imageBuffer->width > 0 && imageBuffer->height > 0) {
        ScaleImageBuffer();
        if (g_hwnd) {
            InvalidateRect(g_hwnd, NULL, FALSE);
        }
    }
}

// DOS thread function
DWORD WINAPI DOSThreadFunction(LPVOID lpParam) {
    DOSThreadData *data = (DOSThreadData *)lpParam;
    printf("DOS thread started with %d arguments\n", data->argc);
    data->result = dos_main(data->argc, data->argv);
    printf("DOS thread finished with result: %d\n", data->result);
    data->finished = TRUE;
    return 0;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_ERASEBKGND:
            return 1;
            
        case WM_PAINT:
            OnPaint(hwnd);
            return 0;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            // --- Allow Alt+F4 to Close ---
            if (uMsg == WM_SYSKEYDOWN && wParam == VK_F4) {
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }

            if (!(lParam & 0x40000000)) { // Not a repeat
                BOOL isCtrl = (GetKeyState(VK_CONTROL) & 0x8000);

                // --- Handle Shortcuts (Ctrl + Key) ---
                if (isCtrl) {
                    switch (wParam) {
                        case 'F': ToggleFullscreen(); return 0;
                        case '1': SetScale(1, !isFullscreen); return 0;
                        case '2': SetScale(2, !isFullscreen); return 0;
                        case '3': SetScale(3, !isFullscreen); return 0;
                        case '4': SetScale(4, !isFullscreen); return 0;
                    }
                }
                
                // If not a shortcut, pass to emulator
                pccore->key = get_scancode(wParam, lParam);
                pccore->memory[BDA_KBD_STATUS_1] = get_statuscode();
                printf("Key pressed: 0x%x 0x%x\n", pccore->key, pccore->memory[BDA_KBD_STATUS_1]);
            }
            return 0;
            
        case WM_KEYUP:
        case WM_SYSKEYUP:
            pccore->memory[BDA_KBD_STATUS_1] = get_statuscode();
            if (pccore->key == get_scancode(wParam, lParam)) {
                pccore->key = 0;
                printf("Key released\n");
            }
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1001: SetScale(1, !isFullscreen); return 0;
                case 1002: SetScale(2, !isFullscreen); return 0;
                case 1003: SetScale(3, !isFullscreen); return 0;
                case 1004: SetScale(4, !isFullscreen); return 0;
                case 1005: ToggleFullscreen(); return 0;
            }
            break;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Timer callback
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    (void)hwnd;
    (void)uMsg;
    (void)idEvent;
    (void)dwTime;
    RenderAndUpdate();
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    // Allocate console for debugging
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    
    printf("PC Core Emulator Starting...\n");
    
    // Initialize pccore
    pccore = malloc(sizeof(PCCORE));
    pccore->mode = CGA40x25;
    pccore->key = 0;
    pccore->port[CGA_COLOR_REGISTER_PORT] = 0x20 | 0x10 | 0x01;
    
    imageBuffer = malloc(sizeof(IMAGE));

    imageBuffer->width = 320;
    imageBuffer->height = 200;
    imageBuffer->aspect_ratio = 1.0f;

    printf("Rendering initial frame...\n");
    render(imageBuffer, pccore);
    printf("Image buffer: %dx%d\n", imageBuffer->width, imageBuffer->height);
    
    if (imageBuffer->width == 0 || imageBuffer->height == 0) {
        MessageBox(NULL, "Failed to initialize image buffer", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "PCCoreEmulator";
    wc.lpszMenuName = NULL;
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create menu
    HMENU hMenu = CreateMenu();
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING, 1005, "Toggle Full Screen\tCtrl+F");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    // Updated Menu Text to reflect Ctrl+Number
    AppendMenu(hViewMenu, MF_STRING, 1001, "1x Scale\tCtrl+1");
    AppendMenu(hViewMenu, MF_STRING | MF_CHECKED, 1002, "2x Scale\tCtrl+2");
    AppendMenu(hViewMenu, MF_STRING, 1003, "3x Scale\tCtrl+3");
    AppendMenu(hViewMenu, MF_STRING, 1004, "4x Scale\tCtrl+4");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, "View");
    g_hScaleMenu = hViewMenu;
    
    // Calculate window size
    int baseWidth = imageBuffer->width;
    int baseHeight = imageBuffer->height;
    int windowWidth = baseWidth * currentScale;
    int windowHeight = baseHeight * currentScale * (int)imageBuffer->aspect_ratio;
    
    printf("Window size: %dx%d\n", windowWidth, windowHeight);
    
    RECT rect = {0, 0, windowWidth, windowHeight};
    // Use fixed style
    AdjustWindowRect(&rect, WINDOW_STYLE_WINDOWED, TRUE);
    
    // Create window
    g_hwnd = CreateWindowEx(
        0,
        "PCCoreEmulator",
        "PC Core Emulator",
        WINDOW_STYLE_WINDOWED, // Fixed style (No resize)
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        hMenu,
        hInstance,
        NULL
    );
    
    if (!g_hwnd) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    printf("Window created successfully\n");
    
    // Show window
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    
    // Do initial render
    ScaleImageBuffer();
    InvalidateRect(g_hwnd, NULL, FALSE);
    
    // Start DOS thread
    dosData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    dosData->finished = FALSE;
    dosData->result = 0;
    dosData->argc = __argc;
    dosData->argv = __argv;
    
    dosThread = CreateThread(NULL, 0, DOSThreadFunction, dosData, 0, NULL);
    if (!dosThread) {
        printf("Warning: Failed to create DOS thread\n");
    }
    
    // Set up timer for 60 FPS
    SetTimer(g_hwnd, 1, 1000 / 60, TimerProc);
    
    printf("Entering message loop...\n");
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    printf("Exiting...\n");
    
    // Cleanup
    KillTimer(g_hwnd, 1);
    if (dosThread) {
        TerminateThread(dosThread, 0);
        CloseHandle(dosThread);
    }
    if (dosData) {
        free(dosData);
    }
    if (g_hdcMem) {
        DeleteDC(g_hdcMem);
    }
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
    }
    
    return (int)msg.wParam;
}