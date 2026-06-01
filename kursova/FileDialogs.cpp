#include "FileDialogs.h"

#include <commdlg.h>
#include <shlobj.h>

bool BrowseForFile(HWND owner, const wchar_t* title, const wchar_t* filter, std::wstring& selectedPath)
{
    wchar_t fileName[MAX_PATH * 2]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileName));
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    selectedPath = fileName;
    return true;
}

bool BrowseForSaveFile(HWND owner, const wchar_t* title, const wchar_t* filter, const wchar_t* defaultExtension, std::wstring& selectedPath)
{
    wchar_t fileName[MAX_PATH * 2]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileName));
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExtension;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn))
    {
        return false;
    }

    selectedPath = fileName;
    return true;
}

bool BrowseForFolder(HWND owner, std::wstring& selectedPath)
{
    wchar_t folderPath[MAX_PATH]{};
    BROWSEINFOW bi{};
    bi.hwndOwner = owner;
    bi.pszDisplayName = folderPath;
    bi.lpszTitle = L"Оберіть папку";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl)
    {
        return false;
    }

    const bool ok = SHGetPathFromIDListW(pidl, folderPath) == TRUE;
    CoTaskMemFree(pidl);

    if (!ok)
    {
        return false;
    }

    selectedPath = folderPath;
    return true;
}
