/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.LoadPlugin

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#include "PipeVideoPlugin.h"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include"MediaInfoStuff.h"

LocaleStringLookup *pluginLocale = NULL;

HINSTANCE hinstMain = NULL;

WNDPROC PipeOKProc = NULL;
WNDPROC PipeCancelProc = NULL;
static HWND Hwnd = NULL;


//CTSTR lpRoxioVideoCaptureGUID = TEXT("{6994AD05-93EF-11D0-A3-CC-00-A0-C9-22-31-96}");
const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 } };

INT_PTR CALLBACK WindowButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (WM_PAINT == message)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		HBRUSH HBrush = CreateSolidBrush(RGB(102, 102, 102));

		FillRect(hdc, &Rect, HBrush);
		DeleteObject(HBrush);

		if (IsWindowEnabled(hwnd))
		{
			SetTextColor(hdc, RGB(255, 255, 255));
		}
		else
		{
			SetTextColor(hdc, RGB(153, 153, 153));
		}
		SetBkColor(hdc, RGB(102, 102, 102));
		SetBkMode(hdc, TRANSPARENT);

		SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

		TCHAR Title[MAX_PATH] = { 0 };

		GetWindowText(hwnd, Title, sizeof Title);

		RECT rtClient;
		GetClientRect(hwnd, &rtClient);

		if (wcslen(Title) > 0)
		{
			DrawText(hdc, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		EndPaint(hwnd, &ps);
	}

	WNDPROC WndProcTem = NULL;
	if (hwnd == GetDlgItem(Hwnd, IDOK))
	{
		WndProcTem = PipeOKProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = PipeCancelProc;
	}


	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

bool GetGUIDFromString(CTSTR lpGUID, GUID &targetGUID)
{
	String strGUID = lpGUID;
	if (strGUID.Length() != 38)
		return false;

	strGUID = strGUID.Mid(1, strGUID.Length() - 1);

	StringList GUIDData;
	strGUID.GetTokenList(GUIDData, '-', FALSE);

	if (GUIDData.Num() != 5)
		return false;

	if (GUIDData[0].Length() != 8 ||
		GUIDData[1].Length() != 4 ||
		GUIDData[2].Length() != 4 ||
		GUIDData[3].Length() != 4 ||
		GUIDData[4].Length() != 12)
	{
		return false;
	}
	targetGUID.Data1 = (UINT)tstring_base_to_uint(GUIDData[0], NULL, 16);
	targetGUID.Data2 = (WORD)tstring_base_to_uint(GUIDData[1], NULL, 16);
	targetGUID.Data3 = (WORD)tstring_base_to_uint(GUIDData[2], NULL, 16);
	targetGUID.Data4[0] = (BYTE)tstring_base_to_uint(GUIDData[3].Left(2), NULL, 16);
	targetGUID.Data4[1] = (BYTE)tstring_base_to_uint(GUIDData[3].Right(2), NULL, 16);
	targetGUID.Data4[2] = (BYTE)tstring_base_to_uint(GUIDData[4].Left(2), NULL, 16);
	targetGUID.Data4[3] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(2, 4), NULL, 16);
	targetGUID.Data4[4] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(4, 6), NULL, 16);
	targetGUID.Data4[5] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(6, 8), NULL, 16);
	targetGUID.Data4[6] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(8, 10), NULL, 16);
	targetGUID.Data4[7] = (BYTE)tstring_base_to_uint(GUIDData[4].Right(2), NULL, 16);

	return true;
}

IBaseFilter* GetExceptionDevice(CTSTR lpGUID)
{
	GUID targetGUID;
	if (!GetGUIDFromString(lpGUID, targetGUID))
		return NULL;

	IBaseFilter *filter;
	if (SUCCEEDED(CoCreateInstance(targetGUID, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filter)))
		return filter;

	return NULL;
}

#define DEV_EXCEPTION_COUNT 1
CTSTR lpExceptionNames[DEV_EXCEPTION_COUNT] = { TEXT("Elgato Game Capture HD") };
CTSTR lpExceptionGUIDs[DEV_EXCEPTION_COUNT] = { TEXT("{39F50F4C-99E1-464a-B6F9-D605B4FB5918}") };

#define PIPE_CLASSNAME TEXT("PipeCapture")

struct ConfigDialogData
{
    CTSTR lpName;
    Json::Value &data;

	bool bGlobalSource;
    bool bCreating;

	StringList audioRenderNameList;
	StringList audioRenderIDList;
 
	ConfigDialogData(Value &data) :data(data)
	{
	}
    ~ConfigDialogData(){
    }
};

bool FindRunProcess(String name)
{
	bool result = false;
	PROCESSENTRY32 pe32;
	// 在使用这个结构之前，先设置它的大小
	pe32.dwSize = sizeof(pe32);

	// 给系统内的所有进程拍一个快照
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// 遍历进程快照，轮流显示每个进程的信息
	BOOL bMore = ::Process32First(hProcessSnap, &pe32);
	while (bMore)
	{
		String tmp(pe32.szExeFile);
		if (tmp == name)
		{
			result = true;
			break;
		}

		bMore = ::Process32Next(hProcessSnap, &pe32);
	}

	// 不要忘记清除掉snapshot对象
	::CloseHandle(hProcessSnap);
	return result;
}

// 遍历显示器
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, List<PipeMonitorInfo> &monitors)
{
	monitors << PipeMonitorInfo(hMonitor, lprcMonitor);
	return TRUE;
}

void InitMonitor(HWND hwndList, XElement *data)
{
	// 空值返回结果
	if (NULL == data || NULL == hwndList)
	{
		AppWarning(L"data is empty");
		return;
	}
	// 获取当前显示器列表
	List<PipeMonitorInfo> monitors;
	monitors.Clear();
	EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)MonitorEnumProc, (LPARAM)&monitors);

	// 添加选项卡列表
	for (UINT i = 0; i < monitors.Num(); i++)
		SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)IntString(i + 1).Array());

	// 选中当前值	
	String strMonitorID = data->GetString(TEXT("MonitorISO"), L"");
	int monitorID = strMonitorID.ToInt();
	if (monitorID >(int)monitors.Num())
		monitorID = 0;
	SendMessage(hwndList, CB_SETCURSEL, (WPARAM)monitorID, 0);
	if (monitors.Num() <=1)
	{
		::EnableWindow(hwndList, FALSE);
	}
	return;
}

INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;

    switch(message)
    {
		case WM_NCACTIVATE:
		case WM_NCPAINT:
		{
						   DrawFrame(hwnd, -1, true);
		}
			return TRUE;
		case WM_NCLBUTTONDOWN:
		{
								 POINTS Pts = MAKEPOINTS(lParam);
								 POINT Pt;
								 RECT rtWid;
								 GetWindowRect(hwnd, &rtWid);
								 Pt.x = Pts.x - rtWid.left;
								 Pt.y = Pts.y - rtWid.top;

								 for (int i = 0; i < GetListTitle().size(); ++i)
								 {
									 if (GetListTitle()[i].IsPointInRect(Pt))
									 {
										 switch (GetListTitle()[i].Type)
										 {
										 case TitleType_Close:
											 SendMessage(hwnd, WM_CLOSE, 0, 0);
											 return 0;
										 default:
											 break;
										 }
									 }

								 }

								 return DefWindowProc(hwnd, message, wParam, lParam);
		}
			break;
		case WM_PAINT:
		{
						 PAINTSTRUCT ps;
						 HDC hdc = BeginPaint(hwnd, &ps);

						 RECT Rect;
						 GetClientRect(hwnd, &Rect);

						 HBRUSH HBrush = CreateSolidBrush(RGB(153, 153, 153));

						 FillRect(hdc, &Rect, HBrush);
						 DeleteObject(HBrush);

						 EndPaint(hwnd, &ps);
		}
			break;
		case WM_NCMOUSEMOVE:
		{

							   if (_bMouseTrack)
							   {
								   TRACKMOUSEEVENT csTME;
								   csTME.cbSize = sizeof (csTME);
								   csTME.dwFlags = TME_LEAVE | TME_HOVER;
								   csTME.hwndTrack = hwnd;
								   csTME.dwHoverTime = 10;
								   ::_TrackMouseEvent(&csTME);


								   _bMouseTrack = FALSE;
							   }

							   POINTS Pts = MAKEPOINTS(lParam);
							   POINT Pt;
							   RECT rtWid;
							   GetWindowRect(hwnd, &rtWid);
							   Pt.x = Pts.x - rtWid.left;
							   Pt.y = Pts.y - rtWid.top;
							   static bool FirstFind = false;
							   bool bFind = false;
							   for (int i = 0; i < GetListTitle().size(); ++i)
							   {

								   if (GetListTitle()[i].IsPointInRect(Pt))
								   {
									   DrawFrame(hwnd,GetListTitle()[i].Type, true);
									   bFind = true;
									   FirstFind = false;

								   }

							   }

							   if (!bFind && !FirstFind)
							   {
								   DrawFrame(hwnd, -1, true);
								   FirstFind = true;
							   }

							   return DefWindowProc(hwnd, message, wParam, lParam);
		}
			break;
		case WM_NCMOUSELEAVE:
		{
								_bMouseTrack = true;
								DrawFrame(hwnd, -1, true);
		}
			break;

		case WM_CTLCOLORSTATIC:
		{
								  HDC hdc = (HDC)wParam;
								  SetTextColor(hdc, RGB(255, 255, 255));
								  SetBkColor(hdc, RGB(153, 153, 153));

								  if (HBrush)
								  {
									  DeleteObject(HBrush);
								  }
								  HBrush = CreateSolidBrush(RGB(153, 153, 153));

								  return (LRESULT)HBrush;
		}
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		{
								   HDC hdc = (HDC)wParam;
								   SetTextColor(hdc, RGB(255, 255, 255));
								   SetBkColor(hdc, RGB(121, 121, 121));

								   if (HBrush)
								   {
									   DeleteObject(HBrush);
								   }
								   HBrush = CreateSolidBrush(RGB(121, 121, 121));
								   return (LRESULT)HBrush;
		}
			break;
		case WM_DESTROY:
			if (HBrush)
				DeleteObject(HBrush);
			break;
        case WM_INITDIALOG:
            {
							  PipeOKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
							  PipeCancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);

							  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);

                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
                ConfigDialogData *configData = (ConfigDialogData*)lParam;
				LocalizeWindow(hwnd, pluginLocale);

				HWND hwndTemp = NULL;
				int RenderSide = configData->data["RenderSide"].asInt();
				switch (RenderSide) 
				{
					case 0: 
						hwndTemp = GetDlgItem(hwnd, IDC_RADIO_LEFT); 
						break;
					case 1: 
						hwndTemp = GetDlgItem(hwnd, IDC_RADIO_CENTER); 
						break;
					case 2: 
						hwndTemp = GetDlgItem(hwnd, IDC_RADIO_RIGHT); 
						break;
				}
				SendMessage(hwndTemp, BM_SETCHECK, BST_CHECKED, 0);

				bool bInterace = configData->data["ScanInterlace"].asUInt() == 1;

				if (bInterace)
					SendMessage(GetDlgItem(hwnd, IDC_INTERLACE), BM_SETCHECK,BST_CHECKED,0);
				else
				{
					SendMessage(GetDlgItem(hwnd, IDC_PROGRESSIVE), BM_SETCHECK, BST_CHECKED, 0);
				}

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

				return TRUE;
            }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDOK:
					{
						 ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

						 // 渲染位置
						 if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_LEFT), BM_GETCHECK, 0, 0) == BST_CHECKED)
						 {
							 configData->data["RenderSide"] = 0;
						 }
						 else if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_CENTER), BM_GETCHECK, 0, 0) == BST_CHECKED)
						 {
							 configData->data["RenderSide"] = 1;
						 }
						 else
						 {
							 configData->data["RenderSide"] = 2;
						 }


						 configData->data["ScanInterlace"] = SendMessage(GetDlgItem(hwnd, IDC_INTERLACE), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;

						 // 音频是否采集
						 BOOL AudioOut = SendMessage(GetDlgItem(hwnd, IDC_CHECK_AUDIO), BM_GETCHECK, 0, 0) == BST_CHECKED;
						 configData->data["AudioOut"] = AudioOut;
					}

                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
            }
    }

    return FALSE;
}


static bool bFirst = true;

bool STDCALL ConfigurePipeSource(Json::Value &element, bool bCreating)
{

	if (bFirst)
	{
		pluginLocale = new LocaleStringLookup;

		if (!pluginLocale->LoadStringFile(PIPECAPTURELOCALPATH))
			Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(PIPECAPTURELOCALPATH).c_str());
		bFirst = false;
	}
	
    ConfigDialogData configData(element);
	configData.bGlobalSource = true;
    configData.bCreating = bCreating;

    if(BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG), GetMainWindow(), ConfigureDialogProc, (LPARAM)&configData) == IDOK)
    {
		UINT Width, Height;
		GetBaseSize(Width, Height);
		element["cx"] = Width;
		element["cy"] = Height;

        return true;
    }

    return false;
}

REGINST_CONFIGFUNC(PipeVideo, ConfigurePipeSource)

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#if defined _M_X64 && _MSC_VER == 1800
        _set_FMA3_enable(0);
#endif
        hinstMain = hInst;
    }
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		if (pluginLocale)
			delete pluginLocale;
	}

    return TRUE;
}
