#pragma once

#include "framework.h"

#include <string>

bool BrowseForFile(HWND owner, const wchar_t* title, const wchar_t* filter, std::wstring& selectedPath);
bool BrowseForSaveFile(HWND owner, const wchar_t* title, const wchar_t* filter, const wchar_t* defaultExtension, std::wstring& selectedPath);
bool BrowseForFolder(HWND owner, std::wstring& selectedPath);
