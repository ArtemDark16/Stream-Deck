#include "UiControls.h"

#include "App.h"

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h)
{
    return CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, nullptr, g_hInst, nullptr);
}

HWND CreateLabelWithId(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h)
{
    return CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInst, nullptr);
}

HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h)
{
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInst, nullptr);
}

HWND CreateCombo(HWND parent, int id, int x, int y, int w, int h)
{
    return CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInst, nullptr);
}
