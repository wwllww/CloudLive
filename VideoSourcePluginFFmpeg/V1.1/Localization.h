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
    KEY("PluginName"),          L"点播源",
    KEY("PluginDescription"),   L"使用 libvlc 库播放本地的音视频点播文件\n\n"
                                L"Plugin Version: " VSP_VERSION,
    KEY("ClassName"),           L"点播源",
    KEY("Settings"),			L"视频源设置",
    KEY("PathOrUrl"),			L"输入你想要播放的点播文件路径:",
    KEY("Video"),               L"视频设置",
    KEY("VideoWidth"),          L"视频宽度:",
    KEY("VideoHeight"),         L"视频高度:",
    KEY("StretchImage"),        L"扩展图像",
    KEY("Audio"),               L"音频设置",
    KEY("Volume"),              L"音频音量 (0-100)",
    KEY("AudioOutputToStream"), L"音频输出到流",
    KEY("AudioOutputToDevice"), L"音频输出到设备",
    KEY("AudioOutputType"),     L"音频模式",
    KEY("AudioOutputDevice"),   L"音频设备",
    KEY("MediaFileOrUrl"),      L"输入本地点播文件或者URL地址",
    KEY("Playlist"),            L"播放列表",
    KEY("PlaylistLoop"),        L"循环播放",
    KEY("PlaylistEditor"),      L"播放列表编辑",
    KEY("Deinterlacing"),       L"隔行转换:",
    KEY("ApplyVideoFilter"),    L"使用过滤器",
    KEY("VideoFilter"),         L"视频过滤",
    KEY("VideoGamma"),          L"视频伽玛值",
    KEY("VideoContrast"),       L"视频对比度",
    KEY("VideoBrightness"),     L"视频亮度值",
    KEY("ResetVideoFilter"),    L"重置过滤器"
};

