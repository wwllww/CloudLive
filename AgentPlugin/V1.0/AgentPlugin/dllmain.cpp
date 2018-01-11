// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "BaseAfx.h"
#include "stdafx.h"

HINSTANCE hinstMain = NULL;
extern LocaleStringLookup *pluginLocale;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hinstMain = hModule;
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		if (pluginLocale)
			delete pluginLocale;
		break;
	}
	return TRUE;
}

