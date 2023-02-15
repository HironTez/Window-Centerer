#include <iostream>
#include <Windows.h>
#include <vector>
#include <math.h>
#include <thread>

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

// Center provided window
const void centerWindow(HWND window)
{
    // If no window is provided
    if (!window)
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

        // If it's the closest monitor and the window is not centered, save the point
        if ((distance < closestDistance || closestDistance == -1) && distance)
        {
            closestDistance = distance;
            closestMonitorCenter = monitorCenter;
        }
    }

    // Set window position
    if (closestDistance > 1)
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

    // If shift is pressed
    if (nCode == HC_ACTION && wParam == WM_KEYUP && pKeyBoard->vkCode == 160)
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

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Register the triple shift shortcut detection
const void registerTripleShiftPressEventHandler()
{
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook)
        return;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
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
        return;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWinEvent(hHook);
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

    // Wait for threads to complete
    th1.join();
    th2.join();

    return 0;
}

// TODO: Notifications
// TODO: App installer
// TODO: Do not start the application twice
// ? TODO: Add support for system apps such as task manager
