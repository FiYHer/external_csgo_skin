#include "gui.h"
common_info g_info;

hresult _stdcall window_proc(hwnd window_hwnd, uint msg, wparam wpa, lparam lpa)
{
	static HDC hdc;
	static PAINTSTRUCT ps;

	switch (msg)
	{
	case WM_CREATE:
		initialize_controls(window_hwnd);
		break;
	case WM_PAINT:
		hdc = BeginPaint(window_hwnd, &ps);
		EndPaint(window_hwnd, &ps);
		break;
	case WM_COMMAND:
		switch (wpa)
		{
		case button_load_fil_id:
			load_weapon_skin_str();
			break;
		case button_initialize_csgo_id:
			initialize_csgo();
			break;
		case button_change_skin_id:
			EnableWindow(g_info.button_stop_change_thread, TRUE);
			change_skin();
			break;
		case button_stop_change_thread_id:
			g_info.skin_changing = false;
			EnableWindow(g_info.button_stop_change_thread, FALSE);
			break;
		}
		break;
	case WM_CTLCOLORSTATIC:
		SetBkMode((HDC)wpa, TRANSPARENT);
		return (BOOL)((HBRUSH)GetStockObject(NULL_BRUSH));
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default: return DefWindowProcA(window_hwnd, msg, wpa, lpa);
	}
	return 1;
}

void create_and_display()
{
	const char *name = "external_csgo";

	WNDCLASSEXA window_class{ 0 };
	window_class.cbSize = sizeof(window_class);
	window_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	window_class.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	window_class.hInstance = GetModuleHandleA(nullptr);
	window_class.lpfnWndProc = window_proc;
	window_class.lpszClassName = name;
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	error(RegisterClassExA(&window_class), "RegisterClassExA失败");

	g_info.window_hwnd = CreateWindowA(name, name, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX &~WS_THICKFRAME,
		100, 100, 600, 500, 0, 0, GetModuleHandleA(nullptr), 0);
	error(g_info.window_hwnd, "CreateWindowA失败");

	ShowWindow(g_info.window_hwnd, SW_SHOW);
	UpdateWindow(g_info.window_hwnd);
}

void msg_handle()
{
	MSG msg{ 0 };
	while (GetMessageA(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

void create_font()
{
	g_info.font = CreateFontA(18, 0, 0, 0, FW_HEAVY,
		0, 0, 0, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "微软雅黑");
	warning(g_info.font, "CreateFontIndirectA失败");
}

void initialize_controls(hwnd window_hwnd)
{
	create_font();

	dword style = WS_VISIBLE | WS_BORDER | WS_CHILD;

	g_info.button_initialize_csgo = CreateWindowA("Button", "初始化CSGO", style | BS_PUSHBOX | BS_CENTER,
		0, 0, 200, 50, window_hwnd, (HMENU)button_initialize_csgo_id, 0, 0);
	SendMessageA(g_info.button_initialize_csgo, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.listbox_address = CreateWindowA("ListBox", nullptr, style | LBS_NOTIFY | WS_VSCROLL,
		0, 50, 200, 300, window_hwnd, (HMENU)listbox_address_id, 0, 0);
	SendMessageA(g_info.listbox_address, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.button_change_skin = CreateWindowA("Button", "修改皮肤", style | BS_PUSHBOX | BS_CENTER,
		200, 0, 200, 50, window_hwnd, (HMENU)button_change_skin_id, 0, 0);
	SendMessageA(g_info.button_change_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.text_knife_model = CreateWindowA("Static", "小刀模型", style & ~WS_BORDER,
		210, 70, 70, 20, window_hwnd, 0, 0, 0);
	SendMessageA(g_info.text_knife_model, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.combo_skin_model = CreateWindowA("ComboBox", nullptr, style | CBS_DROPDOWNLIST,
		280, 70 - 4, 100, 23, window_hwnd, (HMENU)combo_knife_model_id, 0, 0);
	SendMessageA(g_info.combo_skin_model, WM_SETFONT, (wparam)g_info.font, 1);
	load_knife_model_str();

	g_info.text_knife_skin = CreateWindowA("Static", "小刀皮肤", style & ~WS_BORDER,
		210, 120, 70, 20, window_hwnd, 0, 0, 0);
	SendMessageA(g_info.text_knife_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.edit_knife_skin = CreateWindowA("Edit", nullptr, style,
		280, 120 - 4, 100, 23, window_hwnd, (HMENU)edit_knife_skin_id, 0, 0);
	SendMessageA(g_info.edit_knife_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.text_main_weapon_skin = CreateWindowA("Static", "主武器", style & ~WS_BORDER,
		210, 170, 70, 20, window_hwnd, 0, 0, 0);
	SendMessageA(g_info.text_main_weapon_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.edit_main_weapon_skin = CreateWindowA("Edit", nullptr, style,
		280, 170 - 4, 100, 23, window_hwnd, (HMENU)edit_main_weapon_skin_id, 0, 0);
	SendMessageA(g_info.edit_main_weapon_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.text_secondary_weapon_skin = CreateWindowA("Static", "副武器", style & ~WS_BORDER,
		210, 210, 70, 20, window_hwnd, 0, 0, 0);
	SendMessageA(g_info.text_secondary_weapon_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.edit_secondary_weapon_skin = CreateWindowA("Edit", nullptr, style,
		280, 210 - 4, 100, 23, window_hwnd, (HMENU)edit_secondary_weapon_skin_id, 0, 0);
	SendMessageA(g_info.edit_secondary_weapon_skin, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.button_stop_change_thread = CreateWindowA("Button", "我不想换肤了", style | BS_PUSHBOX | BS_CENTER,
		200, 274, 200, 50, window_hwnd, (HMENU)button_stop_change_thread_id, 0, 0);
	SendMessageA(g_info.button_stop_change_thread, WM_SETFONT, (wparam)g_info.font, 1);
	EnableWindow(g_info.button_stop_change_thread, FALSE);

	g_info.button_load_fil = CreateWindowA("Button", "加载皮肤文件", style | BS_PUSHBOX | BS_CENTER,
		400, 0, 200, 50, window_hwnd, (HMENU)button_load_fil_id, 0, 0);
	SendMessageA(g_info.button_load_fil, WM_SETFONT, (wparam)g_info.font, 1);

	g_info.listbox_skin_info = CreateWindowA("ListBox", nullptr, style | LBS_NOTIFY | WS_VSCROLL,
		400, 50, 185, 300, window_hwnd, (HMENU)listbox_skin_info_id, 0, 0);
	SendMessageA(g_info.listbox_skin_info, WM_SETFONT, (wparam)g_info.font, 1);

	hwnd text1 = CreateWindowA("Static", "玩家死亡后武器皮肤才开始更新...", style | SS_CENTER,
		0, 350, 600, 20, window_hwnd, 0, 0, 0);
	SendMessageA(text1, WM_SETFONT, (wparam)g_info.font, 1);
}
