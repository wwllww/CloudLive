#pragma once


#define COLOR_CONTROL_CLASS TEXT("BLiveColorControl")

API_EXPORT void InitColorControl(HINSTANCE hInstance);

API_EXPORT void CCSetCustomColors(DWORD *colors);
API_EXPORT void CCGetCustomColors(DWORD *colors);

API_EXPORT DWORD CCGetColor(HWND hwnd);
API_EXPORT void  CCSetColor(HWND hwnd, DWORD color);
API_EXPORT void  CCSetColor(HWND hwnd, const Color3 &color);


#define CCN_CHANGED     0
