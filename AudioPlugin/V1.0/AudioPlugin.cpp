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

#include "AudioPlugin.h"
#include <sstream>

LocaleStringLookup *pluginLocale = NULL;
#define PluginStr(param)  pluginLocale->LookupString(TEXT2(param))
HINSTANCE hinstMain = NULL;

WNDPROC AudioOKProc = NULL;
WNDPROC AudioCancelProc = NULL;
static HWND Hwnd = NULL;

#define AUDIO_CLASSNAME TEXT("AudioCapture")

struct ConfigDialogData
{
    CTSTR lpName;
    Value &data;
    StringList audioNameList;
	StringList audioIDList;

	ConfigDialogData(Value &e) :data(e){}

	bool bGlobalSource;
    bool bCreating;
};

//#include <Windows.h>  
//#pragma comment(lib, "winmm.lib")

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
		WndProcTem = AudioOKProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = AudioCancelProc;
	}


	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

//===================================================================
const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 } };

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


IBaseFilter* GetDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName, WSTR lpType2, CTSTR lpName2)
{
	//---------------------------------
	// exception devices
	if (scmpi(lpType2, L"DevicePath") == 0 && lpName2 && *lpName2 == '{')
		return GetExceptionDevice(lpName2);

	//---------------------------------

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return NULL;
	}

	err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return NULL;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices, so NO ENUM FO U
		return NULL;

	//---------------------------------

	IBaseFilter *bestFilter = NULL;

	IMoniker *deviceInfo;
	DWORD count;
	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT valueThingy;
			VARIANT valueThingy2;
			VariantInit(&valueThingy);
			VariantInit(&valueThingy2);

			if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
			{
				if (lpType2 && lpName2)
				{
					if (FAILED(propertyData->Read(lpType2, &valueThingy2, NULL)))
						nop();
				}

				SafeRelease(propertyData);

				String strVal1 = (CWSTR)valueThingy.bstrVal;

				if (strVal1 == lpName)
				{
					IBaseFilter *filter;
					err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
					if (FAILED(err))
					{
						AppWarning(TEXT("GetDeviceByValue: deviceInfo->BindToObject failed, result = %08lX"), err);
						continue;
					}

					if (!bestFilter)
					{
						bestFilter = filter;

						if (!lpType2 || !lpName2)
						{
							SafeRelease(deviceInfo);
							SafeRelease(videoDeviceEnum);

							return bestFilter;
						}
					}
					else if (lpType2 && lpName2)
					{
						String strVal2 = (CWSTR)valueThingy2.bstrVal;
						if (strVal2 == lpName2)
						{
							bestFilter->Release();

							bestFilter = filter;

							SafeRelease(deviceInfo);
							SafeRelease(videoDeviceEnum);

							return bestFilter;
						}
					}
					else
						filter->Release();
				}
			}
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return bestFilter;
}

bool PinHasMajorType(IPin *pin, const GUID *majorType)
{
	HRESULT hRes;

	IAMStreamConfig *config;
	if (SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
	{
		int count, size;
		if (SUCCEEDED(config->GetNumberOfCapabilities(&count, &size)))
		{
			BYTE *capsData = (BYTE*)Allocate_Bak(size);

			int priority = -1;
			for (int i = 0; i<count; i++)
			{
				AM_MEDIA_TYPE *pMT;
				if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
				{
					BOOL bDesiredMediaType = (pMT->majortype == *majorType);

					FreeMediaType(*pMT);
					CoTaskMemFree(pMT);

					if (bDesiredMediaType) {
						Free(capsData);
						SafeRelease(config);

						return true;
					}
				}
			}

			Free(capsData);
		}

		SafeRelease(config);
	}

	AM_MEDIA_TYPE *pinMediaType;

	IEnumMediaTypes *mediaTypesEnum;
	if (FAILED(pin->EnumMediaTypes(&mediaTypesEnum)))
		return false;

	ULONG curVal = 0;
	hRes = mediaTypesEnum->Next(1, &pinMediaType, &curVal);

	mediaTypesEnum->Release();

	if (hRes != S_OK)
		return false;

	BOOL bDesiredMediaType = (pinMediaType->majortype == *majorType);
	DeleteMediaType(pinMediaType);

	if (!bDesiredMediaType)
		return false;

	return true;
}

IPin* GetOutputPin(IBaseFilter *filter, const GUID *majorType)
{
	IPin *foundPin = NULL;
	IEnumPins *pins;

	if (!filter) return NULL;
	if (FAILED(filter->EnumPins(&pins))) return NULL;

	IPin *curPin;
	ULONG num;
	while (pins->Next(1, &curPin, &num) == S_OK)
	{
		if (majorType)
		{
			if (!PinHasMajorType(curPin, majorType))
			{
				SafeRelease(curPin);
				continue;
			}
		}

		//------------------------------

		PIN_DIRECTION pinDir;
		if (SUCCEEDED(curPin->QueryDirection(&pinDir)))
		{
			if (pinDir == PINDIR_OUTPUT)
			{
				IKsPropertySet *propertySet;
				if (SUCCEEDED(curPin->QueryInterface(IID_IKsPropertySet, (void**)&propertySet)))
				{
					GUID pinCategory;
					DWORD retSize;

					PIN_INFO chi;
					curPin->QueryPinInfo(&chi);

					if (chi.pFilter)
						chi.pFilter->Release();

					if (SUCCEEDED(propertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &retSize)))
					{
						if (pinCategory == PIN_CATEGORY_CAPTURE || pinCategory == PIN_CATEGORY_ROXIOCAPTURE)
						{
							SafeRelease(propertySet);
							SafeRelease(pins);

							return curPin;
						}
					}

					SafeRelease(propertySet);
				}
			}
		}

		SafeRelease(curPin);
	}

	SafeRelease(pins);

	return foundPin;
}


#define DEV_EXCEPTION_COUNT 1
CTSTR lpExceptionNames[DEV_EXCEPTION_COUNT] = { TEXT("Elgato Game Capture HD") };
CTSTR lpExceptionGUIDs[DEV_EXCEPTION_COUNT] = { TEXT("{39F50F4C-99E1-464a-B6F9-D605B4FB5918}") };

bool FillOutListOfDevices(HWND hwndCombo, GUID matchGUID, StringList *deviceList, StringList *deviceIDList)
{
	deviceIDList->Clear();
	deviceList->Clear();
	//if (hwndCombo != NULL) SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

	//------------------------------------------

	for (int i = 0; i<DEV_EXCEPTION_COUNT; i++)
	{
		IBaseFilter *exceptionFilter = GetExceptionDevice(lpExceptionGUIDs[i]);
		if (exceptionFilter)
		{
			deviceList->Add(lpExceptionNames[i]);
			deviceIDList->Add(lpExceptionGUIDs[i]);

			if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)lpExceptionNames[i]);

			exceptionFilter->Release();
		}
	}

	//------------------------------------------

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("FillOutListDevices: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return false;
	}

	err = deviceEnum->CreateClassEnumerator(matchGUID, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("FillOutListDevices: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return false;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices
		return false;

	//------------------------------------------

	IMoniker *deviceInfo;
	DWORD count;

// 	bool bUseConfig = true;
// 
// 	String strConfigPath = BLiveGetPluginDataPath() + "\\dshowDevices.xconfig";
// 
// 	XConfig dshowDevicesConfig;
// 	XElement *devices = NULL;
// 
// 	if (!dshowDevicesConfig.Open(strConfigPath.Array()))
// 		bUseConfig = false;
// 
// 	if (bUseConfig) {
// 		devices = dshowDevicesConfig.GetElement(TEXT("dshowDevices"));
// 		if (!devices)
// 			devices = dshowDevicesConfig.CreateElement(TEXT("dshowDevices"));
// 	}

	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT friendlyNameValue, devicePathValue;
			friendlyNameValue.vt = VT_BSTR;
			friendlyNameValue.bstrVal = NULL;
			devicePathValue.vt = VT_BSTR;
			devicePathValue.bstrVal = NULL;

			err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
			propertyData->Read(L"DevicePath", &devicePathValue, NULL);

			if (SUCCEEDED(err))
			{
				if (sstri(friendlyNameValue.bstrVal, L"Blackmagic") || sstri(friendlyNameValue.bstrVal, L"Decklink"))
				{
					if (propertyData)
						propertyData->Release();

					if (deviceInfo)
						deviceInfo->Release();

					continue;
				}

				IBaseFilter *filter;
				err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
				if (SUCCEEDED(err))
				{
					String strDeviceName = (CWSTR)friendlyNameValue.bstrVal;
					deviceList->Add(strDeviceName);

					UINT count2 = 0;
					UINT id = INVALID;
					while ((id = deviceList->FindNextValueIndexI(strDeviceName, id)) != INVALID) count2++;

					if (count2 > 1)
						strDeviceName << TEXT(" (") << UIntString(count2) << TEXT(")");

					String strDeviceID = (CWSTR)devicePathValue.bstrVal;
// 					if (bUseConfig) {
// 						XElement *chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
// 						if (!chkDevice) {
// 							if (strDeviceID.Length() != 0) {
// 								devices->CreateElement((CWSTR)devicePathValue.bstrVal);
// 								chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
// 								chkDevice->AddString(TEXT("deviceName"), strDeviceName.Array());
// 							}
// 							if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
// 						}
// 						else {
// 							if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)chkDevice->GetString(TEXT("deviceName")));
// 						}
// 					}
// 					else 
					{
						if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
					}
					deviceIDList->Add(strDeviceID);

					SafeRelease(filter);
				}
			}

			SafeRelease(propertyData);
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return true;
}


bool  CheckDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName)
{
	//---------------------------------

	bool  bReturn = false;

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return bReturn;
	}

	err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return bReturn;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices, so NO ENUM FO U
		return bReturn;

	IMoniker *deviceInfo;
	DWORD count;

	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT valueThingy;
			VARIANT valueThingy2;
			VariantInit(&valueThingy);
			VariantInit(&valueThingy2);
			/*valueThingy.vt  = VT_BSTR;
			valueThingy.pbstrVal = NULL;

			valueThingy2.vt = VT_BSTR;
			valueThingy2.bstrVal = NULL;*/

			if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
			{
				SafeRelease(propertyData);

				String strVal1 = (CWSTR)valueThingy.bstrVal;

				if (strVal1 == lpName)
				{
					bReturn = true;
					break;
				}
			}
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return bReturn;
}

//===================================================================

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
									   DrawFrame(hwnd, GetListTitle()[i].Type, true);
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
							  AudioOKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
							  AudioCancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);

							  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);


                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);

				//=======================================================
                ConfigDialogData *configData = (ConfigDialogData*)lParam;
                HWND hwndAudioList      = GetDlgItem(hwnd, IDC_AUDIOLIST);

				UINT deviceID = 0;
				CTSTR pDisabled = L"禁用";
				SendMessage(hwndAudioList, CB_INSERTSTRING, 0, (LPARAM)pDisabled);

				String strAudioDevice = L"";
				if (!configData->data["audioDeviceName"].isNull())
				{
					strAudioDevice = Asic2WChar(configData->data["audioDeviceName"].asString().c_str()).c_str();
				}

				FillOutListOfDevices(hwndAudioList, CLSID_AudioInputDeviceCategory, &configData->audioNameList, &configData->audioIDList);
				
				for (int i = 0; i < configData->audioNameList.Num(); ++i)
				{
					if (configData->audioNameList[i] == strAudioDevice)
					{
						deviceID = i + 1;
						break;
					}
				}

				SendMessage(hwndAudioList, CB_SETCURSEL, deviceID, 0);

				bool enable = deviceID != 0;
				EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET), enable);
				EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT), enable);
				EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), enable);

                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIOLIST, CBN_SELCHANGE), (LPARAM)hwndAudioList);

				//==============================================
				float fVol = 1.0f; 

				if (!configData->data["volume"].isNull())
					fVol = configData->data["volume"].asDouble();

                SetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME), fVol);
                //------------------------------------------

                int pos = configData->data["soundTimeOffset"].asInt();

                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETRANGE32, -3000, 20000);
                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETPOS32, 0, pos);

				LocalizeWindow(hwnd, pluginLocale);

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

                return TRUE;
            }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_VOLUME:
                    if(HIWORD(wParam) == VOLN_ADJUSTING || HIWORD(wParam) == VOLN_FINALVALUE)
                    {
                        if(IsWindowEnabled((HWND)lParam))
                        {
                            float fVol = GetVolumeControlValue((HWND)lParam);

                            ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;

							if (Video)
								Video->SetFloat(TEXT("volume"), fVol);
                        }
                    }
                    break;

                case IDC_TIMEOFFSET_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(configData)
                        {
							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;

							if (Video)
                            {
                                HWND hwndVal = NULL;
                                switch(LOWORD(wParam))
                                {
                                    case IDC_TIMEOFFSET_EDIT:       hwndVal = GetDlgItem(hwnd, IDC_TIMEOFFSET); break;
                                }

                                int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
                                switch(LOWORD(wParam))
                                {
								case IDC_TIMEOFFSET_EDIT:       Video->SetInt(TEXT("timeOffset"), val); break;
                                }
                            }
                        }
                    }
                    break;

                case IDC_AUDIOLIST:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hwndDevices = (HWND)lParam;
                        UINT id = (UINT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);

                        if (id == CB_ERR || id == 0) {
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET),       FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT),  FALSE);
							EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), FALSE);
                        }
						else 
						{
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET),       TRUE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT),  TRUE);
							EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), TRUE);
                        }
                    }
                    break;

				case IDOK:
				{
							 ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

							 UINT audioDeviceID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_AUDIOLIST), CB_GETCURSEL, 0, 0);

							 if (audioDeviceID) {
								 String strAudioDevice = configData->audioNameList[audioDeviceID - 1];
								 String strAudioID = configData->audioIDList[audioDeviceID - 1];
								 configData->data["audioDeviceName"] =  WcharToAnsi(strAudioDevice.Array()).c_str();
								 if (!strAudioID.IsEmpty())
									configData->data["audioDeviceId"] =  WcharToAnsi(strAudioID.Array()).c_str();
							 }
							 else
							 {
								 configData->data["audioDeviceName"] =  "";
								 configData->data["audioDeviceID"] =  "";
							 }

							 int soundTimeOffset = (int)SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_GETPOS32, 0, 0);
							 configData->data["soundTimeOffset"] =  soundTimeOffset;

							 float fVol = GetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME));
							 configData->data["volume"] = fVol;

							 EndDialog(hwnd, LOWORD(wParam));
							 break;
				}
                case IDCANCEL:
                    if(LOWORD(wParam) == IDCANCEL)
                    {
                       ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
					   std::stringstream SourceId;
					   uint64_t VideoId = 0;
					   if (!configData->data["SourceID"].isNull())
					   {
						   SourceId << configData->data["SourceID"].asString().c_str();
						   SourceId >> VideoId;
					   }

					   IBaseVideo *Video = (IBaseVideo*)VideoId;

					   if (Video)
                        {
						   Video->SetInt(TEXT("timeOffset"), configData->data["soundTimeOffset"].asInt());

						   if (!configData->data["volume"].isNull())
							   Video->SetFloat(TEXT("volume"), configData->data["volume"].asDouble());
						   else
						   {
							   Video->SetFloat(TEXT("volume"), 1.0f);
						   }
						 }
                    }

                    EndDialog(hwnd, LOWORD(wParam));
					break;
            }
    }

    return FALSE;
}

bool bFirst = true;
bool STDCALL ConfigureDShowSource(Value &element, bool bCreating)
{
	if (bFirst)
	{
		InitColorControl(hinstMain);
		InitVolumeControl(hinstMain);
		InitVolumeMeter(hinstMain);

		pluginLocale = new LocaleStringLookup;

		if (!pluginLocale->LoadStringFile(AUDIOLOCALPATH))
			Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AUDIOLOCALPATH).c_str());

		bFirst = false;
	}

    ConfigDialogData configData(element);
    configData.bGlobalSource = true;
    configData.bCreating = bCreating;

    if(BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG), GetMainWindow(), ConfigureDialogProc, (LPARAM)&configData) == IDOK)
    {
        return true;
    }

    return false;
}

REGINST_CONFIGFUNC(DSource, ConfigureDShowSource)

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#if defined _M_X64 && _MSC_VER == 1800
        //workaround AVX2 bug in VS2013, http://connect.microsoft.com/VisualStudio/feedback/details/811093
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
