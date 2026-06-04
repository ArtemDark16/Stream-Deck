#include "Actions.h"

#include "App.h"

#include <shellapi.h>
#include <tlhelp32.h>

#include <cwctype>

std::wstring ExpandEnvironment(const std::wstring& text)
{
    wchar_t buffer[MAX_PATH * 4]{};
    const DWORD length = ExpandEnvironmentStringsW(text.c_str(), buffer, static_cast<DWORD>(std::size(buffer)));
    return length > 0 && length < std::size(buffer) ? std::wstring(buffer) : text;
}

namespace
{
    struct WindowSearch
    {
        DWORD processId = 0;
        HWND window = nullptr;
    };

    std::wstring TrimQuotes(std::wstring text)
    {
        text = Trim(text);
        if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"')
        {
            return text.substr(1, text.size() - 2);
        }
        return text;
    }

    std::wstring ExtractExecutableName(std::wstring command)
    {
        command = TrimQuotes(command);
        if (command.empty())
        {
            return L"";
        }

        if (command.front() == L'"')
        {
            const size_t endQuote = command.find(L'"', 1);
            if (endQuote != std::wstring::npos)
            {
                command = command.substr(1, endQuote - 1);
            }
        }
        else
        {
            const size_t exeEnd = ToLower(command).find(L".exe");
            if (exeEnd != std::wstring::npos)
            {
                command = command.substr(0, exeEnd + 4);
            }
            else
            {
                const size_t firstSpace = command.find(L' ');
                if (firstSpace != std::wstring::npos)
                {
                    command = command.substr(0, firstSpace);
                }
            }
        }

        const size_t slash = command.find_last_of(L"\\/");
        std::wstring name = slash == std::wstring::npos ? command : command.substr(slash + 1);
        if (name.empty())
        {
            return L"";
        }

        if (ToLower(name).find(L".exe") == std::wstring::npos)
        {
            name += L".exe";
        }

        return name;
    }

    BOOL CALLBACK FindProcessWindow(HWND hwnd, LPARAM lParam)
    {
        WindowSearch* search = reinterpret_cast<WindowSearch*>(lParam);

        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        if (windowProcessId != search->processId)
        {
            return TRUE;
        }

        if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER))
        {
            return TRUE;
        }

        search->window = hwnd;
        return FALSE;
    }

    bool ActivateWindowForProcess(DWORD processId)
    {
        WindowSearch search{};
        search.processId = processId;
        EnumWindows(FindProcessWindow, reinterpret_cast<LPARAM>(&search));

        if (!search.window)
        {
            return false;
        }

        if (IsIconic(search.window))
        {
            ShowWindow(search.window, SW_RESTORE);
        }
        else
        {
            ShowWindow(search.window, SW_SHOWNORMAL);
        }

        SetForegroundWindow(search.window);
        return true;
    }

    bool ActivateRunningProgram(const std::wstring& command)
    {
        const std::wstring exeName = ToLower(ExtractExecutableName(command));
        if (exeName.empty())
        {
            return false;
        }

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        bool activated = false;

        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (ToLower(entry.szExeFile) == exeName && ActivateWindowForProcess(entry.th32ProcessID))
                {
                    activated = true;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return activated;
    }

    void SendVirtualKey(WORD vk, bool down)
    {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    WORD KeyNameToVk(const std::wstring& key)
    {
        if (key.size() == 1)
        {
            const wchar_t ch = towupper(key[0]);
            if (ch >= L'A' && ch <= L'Z') return static_cast<WORD>(ch);
            if (ch >= L'0' && ch <= L'9') return static_cast<WORD>(ch);
        }

        if (key == L"space") return VK_SPACE;
        if (key == L"enter") return VK_RETURN;
        if (key == L"tab") return VK_TAB;
        if (key == L"esc" || key == L"escape") return VK_ESCAPE;
        if (key == L"left") return VK_LEFT;
        if (key == L"right") return VK_RIGHT;
        if (key == L"up") return VK_UP;
        if (key == L"down") return VK_DOWN;
        if (key.size() >= 2 && key[0] == L'f')
        {
            const int number = _wtoi(key.c_str() + 1);
            if (number >= 1 && number <= 24) return static_cast<WORD>(VK_F1 + number - 1);
        }

        return 0;
    }

    void SendHotkey(const std::wstring& hotkey)
    {
        const std::vector<std::wstring> keys = Split(ToLower(hotkey), L'+');
        std::vector<WORD> modifiers;
        WORD mainKey = 0;

        for (const std::wstring& key : keys)
        {
            if (key == L"ctrl" || key == L"control") modifiers.push_back(VK_CONTROL);
            else if (key == L"alt") modifiers.push_back(VK_MENU);
            else if (key == L"shift") modifiers.push_back(VK_SHIFT);
            else if (key == L"win") modifiers.push_back(VK_LWIN);
            else mainKey = KeyNameToVk(key);
        }

        if (!mainKey)
        {
            return;
        }

        for (WORD modifier : modifiers) SendVirtualKey(modifier, true);
        SendVirtualKey(mainKey, true);
        SendVirtualKey(mainKey, false);
        for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) SendVirtualKey(*it, false);
    }

    void SendText(const std::wstring& text)
    {
        std::vector<INPUT> inputs;
        inputs.reserve(text.size() * 2);

        for (wchar_t ch : text)
        {
            INPUT down{};
            down.type = INPUT_KEYBOARD;
            down.ki.wScan = ch;
            down.ki.dwFlags = KEYEVENTF_UNICODE;

            INPUT up = down;
            up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

            inputs.push_back(down);
            inputs.push_back(up);
        }

        if (!inputs.empty())
        {
            SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
        }
    }
}

void SwitchToPage(int index)
{
    if (g_pages.empty())
    {
        return;
    }

    if (index < 0 || index >= static_cast<int>(g_pages.size()))
    {
        index = (g_currentPage + 1) % static_cast<int>(g_pages.size());
    }

    g_currentPage = index;
    g_selectedButton = 0;
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

void ExecuteButton(int index)
{
    if (g_pages.empty() || index < 0 || index >= ButtonCount())
    {
        return;
    }

    const ButtonAction& button = g_pages[g_currentPage].buttons[index];
    const std::wstring value = ExpandEnvironment(button.value);

    switch (button.type)
    {
    case ActionType::Website:
    case ActionType::Folder:
        ShellExecuteW(nullptr, L"open", value.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case ActionType::Program:
        if (!ActivateRunningProgram(value))
        {
            ShellExecuteW(nullptr, L"open", value.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case ActionType::Hotkey:
        SendHotkey(value);
        break;
    case ActionType::Text:
        SendText(value);
        break;
    case ActionType::NextPage:
        SwitchToPage(_wtoi(value.c_str()));
        break;
    case ActionType::None:
        break;
    }
}
