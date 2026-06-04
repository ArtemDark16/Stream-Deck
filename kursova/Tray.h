#pragma once

#include "framework.h"

constexpr UINT WM_TRAYICON = WM_APP + 1;

bool AddTrayIcon();
void RemoveTrayIcon();
void ShowTrayMenu();
LRESULT HandleTrayMessage(LPARAM lParam);
