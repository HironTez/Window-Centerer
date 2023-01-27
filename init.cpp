#include <iostream>
#include <Windows.h>
#include <vector>
#include <map>
#include <math.h>

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

// To delete
template <typename T>
const void print(const T &data)
{
    std::cout << data << std::endl;
}

template <typename T, typename... Args>
const void print(const T &first, const Args &...args)
{
    std::cout << first << " ";
    print(args...);
}
// To delete

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

// Center the currently opened window
const void centerForegroundWindow() {
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;
    centerWindow(hwnd);
}

// Detect triple shift press and execute callback
const void onTripleShiftPress(const void (*callback)())
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
                    callback(); // Call callback
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

int main()
{
    onTripleShiftPress(centerForegroundWindow);
    return 0;
}
