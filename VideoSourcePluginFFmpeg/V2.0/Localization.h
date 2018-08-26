/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "BaseAfx.h"

#define STR_PREFIX L"Plugins.VideoSource."
#define KEY(k) (STR_PREFIX L ## k)
#define STR(text) locale->LookupString(KEY(text))

#ifndef VSP_VERSION
#define VSP_VERSION L" !INTERNAL VERSION!"
#endif

static CTSTR localizationStrings[] = {
    KEY("PluginName"),          L"�㲥Դ",
    KEY("PluginDescription"),   L"ʹ�� libvlc �ⲥ�ű��ص�����Ƶ�㲥�ļ�\n\n"
                                L"Plugin Version: " VSP_VERSION,
    KEY("ClassName"),           L"�㲥Դ",
    KEY("Settings"),			L"��ƵԴ����",
    KEY("PathOrUrl"),			L"��������Ҫ���ŵĵ㲥�ļ�·��:",
    KEY("Video"),               L"��Ƶ����",
    KEY("VideoWidth"),          L"��Ƶ���:",
    KEY("VideoHeight"),         L"��Ƶ�߶�:",
    KEY("StretchImage"),        L"��չͼ��",
    KEY("Audio"),               L"��Ƶ����",
    KEY("Volume"),              L"��Ƶ���� (0-100)",
    KEY("AudioOutputToStream"), L"��Ƶ�������",
    KEY("AudioOutputToDevice"), L"��Ƶ������豸",
    KEY("AudioOutputType"),     L"��Ƶģʽ",
    KEY("AudioOutputDevice"),   L"��Ƶ�豸",
    KEY("MediaFileOrUrl"),      L"���뱾�ص㲥�ļ�����URL��ַ",
    KEY("Playlist"),            L"�����б�",
    KEY("PlaylistLoop"),        L"ѭ������",
    KEY("PlaylistEditor"),      L"�����б�༭",
    KEY("Deinterlacing"),       L"����ת��:",
    KEY("ApplyVideoFilter"),    L"ʹ�ù�����",
    KEY("VideoFilter"),         L"��Ƶ����",
    KEY("VideoGamma"),          L"��Ƶ٤��ֵ",
    KEY("VideoContrast"),       L"��Ƶ�Աȶ�",
    KEY("VideoBrightness"),     L"��Ƶ����ֵ",
    KEY("ResetVideoFilter"),    L"���ù�����"
};

