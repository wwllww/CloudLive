/**
* John Bradley (jrb@turrettech.com)
*/

#include "VideoSourcePluginWrapper.h"

#include <Ole2.h>

static HINSTANCE hinstDLL = 0;
static HMODULE hmodVspPlugin;
static HMODULE hmodMediaProcessSDK;
static HMODULE hmodAvcodec;
static HMODULE hmodAvformat;

static LOADPLUGIN_PROC InternalLoadPlugin = 0;
static UNLOADPLUGIN_PROC InternalUnloadPlugin = 0;
static ONSTARTSTREAM_PROC InternalOnStartStream = 0;
static ONSTOPSTREAM_PROC InternalOnStopStream = 0;
static GETPLUGINNAME_PROC InternalGetPluginName = 0;
static GETPLUGINDESCRIPTION_PROC InternalGetPluginDescription = 0;

bool LoadPlugin()
{
    if (InternalLoadPlugin &&
        InternalUnloadPlugin &&
        InternalOnStartStream && 
        InternalOnStopStream &&
        InternalGetPluginName &&
        InternalGetPluginDescription) 
    {
        return InternalLoadPlugin();
    }

    return false;
}

void UnloadPlugin()
{
    InternalUnloadPlugin();
}

void OnStartStream()
{
    InternalOnStartStream();
}

void OnStopStream()
{
    InternalOnStopStream();
}

CTSTR GetPluginName()
{
    return InternalGetPluginName();
}

CTSTR GetPluginDescription()
{
    return InternalGetPluginDescription();
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    static bool isOleInitialized = false;
        
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        {
            isOleInitialized = OleInitialize(0) ? true : false;

            // order is important!
			
			hmodAvcodec = LoadLibrary(L".\\plugins\\VideoSourcePlugin\\avcodec-57.dll");

			hmodAvformat = LoadLibrary(L".\\plugins\\VideoSourcePlugin\\avformat-57.dll");

			hmodMediaProcessSDK = LoadLibrary(L".\\plugins\\VideoSourcePlugin\\MediaProcessSDK.dll");
            // main plugin dll
            hmodVspPlugin = LoadLibrary(L".\\plugins\\VideoSourcePlugin\\VideoSourcePluginFFmpegZHIBO.dll"); 

            if (hmodVspPlugin != NULL) {
                InternalLoadPlugin = (LOADPLUGIN_PROC)GetProcAddress(hmodVspPlugin, "LoadPlugin");
                InternalUnloadPlugin = (UNLOADPLUGIN_PROC)GetProcAddress(hmodVspPlugin, "UnloadPlugin");
                InternalOnStartStream = (ONSTARTSTREAM_PROC)GetProcAddress(hmodVspPlugin, "OnStartStream");
                InternalOnStopStream = (ONSTOPSTREAM_PROC)GetProcAddress(hmodVspPlugin, "OnStopStream");
                InternalGetPluginName = (GETPLUGINNAME_PROC)GetProcAddress(hmodVspPlugin, "GetPluginName");
                InternalGetPluginDescription = (GETPLUGINDESCRIPTION_PROC)GetProcAddress(hmodVspPlugin, "GetPluginDescription");
            }
            break;
        }
    case DLL_PROCESS_DETACH:
        {
            if (isOleInitialized) {
                OleUninitialize();
            }

			if (hmodVspPlugin) FreeLibrary(hmodVspPlugin);
			if (hmodMediaProcessSDK) FreeLibrary(hmodMediaProcessSDK);
			if (hmodAvformat) FreeLibrary(hmodAvformat);
			if (hmodAvcodec) FreeLibrary(hmodAvcodec);
          
            break;
        }
    }
    return TRUE;
}
