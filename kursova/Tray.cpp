#include "framework.h"
#include "kursova.h"
#include "Resource.h"
#include "Tray.h"

#include <shellapi.h>

constexpr UINT TRAY_ID = 100;
constexpr UINT ID_TRAY_SHOW = 4100;
constexpr UINT ID_TRAY_SETTINGS = 4101;
constexpr UINT ID_TRAY_EXIT = 4102;

extern HINSTANCE g_hInst;
extern HWND g_hWnd;

void ShowPanel();
void OpenEditor();
void ExitApplication();

namespace
{
    NOTIFYICONDATAW g_trayIcon{};
}

bool AddTrayIcon()
{
    g_trayIcon = {};
    g_trayIcon.cbSize = sizeof(g_trayIcon);
    g_trayIcon.hWnd = g_hWnd;
    g_trayIcon.uID = TRAY_ID;
    g_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_trayIcon.uCallbackMessage = WM_TRAYICON;
    g_trayIcon.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SMALL));
    wcscpy_s(g_trayIcon.szTip, L"Virtual Stream Deck");
    return Shell_NotifyIconW(NIM_ADD, &g_trayIcon) == TRUE;
}

void RemoveTrayIcon()
{
    if (g_trayIcon.hWnd)
    {
        Shell_NotifyIconW(NIM_DELETE, &g_trayIcon);
        g_trayIcon = {};
    }
}

void ShowTrayMenu()
{
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_TRAY_SHOW, L"Показати панель");
    AppendMenuW(menu, MF_STRING, ID_TRAY_SETTINGS, L"Налаштування");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Вийти");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(g_hWnd);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        cursor.x, cursor.y, 0, g_hWnd, nullptr);
    DestroyMenu(menu);

    switch (command)
    {
    case ID_TRAY_SHOW:
        ShowPanel();
        break;
    case ID_TRAY_SETTINGS:
        OpenEditor();
        break;
    case ID_TRAY_EXIT:
        ExitApplication();
        break;
    }
}

LRESULT HandleTrayMessage(LPARAM lParam)
{
    switch (LOWORD(lParam))
    {
    case WM_LBUTTONDBLCLK:
        ShowPanel();
        return 0;
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        ShowTrayMenu();
        return 0;
    }

    return 0;
}
