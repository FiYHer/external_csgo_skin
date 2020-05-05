#include "common.h"
#include <TlHelp32.h>
#include <time.h>
#include <process.h>

void error(bool state, const char* text)
{
	if (state) return;

	char buffer[1024];
	wsprintfA(buffer, "发生错误 : %s  %d \n", text, GetLastError());
	MessageBoxA(nullptr, buffer, nullptr, 0);
	exit(-1);
}

void warning(bool state, const char* text)
{
	if (state) return;

	char buffer[1024];
	wsprintfA(buffer, "警告 : %s   %d", text, GetLastError());
	MessageBoxA(nullptr, buffer, nullptr, 0);
}

char* to_char_array(char* buffer, const char* text, dword address)
{
	wsprintfA(buffer, "%s[%x]", text, address);
	return buffer;
}

void load_weapon_skin_str()
{
	SendMessageA(g_info.listbox_skin_info, LB_RESETCONTENT, 0, 0);

	for (int i = 0; i < 814; i++)
		SendMessageA(g_info.listbox_skin_info, LB_ADDSTRING, 0, (lparam)skin_str[i]);
}

void load_knife_model_str()
{
	SendMessageA(g_info.combo_skin_model, CB_RESETCONTENT, 0, 0);

	for (int i = 0; i < 19; i++)
		SendMessageA(g_info.combo_skin_model, CB_ADDSTRING, 0, (lparam)knife_model_str[i]);

	SendMessageA(g_info.combo_skin_model, CB_SETCURSEL, 5, 0);
}

void get_all_skin_index()
{
	g_info.knife_model_index = SendMessageA(g_info.combo_skin_model, CB_GETCURSEL, 0, 0);

	char text[100]{ 0 };
	GetWindowTextA(g_info.edit_knife_skin, text, 100);
	if (strlen(text) == 0)
	{
		g_info.knife_skin_index = rand() % 500;
		SetWindowTextA(g_info.edit_knife_skin, _itoa(g_info.knife_skin_index, text, 10));
	}
	else g_info.knife_skin_index = atoi(text);

	GetWindowTextA(g_info.edit_main_weapon_skin, text, 100);
	if (strlen(text) == 0)
	{
		g_info.main_weapon_skin_index = rand() % 500;
		SetWindowTextA(g_info.edit_main_weapon_skin, _itoa(g_info.main_weapon_skin_index, text, 10));
	}
	g_info.main_weapon_skin_index = atoi(text);

	GetWindowTextA(g_info.edit_secondary_weapon_skin, text, 100);
	if (strlen(text) == 0)
	{
		g_info.secondary_weapon_skin_index = rand() % 500;
		SetWindowTextA(g_info.edit_secondary_weapon_skin, _itoa(g_info.secondary_weapon_skin_index, text, 10));
	}
	g_info.secondary_weapon_skin_index = atoi(text);
}

void change_skin()
{
	if (g_info.initialize_state == false)
	{
		warning(false, "没有成功初始化");
		return;
	}

	srand((unsigned int)time(nullptr));
	get_all_skin_index();

	if(g_info.skin_changing == false) 
		warning(_beginthread(change_skin_thread, 0, nullptr), "创建换肤线程失败");
}

bool is_knife(int i)
{
	return (i >= WEAPON_KNIFE_BAYONET && i < GLOVE_STUDDED_BLOODHOUND) || i == WEAPON_KNIFE_T || i == WEAPON_KNIFE;
}

void __cdecl change_skin_thread(void* data)
{
	g_info.skin_changing = true;
	MessageBoxA(g_info.window_hwnd, "换肤线程开启", nullptr, 0);

	const int item_id_high = -1;
	const int entity_quality = 3;
	const float fallback_wear = 0.0001f;

	unsigned int last_knife_model_index = g_info.knife_model_index;
	unsigned int model_index = 0;
	dword local_player = 0;

	while (g_info.skin_changing)
	{
		dword temp_player = (dword)read_memory(g_info.local_player_address, nullptr, sizeof(dword));
		if (temp_player == 0)//第一次
		{
			model_index = 0;
			continue;
		}
		else if (temp_player != local_player)//另一局
		{
			local_player = temp_player;
			model_index = 0;
		}

		if (last_knife_model_index != g_info.knife_model_index)
		{
			last_knife_model_index = g_info.knife_model_index;
			model_index = 0;
		}

		short knife_model = knife_model_ids[g_info.knife_model_index];
		while(model_index == 0) model_index = get_model_index(knife_model);

		unsigned int knife_skin = g_info.knife_skin_index;
		for (int i = 0; i < 8; i++)
		{
			dword current_weapon = (dword)read_memory(local_player + g_info.my_weapons_address + (i * 0x4), nullptr, sizeof(dword)) & 0xfff;
			current_weapon = (dword)read_memory(g_info.entity_list_address + (current_weapon - 1) * 0x10, nullptr, sizeof(dword));
			if (current_weapon == 0) continue;

			short weapon_index = (short)read_memory(current_weapon + g_info.item_definition_index_address, nullptr, sizeof(short));
			unsigned int main_weapon_skin = g_info.main_weapon_skin_index;
			unsigned int secondary_weapon_skin = g_info.secondary_weapon_skin_index;

			if (weapon_index == WEAPON_KNIFE
				|| weapon_index == WEAPON_KNIFE_T
				|| weapon_index == knife_model)
			{
				write_memory(current_weapon + g_info.item_definition_index_address, &knife_model, sizeof(short));
				write_memory(current_weapon + g_info.model_index_address, &model_index, sizeof(unsigned int));
				write_memory(current_weapon + g_info.view_model_index_address, &model_index, sizeof(unsigned int));
				write_memory(current_weapon + g_info.entity_quality_address, &entity_quality, sizeof(int));
				main_weapon_skin = knife_skin;
			}

			if (is_secondary_weapon(weapon_index)) main_weapon_skin = secondary_weapon_skin;
			if (main_weapon_skin)
			{
				write_memory(current_weapon + g_info.item_id_high_address, &item_id_high, sizeof(int));
				write_memory(current_weapon + g_info.fallback_paintkit_address, &main_weapon_skin, sizeof(unsigned int));
				write_memory(current_weapon + g_info.fallback_wear_address, &fallback_wear, sizeof(float));
			}
		}

		dword active_weapon = (dword)read_memory(local_player + g_info.active_weapon_address, nullptr, sizeof(dword)) & 0xfff;
		active_weapon = (dword)read_memory(g_info.entity_list_address + (active_weapon - 1) * 0x10, nullptr, sizeof(dword));
		if (active_weapon == 0) continue;

		short weapon_index = (short)read_memory(active_weapon + g_info.item_definition_index_address, nullptr, sizeof(short));
		if (weapon_index != knife_model) continue;

		dword active_view_model = (dword)read_memory(local_player + g_info.view_model_address, nullptr, sizeof(dword)) & 0xfff;
		active_view_model = (dword)read_memory(g_info.entity_list_address + (active_view_model - 1) * 0x10, nullptr, sizeof(dword));
		if (active_view_model == 0) continue;

		write_memory(active_view_model + g_info.model_index_address, &model_index, sizeof(unsigned int));

	}

	MessageBoxA(nullptr, "换肤线程退出", nullptr, 0);
	g_info.skin_changing = false;
}


bool is_secondary_weapon(short index)
{
	switch (index)
	{
	case WEAPON_HKP2000:
	case WEAPON_P250:
	case WEAPON_TEC9:
	case WEAPON_GLOCK:
	case WEAPON_USP_SILENCER:
	case WEAPON_TASER:
		return true;
	}
	return false;
}

void initialize_csgo()
{
	char buffer[2000]{ 0 };
	SendMessageA(g_info.listbox_address, LB_RESETCONTENT, 0, 0);
	g_info.initialize_state = false;

	if (get_process_id("csgo.exe") == false)
	{
		warning(false, "获取进程ID失败");
		return;
	}
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"游戏进程", g_info.process_id));

	if (get_process_handle() == false)
	{
		warning(false, "获取进程句柄失败");
		return;
	}
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"游戏句柄", (dword)g_info.process_handle));

	if (get_module_info("client_panorama.dll", g_info.client_panorama_base_address, g_info.client_panorama_size) == false)
	{
		warning(false, "获取client_panorama模块信息失败");
		return;
	}
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"client_panorama基址", g_info.client_panorama_base_address));
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"client_panorama大小", g_info.client_panorama_size));

	if (get_module_info("engine.dll", g_info.engine_base_address, g_info.engine_size) == false)
	{
		warning(false, "获取engine模块信息失败");
		return;
	}
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"engine基址", g_info.engine_base_address));
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer,"engine大小", g_info.engine_size));

	pbyte engine_memory = (pbyte)alloc_memory(g_info.engine_size);
	if (engine_memory == 0)
	{
		warning(false, "申请内存失败");
		return;
	}

	if (read_memory(g_info.engine_base_address, engine_memory, g_info.engine_size) == false)
	{
		warning(false, "读取进程内存失败");
		free_memory(engine_memory);
		return;
	}

	{
		const char* pattern = "\xA1\xAA\xAA\xAA\xAA\x33\xD2\x6A\x00\x6A\x00\x33\xC9\x89\xB0";
		g_info.client_state_address = find_pattern(
			engine_memory,
			g_info.engine_base_address,
			g_info.engine_size,
			(pbyte)pattern,
			sizeof("\xA1\xAA\xAA\xAA\xAA\x33\xD2\x6A\x00\x6A\x00\x33\xC9\x89\xB0") - 1,
			0xAA,
			0x1,
			0x0,
			true,
			false);
		if (g_info.client_state_address == 0)
		{
			warning(false, "client_state_address查找失败");
			free_memory(engine_memory);
			return;
		}
		SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "client_state", g_info.client_state_address));
	}

	{
		const char* pattern = "\x0C\x3B\x81\xAA\xAA\xAA\xAA\x75\x11\x8B\x45\x10\x83\xF8\x01\x7C\x09\x50\x83";
		g_info.model_precache_address = find_pattern(
			engine_memory,
			g_info.engine_base_address,
			g_info.engine_size,
			(pbyte)pattern,
			sizeof("\x0C\x3B\x81\xAA\xAA\xAA\xAA\x75\x11\x8B\x45\x10\x83\xF8\x01\x7C\x09\x50\x83") - 1,
			0xAA,
			0x3,
			0x0,
			true,
			false);
		if (g_info.model_precache_address == 0)
		{
			warning(false, "model_precache_address查找失败");
			free_memory(engine_memory);
			return;
		}
		SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "model_precache", g_info.model_precache_address));
	}

	free_memory(engine_memory);

	pbyte client_memory = (pbyte)alloc_memory(g_info.client_panorama_size);
	if(client_memory == 0)
	{
		warning(false, "申请内存失败");
		return;
	}

	if (read_memory(g_info.client_panorama_base_address, client_memory, g_info.client_panorama_size) == false)
	{
		warning(false, "读取进程数据失败");
		free_memory(client_memory);
		return;
	}

	{
		const char* pattern = "\xBB\xAA\xAA\xAA\xAA\x83\xFF\x01\x0F\x8C\xAA\xAA\xAA\xAA\x3B\xF8";
		g_info.entity_list_address = find_pattern(
			client_memory,
			g_info.client_panorama_base_address,
			g_info.client_panorama_size,
			(pbyte)pattern,
			sizeof("\xBB\xAA\xAA\xAA\xAA\x83\xFF\x01\x0F\x8C\xAA\xAA\xAA\xAA\x3B\xF8") - 1,
			0xAA,
			0x1,
			0x0,
			true,
			false);
		if (g_info.entity_list_address == 0)
		{
			free_memory(client_memory);
			warning(false, "entity_list_address查找失败");
			return;
		}
		SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "entity_list", g_info.entity_list_address));
	}

	{
		const char* pattern = "\x8D\x34\x85\xAA\xAA\xAA\xAA\x89\x15\xAA\xAA\xAA\xAA\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF";
		g_info.local_player_address = find_pattern(client_memory,
			g_info.client_panorama_base_address,
			g_info.client_panorama_size,
			(pbyte)pattern,
			sizeof("\x8D\x34\x85\xAA\xAA\xAA\xAA\x89\x15\xAA\xAA\xAA\xAA\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF") - 1,
			0xAA,
			0x3,
			0x4,
			true,
			false);
		if (g_info.local_player_address == 0)
		{
			free_memory(client_memory);
			warning(false, "local_player_address查找失败");
			return;
		}
		SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "local_player", g_info.local_player_address));
	}

	{
		const char* pattern = "\x44\x54\x5F\x54\x45\x57\x6F\x72\x6C\x64\x44\x65\x63\x61\x6C";
		g_info.get_all_classes_address = find_pattern(
			client_memory,
			g_info.client_panorama_base_address,
			g_info.client_panorama_size,
			(pbyte)pattern,
			sizeof("\x44\x54\x5F\x54\x45\x57\x6F\x72\x6C\x64\x44\x65\x63\x61\x6C") - 1,
			0xAA,
			0x0,
			0x0,
			false,
			false);

		g_info.get_all_classes_address = find_pattern(
			client_memory,
			g_info.client_panorama_base_address,
			g_info.client_panorama_size,
			(pbyte)&g_info.get_all_classes_address,
			sizeof(pbyte),
			0x0,
			0x2B,
			0x0,
			true,
			false
			);

		if (g_info.get_all_classes_address == 0)
		{
			free_memory(client_memory);
			warning(false, "get_all_classes_address查找失败");
			return;
		}
		SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "get_all_classes", g_info.get_all_classes_address));
	}

	g_info.view_model_address = find_netvar(g_info.get_all_classes_address, "DT_BasePlayer", "m_hViewModel[0]");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "view_model", g_info.view_model_address));
	
	g_info.view_model_index_address = find_netvar(g_info.get_all_classes_address, "DT_BaseCombatWeapon", "m_iViewModelIndex");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "view_model_index", g_info.view_model_index_address));

	g_info.fallback_wear_address = find_netvar(g_info.get_all_classes_address, "DT_BaseAttributableItem", "m_flFallbackWear");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "fallback_wear", g_info.fallback_wear_address));

	g_info.fallback_paintkit_address = find_netvar(g_info.get_all_classes_address, "DT_BaseAttributableItem", "m_nFallbackPaintKit");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "fallback_paintkit", g_info.fallback_paintkit_address));

	g_info.item_id_high_address = find_netvar(g_info.get_all_classes_address, "DT_BaseAttributableItem", "m_iItemIDHigh");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "item_id_high", g_info.item_id_high_address));

	g_info.entity_quality_address = find_netvar(g_info.get_all_classes_address, "DT_BaseAttributableItem", "m_iEntityQuality");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "entity_quality", g_info.entity_quality_address));

	g_info.item_definition_index_address = find_netvar(g_info.get_all_classes_address, "DT_BaseAttributableItem", "m_iItemDefinitionIndex");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "item_definition_index", g_info.item_definition_index_address));
	
	g_info.active_weapon_address = find_netvar(g_info.get_all_classes_address, "DT_BaseCombatCharacter", "m_hActiveWeapon");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "active_weapon", g_info.active_weapon_address));

	g_info.my_weapons_address = find_netvar(g_info.get_all_classes_address, "DT_BaseCombatCharacter", "m_hMyWeapons");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "my_weapons", g_info.my_weapons_address));

	g_info.model_index_address = find_netvar(g_info.get_all_classes_address, "DT_BaseViewModel", "m_nModelIndex");
	SendMessageA(g_info.listbox_address, LB_ADDSTRING, 0, (lparam)to_char_array(buffer, "model_index", g_info.model_index_address));

	bool finish_state = g_info.view_model_address
		&& g_info.view_model_index_address
		&& g_info.fallback_wear_address
		&& g_info.fallback_paintkit_address
		&& g_info.item_id_high_address
		&& g_info.entity_quality_address
		&& g_info.item_definition_index_address
		&& g_info.active_weapon_address
		&& g_info.my_weapons_address
		&& g_info.model_index_address;

	warning(finish_state, "find_netvar查找失败");
	free_memory(client_memory);

	if (finish_state)
	{
		g_info.initialize_state = true;
		MessageBoxA(nullptr, "初始化成功", "message", 0);
	}
}

bool get_process_id(const char* name)
{
	g_info.process_id = 0;

	handle snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return false;

	PROCESSENTRY32 proc_info{ 0 };
	proc_info.dwSize = sizeof(proc_info);

	BOOL state = Process32First(snap, &proc_info);
	while (state)
	{
		if (strncmp(name, proc_info.szExeFile, strlen(name)) == 0)
		{
			g_info.process_id = proc_info.th32ProcessID;
			break;
		}
		state = Process32Next(snap, &proc_info);
	}

	CloseHandle(snap);
	return g_info.process_id != 0;
}

bool get_process_handle()
{
	if (g_info.process_handle) CloseHandle(g_info.process_handle);

	g_info.process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_info.process_id);
	return g_info.process_handle != 0;
}

bool get_module_info(const char* name, dword& address, dword& size)
{
	address = size = 0;

	handle snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, g_info.process_id);
	if (snap == INVALID_HANDLE_VALUE) return false;

	MODULEENTRY32 mod_info{ 0 };
	mod_info.dwSize = sizeof(mod_info);

	BOOL state = Module32First(snap, &mod_info);
	while (state)
	{
		if (strncmp(name, mod_info.szModule, strlen(name)) == 0)
		{
			address = (dword)mod_info.modBaseAddr;
			size = mod_info.modBaseSize;
			break;
		}
		state = Module32Next(snap, &mod_info);
	}

	CloseHandle(snap);
	return address;
}

void* alloc_memory(int size)
{
	return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void free_memory(void* data)
{
	if (data) VirtualFree(data, 0, MEM_RELEASE);
	data = nullptr;
}

pvoid read_memory(dword address, lpvoid buffer, dword size)
{
	dword finish = 0;
	pvoid res = nullptr;
	BOOL state = ReadProcessMemory(g_info.process_handle, (LPCVOID)address, buffer ? buffer : &res, size, &finish);
	return buffer ? (pvoid)state : res;
}

bool write_memory(dword address, lpcvoid buffer, dword size)
{
	dword finish = 0;
	WriteProcessMemory(g_info.process_handle, (LPVOID)address, buffer, size, &finish);
	return finish;
}

dword find_pattern(pbyte buffer, dword address, dword size, pbyte pattern, dword len, unsigned char wildcard, dword offset, dword extra, bool relative, dword subtract)
{
	dword base_address = 0;

	for (dword i = 0; i < size - len; i++)
	{
		if (check_pattern(buffer + i, pattern, len, wildcard))//匹配成功
		{
			base_address = address + i;//基址 + 偏移

			if (relative) base_address = *(dword*)(buffer + i + offset);//相对位置
			if (subtract) base_address -= address;//需要减去
			base_address += extra;//加上额外
			break;
		}
	}
	return base_address;
}

bool check_pattern(pbyte buffer, pbyte pattern, dword len, unsigned char wildcard)
{
	for (dword i = 0; i < len; i++)
		if (pattern[i] != wildcard && buffer[i] != pattern[i]) return false;
	return true;
}

unsigned int get_model_index_by_name(const char* model_name)
{
	if (g_info.client_state_address == 0
		|| g_info.model_precache_address == 0)
	{
		g_info.skin_changing = false;
		return 1;
	}

	dword address_1 = (dword)read_memory(g_info.client_state_address, nullptr, sizeof(dword));
	dword address_2 = (dword)read_memory(address_1 + g_info.model_precache_address, nullptr, sizeof(dword));
	dword address_3 = (dword)read_memory(address_2 + 0x40, nullptr, sizeof(dword));
	dword address_4 = (dword)read_memory(address_3 + 0xC, nullptr, sizeof(dword));
	for (int i = 0; i < 1024; i++)
	{
		dword address_5 = (dword)read_memory(address_4 + 0xC + i * 0x34, nullptr, sizeof(dword));
		char buffer[128]{ 0 };
		if (read_memory(address_5, (pbyte)buffer, 128))
			if (_stricmp(buffer, model_name) == 0) return i;
	}
	return 0;
}

unsigned int get_model_index(short index)
{
	unsigned int value = 0;
	switch (index)
	{
	case WEAPON_KNIFE:
		value = get_model_index_by_name("models/weapons/v_knife_default_ct.mdl");
		break;
	case WEAPON_KNIFE_T:
		value = get_model_index_by_name("models/weapons/v_knife_default_t.mdl");
		break;
	case WEAPON_KNIFE_BAYONET:
		value = get_model_index_by_name("models/weapons/v_knife_bayonet.mdl");
		break;
	case WEAPON_KNIFE_FLIP:
		value = get_model_index_by_name("models/weapons/v_knife_flip.mdl");
		break;
	case WEAPON_KNIFE_GUT:
		value = get_model_index_by_name("models/weapons/v_knife_gut.mdl");
		break;
	case WEAPON_KNIFE_KARAMBIT:
		value = get_model_index_by_name("models/weapons/v_knife_karam.mdl");
		break;
	case WEAPON_KNIFE_M9_BAYONET:
		value = get_model_index_by_name("models/weapons/v_knife_m9_bay.mdl");
		break;
	case WEAPON_KNIFE_TACTICAL:
		value = get_model_index_by_name("models/weapons/v_knife_tactical.mdl");
		break;
	case WEAPON_KNIFE_FALCHION:
		value = get_model_index_by_name("models/weapons/v_knife_falchion_advanced.mdl");
		break;
	case WEAPON_KNIFE_SURVIVAL_BOWIE:
		value = get_model_index_by_name("models/weapons/v_knife_survival_bowie.mdl");
		break;
	case WEAPON_KNIFE_BUTTERFLY:
		value = get_model_index_by_name("models/weapons/v_knife_butterfly.mdl");
		break;
	case WEAPON_KNIFE_PUSH:
		value = get_model_index_by_name("models/weapons/v_knife_push.mdl");
		break;
	case WEAPON_KNIFE_URSUS:
		value = get_model_index_by_name("models/weapons/v_knife_ursus.mdl");
		break;
	case WEAPON_KNIFE_GYPSY_JACKKNIFE:
		value = get_model_index_by_name("models/weapons/v_knife_gypsy_jackknife.mdl");
		break;
	case WEAPON_KNIFE_STILETTO:
		value = get_model_index_by_name("models/weapons/v_knife_stiletto.mdl");
		break;
	case WEAPON_KNIFE_WIDOWMAKER:
		value = get_model_index_by_name("models/weapons/v_knife_widowmaker.mdl");
		break;
	case WEAPON_KNIFE_CSS:
		value = get_model_index_by_name("models/weapons/v_knife_css.mdl");
		break;
	case WEAPON_KNIFE_CORD:
		value = get_model_index_by_name("models/weapons/v_knife_cord.mdl");
		break;
	case WEAPON_KNIFE_CANIS:
		value = get_model_index_by_name("models/weapons/v_knife_canis.mdl");
		break;
	case WEAPON_KNIFE_OUTDOOR:
		value = get_model_index_by_name("models/weapons/v_knife_outdoor.mdl");
		break;
	case WEAPON_KNIFE_SKELETON:
		value = get_model_index_by_name("models/weapons/v_knife_skeleton.mdl");
		break;
	}
	return value;
}

dword get_next_class(dword address)
{
	return (dword)read_memory(address + 0x10, nullptr, sizeof(dword));
}

dword get_table(dword address)
{
	return (dword)read_memory(address + 0xc, nullptr, sizeof(dword));
}

bool get_table_name(dword address, char* data)
{
	dword name_address = 0;
	return read_memory(address + 0xc, (pbyte)&name_address, sizeof(name_address))
		&& read_memory(name_address, (pbyte)data, 128);
}

dword get_data_table(dword address)
{
	return (dword)read_memory(address + 0x28, nullptr, sizeof(dword));
}

dword get_offset(dword address)
{
	return (dword)read_memory(address + 0x2C, nullptr, sizeof(dword));
}

bool get_prop_name(dword address, char* data)
{
	dword name_address = 0;
	return read_memory(address + 0x0, (pbyte)&name_address, sizeof(dword))
		&& read_memory(name_address, (pbyte)data, 128);
}

dword get_prop_by_id(dword address, dword index)
{
	dword prop_address = (dword)read_memory(address + 0x0, nullptr, sizeof(dword));
	return (dword)(prop_address + 0x3C * index);
}

dword get_prop_count(dword address)
{
	return (dword)read_memory(address + 0x4, nullptr, sizeof(dword));
}

dword scan_table(dword table_address, const char* var_name, dword level)
{
	for (dword i = 0; i < get_prop_count(table_address); i++)
	{
		dword prop_address = get_prop_by_id(table_address, i);
		if(prop_address == 0) continue;

		char prop_name[128]{ 0 };
		if (get_prop_name(prop_address, prop_name) == false || isdigit(prop_name[0])) continue;

		dword offset = get_offset(prop_address);
		if (_stricmp(prop_name, var_name) == 0) return level + offset;

		dword table_address = get_data_table(prop_address);
		if (table_address == 0) continue;

		dword result = scan_table(table_address, var_name, level + offset);
		if (result) return result;
	}
	return 0;
}

dword find_netvar(dword start, const char* class_name, const char* netvar_name)
{
	for (dword i = start; i; i = get_next_class(i))
	{
		dword table_address = get_table(i);

		char table_name[128]{ 0 };
		if (get_table_name(table_address, table_name) == false) 
			continue;

		if (_stricmp(class_name, table_name) == 0)
			return scan_table(table_address, netvar_name, 0);
	}
	return 0;
}










