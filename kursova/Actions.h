#pragma once

#include "framework.h"

#include <string>

std::wstring ExpandEnvironment(const std::wstring& text);
void SwitchToPage(int index);
void ExecuteButton(int index);
