// Virtual Stream Deck: бокова панель 3x3 з глобальною гарячою клавішею.

#include "framework.h"
#include "kursova.h"
#include "Actions.h"
#include "App.h"
#include "Config.h"
#include "FileDialogs.h"
#include "Tray.h"
#include "UiControls.h"

#include <commdlg.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <windowsx.h>

#include <algorithm>
#include <string>
#include <vector>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Shell32.lib")

std::wstring GetWindowTextValue(HWND hwnd)
{
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring text(length + 1, L'\0');
    if (length > 0)
    {
        GetWindowTextW(hwnd, text.data(), static_cast<int>(text.size()));
    }
    text.resize(length);
    return text;
}

void SetComboAction(HWND combo, ActionType type)
{
    const std::wstring value = ActionTypeToString(type);
    for (int i = 0; i < ComboBox_GetCount(combo); ++i)
    {
        wchar_t item[64]{};
        ComboBox_GetLBText(combo, i, item);
        if (value == item)
        {
            ComboBox_SetCurSel(combo, i);
            return;
        }
    }
    ComboBox_SetCurSel(combo, 6);
}

ActionType GetComboAction(HWND combo)
{
    const int index = ComboBox_GetCurSel(combo);
    if (index == CB_ERR)
    {
        return ActionType::None;
    }

    wchar_t item[64]{};
    ComboBox_GetLBText(combo, index, item);
    return ParseActionType(item);
}

void FillPanelPositionCombo(HWND combo)
{
    ComboBox_ResetContent(combo);
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::TopLeft).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::TopCenter).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::TopRight).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::LeftCenter).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::Center).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::RightCenter).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::BottomLeft).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::BottomCenter).c_str());
    ComboBox_AddString(combo, PanelPositionToLabel(PanelPosition::BottomRight).c_str());
    ComboBox_SetCurSel(combo, static_cast<int>(g_panelPosition));
}

void CollectPanelPosition()
{
    if (!g_panelPositionCombo)
    {
        return;
    }

    const int index = ComboBox_GetCurSel(g_panelPositionCombo);
    if (index != CB_ERR)
    {
        g_panelPosition = static_cast<PanelPosition>(index);
    }
}

void FillGridSizeCombo(HWND combo)
{
    ComboBox_ResetContent(combo);
    for (int size = MIN_GRID_SIZE; size <= MAX_GRID_SIZE; ++size)
    {
        const std::wstring label = std::to_wstring(size) + L" x " + std::to_wstring(size);
        ComboBox_AddString(combo, label.c_str());
    }
    ComboBox_SetCurSel(combo, g_gridSize - MIN_GRID_SIZE);
}

void CollectGridSize()
{
    if (!g_gridSizeCombo)
    {
        return;
    }

    const int index = ComboBox_GetCurSel(g_gridSizeCombo);
    if (index != CB_ERR)
    {
        g_gridSize = ClampGridSize(MIN_GRID_SIZE + index);
    }
}

void FillThemeCombo(HWND combo)
{
    ComboBox_ResetContent(combo);
    ComboBox_AddString(combo, ThemeToLabel(AppTheme::Dark).c_str());
    ComboBox_AddString(combo, ThemeToLabel(AppTheme::Coffee).c_str());
    ComboBox_AddString(combo, ThemeToLabel(AppTheme::Beige).c_str());
    ComboBox_AddString(combo, ThemeToLabel(AppTheme::BlueGray).c_str());
    ComboBox_SetCurSel(combo, static_cast<int>(g_theme));
}

void CollectTheme()
{
    if (!g_themeCombo)
    {
        return;
    }

    const int index = ComboBox_GetCurSel(g_themeCombo);
    if (index != CB_ERR)
    {
        g_theme = static_cast<AppTheme>(index);
    }
}

void CollectEditorPage()
{
    if (!g_editorWnd || g_editorPage < 0 || g_editorPage >= static_cast<int>(g_pages.size()))
    {
        return;
    }

    CollectPanelPosition();
    CollectGridSize();
    CollectTheme();

    Page& page = g_pages[g_editorPage];
    page.name = GetWindowTextValue(g_pageNameEdit);

    for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
    {
        ButtonAction& button = page.buttons[i];
        button.title = GetWindowTextValue(g_buttonEditors[i].title);
        button.type = GetComboAction(g_buttonEditors[i].type);
        button.value = GetWindowTextValue(g_buttonEditors[i].value);
        button.imagePath = GetWindowTextValue(g_buttonEditors[i].image);
    }
}

void FillPageCombo()
{
    ComboBox_ResetContent(g_pageCombo);
    for (const Page& page : g_pages)
    {
        ComboBox_AddString(g_pageCombo, page.name.c_str());
    }
    ComboBox_SetCurSel(g_pageCombo, g_editorPage);
}

void LoadEditorPage()
{
    if (!g_editorWnd || g_pages.empty())
    {
        return;
    }

    g_editorPage = std::clamp(g_editorPage, 0, static_cast<int>(g_pages.size()) - 1);
    FillPageCombo();
    if (g_panelPositionCombo)
    {
        ComboBox_SetCurSel(g_panelPositionCombo, static_cast<int>(g_panelPosition));
    }
    if (g_gridSizeCombo)
    {
        ComboBox_SetCurSel(g_gridSizeCombo, g_gridSize - MIN_GRID_SIZE);
    }
    if (g_themeCombo)
    {
        ComboBox_SetCurSel(g_themeCombo, static_cast<int>(g_theme));
    }

    const Page& page = g_pages[g_editorPage];
    SetWindowTextW(g_pageNameEdit, page.name.c_str());

    for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
    {
        const ButtonAction& button = page.buttons[i];
        SetWindowTextW(g_buttonEditors[i].title, button.title.c_str());
        SetComboAction(g_buttonEditors[i].type, button.type);
        SetWindowTextW(g_buttonEditors[i].value, button.value.c_str());
        SetWindowTextW(g_buttonEditors[i].image, button.imagePath.c_str());

        const BOOL visible = i < ButtonCount() ? TRUE : FALSE;
        ShowWindow(GetDlgItem(g_editorWnd, ID_BUTTON_TITLE_BASE + MAX_BUTTON_COUNT + i), visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].title, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].type, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].value, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].valueBrowse, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].image, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].browse, visible ? SW_SHOW : SW_HIDE);
        ShowWindow(g_buttonEditors[i].help, visible ? SW_SHOW : SW_HIDE);
    }
}

void ShowHelpForAction(HWND owner, int index)
{
    const ActionType type = GetComboAction(g_buttonEditors[index].type);
    std::wstring message;

    switch (type)
    {
    case ActionType::Website:
        message = L"Для website введіть повну адресу сайту, наприклад:\nhttps://youtube.com";
        break;
    case ActionType::Program:
        message = L"Для program введіть команду або шлях до exe:\nnotepad.exe\nC:\\Program Files\\App\\app.exe";
        break;
    case ActionType::Folder:
        message = L"Для folder введіть шлях до папки:\n%USERPROFILE%\\Documents\nD:\\Projects";
        break;
    case ActionType::Hotkey:
        message = L"Для hotkey введіть комбінацію через '+':\nctrl+c\nctrl+shift+esc\nalt+tab";
        break;
    case ActionType::Text:
        message = L"Для text введіть текст, який потрібно надрукувати через SendInput.";
        break;
    case ActionType::NextPage:
        message = L"Для next_page введіть номер сторінки з нуля:\n0 - перша сторінка\n1 - друга сторінка";
        break;
    case ActionType::None:
    default:
        message = L"none вимикає кнопку. Поле значення можна залишити порожнім.";
        break;
    }

    message += L"\n\nЗображення: вкажіть шлях до PNG, JPG, BMP або ICO. Можна натиснути '...' для вибору файлу.";
    MessageBoxW(owner, message.c_str(), L"Підказка для кнопки", MB_ICONINFORMATION);
}

void BrowseButtonImage(HWND owner, int index)
{
    wchar_t fileName[MAX_PATH * 2]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileName));
    ofn.lpstrFilter = L"Images (*.png;*.jpg;*.jpeg;*.bmp;*.ico)\0*.png;*.jpg;*.jpeg;*.bmp;*.ico\0All files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Оберіть зображення для кнопки";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
    {
        SetWindowTextW(g_buttonEditors[index].image, fileName);
    }
}

void BrowseButtonValue(HWND owner, int index)
{
    const ActionType type = GetComboAction(g_buttonEditors[index].type);
    std::wstring selectedPath;

    switch (type)
    {
    case ActionType::Program:
        if (BrowseForFile(owner, L"Оберіть програму або файл",
            L"Programs (*.exe)\0*.exe\0All files (*.*)\0*.*\0", selectedPath))
        {
            SetWindowTextW(g_buttonEditors[index].value, selectedPath.c_str());
        }
        break;
    case ActionType::Folder:
        if (BrowseForFolder(owner, selectedPath))
        {
            SetWindowTextW(g_buttonEditors[index].value, selectedPath.c_str());
        }
        break;
    default:
        MessageBoxW(owner,
            L"Вибір через Провідник доступний для типів program і folder.\nДля зображення використовуйте кнопку '...' у колонці 'Зображення'.",
            L"Virtual Stream Deck",
            MB_ICONINFORMATION);
        break;
    }
}

void AddEditorPage()
{
    CollectEditorPage();
    Page page;
    page.name = L"Нова сторінка";
    page.buttons.resize(MAX_BUTTON_COUNT);
    page.buttons[ButtonCount() - 1] = { L"Next", ActionType::NextPage, L"0", L"" };
    g_pages.push_back(page);
    g_editorPage = static_cast<int>(g_pages.size()) - 1;
    LoadEditorPage();
}

void ShowPanel();

void SaveEditorConfig()
{
    CollectEditorPage();
    NormalizePages();

    if (g_currentPage >= static_cast<int>(g_pages.size()))
    {
        g_currentPage = 0;
    }

    if (!SaveButtons())
    {
        MessageBoxW(g_editorWnd, L"Не вдалося зберегти data\\buttons.txt.", L"Virtual Stream Deck", MB_ICONERROR);
        return;
    }

    InvalidateRect(g_hWnd, nullptr, TRUE);
    if (g_visible)
    {
        ShowPanel();
    }
    RefreshEditorBrushes();
    InvalidateRect(g_editorWnd, nullptr, TRUE);
    MessageBoxW(g_editorWnd, L"Налаштування збережено.", L"Virtual Stream Deck", MB_OK);
}

void ImportButtonSet()
{
    std::wstring path;
    if (!BrowseForFile(g_editorWnd, L"Імпорт сету кнопок",
        L"Virtual Stream Deck sets (*.txt)\0*.txt\0All files (*.*)\0*.*\0", path))
    {
        return;
    }

    if (!LoadButtonsFromFile(path))
    {
        MessageBoxW(g_editorWnd, L"Не вдалося імпортувати сет. Перевірте формат txt-файлу.", L"Virtual Stream Deck", MB_ICONERROR);
        return;
    }

    g_currentPage = 0;
    g_editorPage = 0;
    RefreshEditorBrushes();
    LoadEditorPage();
    InvalidateRect(g_hWnd, nullptr, TRUE);
    InvalidateRect(g_editorWnd, nullptr, TRUE);

    if (!SaveButtons())
    {
        MessageBoxW(g_editorWnd, L"Сет імпортовано, але не вдалося зберегти його як поточний data\\buttons.txt.", L"Virtual Stream Deck", MB_ICONWARNING);
        return;
    }

    MessageBoxW(g_editorWnd, L"Сет імпортовано і застосовано.", L"Virtual Stream Deck", MB_OK);
}

void ExportButtonSet()
{
    CollectEditorPage();
    NormalizePages();

    std::wstring path;
    if (!BrowseForSaveFile(g_editorWnd, L"Експорт сету кнопок",
        L"Virtual Stream Deck sets (*.txt)\0*.txt\0All files (*.*)\0*.*\0", L"txt", path))
    {
        return;
    }

    if (!SaveButtonsToFile(path))
    {
        MessageBoxW(g_editorWnd, L"Не вдалося експортувати сет у вибраний файл.", L"Virtual Stream Deck", MB_ICONERROR);
        return;
    }

    MessageBoxW(g_editorWnd, L"Сет експортовано. Цим txt-файлом можна поділитися.", L"Virtual Stream Deck", MB_OK);
}

RECT GetButtonRect(int index)
{
    const int titleHeight = 42;
    const int gridTop = PADDING + titleHeight;
    const int panelWidth = PanelWidth();
    const int panelHeight = PanelHeight();
    const int cellWidth = (panelWidth - PADDING * 2 - GAP * (g_gridSize - 1)) / g_gridSize;
    const int cellHeight = (panelHeight - gridTop - PADDING - GAP * (g_gridSize - 1)) / g_gridSize;
    const int row = index / g_gridSize;
    const int col = index % g_gridSize;

    RECT rect{};
    rect.left = PADDING + col * (cellWidth + GAP);
    rect.top = gridTop + row * (cellHeight + GAP);
    rect.right = rect.left + cellWidth;
    rect.bottom = rect.top + cellHeight;
    return rect;
}

int HitTestButton(int x, int y)
{
    POINT point{ x, y };
    for (int i = 0; i < ButtonCount(); ++i)
    {
        RECT rect = GetButtonRect(i);
        if (PtInRect(&rect, point))
        {
            return i;
        }
    }

    return -1;
}

RECT GetSettingsRect()
{
    const int panelWidth = PanelWidth();
    return RECT{ panelWidth - PADDING - SETTINGS_SIZE, PADDING, panelWidth - PADDING, PADDING + SETTINGS_SIZE };
}

bool HitTestSettings(int x, int y)
{
    RECT rect = GetSettingsRect();
    return PtInRect(&rect, POINT{ x, y });
}

void GetPanelTargetRect(RECT& workArea, int& x, int& y, int& panelWidth, int& panelHeight)
{
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0))
    {
        workArea = RECT{ 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    }

    constexpr int margin = 24;
    panelWidth = PanelWidth();
    panelHeight = PanelHeight();
    x = workArea.right - panelWidth - margin;
    y = workArea.top + (workArea.bottom - workArea.top - panelHeight) / 2;

    switch (g_panelPosition)
    {
    case PanelPosition::TopLeft:
        x = workArea.left + margin;
        y = workArea.top + margin;
        break;
    case PanelPosition::TopCenter:
        x = workArea.left + (workArea.right - workArea.left - panelWidth) / 2;
        y = workArea.top + margin;
        break;
    case PanelPosition::TopRight:
        x = workArea.right - panelWidth - margin;
        y = workArea.top + margin;
        break;
    case PanelPosition::LeftCenter:
        x = workArea.left + margin;
        y = workArea.top + (workArea.bottom - workArea.top - panelHeight) / 2;
        break;
    case PanelPosition::Center:
        x = workArea.left + (workArea.right - workArea.left - panelWidth) / 2;
        y = workArea.top + (workArea.bottom - workArea.top - panelHeight) / 2;
        break;
    case PanelPosition::RightCenter:
        x = workArea.right - panelWidth - margin;
        y = workArea.top + (workArea.bottom - workArea.top - panelHeight) / 2;
        break;
    case PanelPosition::BottomLeft:
        x = workArea.left + margin;
        y = workArea.bottom - panelHeight - margin;
        break;
    case PanelPosition::BottomCenter:
        x = workArea.left + (workArea.right - workArea.left - panelWidth) / 2;
        y = workArea.bottom - panelHeight - margin;
        break;
    case PanelPosition::BottomRight:
        x = workArea.right - panelWidth - margin;
        y = workArea.bottom - panelHeight - margin;
        break;
    }
}

void AnimatePanelY(int x, int fromY, int toY, int panelWidth, int panelHeight, bool showWindow)
{
    constexpr int animationFrames = 9;
    constexpr int frameDelayMs = 5;

    if (showWindow)
    {
        SetWindowPos(g_hWnd, HWND_TOPMOST, x, fromY, panelWidth, panelHeight, SWP_SHOWWINDOW);
        UpdateWindow(g_hWnd);
    }

    for (int frame = 1; frame <= animationFrames; ++frame)
    {
        const double progress = static_cast<double>(frame) / animationFrames;
        const double eased = 1.0 - (1.0 - progress) * (1.0 - progress);
        const int currentY = fromY + static_cast<int>((toY - fromY) * eased);
        SetWindowPos(g_hWnd, HWND_TOPMOST, x, currentY, panelWidth, panelHeight,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        Sleep(frameDelayMs);
    }
}

void ShowPanel()
{
    RECT workArea{};
    int x = 0;
    int y = 0;
    int panelWidth = 0;
    int panelHeight = 0;
    GetPanelTargetRect(workArea, x, y, panelWidth, panelHeight);

    const bool wasVisible = g_visible && IsWindowVisible(g_hWnd);
    if (wasVisible)
    {
        SetWindowPos(g_hWnd, HWND_TOPMOST, x, y, panelWidth, panelHeight, SWP_SHOWWINDOW);
    }
    else
    {
        const int startY = workArea.bottom + 8;
        AnimatePanelY(x, startY, y, panelWidth, panelHeight, true);

        SetWindowPos(g_hWnd, HWND_TOPMOST, x, y, panelWidth, panelHeight,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }

    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);
    g_visible = true;
}

void HidePanel()
{
    if (g_visible && IsWindowVisible(g_hWnd))
    {
        g_visible = false;

        RECT workArea{};
        int x = 0;
        int y = 0;
        int panelWidth = 0;
        int panelHeight = 0;
        GetPanelTargetRect(workArea, x, y, panelWidth, panelHeight);

        RECT currentRect{};
        GetWindowRect(g_hWnd, &currentRect);
        const int startX = currentRect.left;
        const int startY = currentRect.top;
        const int endY = workArea.bottom + 8;
        AnimatePanelY(startX, startY, endY, panelWidth, panelHeight, false);
    }

    ShowWindow(g_hWnd, SW_HIDE);
}

void SelectNextButton()
{
    g_selectedButton = (g_selectedButton + 1) % ButtonCount();
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

void HandlePanelHotkey()
{
    if (g_visible)
    {
        SelectNextButton();
    }
    else
    {
        ShowPanel();
    }
}

void ExitApplication()
{
    if (g_editorWnd)
    {
        DestroyWindow(g_editorWnd);
        g_editorWnd = nullptr;
    }

    if (g_hWnd)
    {
        DestroyWindow(g_hWnd);
    }
}

void DrawButtonImage(HDC hdc, const RECT& rect, const std::wstring& imagePath)
{
    if (imagePath.empty())
    {
        return;
    }

    const std::wstring path = ExpandEnvironment(imagePath);
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Image image(path.c_str());
    if (image.GetLastStatus() != Gdiplus::Ok)
    {
        return;
    }

    const int boxSize = 42;
    const int x = rect.left + (rect.right - rect.left - boxSize) / 2;
    const int y = rect.top + 14;
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(&image, x, y, boxSize, boxSize);
}

void DrawButton(HDC hdc, const RECT& rect, const ButtonAction& button, bool active)
{
    const ThemeColors colors = CurrentThemeColors();
    const COLORREF fill = active ? colors.buttonActiveFill : colors.buttonFill;
    const COLORREF border = active ? colors.buttonActiveBorder : colors.buttonBorder;
    const COLORREF text = button.type == ActionType::None ? colors.mutedText : colors.text;

    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, active ? 2 : 1, border);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    DrawButtonImage(hdc, rect, button.imagePath);

    RECT textRect = rect;
    InflateRect(&textRect, -8, -8);
    if (!button.imagePath.empty())
    {
        textRect.top += 54;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text);
    DrawTextW(hdc, button.title.empty() ? L"None" : button.title.c_str(), -1, &textRect,
        DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
}

void DrawSettingsButton(HDC hdc)
{
    RECT rect = GetSettingsRect();
    const ThemeColors colors = CurrentThemeColors();
    HBRUSH brush = CreateSolidBrush(colors.settingsFill);
    HPEN pen = CreatePen(PS_SOLID, 1, colors.settingsBorder);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    HFONT font = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, colors.text);
    DrawTextW(hdc, L"...", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void PaintPanel(HWND hWnd)
{
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT client{};
    GetClientRect(hWnd, &client);

    const ThemeColors colors = CurrentThemeColors();
    HBRUSH background = CreateSolidBrush(colors.panelBackground);
    FillRect(hdc, &client, background);
    DeleteObject(background);

    HFONT titleFont = CreateFontW(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT buttonFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    HGDIOBJ oldFont = SelectObject(hdc, titleFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, colors.text);

    RECT titleRect{ PADDING, PADDING, PanelWidth() - PADDING - SETTINGS_SIZE - 8, PADDING + 30 };
    const std::wstring title = g_pages.empty() ? L"Virtual Stream Deck" : g_pages[g_currentPage].name;
    DrawTextW(hdc, title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawSettingsButton(hdc);

    SelectObject(hdc, buttonFont);
    if (!g_pages.empty())
    {
        for (int i = 0; i < ButtonCount(); ++i)
        {
            DrawButton(hdc, GetButtonRect(i), g_pages[g_currentPage].buttons[i], i == g_selectedButton);
        }
    }

    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(buttonFont);

    EndPaint(hWnd, &ps);
}

void MoveSelection(int dx, int dy)
{
    int row = g_selectedButton / g_gridSize;
    int col = g_selectedButton % g_gridSize;
    row = std::clamp(row + dy, 0, g_gridSize - 1);
    col = std::clamp(col + dx, 0, g_gridSize - 1);
    g_selectedButton = row * g_gridSize + col;
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

void FillActionCombo(HWND combo)
{
    ComboBox_AddString(combo, L"website");
    ComboBox_AddString(combo, L"program");
    ComboBox_AddString(combo, L"folder");
    ComboBox_AddString(combo, L"hotkey");
    ComboBox_AddString(combo, L"text");
    ComboBox_AddString(combo, L"next_page");
    ComboBox_AddString(combo, L"none");
}

LRESULT CALLBACK EditorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateLabel(hWnd, L"Сторінка:", 16, 16, 80, 22);
        g_pageCombo = CreateCombo(hWnd, ID_PAGE_COMBO, 96, 12, 210, 240);
        CreateLabel(hWnd, L"Назва:", 320, 16, 55, 22);
        g_pageNameEdit = CreateEdit(hWnd, ID_PAGE_NAME, 376, 12, 190, 24);
        CreateWindowW(L"BUTTON", L"Додати сторінку", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            580, 11, 120, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ADD_PAGE)), g_hInst, nullptr);
        CreateLabel(hWnd, L"Сітка:", 720, 16, 50, 22);
        g_gridSizeCombo = CreateCombo(hWnd, ID_GRID_SIZE, 770, 12, 90, 120);
        FillGridSizeCombo(g_gridSizeCombo);

        CreateLabel(hWnd, L"#", 18, 54, 22, 20);
        CreateLabel(hWnd, L"Назва кнопки", 45, 54, 130, 20);
        CreateLabel(hWnd, L"Тип", 182, 54, 100, 20);
        CreateLabel(hWnd, L"Значення / посилання / шлях", 288, 54, 230, 20);
        CreateLabel(hWnd, L"Зображення", 564, 54, 176, 20);

        for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
        {
            const int column = i / 18;
            const int row = i % 18;
            const int x = 16 + column * 650;
            const int y = 78 + row * 29;
            wchar_t number[8]{};
            wsprintfW(number, L"%d", i + 1);
            CreateLabelWithId(hWnd, ID_BUTTON_TITLE_BASE + MAX_BUTTON_COUNT + i, number, x + 4, y + 4, 22, 20);
            g_buttonEditors[i].title = CreateEdit(hWnd, ID_BUTTON_TITLE_BASE + i, x + 29, y, 110, 24);
            g_buttonEditors[i].type = CreateCombo(hWnd, ID_BUTTON_TYPE_BASE + i, x + 144, y, 90, 160);
            FillActionCombo(g_buttonEditors[i].type);
            g_buttonEditors[i].value = CreateEdit(hWnd, ID_BUTTON_VALUE_BASE + i, x + 239, y, 160, 24);
            g_buttonEditors[i].valueBrowse = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                x + 404, y, 30, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BUTTON_VALUE_BROWSE_BASE + i)), g_hInst, nullptr);
            g_buttonEditors[i].image = CreateEdit(hWnd, ID_BUTTON_IMAGE_BASE + i, x + 439, y, 130, 24);
            g_buttonEditors[i].browse = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                x + 574, y, 30, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BUTTON_BROWSE_BASE + i)), g_hInst, nullptr);
            g_buttonEditors[i].help = CreateWindowW(L"BUTTON", L"?", WS_CHILD | WS_VISIBLE,
                x + 609, y, 26, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BUTTON_HELP_BASE + i)), g_hInst, nullptr);
        }

        CreateLabel(hWnd, L"Позиція:", 16, 615, 65, 22);
        g_panelPositionCombo = CreateCombo(hWnd, ID_PANEL_POSITION, 86, 610, 190, 180);
        FillPanelPositionCombo(g_panelPositionCombo);
        CreateLabel(hWnd, L"Тема:", 300, 615, 55, 22);
        g_themeCombo = CreateCombo(hWnd, ID_THEME, 356, 610, 160, 140);
        FillThemeCombo(g_themeCombo);

        CreateWindowW(L"BUTTON", L"Імпорт сету", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            540, 610, 120, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_IMPORT_SET)), g_hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Експорт сету", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            672, 610, 120, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EXPORT_SET)), g_hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Зберегти", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            984, 610, 100, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SAVE_CONFIG)), g_hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Закрити", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            1096, 610, 100, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CLOSE_EDITOR)), g_hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Вийти", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            1208, 610, 100, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EXIT_APP)), g_hInst, nullptr);

        LoadEditorPage();
        return 0;
    }

    case WM_COMMAND:
    {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);

        if (id == ID_PAGE_COMBO && code == CBN_SELCHANGE)
        {
            CollectEditorPage();
            g_editorPage = ComboBox_GetCurSel(g_pageCombo);
            LoadEditorPage();
            return 0;
        }

        if (id == ID_GRID_SIZE && code == CBN_SELCHANGE)
        {
            CollectEditorPage();
            NormalizePages();
            LoadEditorPage();
            InvalidateRect(g_hWnd, nullptr, TRUE);
            if (g_visible)
            {
                ShowPanel();
            }
            return 0;
        }

        if (id == ID_THEME && code == CBN_SELCHANGE)
        {
            CollectTheme();
            RefreshEditorBrushes();
            InvalidateRect(hWnd, nullptr, TRUE);
            InvalidateRect(g_hWnd, nullptr, TRUE);
            return 0;
        }

        if (id == ID_ADD_PAGE)
        {
            AddEditorPage();
            return 0;
        }

        if (id == ID_SAVE_CONFIG)
        {
            SaveEditorConfig();
            return 0;
        }

        if (id == ID_IMPORT_SET)
        {
            ImportButtonSet();
            return 0;
        }

        if (id == ID_EXPORT_SET)
        {
            ExportButtonSet();
            return 0;
        }

        if (id == ID_CLOSE_EDITOR)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }

        if (id == ID_EXIT_APP)
        {
            ExitApplication();
            return 0;
        }

        if (id >= ID_BUTTON_BROWSE_BASE && id < ID_BUTTON_BROWSE_BASE + MAX_BUTTON_COUNT)
        {
            BrowseButtonImage(hWnd, id - ID_BUTTON_BROWSE_BASE);
            return 0;
        }

        if (id >= ID_BUTTON_VALUE_BROWSE_BASE && id < ID_BUTTON_VALUE_BROWSE_BASE + MAX_BUTTON_COUNT)
        {
            BrowseButtonValue(hWnd, id - ID_BUTTON_VALUE_BROWSE_BASE);
            return 0;
        }

        if (id >= ID_BUTTON_HELP_BASE && id < ID_BUTTON_HELP_BASE + MAX_BUTTON_COUNT)
        {
            ShowHelpForAction(hWnd, id - ID_BUTTON_HELP_BASE);
            return 0;
        }
        break;
    }

    case WM_ERASEBKGND:
    {
        RECT rect{};
        GetClientRect(hWnd, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_editorBackgroundBrush);
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        const ThemeColors colors = CurrentThemeColors();
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, colors.text);
        return reinterpret_cast<LRESULT>(g_editorBackgroundBrush);
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        const ThemeColors colors = CurrentThemeColors();
        SetBkColor(hdc, colors.controlBackground);
        SetTextColor(hdc, colors.text);
        return reinterpret_cast<LRESULT>(g_editorControlBrush);
    }

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        g_editorWnd = nullptr;
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

ATOM RegisterEditorClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = EditorProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KURSOVA));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = g_editorBackgroundBrush;
    wc.lpszClassName = L"VirtualStreamDeckEditor";
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wc);
}

void OpenEditor()
{
    HidePanel();

    if (g_pages.empty())
    {
        AddDefaultButtons();
        NormalizePages();
    }

    g_editorPage = std::clamp(g_currentPage, 0, static_cast<int>(g_pages.size()) - 1);

    if (!g_editorWnd)
    {
        g_editorWnd = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            L"VirtualStreamDeckEditor",
            L"Налаштування Virtual Stream Deck",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1335,
            700,
            g_hWnd,
            nullptr,
            g_hInst,
            nullptr);
    }

    LoadEditorPage();
    ShowWindow(g_editorWnd, SW_SHOWNORMAL);
    SetForegroundWindow(g_editorWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TRAYICON:
        return HandleTrayMessage(lParam);

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && g_visible)
        {
            HidePanel();
        }
        return 0;

    case WM_ACTIVATEAPP:
        if (!wParam && g_visible)
        {
            HidePanel();
        }
        return 0;

    case WM_HOTKEY:
        if (wParam == HOTKEY_ID)
        {
            HandlePanelHotkey();
        }
        else if (wParam == EDIT_HOTKEY_ID)
        {
            OpenEditor();
        }
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_TAB:
            SelectNextButton();
            return 0;
        case VK_RETURN:
            ExecuteButton(g_selectedButton);
            HidePanel();
            return 0;
        case VK_ESCAPE:
            HidePanel();
            return 0;
        case VK_LEFT:
            MoveSelection(-1, 0);
            return 0;
        case VK_RIGHT:
            MoveSelection(1, 0);
            return 0;
        case VK_UP:
            MoveSelection(0, -1);
            return 0;
        case VK_DOWN:
            MoveSelection(0, 1);
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    {
        if (HitTestSettings(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        {
            return 0;
        }

        const int index = HitTestButton(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        if (index >= 0 && index != g_selectedButton)
        {
            g_selectedButton = index;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        if (HitTestSettings(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        {
            return 0;
        }

        const int index = HitTestButton(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        if (index >= 0)
        {
            g_selectedButton = index;
            SetCapture(hWnd);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        ReleaseCapture();
        if (HitTestSettings(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        {
            OpenEditor();
            return 0;
        }

        const int index = HitTestButton(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        if (index >= 0)
        {
            g_selectedButton = index;
            ExecuteButton(index);
            HidePanel();
        }
        return 0;
    }

    case WM_PAINT:
        PaintPanel(hWnd);
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        UnregisterHotKey(hWnd, HOTKEY_ID);
        UnregisterHotKey(hWnd, EDIT_HOTKEY_ID);
        if (g_gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

ATOM RegisterPanelClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KURSOVA));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"VirtualStreamDeckWindow";
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wc);
}

BOOL InitInstance(HINSTANCE hInstance)
{
    g_hInst = hInstance;

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"VirtualStreamDeckWindow",
        L"Virtual Stream Deck",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        PanelWidth(),
        PanelHeight(),
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!g_hWnd)
    {
        return FALSE;
    }

    // Напівпрозорий фон і округлення у стилі Windows 11.
    SetLayeredWindowAttributes(g_hWnd, 0, 238, LWA_ALPHA);
    const DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(g_hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    if (!RegisterHotKey(g_hWnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT, VK_SPACE))
    {
        MessageBoxW(g_hWnd, L"Не вдалося зареєструвати Ctrl + Alt + Space.", L"Virtual Stream Deck", MB_ICONWARNING);
    }

    if (!RegisterHotKey(g_hWnd, EDIT_HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'E'))
    {
        MessageBoxW(g_hWnd, L"Не вдалося зареєструвати Ctrl + Alt + E для редактора.", L"Virtual Stream Deck", MB_ICONWARNING);
    }

    AddTrayIcon();

    return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    LoadButtons();
    RefreshEditorBrushes();

    if (!RegisterPanelClass(hInstance) || !RegisterEditorClass(hInstance) || !InitInstance(hInstance))
    {
        DeleteObject(g_editorBackgroundBrush);
        DeleteObject(g_editorControlBrush);
        if (SUCCEEDED(comInit))
        {
            CoUninitialize();
        }
        return FALSE;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    const int result = static_cast<int>(msg.wParam);
    DeleteObject(g_editorBackgroundBrush);
    DeleteObject(g_editorControlBrush);
    if (SUCCEEDED(comInit))
    {
        CoUninitialize();
    }
    return result;
}
