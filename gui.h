#pragma once
#include "common.h"

hresult _stdcall window_proc(hwnd, uint, wparam, lparam);

void create_and_display();

void msg_handle();

void create_font();

void initialize_controls(hwnd window_hwnd);
