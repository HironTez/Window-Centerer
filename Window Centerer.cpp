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
const void centerWindow(HWND hwnd)
{
    // If no window is provided
    if (!hwnd)
        return;

    // Get the rect of the window
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    const long windowWidth = windowRect.right - windowRect.left;
    const long windowHeight = windowRect.bottom - windowRect.top;
    const POINT windowCenter = {windowRect.left + windowWidth / 2, windowRect.top + windowHeight / 2};

    // Get the number of monitors
    const int numMonitors = GetSystemMetrics(SM_CMONITORS);

    // Find the closest monitor
    double closestDistance = 0;
    POINT closestMonitorCenter = {0, 0};

    // Get rects of monitors
    GetMonitorRects();
    for (int i = 0; i < numMonitors; i++)
    {
        // Get the rect of the monitor
        const RECT &rect = monitorRects[i];
        const POINT monitorCenter = {rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2};

        // Calculate distance between the center of the monitor and the center of the window
        const int distance = getDistance(windowCenter, monitorCenter);

        // If it's the closest monitor and the window is not centered, save the coordinates
        if ((distance < closestDistance || closestDistance == 0) && distance)
        {
            closestDistance = distance;
            closestMonitorCenter = monitorCenter;
        }
    }

    // Set window position
    if (closestDistance)
    {
        SetWindowPos(hwnd, HWND_TOP, closestMonitorCenter.x - windowWidth / 2, closestMonitorCenter.y - windowHeight / 2, 0, 0, SWP_NOSIZE);
    }
}

// Center the focused window
const void centerForegroundWindow()
{
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return;
    centerWindow(hwnd);
}

// Detect triple shift press and execute callback
const void registerTripleShiftPressEventHandler()
{
    // Init variables
    int shiftPressCount = 0;
    int prevShiftState = 0;
    int prevTime = 0;

    while (true)
    {
        // Get shift press state
        int currentShiftState = GetAsyncKeyState(VK_SHIFT);
        int currentTime = clock();
        if (currentShiftState & 0x8000 && (currentShiftState != prevShiftState))
        {
            if (prevShiftState == 0)
            {
                shiftPressCount++;

                if (shiftPressCount >= 3)
                {
                    centerForegroundWindow(); // Call the target function
                    shiftPressCount = 0;
                }
                prevTime = currentTime;
            }
        }
        // If the delay is more than 200 milliseconds, reset counter
        else if ((currentTime - prevTime) / CLOCKS_PER_SEC >= 0.2)
        {
            shiftPressCount = 0;
        }

        // Update previous shift state
        prevShiftState = currentShiftState;
        // Wait 50 ms to avoid hight CPU usage
        Sleep(50);
    }
}

HWINEVENTHOOK g_hHook = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD dwThreadId, dwProcessId;
    dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwThreadId == (DWORD)lParam)
    {
        centerWindow(hwnd);
    }
    return TRUE;
}

VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (event == EVENT_OBJECT_CREATE && IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
    {
        DWORD dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
        EnumThreadWindows(dwThreadId, EnumWindowsProc, dwThreadId);
    }
}

const void registerWindowOpenEventHandler()
{
    g_hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWinEvent(g_hHook);
}

int main()
{
    std::thread th1(registerWindowOpenEventHandler);
    std::thread th2(registerTripleShiftPressEventHandler);

    th1.join();
    th2.join();

    return 0;
}

// TODO: App installer
// TODO: Do not start the application twice
// TODO: Add support for system apps such as task manager
// TODO: Notifications
