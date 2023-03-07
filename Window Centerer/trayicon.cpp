#include "./global.h"

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
