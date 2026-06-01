#include "Config.h"

#include "App.h"

#include <fstream>
#include <iterator>
#include <sstream>

namespace
{
    constexpr const wchar_t* DEFAULT_CONFIG_PATH = L"data\\buttons.txt";
}

bool LoadButtonsFromFile(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }

    std::vector<Page> loadedPages;
    PanelPosition loadedPosition = PanelPosition::RightCenter;
    AppTheme loadedTheme = AppTheme::Dark;
    int loadedGridSize = MIN_GRID_SIZE;
    Page* currentPage = nullptr;
    std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF)
    {
        bytes.erase(0, 3);
    }

    std::wstringstream text(Utf8ToWide(bytes));
    std::wstring line;

    while (std::getline(text, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const std::vector<std::wstring> parts = Split(line, L'|');
        if (parts.size() >= 2 && ToLower(parts[0]) == L"page")
        {
            loadedPages.push_back(Page{ parts[1], {} });
            currentPage = &loadedPages.back();
            continue;
        }

        if (parts.size() >= 3 && ToLower(parts[0]) == L"settings" && ToLower(parts[1]) == L"position")
        {
            loadedPosition = ParsePanelPosition(parts[2]);
            continue;
        }

        if (parts.size() >= 3 && ToLower(parts[0]) == L"settings" && ToLower(parts[1]) == L"grid")
        {
            loadedGridSize = ParseGridSize(parts[2]);
            continue;
        }

        if (parts.size() >= 3 && ToLower(parts[0]) == L"settings" && ToLower(parts[1]) == L"theme")
        {
            loadedTheme = ParseTheme(parts[2]);
            continue;
        }

        if (currentPage && parts.size() >= 2 && currentPage->buttons.size() < MAX_BUTTON_COUNT)
        {
            ButtonAction button;
            button.title = parts[0];
            button.type = ParseActionType(parts[1]);
            button.value = parts.size() >= 3 ? parts[2] : L"";
            button.imagePath = parts.size() >= 4 ? parts[3] : L"";
            currentPage->buttons.push_back(button);
        }
    }

    if (loadedPages.empty())
    {
        return false;
    }

    g_pages = loadedPages;
    g_panelPosition = loadedPosition;
    g_gridSize = loadedGridSize;
    g_theme = loadedTheme;
    NormalizePages();
    return true;
}

void LoadButtons()
{
    if (!LoadButtonsFromFile(DEFAULT_CONFIG_PATH))
    {
        AddDefaultButtons();
        NormalizePages();
    }
}

bool SaveButtonsToFile(const std::wstring& path)
{
    std::wstring output;
    output += L"SETTINGS|position|" + PanelPositionToString(g_panelPosition) + L"\n";
    output += L"SETTINGS|grid|" + std::to_wstring(g_gridSize) + L"\n";
    output += L"SETTINGS|theme|" + ThemeToString(g_theme) + L"\n";
    for (const Page& page : g_pages)
    {
        output += L"PAGE|" + SanitizeConfigField(page.name) + L"\n";
        for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
        {
            const ButtonAction& button = page.buttons[i];
            output += SanitizeConfigField(button.title) + L"|";
            output += ActionTypeToString(button.type) + L"|";
            output += SanitizeConfigField(button.value) + L"|";
            output += SanitizeConfigField(button.imagePath) + L"\n";
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        return false;
    }

    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    const std::string utf8 = WideToUtf8(output);
    file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return true;
}

bool SaveButtons()
{
    CreateDirectoryW(L"data", nullptr);
    return SaveButtonsToFile(DEFAULT_CONFIG_PATH);
}
