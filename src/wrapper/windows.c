#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../pccore/pccore.h"
#include "../pccore/cga.h"
#include "windows_keyboard.h"
#include "../dosapp.h"

// --- Constants ---
#define WINDOW_CLASS_NAME L"PCCoreEmulator"
#define WINDOW_TITLE L"PC Core Emulator"
#define TIMER_ID 1
#define TIMER_INTERVAL 16  // ~60 FPS (16ms)

// --- Global Variables ---
HWND g_hWnd = NULL;
IMAGE g_imageBuffer = {0};
HBITMAP g_hBitmap = NULL;
HDC g_hMemDC = NULL;

// DOS Thread Data
typedef struct {
    int argc;
    char **argv;
    int result;
    BOOL finished;
} DOSThreadData;

HANDLE g_hDOSThread = NULL;
DOSThreadData *g_pDOSData = NULL;

// --- Forward Declarations ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DOSThreadFunction(LPVOID lpParam);
void InitializePCCore(void);
void CreateAppWindow(HINSTANCE hInstance);
void RenderAndUpdate(void);
void CleanupResources(void);

/**
 * @brief DOS Thread Function
 */
DWORD WINAPI DOSThreadFunction(LPVOID lpParam) {
    DOSThreadData *data = (DOSThreadData *)lpParam;
    
    printf("DOS thread started with %d arguments\n", data->argc);
    
    // Call the DOS main function
    data->result = dos_main(data->argc, data->argv);
    
    printf("DOS thread finished with result: %d\n", data->result);
    
    // Mark as finished
    data->finished = TRUE;
    
    // Post message to main thread
    if (g_hWnd) {
        PostMessage(g_hWnd, WM_USER + 1, 0, 0);  // Custom message for DOS completion
    }
    
    return 0;
}

/**
 * @brief Starts the DOS thread
 */
void StartDOSThread(void) {
    // Allocate thread data
    g_pDOSData = (DOSThreadData *)malloc(sizeof(DOSThreadData));
    if (!g_pDOSData) {
        MessageBoxA(NULL, "Failed to allocate DOS thread data", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    g_pDOSData->finished = FALSE;
    g_pDOSData->result = 0;
    
    // Get command line arguments
    LPWSTR *szArglist;
    int nArgs;
    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    
    if (szArglist == NULL) {
        MessageBoxA(NULL, "Failed to parse command line", "Error", MB_OK | MB_ICONERROR);
        free(g_pDOSData);
        g_pDOSData = NULL;
        return;
    }
    
    g_pDOSData->argc = nArgs;
    g_pDOSData->argv = (char **)malloc(sizeof(char *) * (nArgs + 1));
    
    // Convert wide char arguments to char
    for (int i = 0; i < nArgs; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, NULL, 0, NULL, NULL);
        g_pDOSData->argv[i] = (char *)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, g_pDOSData->argv[i], len, NULL, NULL);
    }
    g_pDOSData->argv[nArgs] = NULL;
    
    LocalFree(szArglist);
    
    // Create the thread
    g_hDOSThread = CreateThread(NULL, 0, DOSThreadFunction, g_pDOSData, 0, NULL);
    if (g_hDOSThread == NULL) {
        MessageBoxA(NULL, "Failed to create DOS thread", "Error", MB_OK | MB_ICONERROR);
        for (int i = 0; i < nArgs; i++) {
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
}

/**
 * @brief Create the application window
 */
void CreateAppWindow(HINSTANCE hInstance) {
    // Register window class
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxA(NULL, "Window registration failed", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Calculate window size
    const int baseWidth = g_imageBuffer.width;
    const int baseHeight = g_imageBuffer.height;
    const int defaultScale = 2;
    
    int windowWidth = baseWidth * defaultScale;
    int windowHeight = baseHeight * defaultScale;
    
    // Adjust for window decorations
    RECT rect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    int adjustedWidth = rect.right - rect.left;
    int adjustedHeight = rect.bottom - rect.top;
    
    // Create the window
    g_hWnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        adjustedWidth, adjustedHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (g_hWnd == NULL) {
        MessageBoxA(NULL, "Window creation failed", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Set minimum window size
    // This will be enforced in WM_GETMINMAXINFO
    
    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);
    
    // Start timer for rendering
    SetTimer(g_hWnd, TIMER_ID, TIMER_INTERVAL, NULL);
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
    
    // Get window DC
    HDC hdc = GetDC(g_hWnd);
    if (!hdc) return;
    
    // Create memory DC if needed
    if (!g_hMemDC) {
        g_hMemDC = CreateCompatibleDC(hdc);
    }
    
    // Create or recreate bitmap if size changed
    if (!g_hBitmap) {
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = g_imageBuffer.width;
        bmi.bmiHeader.biHeight = -g_imageBuffer.height;  // Top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        void *pBits;
        g_hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        
        if (g_hBitmap) {
            SelectObject(g_hMemDC, g_hBitmap);
        }
    }
    
    if (g_hBitmap) {
        // Copy image data to bitmap
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = g_imageBuffer.width;
        bmi.bmiHeader.biHeight = -g_imageBuffer.height;  // Top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        SetDIBits(g_hMemDC, g_hBitmap, 0, g_imageBuffer.height, 
                  g_imageBuffer.raw, &bmi, DIB_RGB_COLORS);
        
        // Get client area size
        RECT clientRect;
        GetClientRect(g_hWnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;
        
        // Stretch blit to fill window
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, clientWidth, clientHeight,
                   g_hMemDC, 0, 0, g_imageBuffer.width, g_imageBuffer.height,
                   SRCCOPY);
    }
    
    ReleaseDC(g_hWnd, hdc);
}

/**
 * @brief Window procedure
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            if (wParam == TIMER_ID) {
                RenderAndUpdate();
            }
            return 0;
            
        case WM_KEYDOWN:
            if (!(lParam & 0x40000000)) {  // Check if not a repeat
                pccore.key = get_scancode(wParam, lParam);
                printf("Key pressed: 0x%x\n", pccore.key);
            }
            return 0;
            
        case WM_KEYUP:
            if (pccore.key == get_scancode(wParam, lParam)) {
                pccore.key = 0;
                printf("Key released\n");
            }
            return 0;
            
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            RECT rect = {0, 0, g_imageBuffer.width, g_imageBuffer.height};
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
            mmi->ptMinTrackSize.x = rect.right - rect.left;
            mmi->ptMinTrackSize.y = rect.bottom - rect.top;
            return 0;
        }
        
        case WM_USER + 1:  // DOS thread completion
            if (g_pDOSData) {
                printf("DOS thread completion message received\n");
                printf("DOS main returned: %d\n", g_pDOSData->result);
                
                // Wait for thread to complete
                if (g_hDOSThread) {
                    WaitForSingleObject(g_hDOSThread, INFINITE);
                    CloseHandle(g_hDOSThread);
                    g_hDOSThread = NULL;
                }
                
                // Free allocated memory
                for (int i = 0; i < g_pDOSData->argc; i++) {
                    free(g_pDOSData->argv[i]);
                }
                free(g_pDOSData->argv);
                
                int finalResult = g_pDOSData->result;
                free(g_pDOSData);
                g_pDOSData = NULL;
                
                printf("DOS execution completed with code: %d\n", finalResult);
            }
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            RenderAndUpdate();
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            CleanupResources();
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief Cleanup resources
 */
void CleanupResources(void) {
    // Stop timer
    if (g_hWnd) {
        KillTimer(g_hWnd, TIMER_ID);
    }
    
    // Wait for DOS thread to finish
    if (g_hDOSThread) {
        printf("Waiting for DOS thread to finish...\n");
        WaitForSingleObject(g_hDOSThread, INFINITE);
        CloseHandle(g_hDOSThread);
        g_hDOSThread = NULL;
    }
    
    // Free DOS thread data
    if (g_pDOSData) {
        for (int i = 0; i < g_pDOSData->argc; i++) {
            free(g_pDOSData->argv[i]);
        }
        free(g_pDOSData->argv);
        free(g_pDOSData);
        g_pDOSData = NULL;
    }
    
    // Free GDI resources
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
    
    if (g_hMemDC) {
        DeleteDC(g_hMemDC);
        g_hMemDC = NULL;
    }
}

/**
 * @brief Application entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // Allocate console for printf output
    AllocConsole();
    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    // Initialize PCCORE
    InitializePCCore();
    
    // Create window
    CreateAppWindow(hInstance);
    
    // Start DOS thread
    StartDOSThread();
    
    // Message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}