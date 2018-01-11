#pragma once

#define VOLUME_METER_CLASS TEXT("BLiveVolumeMeter")

API_EXPORT void InitVolumeMeter(HINSTANCE hInst);
API_EXPORT float SetVolumeMeterValue(HWND hwnd, float fTopVal, float fTopMax, float fTopPeak, float fBotVal = FLT_MIN, float fBotMax = FLT_MIN, float fBotPeak = FLT_MIN);

#define VOL_MIN -96
#define VOL_MAX 0

#define VOLN_METERED  0x302