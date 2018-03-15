/**
 * John Bradley (jrb@turrettech.com)
 */
#include "VideoSource.h"
#include "VideoSourceConfigDialog.h"
#include "VideoSourceConfig.h"
#include "resource.h"
#include <windowsx.h>
#include <cstdio>

#define VIDEO_SOURCE_CLASS TEXT("VideoSourceFFmpeg")

LocaleStringLookup *pluginLocale = NULL;
HINSTANCE HinstanceDLL = NULL;

static bool bFirst = true;

bool STDCALL ConfigureVideoSource(Json::Value &element, bool bCreating)
{

	if (bFirst)
	{
		pluginLocale = new LocaleStringLookup;

		if (!pluginLocale->LoadStringFile(VIDEOSOURCELOCALPATH))
			Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(VIDEOSOURCELOCALPATH).c_str());


		bFirst = false;
	}

	VideoSourceConfig *config = new VideoSourceConfig(element);
	if (bCreating) {
        config->Populate();
    }
	if (config->m_bisDirectPlay)
	{
		DirectPlayConfigDialog oDirectPlayConfigDialog(config);

		if (oDirectPlayConfigDialog.Show()) {
		
			config->m_bisDirectPlay = false;
			config->Save();
			delete config;
			return true;
		}
		delete config;
		return false;
	}
    VideoSourceConfigDialog videoSourceConfigDialog(config);
        
    if (videoSourceConfigDialog.Show()) {
        config->Save();
		element["cx"] =  config->width;
		element["cy"] =  config->height;

        delete config;
        return true;
    }

    delete config;
    return false;
}

REGINST_CONFIGFUNC(VideoSource, ConfigureVideoSource)

#ifdef VLC2X
void log_callback(
    void *data, 
    int level, 
    const libvlc_log_t *ctx, 
    const char *format, 
    va_list args)
{
    wchar_t *levelString;
    switch (level) 
    {
    case LIBVLC_DEBUG:  levelString = TEXT("DEBUG  "); break;
    case LIBVLC_NOTICE: levelString = TEXT("NOTICE "); break;
    case LIBVLC_WARNING:levelString = TEXT("WARNING"); break;
    case LIBVLC_ERROR:  levelString = TEXT("ERROR  "); break;
    }
    
    char message[1024 * 16];
    memset(message, 0, sizeof(char) * 1024 * 16);

    vsnprintf(message, 1024 * 16, format, args);
    int messageLength = strlen(message);

    int messageBufferLength = utf8_to_wchar_len(message, messageLength, 0);
    wchar_t *messageBuffer = static_cast<wchar_t *>(calloc(messageBufferLength + 1, sizeof(wchar_t)));
    utf8_to_wchar(message, messageLength, messageBuffer, messageBufferLength, 0);

    Log::writeMessage(LOG_RTSPSERV, 1, ("VideoSourcePlugin::%s | %s"), levelString, messageBuffer);

    free(messageBuffer);
}
#endif


BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		HinstanceDLL = hinstDLL;
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		if (pluginLocale)
			delete pluginLocale;
	}
	return TRUE;
}
