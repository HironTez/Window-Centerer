#include <iostream>
#include <Windows.h>
#include <vector>
#include <math.h>
#include <thread>
#include <atomic>

// #include "./Tray Icon.h"

std::atomic<bool> isExit(false);

std::vector<RECT> monitorRects;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    monitorRects.push_back(*lprcMonitor);
    return TRUE;
}

// Get rects of monitors
const void GetMonitorRects()
{
    monitorRects.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

// Calculate the distance between two points
const double getDistance(POINT point1, POINT point2)
{
    return hypot(point2.x - point1.x, point2.y - point1.y);
}

bool IsWindowFullscreen(HWND hwnd)
{
    WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT)};
    return (GetWindowPlacement(hwnd, &placement) && placement.showCmd == SW_SHOWMAXIMIZED);
}

// Center provided window
const void centerWindow(HWND window)
{
    // If no window is provided
    if (!window)
        return;

    // If window is in a fullscreen mode
    if (IsWindowFullscreen(window))
        return;

    // Get the rect of the window
    RECT windowRect;
    GetWindowRect(window, &windowRect);
    const long windowWidth = windowRect.right - windowRect.left;
    const long windowHeight = windowRect.bottom - windowRect.top;
    const POINT windowCenter = {windowRect.left + windowWidth / 2, windowRect.top + windowHeight / 2};

    // Get the number of monitors
    const int numMonitors = GetSystemMetrics(SM_CMONITORS);

    // Closest monitor
    double closestDistance = -1;
    POINT closestMonitorCenter = {0, 0};

    // Find the closest monitor
    GetMonitorRects();
    for (int i = 0; i < numMonitors; i++)
    {
        // Get the rect of the monitor
        const RECT &rect = monitorRects[i];
        const POINT monitorCenter = {rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2};

        // Calculate distance between the center of the monitor and the center of the window
        const int distance = getDistance(windowCenter, monitorCenter);

        // If it's the closest monitor, save the point
        if (distance < closestDistance || closestDistance == -1)
        {
            closestDistance = distance;
            closestMonitorCenter = monitorCenter;
        }
    }

    // Set window position
    if (closestDistance > 0)
        SetWindowPos(window, HWND_TOP, closestMonitorCenter.x - windowWidth / 2, closestMonitorCenter.y - windowHeight / 2, 0, 0, SWP_NOSIZE);
}

// Center the focused window
const void centerForegroundWindow()
{
    centerWindow(GetForegroundWindow());
}

// Init counters for shift press detection
int shiftPressCount = 0;
int prevTime = 0;

// Detect triple shift press
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

    // If a key is pressed
    if (nCode == HC_ACTION && wParam == WM_KEYUP)
    {
        // If shift is pressed
        if (pKeyBoard->vkCode == 160)
        {
            // Reset the counter if the delay was too long
            int currentTime = clock();
            if ((currentTime - prevTime) / CLOCKS_PER_SEC >= 0.2)
            {
                shiftPressCount = 0;
            }

            // Increment the counter
            shiftPressCount += 1;
            prevTime = currentTime;

            // If it was a triple press
            if (shiftPressCount >= 3)
            {
                centerForegroundWindow(); // Call the target function

                // Reset the counter
                shiftPressCount = 0;
                prevTime = 0;
            }
        }
        // Reset variables if another key is pressed
        else
        {
            shiftPressCount = 0;
            prevTime = 0;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Register the triple shift shortcut detection
const void registerTripleShiftPressEventHandler()
{
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook)
        isExit = true;
        return;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    isExit = true;
}

// Check if the window is a top-level window
bool IsTopLevelWindow(HWND window)
{
    bool hasChildStyle = GetWindowLong(window, GWL_STYLE) & WS_CHILD;
    HWND parent = GetParent(window);
    bool hasParent = parent && (parent != GetDesktopWindow()) && GetWindow(window, GW_OWNER) != NULL;
    return !hasChildStyle && !hasParent;
}

// Check if the window is an application
bool IsAppWindow(HWND window)
{
    char title[2];
    GetWindowTextA(window, title, 2);
    return IsWindowEnabled(window) && strlen(title);
}

// Detect new window opening
VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (IsTopLevelWindow(window) && IsAppWindow(window))
    {
        centerWindow(window);
    }
}

// Register the window opening event handler
const void registerWindowOpenEventHandler()
{
    HWINEVENTHOOK hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (!hHook)
        isExit = true;
        return;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWinEvent(hHook);

    isExit = true;
}

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)

const void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, TEXT("Settings"));
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 2, TEXT("Exit"));

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

const void OnTrayIcon(HWND hwnd, UINT uMsg, UINT uID, DWORD dwMessage)
{
    if (uID != 1)
    {
        return;
    }

    if (dwMessage == WM_RBUTTONUP)
    {
        POINT pt;
        GetCursorPos(&pt);
        ShowContextMenu(hwnd, pt);
    }
}

const void AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, TEXT("My App"));

    Shell_NotifyIcon(NIM_ADD, &nid);
}

const void RemoveTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;

    Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        AddTrayIcon(hwnd);
        break;

    case WM_TRAYICON:
        OnTrayIcon(hwnd, uMsg, wParam, lParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Settings
            // TODO: Show settings dialog
            break;

        case 2: // Exit
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_DESTROY:
        RemoveTrayIcon(hwnd);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

const void registerTrayIcon(HINSTANCE hInstance)
{
    // Register the window class
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = (LPCWSTR) "Window Centerer";
    RegisterClassEx(&wcex);

    // Create a hidden window
    HWND hWnd = CreateWindowEx(0, (LPCWSTR) "Window Centerer", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!hWnd)
    {
        isExit = true;
        return;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    isExit = true;
}

// Run the app as a window to avoid opening the console
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    // Start threads
    std::thread th1(registerWindowOpenEventHandler);
    std::thread th2(registerTripleShiftPressEventHandler);
    std::thread th3(registerTrayIcon, hInstance);

    // Wait for threads to complete or exit when at least one thread is finished
    while (!isExit)
    {
        if (th1.joinable() && th1.joinable() && th1.joinable())
        {
            th1.join();
            th2.join();
            th3.join();
            break;
        }
    }

    return 0;
}

// TODO: Settings
// TODO: Notifications
// TODO: Do not start the application twice
// ? TODO: Add support for system apps such as task manager
