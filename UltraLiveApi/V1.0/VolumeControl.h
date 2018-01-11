#pragma once

#define VOLUME_CONTROL_CLASS TEXT("BLiveVolumeControl")

API_EXPORT void InitVolumeControl(HINSTANCE hInst);
API_EXPORT void SetVolumeControlIcons(HWND hwnd, HICON hiconPlay, HICON hiconMute);
API_EXPORT float ToggleVolumeControlMute(HWND hwnd);
API_EXPORT float SetVolumeControlValue(HWND hwnd, float fValue);
API_EXPORT float GetVolumeControlValue(HWND hwnd);
API_EXPORT void SetVolumeControlMutedVal(HWND hwnd, float val);
API_EXPORT float GetVolumeControlMutedVal(HWND hwnd);


#define VOLN_ADJUSTING  0x300
#define VOLN_FINALVALUE 0x301
#define VOLN_TOGGLEMUTE 0x302

#define VOLN_MUTELEVEL 0.05f

/* 60 db dynamic range values for volume control scale*/
#define VOL_ALPHA .001f
#define VOL_BETA 6.908f
