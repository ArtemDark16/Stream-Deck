#pragma once

#include "framework.h"

#include <string>
#include <vector>

constexpr int HOTKEY_ID = 1;
constexpr int EDIT_HOTKEY_ID = 2;
constexpr int MIN_GRID_SIZE = 3;
constexpr int MAX_GRID_SIZE = 6;
constexpr int MAX_BUTTON_COUNT = MAX_GRID_SIZE * MAX_GRID_SIZE;
constexpr int MIN_PANEL_WIDTH = 390;
constexpr int MIN_PANEL_HEIGHT = 430;
constexpr int PANEL_CELL_SIZE = 92;
constexpr int PADDING = 18;
constexpr int GAP = 10;
constexpr int SETTINGS_SIZE = 30;

constexpr int ID_PAGE_COMBO = 3000;
constexpr int ID_PAGE_NAME = 3001;
constexpr int ID_ADD_PAGE = 3002;
constexpr int ID_SAVE_CONFIG = 3003;
constexpr int ID_CLOSE_EDITOR = 3004;
constexpr int ID_EXIT_APP = 3005;
constexpr int ID_PANEL_POSITION = 3006;
constexpr int ID_GRID_SIZE = 3007;
constexpr int ID_THEME = 3008;
constexpr int ID_IMPORT_SET = 3009;
constexpr int ID_EXPORT_SET = 3010;
constexpr int ID_BUTTON_TITLE_BASE = 3100;
constexpr int ID_BUTTON_TYPE_BASE = 3200;
constexpr int ID_BUTTON_VALUE_BASE = 3300;
constexpr int ID_BUTTON_IMAGE_BASE = 3400;
constexpr int ID_BUTTON_BROWSE_BASE = 3500;
constexpr int ID_BUTTON_HELP_BASE = 3600;
constexpr int ID_BUTTON_VALUE_BROWSE_BASE = 3700;

enum class ActionType
{
    Website,
    Program,
    Folder,
    Hotkey,
    Text,
    NextPage,
    None
};

enum class PanelPosition
{
    TopLeft,
    TopCenter,
    TopRight,
    LeftCenter,
    Center,
    RightCenter,
    BottomLeft,
    BottomCenter,
    BottomRight
};

enum class AppTheme
{
    Dark,
    Coffee,
    Beige,
    BlueGray
};

struct ThemeColors
{
    COLORREF panelBackground;
    COLORREF editorBackground;
    COLORREF controlBackground;
    COLORREF buttonFill;
    COLORREF buttonActiveFill;
    COLORREF buttonBorder;
    COLORREF buttonActiveBorder;
    COLORREF settingsFill;
    COLORREF settingsBorder;
    COLORREF text;
    COLORREF mutedText;
};

struct ButtonAction
{
    std::wstring title;
    ActionType type = ActionType::None;
    std::wstring value;
    std::wstring imagePath;
};

struct Page
{
    std::wstring name;
    std::vector<ButtonAction> buttons;
};

struct ButtonEditorControls
{
    HWND title = nullptr;
    HWND type = nullptr;
    HWND value = nullptr;
    HWND valueBrowse = nullptr;
    HWND image = nullptr;
    HWND browse = nullptr;
    HWND help = nullptr;
};

extern HINSTANCE g_hInst;
extern HWND g_hWnd;
extern HWND g_editorWnd;
extern HWND g_pageCombo;
extern HWND g_pageNameEdit;
extern HWND g_panelPositionCombo;
extern HWND g_gridSizeCombo;
extern HWND g_themeCombo;
extern std::vector<Page> g_pages;
extern int g_currentPage;
extern int g_editorPage;
extern int g_selectedButton;
extern int g_gridSize;
extern bool g_visible;
extern PanelPosition g_panelPosition;
extern AppTheme g_theme;
extern ULONG_PTR g_gdiplusToken;
extern HBRUSH g_editorBackgroundBrush;
extern HBRUSH g_editorControlBrush;
extern std::vector<ButtonEditorControls> g_buttonEditors;

int ButtonCount();
int PanelWidth();
int PanelHeight();
int ClampGridSize(int size);
ThemeColors CurrentThemeColors();
void RefreshEditorBrushes();
void AddDefaultButtons();
void NormalizePages();

std::wstring Trim(const std::wstring& text);
std::wstring ToLower(std::wstring text);
std::vector<std::wstring> Split(const std::wstring& text, wchar_t delimiter);
ActionType ParseActionType(const std::wstring& type);
int ParseGridSize(const std::wstring& size);
PanelPosition ParsePanelPosition(const std::wstring& position);
AppTheme ParseTheme(const std::wstring& theme);
std::wstring ActionTypeToString(ActionType type);
std::wstring PanelPositionToString(PanelPosition position);
std::wstring PanelPositionToLabel(PanelPosition position);
std::wstring ThemeToString(AppTheme theme);
std::wstring ThemeToLabel(AppTheme theme);
std::wstring SanitizeConfigField(std::wstring text);
std::string WideToUtf8(const std::wstring& text);
std::wstring Utf8ToWide(const std::string& text);
