#include "gui.h"

int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	create_and_display();
	msg_handle();
	return 0;
}


