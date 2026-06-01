#pragma once

#include <string>

void LoadButtons();
bool SaveButtons();
bool LoadButtonsFromFile(const std::wstring& path);
bool SaveButtonsToFile(const std::wstring& path);
