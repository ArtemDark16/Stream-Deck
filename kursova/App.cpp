#include "App.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
HWND g_editorWnd = nullptr;
HWND g_pageCombo = nullptr;
HWND g_pageNameEdit = nullptr;
HWND g_panelPositionCombo = nullptr;
HWND g_gridSizeCombo = nullptr;
HWND g_themeCombo = nullptr;
std::vector<Page> g_pages;
int g_currentPage = 0;
int g_editorPage = 0;
int g_selectedButton = 0;
int g_gridSize = MIN_GRID_SIZE;
bool g_visible = false;
PanelPosition g_panelPosition = PanelPosition::RightCenter;
AppTheme g_theme = AppTheme::Dark;
ULONG_PTR g_gdiplusToken = 0;
HBRUSH g_editorBackgroundBrush = nullptr;
HBRUSH g_editorControlBrush = nullptr;
std::vector<ButtonEditorControls> g_buttonEditors(MAX_BUTTON_COUNT);

int ButtonCount()
{
    return g_gridSize * g_gridSize;
}

int PanelWidth()
{
    return (std::max)(MIN_PANEL_WIDTH, PADDING * 2 + GAP * (g_gridSize - 1) + PANEL_CELL_SIZE * g_gridSize);
}

int PanelHeight()
{
    const int titleHeight = 42;
    return (std::max)(MIN_PANEL_HEIGHT, PADDING + titleHeight + GAP * (g_gridSize - 1) + PANEL_CELL_SIZE * g_gridSize + PADDING);
}

int ClampGridSize(int size)
{
    return std::clamp(size, MIN_GRID_SIZE, MAX_GRID_SIZE);
}

ThemeColors CurrentThemeColors()
{
    switch (g_theme)
    {
    case AppTheme::Coffee:
        return ThemeColors{
            RGB(42, 31, 25), RGB(37, 27, 22), RGB(61, 45, 36),
            RGB(67, 49, 39), RGB(126, 86, 55), RGB(112, 83, 65),
            RGB(211, 162, 103), RGB(77, 56, 44), RGB(145, 105, 73),
            RGB(250, 241, 225), RGB(188, 166, 143)
        };
    case AppTheme::Beige:
        return ThemeColors{
            RGB(228, 218, 199), RGB(218, 207, 188), RGB(246, 239, 225),
            RGB(239, 230, 212), RGB(184, 146, 97), RGB(180, 166, 142),
            RGB(119, 88, 54), RGB(236, 226, 207), RGB(169, 148, 116),
            RGB(47, 42, 35), RGB(102, 91, 76)
        };
    case AppTheme::BlueGray:
        return ThemeColors{
            RGB(28, 35, 44), RGB(31, 38, 47), RGB(42, 51, 62),
            RGB(43, 52, 64), RGB(57, 93, 126), RGB(82, 99, 116),
            RGB(126, 178, 220), RGB(47, 57, 69), RGB(95, 116, 135),
            RGB(238, 244, 248), RGB(155, 170, 184)
        };
    case AppTheme::Dark:
    default:
        return ThemeColors{
            RGB(30, 32, 36), RGB(30, 32, 36), RGB(37, 39, 44),
            RGB(37, 39, 44), RGB(49, 103, 178), RGB(82, 86, 96),
            RGB(126, 179, 255), RGB(48, 51, 58), RGB(94, 99, 112),
            RGB(245, 247, 250), RGB(125, 128, 136)
        };
    }
}

void RefreshEditorBrushes()
{
    if (g_editorBackgroundBrush)
    {
        DeleteObject(g_editorBackgroundBrush);
    }
    if (g_editorControlBrush)
    {
        DeleteObject(g_editorControlBrush);
    }

    const ThemeColors colors = CurrentThemeColors();
    g_editorBackgroundBrush = CreateSolidBrush(colors.editorBackground);
    g_editorControlBrush = CreateSolidBrush(colors.controlBackground);
}

void AddDefaultButtons()
{
    Page mainPage;
    mainPage.name = L"Головна";
    mainPage.buttons =
    {
        { L"YouTube", ActionType::Website, L"https://youtube.com", L"" },
        { L"VS Code", ActionType::Program, L"code", L"" },
        { L"Documents", ActionType::Folder, L"%USERPROFILE%\\Documents", L"" },
        { L"Copy", ActionType::Hotkey, L"ctrl+c", L"" },
        { L"Paste", ActionType::Hotkey, L"ctrl+v", L"" },
        { L"GitHub", ActionType::Website, L"https://github.com", L"" },
        { L"Telegram", ActionType::Website, L"https://web.telegram.org", L"" },
        { L"Notepad", ActionType::Program, L"notepad.exe", L"" },
        { L"Next", ActionType::NextPage, L"1", L"" }
    };

    Page secondPage;
    secondPage.name = L"Друга";
    secondPage.buttons =
    {
        { L"Calculator", ActionType::Program, L"calc.exe", L"" },
        { L"Explorer", ActionType::Program, L"explorer.exe", L"" },
        { L"Hello", ActionType::Text, L"Hello from Virtual Stream Deck", L"" },
        { L"Select all", ActionType::Hotkey, L"ctrl+a", L"" },
        { L"Undo", ActionType::Hotkey, L"ctrl+z", L"" },
        { L"Search", ActionType::Hotkey, L"ctrl+f", L"" },
        { L"", ActionType::None, L"", L"" },
        { L"", ActionType::None, L"", L"" },
        { L"Next", ActionType::NextPage, L"0", L"" }
    };

    g_pages = { mainPage, secondPage };
}

void NormalizePages()
{
    g_gridSize = ClampGridSize(g_gridSize);
    for (Page& page : g_pages)
    {
        page.buttons.resize(MAX_BUTTON_COUNT);
    }

    g_selectedButton = std::clamp(g_selectedButton, 0, ButtonCount() - 1);
}

std::wstring Trim(const std::wstring& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), iswspace);
    const auto last = std::find_if_not(text.rbegin(), text.rend(), iswspace).base();
    return first < last ? std::wstring(first, last) : L"";
}

std::wstring ToLower(std::wstring text)
{
    std::transform(text.begin(), text.end(), text.begin(),
        [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
    return text;
}

std::vector<std::wstring> Split(const std::wstring& text, wchar_t delimiter)
{
    std::vector<std::wstring> parts;
    std::wstringstream stream(text);
    std::wstring item;

    while (std::getline(stream, item, delimiter))
    {
        parts.push_back(Trim(item));
    }

    return parts;
}

ActionType ParseActionType(const std::wstring& type)
{
    const std::wstring value = ToLower(type);

    if (value == L"website") return ActionType::Website;
    if (value == L"program") return ActionType::Program;
    if (value == L"folder") return ActionType::Folder;
    if (value == L"hotkey") return ActionType::Hotkey;
    if (value == L"text") return ActionType::Text;
    if (value == L"next_page") return ActionType::NextPage;
    return ActionType::None;
}

int ParseGridSize(const std::wstring& size)
{
    return ClampGridSize(_wtoi(size.c_str()));
}

PanelPosition ParsePanelPosition(const std::wstring& position)
{
    const std::wstring value = ToLower(position);

    if (value == L"top_left") return PanelPosition::TopLeft;
    if (value == L"top_center") return PanelPosition::TopCenter;
    if (value == L"top_right") return PanelPosition::TopRight;
    if (value == L"left_center") return PanelPosition::LeftCenter;
    if (value == L"center") return PanelPosition::Center;
    if (value == L"right_center") return PanelPosition::RightCenter;
    if (value == L"bottom_left") return PanelPosition::BottomLeft;
    if (value == L"bottom_center") return PanelPosition::BottomCenter;
    if (value == L"bottom_right") return PanelPosition::BottomRight;
    return PanelPosition::RightCenter;
}

AppTheme ParseTheme(const std::wstring& theme)
{
    const std::wstring value = ToLower(theme);

    if (value == L"coffee") return AppTheme::Coffee;
    if (value == L"beige") return AppTheme::Beige;
    if (value == L"blue_gray") return AppTheme::BlueGray;
    return AppTheme::Dark;
}

std::wstring ActionTypeToString(ActionType type)
{
    switch (type)
    {
    case ActionType::Website: return L"website";
    case ActionType::Program: return L"program";
    case ActionType::Folder: return L"folder";
    case ActionType::Hotkey: return L"hotkey";
    case ActionType::Text: return L"text";
    case ActionType::NextPage: return L"next_page";
    case ActionType::None:
    default:
        return L"none";
    }
}

std::wstring PanelPositionToString(PanelPosition position)
{
    switch (position)
    {
    case PanelPosition::TopLeft: return L"top_left";
    case PanelPosition::TopCenter: return L"top_center";
    case PanelPosition::TopRight: return L"top_right";
    case PanelPosition::LeftCenter: return L"left_center";
    case PanelPosition::Center: return L"center";
    case PanelPosition::RightCenter: return L"right_center";
    case PanelPosition::BottomLeft: return L"bottom_left";
    case PanelPosition::BottomCenter: return L"bottom_center";
    case PanelPosition::BottomRight: return L"bottom_right";
    default:
        return L"right_center";
    }
}

std::wstring PanelPositionToLabel(PanelPosition position)
{
    switch (position)
    {
    case PanelPosition::TopLeft: return L"Зверху зліва";
    case PanelPosition::TopCenter: return L"Зверху по центру";
    case PanelPosition::TopRight: return L"Зверху справа";
    case PanelPosition::LeftCenter: return L"Зліва по центру";
    case PanelPosition::Center: return L"По центру";
    case PanelPosition::RightCenter: return L"Справа по центру";
    case PanelPosition::BottomLeft: return L"Знизу зліва";
    case PanelPosition::BottomCenter: return L"Знизу по центру";
    case PanelPosition::BottomRight: return L"Знизу справа";
    default:
        return L"Справа по центру";
    }
}

std::wstring ThemeToString(AppTheme theme)
{
    switch (theme)
    {
    case AppTheme::Coffee: return L"coffee";
    case AppTheme::Beige: return L"beige";
    case AppTheme::BlueGray: return L"blue_gray";
    case AppTheme::Dark:
    default:
        return L"dark";
    }
}

std::wstring ThemeToLabel(AppTheme theme)
{
    switch (theme)
    {
    case AppTheme::Coffee: return L"Кавова";
    case AppTheme::Beige: return L"Бежева";
    case AppTheme::BlueGray: return L"Сіро-синя";
    case AppTheme::Dark:
    default:
        return L"Поточна темна";
    }
}

std::wstring SanitizeConfigField(std::wstring text)
{
    std::replace(text.begin(), text.end(), L'|', L' ');
    std::replace(text.begin(), text.end(), L'\r', L' ');
    std::replace(text.begin(), text.end(), L'\n', L' ');
    return Trim(text);
}

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
    {
        return "";
    }

    int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
    {
        return "";
    }

    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
    {
        return L"";
    }

    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
    {
        return L"";
    }

    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}
