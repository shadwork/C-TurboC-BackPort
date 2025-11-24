/**
 * @file windows.c
 * @brief Windows implementation of PC Core Emulator using Win32 API
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "windows_keyboard.h"

// Forward declarations
extern int dos_main(int argc, char *argv[]);

// Global state
static HWND g_hwnd = NULL;
static IMAGE g_imageBuffer = {0};
static HBITMAP g_hBitmap = NULL;
static void* g_bitmapBits = NULL;
static int g_currentScale = 2;
static BOOL g_aspectEnabled = FALSE;
static BOOL g_dosThreadRunning = FALSE;
static HANDLE g_dosThread = NULL;

// Blink timing (60 FPS update rate)
static const int FRAMES_PER_BLINK_HALF_CYCLE = 8;
static int g_blinkFrameCounter = 0;

// DOS thread data
typedef struct {
    int argc;
    char** argv;
    int result;
    BOOL finished;
} DOSThreadData;

static DOSThreadData* g_dosData = NULL;

// Menu IDs
#define ID_VIEW_SCALE_1X    101
#define ID_VIEW_SCALE_2X    102
#define ID_VIEW_SCALE_3X    103
#define ID_VIEW_SCALE_4X    104
#define ID_VIEW_ASPECT      105
#define ID_FILE_EXIT        106

// Timer ID
#define TIMER_RENDER        1

/**
 * @brief DOS thread function
 */
DWORD WINAPI DOSThreadProc(LPVOID lpParam) {
    DOSThreadData* data = (DOSThreadData*)lpParam;
    
    printf("DOS thread started with %d arguments\n", data->argc);
    
    data->result = dos_main(data->argc, data->argv);
    
    printf("DOS thread finished with result: %d\n", data->result);
    
    data->finished = TRUE;
    g_dosThreadRunning = FALSE;
    
    return 0;
}

/**
 * @brief Start the DOS thread
 */
void StartDOSThread(int argc, char** argv) {
    g_dosData = (DOSThreadData*)malloc(sizeof(DOSThreadData));
    g_dosData->finished = FALSE;
    g_dosData->result = 0;
    g_dosData->argc = argc;
    
    // Allocate and copy arguments
    g_dosData->argv = (char**)malloc(sizeof(char*) * (argc + 1));
    for (int i = 0; i < argc; i++) {
        g_dosData->argv[i] = _strdup(argv[i]);
    }
    g_dosData->argv[argc] = NULL;
    
    g_dosThreadRunning = TRUE;
    g_dosThread = CreateThread(NULL, 0, DOSThreadProc, g_dosData, 0, NULL);
    
    if (g_dosThread == NULL) {
        printf("Error creating DOS thread\n");
        free(g_dosData);
        g_dosData = NULL;
        g_dosThreadRunning = FALSE;
    } else {
        printf("DOS thread created successfully\n");
    }
}

/**
 * @brief Create scaled bitmap for rendering
 */
void CreateScaledBitmap(HDC hdc, int width, int height) {
    // Delete old bitmap if it exists
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
        g_bitmapBits = NULL;
    }
    
    // Create DIB section
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    g_hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &g_bitmapBits, NULL, 0);
}

/**
 * @brief Scale pixel buffer
 */
void ScalePixelBuffer() {
    if (g_imageBuffer.raw == NULL || g_bitmapBits == NULL) {
        return;
    }
    
    int srcWidth = g_imageBuffer.width;
    int srcHeight = g_imageBuffer.height;
    
    // Determine effective scales based on aspect setting
    int effectiveScaleX = g_currentScale * (g_aspectEnabled ? (int)g_imageBuffer.aspect_ratio : 1);
    int effectiveScaleY = g_currentScale * (g_aspectEnabled ? 1 : (int)g_imageBuffer.aspect_ratio);
    
    int dstWidth = srcWidth * effectiveScaleX;
    int dstHeight = srcHeight * effectiveScaleY;
    
    unsigned char* src = g_imageBuffer.raw;
    unsigned char* dst = (unsigned char*)g_bitmapBits;
    
    // Nearest-neighbor scaling
    for (int dstY = 0; dstY < dstHeight; dstY++) {
        int srcY = dstY / effectiveScaleY;
        
        for (int dstX = 0; dstX < dstWidth; dstX++) {
            int srcX = dstX / effectiveScaleX;
            
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (dstY * dstWidth + dstX) * 3;
            
            // RGB to BGR conversion for Windows
            dst[dstIndex + 0] = src[srcIndex + 2]; // B
            dst[dstIndex + 1] = src[srcIndex + 1]; // G
            dst[dstIndex + 2] = src[srcIndex + 0]; // R
        }
    }
}

/**
 * @brief Set window scale
 */
void SetWindowScale(int scale) {
    g_currentScale = scale;
    
    int baseWidth = g_imageBuffer.width;
    int baseHeight = g_imageBuffer.height;
    
    // Calculate new client size
    int clientWidth, clientHeight;
    if (g_aspectEnabled) {
        clientWidth = baseWidth * scale * (int)g_imageBuffer.aspect_ratio;
        clientHeight = baseHeight * scale;
    } else {
        clientWidth = baseWidth * scale;
        clientHeight = baseHeight * scale * (int)g_imageBuffer.aspect_ratio;
    }
    
    // Calculate window size including frame
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    // Center window on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    SetWindowPos(g_hwnd, NULL, x, y, windowWidth, windowHeight, SWP_NOZORDER);
    
    printf("Scale set to %dx (%dx%d)\n", scale, clientWidth, clientHeight);
}

/**
 * @brief Update menu checkmarks
 */
void UpdateMenuCheckmarks(HMENU hMenu, int checkedId) {
    CheckMenuItem(hMenu, ID_VIEW_SCALE_1X, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_VIEW_SCALE_2X, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_VIEW_SCALE_3X, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_VIEW_SCALE_4X, MF_UNCHECKED);
    CheckMenuItem(hMenu, checkedId, MF_CHECKED);
}

/**
 * @brief Render and update
 */
void RenderAndUpdate() {
    static int oldWidth = 0;
    static int oldHeight = 0;
    
    // Store old dimensions
    oldWidth = g_imageBuffer.width;
    oldHeight = g_imageBuffer.height;
    
    // Update time
    struct _timeb tb;
    _ftime(&tb);
    pccore.time = (long long)tb.time * 1000LL + tb.millitm;
    
    // Update blink
    g_blinkFrameCounter++;
    if (g_blinkFrameCounter >= FRAMES_PER_BLINK_HALF_CYCLE) {
        pccore.blink = 1 - pccore.blink;
        g_blinkFrameCounter = 0;
    }
    
    // Render
    render(&g_imageBuffer, pccore);
    
    // Check for mode change
    if (g_imageBuffer.width != oldWidth || g_imageBuffer.height != oldHeight) {
        printf("Detected mode change: %dx%d -> %dx%d\n", 
               oldWidth, oldHeight, g_imageBuffer.width, g_imageBuffer.height);
        SetWindowScale(g_currentScale);
    }
    
    // Trigger repaint
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/**
 * @brief Window procedure
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static BYTE keyState[256];
    
    switch (msg) {
        case WM_CREATE: {
            // Create menu
            HMENU hMenuBar = CreateMenu();
            HMENU hFileMenu = CreateMenu();
            HMENU hViewMenu = CreateMenu();
            
            AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
            AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
            
            AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SCALE_1X, "&1x Scale");
            AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SCALE_2X, "&2x Scale");
            AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SCALE_3X, "&3x Scale");
            AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SCALE_4X, "&4x Scale");
            AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ASPECT, "&Aspect");
            AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, "&View");
            
            SetMenu(hwnd, hMenuBar);
            UpdateMenuCheckmarks(hViewMenu, ID_VIEW_SCALE_2X);
            
            // Start render timer (60 FPS)
            SetTimer(hwnd, TIMER_RENDER, 1000 / 60, NULL);
            break;
        }
        
        case WM_TIMER:
            if (wParam == TIMER_RENDER) {
                RenderAndUpdate();
            }
            break;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (g_imageBuffer.width > 0 && g_imageBuffer.height > 0) {
                // Calculate dimensions
                int srcWidth = g_imageBuffer.width;
                int srcHeight = g_imageBuffer.height;
                int effectiveScaleX = g_currentScale * (g_aspectEnabled ? (int)g_imageBuffer.aspect_ratio : 1);
                int effectiveScaleY = g_currentScale * (g_aspectEnabled ? 1 : (int)g_imageBuffer.aspect_ratio);
                int dstWidth = srcWidth * effectiveScaleX;
                int dstHeight = srcHeight * effectiveScaleY;
                
                // Create bitmap if needed
                if (g_hBitmap == NULL) {
                    CreateScaledBitmap(hdc, dstWidth, dstHeight);
                }
                
                // Scale pixel buffer
                ScalePixelBuffer();
                
                // Draw bitmap
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, g_hBitmap);
                
                BitBlt(hdc, 0, 0, dstWidth, dstHeight, hdcMem, 0, 0, SRCCOPY);
                
                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_KEYDOWN: {
            if (lParam & (1 << 30)) { // Check repeat bit
                break;
            }
            
            GetKeyboardState(keyState);
            int scancode = get_scancode(wParam, lParam, keyState);
            int statuscode = get_statuscode(keyState);
            
            pccore.key = scancode;
            pccore.memory[BDA_KBD_STATUS_1] = statuscode;
            
            printf("Key pressed: 0x%04x Status: 0x%02x\n", scancode, statuscode);
            break;
        }
        
        case WM_KEYUP: {
            GetKeyboardState(keyState);
            int scancode = get_scancode(wParam, lParam, keyState);
            int statuscode = get_statuscode(keyState);
            
            if (pccore.key == scancode) {
                pccore.key = 0;
                printf("Key released\n");
            }
            
            pccore.memory[BDA_KBD_STATUS_1] = statuscode;
            break;
        }
        
        case WM_COMMAND: {
            HMENU hMenu = GetMenu(hwnd);
            HMENU hViewMenu = GetSubMenu(hMenu, 1);
            
            switch (LOWORD(wParam)) {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    break;
                case ID_VIEW_SCALE_1X:
                    SetWindowScale(1);
                    UpdateMenuCheckmarks(hViewMenu, ID_VIEW_SCALE_1X);
                    break;
                case ID_VIEW_SCALE_2X:
                    SetWindowScale(2);
                    UpdateMenuCheckmarks(hViewMenu, ID_VIEW_SCALE_2X);
                    break;
                case ID_VIEW_SCALE_3X:
                    SetWindowScale(3);
                    UpdateMenuCheckmarks(hViewMenu, ID_VIEW_SCALE_3X);
                    break;
                case ID_VIEW_SCALE_4X:
                    SetWindowScale(4);
                    UpdateMenuCheckmarks(hViewMenu, ID_VIEW_SCALE_4X);
                    break;
                case ID_VIEW_ASPECT: {
                    g_aspectEnabled = !g_aspectEnabled;
                    CheckMenuItem(hViewMenu, ID_VIEW_ASPECT, 
                                g_aspectEnabled ? MF_CHECKED : MF_UNCHECKED);
                    SetWindowScale(g_currentScale);
                    printf("Aspect correction %s\n", g_aspectEnabled ? "enabled" : "disabled");
                    break;
                }
            }
            break;
        }
        
        case WM_CLOSE:
            KillTimer(hwnd, TIMER_RENDER);
            DestroyWindow(hwnd);
            break;
        
        case WM_DESTROY:
            if (g_hBitmap) {
                DeleteObject(g_hBitmap);
                g_hBitmap = NULL;
            }
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

/**
 * @brief Initialize pccore
 */
void InitializePCCore() {
    memset(&pccore, 0, sizeof(PCCORE));
    pccore.mode = CGA320x200x2;
    pccore.key = 0;
    pccore.port[CGA_COLOR_REGISTER_PORT] = 0x31;
    
    // Initial render to get dimensions
    render(&g_imageBuffer, pccore);
}

/**
 * @brief WinMain entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // Parse command line
    int argc = __argc;
    char** argv = __argv;
    
    // Initialize pccore
    InitializePCCore();
    
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PCCoreEmulator";
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Calculate initial window size
    int baseWidth = g_imageBuffer.width;
    int baseHeight = g_imageBuffer.height;
    int clientWidth = baseWidth * g_currentScale;
    int clientHeight = baseHeight * g_currentScale * (int)g_imageBuffer.aspect_ratio;
    
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    // Center window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // Create window
    g_hwnd = CreateWindowEx(
        0,
        "PCCoreEmulator",
        "PC Core Emulator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );
    
    if (g_hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    
    // Start DOS thread
    StartDOSThread(argc, argv);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
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
    
    return (int)msg.wParam;
}