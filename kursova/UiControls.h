#pragma once

#include "framework.h"

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h);
HWND CreateLabelWithId(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h);
HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h);
HWND CreateCombo(HWND parent, int id, int x, int y, int w, int h);
