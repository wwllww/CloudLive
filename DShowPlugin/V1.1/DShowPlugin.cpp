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


#include "DShowPlugin.h"
#include <sstream>
#include <ShlObj.h>

//todo: super long file.  this is another one of those abominations.
//fix it jim

LocaleStringLookup *pluginLocale = NULL;
#define PluginStr(param)  pluginLocale->LookupString(TEXT2(param))

HINSTANCE hinstMain = NULL;
bool NoExist = false;

#define DSHOW_CLASSNAME TEXT("DeviceCapture")


//CTSTR lpRoxioVideoCaptureGUID = TEXT("{6994AD05-93EF-11D0-A3-CC-00-A0-C9-22-31-96}");
const GUID PIN_CATEGORY_ROXIOCAPTURE = {0x6994AD05, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

HBRUSH HBrush;
WNDPROC buttonproc = NULL;

static INT_PTR CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_153 = CreateSolidBrush(RGB(153, 153, 153));
					 HBRUSH hBK_57 = CreateSolidBrush(RGB(57, 57, 59));
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(102, 102, 102));
					 HFONT  m_hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(176, 176, 176));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);
					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);
					 String DB_Tianjia = WndTitle;
					 TextOut(hDC, 23, 0, DB_Tianjia, DB_Tianjia.Length());
					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 DeleteObject(hBK_153);
					 DeleteObject(hBK_102);
					 DeleteObject(hBK_57);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(buttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

bool SourceListHasDevice(CTSTR lpDevice, XElement *sourceList, bool& hide)
{
    UINT numSources = sourceList->NumElements();
    for(UINT i=0; i<numSources; i++)
    {
        XElement *sourceElement = sourceList->GetElementByID(i);
        if(scmpi(sourceElement->GetString(TEXT("class")), DSHOW_CLASSNAME) == 0)
        {
            XElement *data = sourceElement->GetElement(TEXT("data"));
			if (scmpi(data->GetString(TEXT("deviceID")), lpDevice) == 0)
			{
				hide = data->GetInt(TEXT("Hide"), 0);
				return true;
			}   
			if (scmpi(data->GetString(TEXT("audioDeviceID")), lpDevice) == 0)
			{
				return true;
			}
        }
    }

    return false;
}


bool GetGUIDFromString(CTSTR lpGUID, GUID &targetGUID)
{
    String strGUID = lpGUID;
    if(strGUID.Length() != 38)
        return false;

    strGUID = strGUID.Mid(1, strGUID.Length()-1);

    StringList GUIDData;
    strGUID.GetTokenList(GUIDData, '-', FALSE);

    if (GUIDData.Num() != 5)
        return false;

    if (GUIDData[0].Length() != 8  ||
        GUIDData[1].Length() != 4  ||
        GUIDData[2].Length() != 4  ||
        GUIDData[3].Length() != 4  ||
        GUIDData[4].Length() != 12 )
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
    if(SUCCEEDED(CoCreateInstance(targetGUID, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filter)))
        return filter;

    return NULL;
}

IBaseFilter* GetDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName, WSTR lpType2, CTSTR lpName2)
{
    //---------------------------------
    // exception devices
    if(scmpi(lpType2, L"DevicePath") == 0 && lpName2 && *lpName2 == '{')
        return GetExceptionDevice(lpName2);

    //---------------------------------

    ICreateDevEnum *deviceEnum;
    IEnumMoniker *videoDeviceEnum;

    HRESULT err;
    err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
    if(FAILED(err))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX", err);
        return NULL;
    }

    err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
    if(FAILED(err))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX", err);
        deviceEnum->Release();
        return NULL;
    }

    SafeRelease(deviceEnum);

    if(err == S_FALSE) //no devices, so NO ENUM FO U
        return NULL;

    //---------------------------------

    IBaseFilter *bestFilter = NULL;

    IMoniker *deviceInfo;
    DWORD count;
    while(videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
    {
        IPropertyBag *propertyData;
        err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
        if(SUCCEEDED(err))
        {
            VARIANT valueThingy;
            VARIANT valueThingy2;
            VariantInit(&valueThingy);
            VariantInit(&valueThingy2);
            /*valueThingy.vt  = VT_BSTR;
            valueThingy.pbstrVal = NULL;

            valueThingy2.vt = VT_BSTR;
            valueThingy2.bstrVal = NULL;*/

            if(SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
            {
                if(lpType2 && lpName2)
                {
                    if(FAILED(propertyData->Read(lpType2, &valueThingy2, NULL)))
                        nop();
                }

                SafeRelease(propertyData);

                String strVal1 = (CWSTR)valueThingy.bstrVal;

                if(strVal1 == lpName)
                {
                    IBaseFilter *filter;
                    err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
                    if(FAILED(err))
                    {
						Log::writeMessage(LOG_RTSPSERV, 1, "GetDeviceByValue: deviceInfo->BindToObject failed, result = % 08lX", err);
                        continue;
                    }

                    if(!bestFilter)
                    {
                        bestFilter = filter;

                        if(!lpType2 || !lpName2)
                        {
                            SafeRelease(deviceInfo);
                            SafeRelease(videoDeviceEnum);

                            return bestFilter;
                        }
                    }
                    else if(lpType2 && lpName2)
                    {
                        String strVal2 = (CWSTR)valueThingy2.bstrVal;
                        if(strVal2 == lpName2)
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
		Log::writeMessage(LOG_RTSPSERV, 1, "GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX", err);
		return bReturn;
	}

	err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX", err);
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


bool PinHasMajorType(IPin *pin, const GUID *majorType)
{
    HRESULT hRes;

    IAMStreamConfig *config;
    if(SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
    {
        int count, size;
        if(SUCCEEDED(config->GetNumberOfCapabilities(&count, &size)))
        {
			BYTE *capsData = (BYTE*)Allocate_Bak(size);

            int priority = -1;
            for(int i=0; i<count; i++)
            {
                AM_MEDIA_TYPE *pMT;
                if(SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
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
    if(FAILED(pin->EnumMediaTypes(&mediaTypesEnum)))
        return false;

    ULONG curVal = 0;
    hRes = mediaTypesEnum->Next(1, &pinMediaType, &curVal);

    mediaTypesEnum->Release();

    if(hRes != S_OK)
        return false;

    BOOL bDesiredMediaType = (pinMediaType->majortype == *majorType);
    DeleteMediaType(pinMediaType);

    if(!bDesiredMediaType)
        return false;

    return true;
}

IPin* GetOutputPin(IBaseFilter *filter, const GUID *majorType)
{
    IPin *foundPin = NULL;
    IEnumPins *pins;

    if(!filter) return NULL;
    if(FAILED(filter->EnumPins(&pins))) return NULL;

    IPin *curPin;
    ULONG num;
    while(pins->Next(1, &curPin, &num) == S_OK)
    {
        if(majorType)
        {
            if (!PinHasMajorType(curPin, majorType))
            {
                SafeRelease(curPin);
                continue;
            }
        }

        //------------------------------

        PIN_DIRECTION pinDir;
        if(SUCCEEDED(curPin->QueryDirection(&pinDir)))
        {
            if(pinDir == PINDIR_OUTPUT)
            {
                IKsPropertySet *propertySet;
                if(SUCCEEDED(curPin->QueryInterface(IID_IKsPropertySet, (void**)&propertySet)))
                {
                    GUID pinCategory;
                    DWORD retSize;

                    PIN_INFO chi;
                    curPin->QueryPinInfo(&chi);

                    if(chi.pFilter)
                        chi.pFilter->Release();

                    if(SUCCEEDED(propertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &retSize)))
                    {
                        if(pinCategory == PIN_CATEGORY_CAPTURE || pinCategory == PIN_CATEGORY_ROXIOCAPTURE)
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

static void AddElgatoRes(AM_MEDIA_TYPE *pMT, int cx, int cy, VideoOutputType type, List<MediaOutputInfo> &outputInfoList)
{
    MediaOutputInfo *outputInfo = outputInfoList.CreateNew();
    BITMAPINFOHEADER *bmiHeader = GetVideoBMIHeader(pMT);

    outputInfo->minCX = outputInfo->maxCX = cx;
    outputInfo->minCY = outputInfo->maxCY = cy;
    outputInfo->minFrameInterval = outputInfo->maxFrameInterval = 10010000000LL/60000LL;

    outputInfo->xGranularity = outputInfo->yGranularity = 1;

    outputInfo->mediaType = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    memset(outputInfo->mediaType, 0, sizeof(AM_MEDIA_TYPE));
    CopyMediaType(outputInfo->mediaType, pMT);

    outputInfo->videoType = type;
    outputInfo->bUsingFourCC = false;
}

void AddOutput(AM_MEDIA_TYPE *pMT, BYTE *capsData, bool bAllowV2, List<MediaOutputInfo> &outputInfoList)
{
    VideoOutputType type = GetVideoOutputType(*pMT);

    if(pMT->formattype == FORMAT_VideoInfo || (bAllowV2 && pMT->formattype == FORMAT_VideoInfo2))
    {
        VIDEO_STREAM_CONFIG_CAPS *pVSCC = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(capsData);
        VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat);
        BITMAPINFOHEADER *bmiHeader = GetVideoBMIHeader(pMT);

        bool bUsingFourCC = false;
        if(type == VideoOutputType_None)
        {
            type = GetVideoOutputTypeFromFourCC(bmiHeader->biCompression);
            bUsingFourCC = true;
        }

        if(type != VideoOutputType_None)
        {
            if (!pVSCC && bAllowV2)
            {
                AddElgatoRes(pMT, 480,  360,  type, outputInfoList);
                AddElgatoRes(pMT, 640,  480,  type, outputInfoList);
                AddElgatoRes(pMT, 1280, 720,  type, outputInfoList);
                AddElgatoRes(pMT, 1920, 1080, type, outputInfoList);
                DeleteMediaType(pMT);
                return;
            }

            MediaOutputInfo *outputInfo = outputInfoList.CreateNew();

            if(pVSCC)
            {
                outputInfo->minFrameInterval = pVSCC->MinFrameInterval;
                outputInfo->maxFrameInterval = pVSCC->MaxFrameInterval;
                outputInfo->minCX = pVSCC->MinOutputSize.cx;
                outputInfo->maxCX = pVSCC->MaxOutputSize.cx;
                outputInfo->minCY = pVSCC->MinOutputSize.cy;
                outputInfo->maxCY = pVSCC->MaxOutputSize.cy;

				Log(TEXT("org size: minCX %d, maxCX %d, minCY %d, maxCY %d"), outputInfo->minCX, outputInfo->maxCX, outputInfo->minCY, outputInfo->maxCY);

                if (!outputInfo->minCX || !outputInfo->minCY || !outputInfo->maxCX || !outputInfo->maxCY) {
                    outputInfo->minCX = outputInfo->maxCX = bmiHeader->biWidth;
                    outputInfo->minCY = outputInfo->maxCY = bmiHeader->biHeight;
                }

                //actually due to the other code in GetResolutionFPSInfo, we can have this granularity
                // back to the way it was.  now, even if it's corrupted, it will always work
                outputInfo->xGranularity = max(pVSCC->OutputGranularityX, 1);
                outputInfo->yGranularity = max(pVSCC->OutputGranularityY, 1);
            }
            else
            {
                outputInfo->minCX = outputInfo->maxCX = bmiHeader->biWidth;
                outputInfo->minCY = outputInfo->maxCY = bmiHeader->biHeight;
                if(pVih->AvgTimePerFrame != 0)
                    outputInfo->minFrameInterval = outputInfo->maxFrameInterval = pVih->AvgTimePerFrame;
                else
                    outputInfo->minFrameInterval = outputInfo->maxFrameInterval = 10000000/30; //elgato hack

                outputInfo->xGranularity = outputInfo->yGranularity = 1;
            }

            outputInfo->mediaType = pMT;
            outputInfo->videoType = type;
            outputInfo->bUsingFourCC = bUsingFourCC;

            return;
        }
    }

    DeleteMediaType(pMT);
}

void GetOutputList(IPin *curPin, List<MediaOutputInfo> &outputInfoList)
{
    HRESULT hRes;

    IAMStreamConfig *config;
    if(SUCCEEDED(curPin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
    {
        int count, size;
        if(SUCCEEDED(hRes = config->GetNumberOfCapabilities(&count, &size)))
        {
            BYTE *capsData = (BYTE*)Allocate_Bak(size);

            int priority = -1;
            for(int i=0; i<count; i++)
            {
                AM_MEDIA_TYPE *pMT;
                if(SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
                    AddOutput(pMT, capsData, false, outputInfoList);
            }

            Free(capsData);
        }
        else if(hRes == E_NOTIMPL) //...usually elgato.
        {
            IEnumMediaTypes *mediaTypes;
            if(SUCCEEDED(curPin->EnumMediaTypes(&mediaTypes)))
            {
                ULONG i;

                AM_MEDIA_TYPE *pMT;
                if(mediaTypes->Next(1, &pMT, &i) == S_OK)
                    AddOutput(pMT, NULL, true, outputInfoList);

                mediaTypes->Release();
            }
        }

        SafeRelease(config);
    }
}


inline bool ResolutionListHasValue(const List<SIZE> &resolutions, SIZE &size)
{
    bool bHasResolution = false;

    for(UINT i=0; i<resolutions.Num(); i++)
    {
        SIZE &testSize = resolutions[i];
        if(size.cx == testSize.cx && size.cy == testSize.cy)
        {
            bHasResolution = true;
            break;
        }
    }

    return bHasResolution;
}


struct FPSInterval
{
    inline FPSInterval(UINT64 minVal, UINT64 maxVal) : minFrameInterval(minVal), maxFrameInterval(maxVal) {}
    UINT64 minFrameInterval, maxFrameInterval;
};

struct FPSInfo
{
    List<FPSInterval> supportedIntervals;
};

static inline UINT64 GetFrameIntervalDist(UINT64 minInterval, UINT64 maxInterval, UINT64 desiredInterval)
{
    INT64 minDist = INT64(minInterval) - INT64(desiredInterval);
    INT64 maxDist = INT64(desiredInterval) - INT64(maxInterval);

    if (minDist < 0) minDist = 0;
    if (maxDist < 0) maxDist = 0;

    return UINT64(MAX(minDist, maxDist));
}

bool GetClosestResolutionFPS(List<MediaOutputInfo> &outputList, SIZE &resolution, UINT64 &frameInterval, bool bPrioritizeFPS)
{
    LONG width, height;
    UINT64 internalFrameInterval = 10000000/UINT64(GetMaxFPS());
    GetBaseSize((UINT&)width, (UINT&)height);

    LONG bestDistance = 0x7FFFFFFF;
    SIZE bestSize;
    UINT64 maxFrameInterval = 0;
    UINT64 minFrameInterval = 0;
    UINT64 bestFrameIntervalDist = 0xFFFFFFFFFFFFFFFFLL;

    for(UINT i = 0; i < outputList.Num(); i++)
    {
        MediaOutputInfo &outputInfo = outputList[i];

        LONG outputWidth  = outputInfo.minCX;
        do
        {
            LONG distWidth = width - outputWidth;
            if(distWidth < 0)
                break;

            if(distWidth > bestDistance)
            {
                outputWidth += outputInfo.xGranularity;
                continue;
            }

            LONG outputHeight = outputInfo.minCY;
            do
            {
                LONG distHeight = height - outputHeight;
                if(distHeight < 0)
                    break;

                LONG totalDist = distHeight + distWidth;

                UINT64 frameIntervalDist = GetFrameIntervalDist(outputInfo.minFrameInterval,
                    outputInfo.maxFrameInterval, internalFrameInterval);

                bool bBetter;
                if (bPrioritizeFPS)
                    bBetter = (frameIntervalDist != bestFrameIntervalDist) ?
                        (frameIntervalDist < bestFrameIntervalDist) :
                        (totalDist < bestDistance);
                else
                    bBetter = (totalDist != bestDistance) ?
                        (totalDist < bestDistance) :
                        (frameIntervalDist < bestFrameIntervalDist);

                if (bBetter) {
                    bestDistance = totalDist;
                    bestSize.cx = outputWidth;
                    bestSize.cy = outputHeight;
                    maxFrameInterval = outputInfo.maxFrameInterval;
                    minFrameInterval = outputInfo.minFrameInterval;
                    bestFrameIntervalDist = frameIntervalDist;
                }

                outputHeight += outputInfo.yGranularity;
            }while((UINT)outputHeight <= outputInfo.maxCY);

            outputWidth  += outputInfo.xGranularity;
        }while((UINT)outputWidth <= outputInfo.maxCX);
    }

    if(bestDistance != 0x7FFFFFFF)
    {
        resolution.cx = bestSize.cx;
        resolution.cy = bestSize.cy;

        if(internalFrameInterval > maxFrameInterval)
            frameInterval = maxFrameInterval;
        else if(internalFrameInterval < minFrameInterval)
            frameInterval = minFrameInterval;
        else
            frameInterval = internalFrameInterval;
        return true;
    }

    return false;
}

struct ConfigDialogData
{
    CTSTR lpName;
	Json::Value& data;
    List<MediaOutputInfo> outputList;
    List<SIZE> resolutions;
    StringList deviceNameList;
    StringList deviceIDList;
    StringList audioNameList;
    StringList audioIDList;
	StringList audioRenderNameList;
	StringList audioRenderIDList;
	bool bGlobalSource;
    bool bCreating;
    bool bDeviceHasAudio;

	ConfigDialogData(Json::Value& data) :data(data)
	{
	}
    ~ConfigDialogData()
    {
        ClearOutputList();
    }

    void ClearOutputList()
    {
        for(UINT i=0; i<outputList.Num(); i++)
            outputList[i].FreeData();
        outputList.Clear();
    }

    void GetResolutions(List<SIZE> &resolutions)
    {
        resolutions.Clear();

		SIZE size0 = { 640, 480 };
		resolutions << size0;
		SIZE size1 = { 704, 576 };
		resolutions << size1;
		SIZE size2 = { 720, 480 };
		resolutions << size2;
		SIZE size3 = { 720, 576 };
		resolutions << size3;
		SIZE size4 = { 800, 600 };
		resolutions << size4;
		SIZE size5 = { 1024, 768 };
		resolutions << size5;
		SIZE size6 = { 1280, 720 };
		resolutions << size6;
		SIZE size7 = { 1280, 960 };
		resolutions << size7;
		SIZE size8 = { 1920, 1080 };
		resolutions << size8;

        for(UINT i = 0; i < outputList.Num(); i++)
        {
            MediaOutputInfo &outputInfo = outputList[i];
            SIZE size;

            size.cx = outputInfo.minCX;
            size.cy = outputInfo.minCY;
            if(!ResolutionListHasValue(resolutions, size))
                resolutions << size;

            size.cx = outputInfo.maxCX;
            size.cy = outputInfo.maxCY;
            if(!ResolutionListHasValue(resolutions, size))
                resolutions << size;

			Log(TEXT("minCX %d, maxCX %d, minCY %d, maxCY %d"), outputInfo.minCX, outputInfo.maxCX, outputInfo.minCY, outputInfo.maxCY);
        }

        //sort
        for(UINT i = 0; i < resolutions.Num(); i++)
        {
            SIZE &rez = resolutions[i];

            for(UINT j = i + 1; j < resolutions.Num(); j++)
            {
                SIZE &testRez = resolutions[j];

                if(testRez.cy < rez.cy)
                {
                    resolutions.SwapValues(i, j);
                    j = i;
                }
            }
        }
    }

    bool GetResolutionFPSInfo(SIZE &resolution, FPSInfo &fpsInfo)
    {
        fpsInfo.supportedIntervals.Clear();

        for(UINT i=0; i<outputList.Num(); i++)
        {
            MediaOutputInfo &outputInfo = outputList[i];

            if( UINT(resolution.cx) >= outputInfo.minCX && UINT(resolution.cx) <= outputInfo.maxCX &&
                UINT(resolution.cy) >= outputInfo.minCY && UINT(resolution.cy) <= outputInfo.maxCY )
            {
                if((resolution.cx-outputInfo.minCX) % outputInfo.xGranularity || (resolution.cy-outputInfo.minCY) % outputInfo.yGranularity)
                    return false;

                fpsInfo.supportedIntervals << FPSInterval(outputInfo.minFrameInterval, outputInfo.maxFrameInterval);
            }
        }

        return fpsInfo.supportedIntervals.Num() != 0;
    }
};


#define DEV_EXCEPTION_COUNT 1
CTSTR lpExceptionNames[DEV_EXCEPTION_COUNT] = {TEXT("Elgato Game Capture HD")};
CTSTR lpExceptionGUIDs[DEV_EXCEPTION_COUNT] = {TEXT("{39F50F4C-99E1-464a-B6F9-D605B4FB5918}")};

bool FillOutListOfDevices(HWND hwndCombo, GUID matchGUID, StringList *deviceList, StringList *deviceIDList)
{
    deviceIDList->Clear();
    deviceList->Clear();
    if(hwndCombo != NULL) SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

    //------------------------------------------

    for(int i = 0; i < DEV_EXCEPTION_COUNT; i++)
    {
        IBaseFilter *exceptionFilter = GetExceptionDevice(lpExceptionGUIDs[i]);
        if(exceptionFilter)
        {
            deviceList->Add(lpExceptionNames[i]);
            deviceIDList->Add(lpExceptionGUIDs[i]);

            if(hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)lpExceptionNames[i]);

            exceptionFilter->Release();
        }
    }

    //------------------------------------------

    ICreateDevEnum *deviceEnum;
    IEnumMoniker *videoDeviceEnum;

    HRESULT err;
    err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
    if(FAILED(err))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "FillOutListDevices: CoCreateInstance for the device enum failed, result = %08lX", err);
        return false;
    }

    err = deviceEnum->CreateClassEnumerator(matchGUID, &videoDeviceEnum, 0);
    if(FAILED(err))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "FillOutListDevices: deviceEnum->CreateClassEnumerator failed, result = %08lX", err);
        deviceEnum->Release();
        return false;
    }

    SafeRelease(deviceEnum);

    if(err == S_FALSE) //no devices
        return false;

    //------------------------------------------

    IMoniker *deviceInfo;
    DWORD count;

    bool bUseConfig = true;

//     String strConfigPath = BLiveGetPluginDataPath() + "\\dshowDevices.xconfig";
// 
//     XConfig dshowDevicesConfig;
//     XElement *devices = NULL;
// 
//     if(!dshowDevicesConfig.Open(strConfigPath.Array()))
//         bUseConfig = false;
//     
//     if(bUseConfig) {
//         devices = dshowDevicesConfig.GetElement(TEXT("dshowDevices"));
//         if(!devices)
//             devices = dshowDevicesConfig.CreateElement(TEXT("dshowDevices"));
//     }

    while(videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
    {
        IPropertyBag *propertyData;
        err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
        if(SUCCEEDED(err))
        {
            VARIANT friendlyNameValue, devicePathValue;
            friendlyNameValue.vt = VT_BSTR;
            friendlyNameValue.bstrVal = NULL;
            devicePathValue.vt = VT_BSTR;
            devicePathValue.bstrVal = NULL;

            err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
            propertyData->Read(L"DevicePath", &devicePathValue, NULL);

            if(SUCCEEDED(err))
            {
                IBaseFilter *filter;
                err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
                if(SUCCEEDED(err))
                {
                    String strDeviceName = (CWSTR)friendlyNameValue.bstrVal;
                    deviceList->Add(strDeviceName);

                    UINT count2 = 0;
                    UINT id = INVALID;
                    while((id = deviceList->FindNextValueIndexI(strDeviceName, id)) != INVALID) count2++;

                    if(count2 > 1)
                        strDeviceName << TEXT(" (") << UIntString(count2) << TEXT(")");

                    String strDeviceID = (CWSTR)devicePathValue.bstrVal;
//                     if(bUseConfig) {
//                         XElement *chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
//                         if(!chkDevice) {
//                             if(strDeviceID.Length() != 0) {
//                                 devices->CreateElement((CWSTR)devicePathValue.bstrVal);
//                                 chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
//                                 chkDevice->AddString(TEXT("deviceName"), strDeviceName.Array());
//                             }
//                             if(hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
//                         }
//                         else {
//                             if(hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)chkDevice->GetString(TEXT("deviceName")));
//                         }
//                     }
//                     else 
					{
                        if(hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
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
    //dshowDevicesConfig.Close(true);

    return true;
}


bool GetResolution(HWND hwndResolution, SIZE &resolution, BOOL bSelChange)
{
    String strResolution;
    if(bSelChange)
        strResolution = GetCBText(hwndResolution);
    else
        strResolution = GetEditText(hwndResolution);

    if(strResolution.NumTokens('x') != 2)
        return false;

    String strCX = strResolution.GetToken(0, 'x');
    String strCY = strResolution.GetToken(1, 'x');

    if(strCX.IsEmpty() || strCY.IsEmpty() || !ValidIntString(strCX) || !ValidIntString(strCY))
        return false;

    UINT cx = strCX.ToInt();
    UINT cy = strCY.ToInt();

    if(cx < 32 || cy < 32 || cx > 8192 || cy > 8192)
        return false;

    resolution.cx = cx;
    resolution.cy = cy;

    return true;
}

struct ColorSelectionData
{
    HDC hdcDesktop;
    HDC hdcDestination;
    HBITMAP hBitmap;
    bool bValid;

    inline ColorSelectionData() : hdcDesktop(NULL), hdcDestination(NULL), hBitmap(NULL), bValid(false) {}
    inline ~ColorSelectionData() {Clear();}

    inline bool Init()
    {
        hdcDesktop = GetDC(NULL);
        if(!hdcDesktop)
            return false;

        hdcDestination = CreateCompatibleDC(hdcDesktop);
        if(!hdcDestination)
            return false;

        hBitmap = CreateCompatibleBitmap(hdcDesktop, 1, 1);
        if(!hBitmap)
            return false;

        SelectObject(hdcDestination, hBitmap);
        bValid = true;

        return true;
    }

    inline void Clear()
    {
        if(hdcDesktop)
        {
            ReleaseDC(NULL, hdcDesktop);
            hdcDesktop = NULL;
        }

        if(hdcDestination)
        {
            DeleteDC(hdcDestination);
            hdcDestination = NULL;
        }

        if(hBitmap)
        {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }

        bValid = false;
    }

    inline DWORD GetColor()
    {
        POINT p;
        if(GetCursorPos(&p))
        {
            BITMAPINFO data;
            zero(&data, sizeof(data));

            data.bmiHeader.biSize = sizeof(data.bmiHeader);
            data.bmiHeader.biWidth = 1;
            data.bmiHeader.biHeight = 1;
            data.bmiHeader.biPlanes = 1;
            data.bmiHeader.biBitCount = 24;
            data.bmiHeader.biCompression = BI_RGB;
            data.bmiHeader.biSizeImage = 4;

            if(BitBlt(hdcDestination, 0, 0, 1, 1, hdcDesktop, p.x, p.y, SRCCOPY|CAPTUREBLT))
            {
                DWORD buffer;
                if(GetDIBits(hdcDestination, hBitmap, 0, 1, &buffer, &data, DIB_RGB_COLORS))
                    return 0xFF000000|buffer;
            }
            else
            {
                int err = GetLastError();
                nop();
            }
        }

        return 0xFF000000;
    }
};

static void OpenPropertyPages(HWND hwnd, IUnknown *propObject)
{
    if(!propObject)
        return;

    ISpecifyPropertyPages *propPages;
    CAUUID cauuid;

	if (SUCCEEDED(propObject->QueryInterface(__uuidof(ISpecifyPropertyPages), (void**)&propPages)))
    {
        if(SUCCEEDED(propPages->GetPages(&cauuid)))
        {
            if(cauuid.cElems)
            {
                OleCreatePropertyFrame(hwnd, 0, 0, NULL, 1, (LPUNKNOWN*)&propObject, cauuid.cElems, cauuid.pElems, 0, 0, NULL);
                CoTaskMemFree(cauuid.pElems);
            }
        }

        propPages->Release();
    }
}

static void OpenPropertyPagesByName(HWND hwnd, String devicename, String deviceid, GUID matchGUID,IBaseFilter *BaseFilter = NULL)
{
	if (!BaseFilter)
	{
		IBaseFilter *filter = GetDeviceByValue(matchGUID,
			L"FriendlyName", devicename,
			L"DevicePath", deviceid);

		if (filter)
		{
			OpenPropertyPages(hwnd, filter);
			filter->Release();
		}
	}
	else
	{
		OpenPropertyPages(hwnd, BaseFilter);
	}
}

int SetSliderText(HWND hwndParent, int controlSlider, int controlText)
{
    HWND hwndSlider = GetDlgItem(hwndParent, controlSlider);
    HWND hwndText   = GetDlgItem(hwndParent, controlText);

    int sliderVal = (int)SendMessage(hwndSlider, TBM_GETPOS, 0, 0);
    float floatVal = float(sliderVal)*0.01f;

    SetWindowText(hwndText, FormattedString(TEXT("%.02f"), floatVal));

    return sliderVal;
}

static IAMCrossbar *GetFilterCrossbar(IBaseFilter *filter)
{
    IAMCrossbar *crossbar = NULL;
    IGraphBuilder *graph = NULL;
    ICaptureGraphBuilder2 *capture = NULL;
    IAMStreamConfig *configVideo = NULL;

    if (FAILED(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_IFilterGraph, (void**)&graph)))
        goto crossbar_exit;
    if (FAILED(CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_ICaptureGraphBuilder2, (void**)&capture)))
        goto crossbar_exit;
    if (FAILED(capture->SetFiltergraph(graph)))
        goto crossbar_exit;
    if (FAILED(graph->AddFilter(filter, L"Capture device")))
        goto crossbar_exit;

    if (FAILED(capture->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, filter, IID_IAMStreamConfig, (void**)&configVideo)))
    {
        graph->RemoveFilter(filter);
        goto crossbar_exit;
    }

    capture->FindInterface(NULL, NULL, filter, IID_IAMCrossbar, (void**)&crossbar);

    graph->RemoveFilter(filter);

crossbar_exit:
    SafeRelease(configVideo);
    SafeRelease(capture);
    SafeRelease(graph);

    return crossbar;
}
INT CALLBACK _BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM pData)
{
	TCHAR szDir[MAX_PATH] = { 0 };
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		// WParam is TRUE since you are passing a path.   
		// It would be FALSE if you were passing a pidl.   
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pData);
		break;
	case BFFM_SELCHANGED:
		// Set the status window to the currently selected path.   
		if (SHGetPathFromIDList((LPITEMIDLIST)lParam, szDir))
		{
			SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
		}
		break;
	}
	return 0;
}


INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool bSelectingColor = false;
    static bool bMouseDown = false, bAudioDevicesPresent = true;
    static ColorSelectionData colorData;
    static DeinterlacerConfig deinterlacingConfig;
	static bool _bMouseTrack = true;

    switch(message)
    {
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
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
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_153 = CreateSolidBrush(RGB(153, 153, 153));
					 RECT rect;

					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_153);
					 EndPaint(hwnd, &ps);

					 DeleteObject(hBK_153);

					 break;
	}
	case WM_CTLCOLORSTATIC:
	{
							  HDC hdc = (HDC)wParam;
							  SetTextColor(hdc, RGB(255, 255, 255));
							  SetBkColor(hdc, RGB(153, 153, 153));
							  //HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
							  							  							   							   							   							   		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
							  //HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
							  if (HBrush)
							  {
								  DeleteObject(HBrush);
							  }
							  HBrush = CreateSolidBrush(RGB(153, 153, 153));
							  return (INT_PTR)HBrush;
							  break;
	}
	case WM_CTLCOLORLISTBOX:
	{
							   HDC hdc = (HDC)wParam;
							   SetTextColor(hdc, RGB(255, 255, 255));
							   SetBkColor(hdc, RGB(153, 153, 153));
							   //HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
							   							   							   							   		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
							   //HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
							   if (HBrush)
							   {
								   DeleteObject(HBrush);
							   }
							   HBrush = CreateSolidBrush(RGB(153, 153, 153));
							   return (INT_PTR)HBrush;
							   break;

	}
        case WM_INITDIALOG:
            {
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);

                ConfigDialogData *configData = (ConfigDialogData*)lParam;

                HWND hwndDeviceList     = GetDlgItem(hwnd, IDC_DEVICELIST);
                HWND hwndAudioList      = GetDlgItem(hwnd, IDC_AUDIOLIST);
				HWND hwndAudioRenderList = GetDlgItem(hwnd, IDC_AUDIORENDERLIST);
                HWND hwndResolutionList = GetDlgItem(hwnd, IDC_RESOLUTION);
                HWND hwndFPS            = GetDlgItem(hwnd, IDC_FPS);
                HWND hwndFlip           = GetDlgItem(hwnd, IDC_FLIPIMAGE);
                HWND hwndFlipHorizontal = GetDlgItem(hwnd, IDC_FLIPIMAGEH);

				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_CONFIG), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_CONFIGAUDIO), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_REFRESH), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
				buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_BROSE), GWLP_WNDPROC, (LONG_PTR)ButtonProc);

#ifdef _WIN64
                ShowWindow(GetDlgItem(hwnd, IDC_64BIT_WARNING), SW_SHOW);
				SetWindowText(GetDlgItem(hwnd, IDC_64BIT_WARNING), TEXT(""));
#endif

                //------------------------------------------
				//enble
				//bool allowSelect = configData->data->GetInt(TEXT("AllowSelect"), 1);
				//EnableWindow(GetDlgItem(hwnd, IDC_DEVICELIST), allowSelect);
				//configData->data->SetInt(TEXT("AllowSelect"), 0);

                bool bFlipVertical   = configData->data["flipImage"].asInt() != 0;
                bool bFlipHorizontal = configData->data["flipImageHorizontal"].asInt() != 0;

                SendMessage(hwndFlip, BM_SETCHECK, bFlipVertical ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(hwndFlipHorizontal, BM_SETCHECK, bFlipHorizontal ? BST_CHECKED : BST_UNCHECKED, 0);

                //------------------------------------------
				UINT opacity = 100;
				if (!configData->data["opacity"].isNull())
				{
					opacity = configData->data["opacity"].asUInt();
				}

                SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_SETPOS32, 0, opacity);

                int gammaVal = 100;

				if (!configData->data["gamma"].isNull())
				{
					gammaVal = configData->data["gamma"].asUInt();
				}

                HWND hwndTemp = GetDlgItem(hwnd, IDC_GAMMA);
                SendMessage(hwndTemp, TBM_CLEARTICS, FALSE, 0);
                SendMessage(hwndTemp, TBM_SETRANGE, FALSE, MAKELPARAM(50, 175));
                SendMessage(hwndTemp, TBM_SETTIC, 0, 100);
                SendMessage(hwndTemp, TBM_SETPOS, TRUE, gammaVal);

                SetSliderText(hwnd, IDC_GAMMA, IDC_GAMMAVAL);

                //------------------------------------------
				LocalizeWindow(hwnd, pluginLocale);

				String strDevice; 
				std::string Data = configData->data["deviceName"].asString();
				if (!Data.empty())
				{
					strDevice = Asic2WChar(Data).c_str();
				}
				String strDeviceID;
				Data = configData->data["deviceID"].asString();
				if (!Data.empty())
				{
					strDeviceID = Asic2WChar(Data).c_str();
				}

				String strAudioDevice;
				Data = configData->data["audioDeviceName"].asString();
				if (!Data.empty())
				{
					strAudioDevice = Asic2WChar(Data).c_str();
				}

				String strAudioDeviceID;
				Data = configData->data["audioDeviceID"].asString();
				if (!Data.empty())
				{
					strAudioDeviceID = Asic2WChar(Data).c_str();
				}

				String strAudioRenderDevice;
				Data = configData->data["audioRenderDeviceName"].asString();
				if (!Data.empty())
				{
					strAudioRenderDevice = Asic2WChar(Data).c_str();
				}

				String strAudioRenderDeviceID;
				Data = configData->data["audioRenderDeviceID"].asString();
				if (!Data.empty())
				{
					strAudioRenderDeviceID = Asic2WChar(Data).c_str();
				}

				//扫描方式

				bool bInterace = configData->data["ScanInterlace"].asUInt() == 1;

				if (bInterace)
					SendMessage(GetDlgItem(hwnd, IDC_INTERLACE), BM_SETCHECK, BST_CHECKED, 0);
				else
				{
					SendMessage(GetDlgItem(hwnd, IDC_PROGRESSIVE), BM_SETCHECK, BST_CHECKED, 0);
				}

				UINT cx = configData->data["resolutionWidth"].asUInt();
                UINT cy  = configData->data["resolutionHeight"].asUInt();
                UINT64 frameInterval = configData->data["frameInterval"].asUInt();

                //------------------------------------------

                deinterlacingConfig.type        = configData->data["deinterlacingType"].asInt();
                deinterlacingConfig.fieldOrder  = configData->data["deinterlacingFieldOrder"].asInt();
                deinterlacingConfig.processor   = configData->data["deinterlacingProcessor"].asInt();
                if(deinterlacingConfig.type >= DEINTERLACING_TYPE_LAST)
                    deinterlacingConfig.type = DEINTERLACING_NONE;

                hwndTemp = GetDlgItem(hwnd, IDC_DEINTERLACELIST);

                // Populate deinterlacing type list
                for(size_t i = 0; i < DEINTERLACING_TYPE_LAST; i++)
                {
#ifndef _DEBUG
                    if(i == DEINTERLACING__DEBUG)
                        continue;
#endif
					SendMessage(hwndTemp, CB_ADDSTRING, 0, (LPARAM)pluginLocale->LookupString(deinterlacerLocalizations[i]));

                    DeinterlacerConfig& config = deinterlacerConfigs[i];
                    if(deinterlacingConfig.type != config.type)
                        continue;
                    
                    const int deintProcBoth = (DEINTERLACING_PROCESSOR_CPU | DEINTERLACING_PROCESSOR_GPU);
                    
                    HWND checkbox = GetDlgItem(hwnd, IDC_GPUDEINT);
                    EnableWindow(checkbox, config.processor == deintProcBoth);
                    if(config.processor != deintProcBoth && deinterlacingConfig.processor != config.processor)
                    {
                        configData->data["deinterlacingGPU"] =  config.processor;
                        deinterlacingConfig.processor = config.processor;
                    }
                    Button_SetCheck(checkbox, deinterlacingConfig.processor == DEINTERLACING_PROCESSOR_GPU);

                    const int deintFieldsBoth = (FIELD_ORDER_TFF | FIELD_ORDER_BFF);

                    HWND tff = GetDlgItem(hwnd, IDC_TFF),
                         bff = GetDlgItem(hwnd, IDC_BFF);
                    if(config.fieldOrder != deintFieldsBoth && deinterlacingConfig.fieldOrder != config.fieldOrder)
                    {
                        configData->data["deinterlacingFieldOrder"] =  config.fieldOrder;
                        deinterlacingConfig.fieldOrder = config.fieldOrder;
                    }
                    Button_SetCheck(tff, deinterlacingConfig.fieldOrder == FIELD_ORDER_TFF);
                    Button_SetCheck(bff, deinterlacingConfig.fieldOrder == FIELD_ORDER_BFF);
                    EnableWindow(tff, config.fieldOrder & FIELD_ORDER_TFF);
                    EnableWindow(bff, config.fieldOrder & FIELD_ORDER_BFF);
                }

                SendMessage(hwndTemp, CB_SETCURSEL, deinterlacingConfig.type, 0);

                //------------------------------------------

				BOOL bCustomResolution = TRUE;//configData->data["customResolution"].asInt();
                SendMessage(GetDlgItem(hwnd, IDC_CUSTOMRESOLUTION), BM_SETCHECK, bCustomResolution ? BST_CHECKED : BST_UNCHECKED, 0);

                
                FillOutListOfDevices(hwndDeviceList, CLSID_VideoInputDeviceCategory, &configData->deviceNameList, &configData->deviceIDList);

				bool bUseRecorder = configData->data["UseRecorder"].asInt() == 1;
				bool bSupportRecord = IsSupportRecord(L"NVIDIA") || IsSupportRecord(L"Intel");

				if (!bSupportRecord)
				{
					bUseRecorder = false;
					EnableWindow(GetDlgItem(hwnd, IDC_STARTRECORD), FALSE);
				}

				SendMessage(GetDlgItem(hwnd, IDC_STARTRECORD), BM_SETCHECK, bUseRecorder ? BST_CHECKED : BST_UNCHECKED, 0);

				if (!bUseRecorder)
				{
					EnableWindow(GetDlgItem(hwnd, IDC_AUDIOLIST), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_RECORDBITRATE), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_RECORDPATH), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_BROSE), FALSE);
				}

				UINT BitRate = 4000;
				if (!configData->data["RecorderBitRate"].isNull())
				{
					BitRate = configData->data["RecorderBitRate"].asUInt();
				}
				TCHAR *BitRa[] = { L"3000", L"4000", L"5000", L"6000", L"7000", L"8000", L"9000", L"10000" };
				int Index = -1;
				String sBitRate = FormattedString(L"%d", BitRate).Array();

				for (int i = 0; i < sizeof BitRa / sizeof (TCHAR *); ++i)
				{
					SendMessage(GetDlgItem(hwnd, IDC_RECORDBITRATE), CB_ADDSTRING, 0, (LPARAM)BitRa[i]);

					if (wcscmp(sBitRate.Array(), BitRa[i]) == 0)
					{
						Index = i;
					}
				}

				if (Index != -1)
				{
					SendMessage(GetDlgItem(hwnd, IDC_RECORDBITRATE), CB_SETCURSEL, Index, 0);
				}
				else
				{
					SendMessage(GetDlgItem(hwnd, IDC_RECORDBITRATE), WM_SETTEXT, 0, (LPARAM)sBitRate.Array());
				}
				
				String RecordPath = L"Videos\\.flv";

				if (!configData->data["RecordPath"].isNull())
				{
					RecordPath = Asic2WChar(configData->data["RecordPath"].asString().c_str()).c_str();
				}

				SetWindowText(GetDlgItem(hwnd, IDC_RECORDPATH), RecordPath.Array());

                UINT deviceID = CB_ERR;

                if(strDevice.IsValid() && cx > 0 && cy > 0 && frameInterval > 0) {
                    for(UINT i=0; i < configData->deviceNameList.Num(); i++) {
                        if(configData->deviceNameList[i].CompareI(strDevice.Array())) {
							if (configData->deviceIDList[i].CompareI(strDeviceID.Array()))
                                deviceID = i;
                        }
                    }
                }

				NoExist = false;
                if(deviceID == CB_ERR)
                {
					if (configData->deviceNameList.Num())
					{
						strDevice = configData->deviceNameList[0];
						SendMessage(hwndDeviceList, CB_SETCURSEL, 0, 0);
						ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_DEVICELIST, CBN_SELCHANGE), (LPARAM)hwndDeviceList);

// 						SendMessage(hwndDeviceList, CB_ADDSTRING, 0, (LPARAM)strDevice.Array());
// 						configData->deviceIDList.Add(strDeviceID.Array());
// 						configData->deviceNameList.Add(strDevice.Array());
// 						deviceID = configData->deviceNameList.Num() - 1;
					}
					else
					{
						EnableWindow(GetDlgItem(hwnd, IDOK), false);
					}
					//NoExist = true;
                }
                else
                {
                    SendMessage(hwndDeviceList, CB_SETCURSEL, deviceID, 0);
                    ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_DEVICELIST, CBN_SELCHANGE), (LPARAM)hwndDeviceList);

                    if(bCustomResolution)
                    {
                        String strResolution;
                        strResolution << UIntString(cx) << TEXT("x") << UIntString(cy);

                        SendMessage(hwndResolutionList, WM_SETTEXT, 0, (LPARAM)strResolution.Array());
                        ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RESOLUTION, CBN_EDITCHANGE), (LPARAM)hwndResolutionList);

						//if (10000000.0 / double(frameInterval) <= 30)
						{
							SetWindowText(hwndFPS, FloatString(10000000.0 / double(frameInterval)));
						}
						//else
						//{
						//	SetWindowText(hwndFPS, FloatString(30));
						//}
                    }
                }

                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_CUSTOMRESOLUTION, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_CUSTOMRESOLUTION));

                HWND hwndPreferredList = GetDlgItem(hwnd, IDC_PREFERREDOUTPUT);

				BOOL bUsePreferredOutput = true;// configData->data->GetInt(TEXT("usePreferredType"));
                EnableWindow(hwndPreferredList, bUsePreferredOutput);

                SendMessage(GetDlgItem(hwnd, IDC_USEPREFERREDOUTPUT), BM_SETCHECK, bUsePreferredOutput ? BST_CHECKED : BST_UNCHECKED, 0);

                //------------------------------------------

                bool bDefaultUseBuffering = strDevice.IsValid() && (sstri(strDevice, TEXT("Elgato")) != NULL);

				bool bUseBuffering = false;
				if (configData->data["useBuffering"].isNull())
				{
					bUseBuffering = bDefaultUseBuffering != 0;
				}
				else
				{
					bUseBuffering = configData->data["useBuffering"].asInt() != 0;
				}

                EnableWindow(GetDlgItem(hwnd, IDC_DELAY_EDIT), bUseBuffering);
                EnableWindow(GetDlgItem(hwnd, IDC_DELAY),      bUseBuffering);

                SendMessage(GetDlgItem(hwnd, IDC_USEBUFFERING), BM_SETCHECK, bUseBuffering ? BST_CHECKED : BST_UNCHECKED, 0);

                DWORD bufferTime = configData->data["bufferTime"].asInt();
                SendMessage(GetDlgItem(hwnd, IDC_DELAY), UDM_SETRANGE32, 0, 8000);
                SendMessage(GetDlgItem(hwnd, IDC_DELAY), UDM_SETPOS32, 0, bufferTime);

                //------------------------------------------

                UINT audioDeviceID = CB_ERR;

                if(strAudioDevice.IsValid() && cx > 0 && cy > 0 && frameInterval > 0) {
                    for(UINT i=0; i < configData->audioNameList.Num(); i++) {
                        if(configData->audioNameList[i].CompareI(strAudioDevice.Array())) {
                            if(configData->audioIDList[i].CompareI(strAudioDeviceID))
                                audioDeviceID = i;
                        }
                    }
                }

                /*if(strAudioDevice.IsValid())
                    audioDeviceID = (UINT)SendMessage(hwndAudioList, CB_FINDSTRINGEXACT, -1, (LPARAM)strAudioDevice.Array());*/

                if(audioDeviceID == CB_ERR)
                    SendMessage(hwndAudioList, CB_SETCURSEL, configData->bDeviceHasAudio ? 1 : 0, 0); //yes, I know, but the output is not a bool
                else
                    SendMessage(hwndAudioList, CB_SETCURSEL, audioDeviceID, 0);

                //------------------------------------------

				int soundOutputType = 1;
				if (!configData->data["soundOutputType"].isNull())
				{
					soundOutputType = configData->data["soundOutputType"].asInt();
				}
                switch(soundOutputType) {
                    case 0:
                    case 1: hwndTemp = GetDlgItem(hwnd, IDC_OUTPUTSOUND); break;
                    case 2: hwndTemp = GetDlgItem(hwnd, IDC_PLAYDESKTOPSOUND); break;
                }

                SendMessage(hwndTemp, BM_SETCHECK, BST_CHECKED, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET), soundOutputType == 1);
                EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT), soundOutputType == 1);
                EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), soundOutputType != 0);
				EnableWindow(GetDlgItem(hwnd, IDC_AUDIORENDERLIST), soundOutputType == 2);
                if (soundOutputType == 0)
                    SendMessage(hwndAudioList, CB_SETCURSEL, 0, 0);

                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIOLIST, CBN_SELCHANGE), (LPARAM)hwndAudioList);

                //------------------------------------------

                float fVol = 1.0f;

				if (!configData->data["volume"].isNull())
				{
					fVol = configData->data["volume"].asDouble();
				}

                SetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME), fVol);
                BOOL bUseAudioRender = configData->data["useAudioRender"].asBool();
                SendMessage(GetDlgItem(hwnd, IDC_USEAUDIORENDER), BM_SETCHECK, bUseAudioRender ? BST_CHECKED : BST_UNCHECKED, 0);

                //------------------------------------------

                int pos = configData->data["soundTimeOffset"].asInt();

                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETRANGE32, -3000, 20000);
                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETPOS32, 0, pos);

                //------------------------------------------

				BOOL  bUseChromaKey         = configData->data["useChromaKey"].asBool();
				BOOL  bUsePointFiltering    = configData->data["usePointFiltering"].asBool();
                BOOL  bPreserveSourceSize   = configData->data["preserveSourceSize"].asBool();
                DWORD keyColor              = 0xFFFFFFFF;
				if (!configData->data["keyColor"].isNull())
				{
					keyColor = configData->data["keyColor"].asUInt();
				}
				UINT  similarity            = configData->data["keySimilarity"].asUInt();
                UINT  blend                 =  80;

				if (!configData->data["keyBlend"].isNull())
				{
					blend = configData->data["keyBlend"].asUInt();
				}
                UINT  gamma                 = 50;

				if (!configData->data["keySpillReduction"].isNull())
				{
					gamma = configData->data["keySpillReduction"].asUInt();
				}

                SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_SETCHECK, bUsePointFiltering ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_PRESERVESIZE), BM_SETCHECK, bPreserveSourceSize ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_USECHROMAKEY), BM_SETCHECK, bUseChromaKey ? BST_CHECKED : BST_UNCHECKED, 0);
                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), keyColor);

                SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETRANGE32, 0, 1000);
                SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETPOS32, 0, similarity);

                SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETRANGE32, 0, 1000);
                SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETPOS32, 0, blend);

                SendMessage(GetDlgItem(hwnd, IDC_SPILLREDUCTION), UDM_SETRANGE32, 0, 1000);
                SendMessage(GetDlgItem(hwnd, IDC_SPILLREDUCTION), UDM_SETPOS32, 0, gamma);

                EnableWindow(GetDlgItem(hwnd, IDC_COLOR), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_SELECTCOLOR), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_BLEND), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_SPILLREDUCTION_EDIT), bUseChromaKey);
                EnableWindow(GetDlgItem(hwnd, IDC_SPILLREDUCTION), bUseChromaKey);

				//------------------------------------------

				UINT audioRenderDeviceID = CB_ERR;

				if ( strAudioRenderDevice.IsValid() ) {
					for (UINT i = 0; i < configData->audioRenderNameList.Num(); i++) {
						if (configData->audioRenderNameList[i].CompareI(strAudioRenderDevice.Array())) {
							if (configData->audioRenderIDList[i].CompareI(strAudioRenderDeviceID))
								audioRenderDeviceID = i;
						}
					}
				}
				if (audioRenderDeviceID != CB_ERR)
					SendMessage(hwndAudioRenderList, CB_SETCURSEL, audioRenderDeviceID, 0);
				ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIORENDERLIST, CBN_SELCHANGE), (LPARAM)hwndAudioRenderList);

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

                return TRUE;
            }

        case WM_DESTROY:
            if(colorData.bValid)
            {
                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
                colorData.Clear();
            }
            break;

        case WM_LBUTTONDOWN:
            if(bSelectingColor)
            {
                bMouseDown = true;
                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
            }
            break;

        case WM_MOUSEMOVE:
            if(bSelectingColor && bMouseDown)
            {
                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
            }
            break;

        case WM_LBUTTONUP:
            if(bSelectingColor)
            {
                colorData.Clear();
                ReleaseCapture();
                bMouseDown = false;
                bSelectingColor = false;

                ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configData->data["DeviceSourceID"].isNull())
				{
					SourceId << configData->data["DeviceSourceID"].asString().c_str();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;
				if (Video)
					Video->SetInt(TEXT("useChromaKey"), true);
            }
            break;

        case WM_CAPTURECHANGED:
            if(bSelectingColor)
            {
                if(colorData.bValid)
                {
                    CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
                    ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
                    colorData.Clear();
                }

                ReleaseCapture();
                bMouseDown = false;
                bSelectingColor = false;

                ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configData->data["DeviceSourceID"].isNull())
				{
					SourceId << configData->data["DeviceSourceID"].asString().c_str();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;
				if (Video)
					Video->SetInt(TEXT("useChromaKey"), true);
            }
            break;

        case WM_HSCROLL:
            {
                if(GetDlgCtrlID((HWND)lParam) == IDC_GAMMA)
                {
                    int gamma = SetSliderText(hwnd, IDC_GAMMA, IDC_GAMMAVAL);

                    ConfigDialogData *info = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

					std::stringstream SourceId;
					uint64_t VideoId = 0;
					if (!info->data["DeviceSourceID"].isNull())
					{
						SourceId << info->data["DeviceSourceID"].asString().c_str();
						SourceId >> VideoId;
					}

					IBaseVideo *Video = (IBaseVideo*)VideoId;
					if (Video)
						Video->SetInt(TEXT("gamma"), gamma);
                }
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_CUSTOMRESOLUTION:
                    {
                        HWND hwndUseCustomResolution = (HWND)lParam;
                        BOOL bCustomResolution = SendMessage(hwndUseCustomResolution, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        EnableWindow(GetDlgItem(hwnd, IDC_RESOLUTION), bCustomResolution);
                        EnableWindow(GetDlgItem(hwnd, IDC_FPS), bCustomResolution);
                        break;
                    }
				case IDC_STARTRECORD:
					{
						HWND hwndStartRecorde = (HWND)lParam;
						BOOL bStartRecorde = SendMessage(hwndStartRecorde, BM_GETCHECK, 0, 0) == BST_CHECKED;
						EnableWindow(GetDlgItem(hwnd, IDC_AUDIOLIST), bStartRecorde);
						EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), bStartRecorde);
						EnableWindow(GetDlgItem(hwnd, IDC_RECORDBITRATE), bStartRecorde);
						EnableWindow(GetDlgItem(hwnd, IDC_RECORDPATH), bStartRecorde);
						EnableWindow(GetDlgItem(hwnd, IDC_BROSE), bStartRecorde);

						UINT preferredType = -1;
						int id = (int)SendMessage(GetDlgItem(hwnd, IDC_PREFERREDOUTPUT), CB_GETCURSEL, 0, 0);
						if (id != -1)
							preferredType = (UINT)SendMessage(GetDlgItem(hwnd, IDC_PREFERREDOUTPUT), CB_GETITEMDATA, id, 0);

						if (preferredType != VideoOutputType_I420 && preferredType != VideoOutputType_YV12)
						{
							if (bStartRecorde)
							{
								SendMessage(GetDlgItem(hwnd, IDC_STARTRECORD), BM_SETCHECK, BST_UNCHECKED, 0);

								EnableWindow(GetDlgItem(hwnd, IDC_AUDIOLIST), false);
								EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), false);
								EnableWindow(GetDlgItem(hwnd, IDC_RECORDBITRATE), false);
								EnableWindow(GetDlgItem(hwnd, IDC_RECORDPATH), false);
								EnableWindow(GetDlgItem(hwnd, IDC_BROSE), false);

								BLiveMessageBox(hwnd, PluginStr("DeviceSelection.RecordWarning"), NULL, 0);
							}
						}

						break;
					}
				case IDC_BROSE:
				{
								  TCHAR Path[MAX_PATH] = { 0 };
							
								  GetWindowText(GetDlgItem(hwnd, IDC_RECORDPATH), Path, sizeof Path);
							

								  String PathE = Path;
								  PathE.FindReplace(TEXT(".mp4"), TEXT(""));
								  PathE.FindReplace(TEXT(".flv"), TEXT(""));
								  TCHAR SelectPath[MAX_PATH] = { L"Videos" };
								  BROWSEINFO m_bi;
								  ZeroMemory(&m_bi, sizeof(BROWSEINFO));
								  m_bi.hwndOwner = hwnd;
								  m_bi.pidlRoot = NULL;
								  m_bi.lpszTitle = TEXT("请选择文件夹");
								  m_bi.ulFlags = 0;
								  if (wcscmp(Path, TEXT("")) != 0)
								  {
									  m_bi.lParam = (LPARAM)PathE.Array();
									  m_bi.lpfn = _BrowseCallbackProc;
								  }

								  PIDLIST_ABSOLUTE pItem = SHBrowseForFolder(&m_bi);
								  if (pItem != NULL)
								  {
									  SHGetPathFromIDList(pItem, SelectPath);
									  CoTaskMemFree(pItem);

									  if (wcscmp(SelectPath, L"") == 0)
									  {
										  wcscat(SelectPath, TEXT("Videos\\.flv"));
									  }
									  else
									  {
										  wcscat(SelectPath, TEXT("\\.flv"));
									  }
									 

									  SetWindowText(GetDlgItem(hwnd, IDC_RECORDPATH), SelectPath);
								  }
								  else
								  {
									  wcscat(SelectPath, TEXT("\\.flv"));
									  SetWindowText(GetDlgItem(hwnd, IDC_RECORDPATH), SelectPath);
								  }

								  break;
				}
                case IDC_USEBUFFERING:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        bool bUseBuffering = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        EnableWindow(GetDlgItem(hwnd, IDC_DELAY_EDIT), bUseBuffering);
                        EnableWindow(GetDlgItem(hwnd, IDC_DELAY),      bUseBuffering);
                    }
                    break;

                case IDC_VOLUME:
                    if(HIWORD(wParam) == VOLN_ADJUSTING || HIWORD(wParam) == VOLN_FINALVALUE)
                    {
                        if(IsWindowEnabled((HWND)lParam))
                        {
                            float fVol = GetVolumeControlValue((HWND)lParam);

                            ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["DeviceSourceID"].isNull())
							{
								SourceId << configData->data["DeviceSourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;
							if (Video)
								Video->SetFloat(TEXT("volume"), fVol);
                        }
                    }
                    break;

                case IDC_USECHROMAKEY:
                    {
                        HWND hwndUseColorKey = (HWND)lParam;
                        BOOL bUseChromaKey = SendMessage(hwndUseColorKey, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["DeviceSourceID"].isNull())
						{
							SourceId << configData->data["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
							Video->SetInt(TEXT("useChromaKey"), bUseChromaKey);

                        EnableWindow(GetDlgItem(hwnd, IDC_COLOR),               bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_SELECTCOLOR),         bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT),  bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD),       bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT),          bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BLEND),               bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_SPILLREDUCTION_EDIT), bUseChromaKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_SPILLREDUCTION),      bUseChromaKey);
                        break;
                    }

                case IDC_SELECTCOLOR:
                    {
                        if(!bSelectingColor)
                        {
                            if(colorData.Init())
                            {
                                bMouseDown = false;
                                bSelectingColor = true;
                                SetCapture(hwnd);
                                HCURSOR hCursor = (HCURSOR)LoadImage(hinstMain, MAKEINTRESOURCE(IDC_COLORPICKER), IMAGE_CURSOR, 32, 32, 0);
                                SetCursor(hCursor);

                                ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

								std::stringstream SourceId;
								uint64_t VideoId = 0;
								if (!configData->data["DeviceSourceID"].isNull())
								{
									SourceId << configData->data["DeviceSourceID"].asString().c_str();
									SourceId >> VideoId;
								}

								IBaseVideo *Video = (IBaseVideo*)VideoId;
								if (Video)
									Video->SetInt(TEXT("useChromaKey"), false);
                            }
                            else
                                colorData.Clear();
                        }
                        break;
                    }

                case IDC_COLOR:
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["DeviceSourceID"].isNull())
						{
							SourceId << configData->data["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
						{
							DWORD color = CCGetColor((HWND)lParam);
							Video->SetInt(TEXT("keyColor"), color);
						}
                        break;
                    }

                case IDC_FLIPIMAGE:
                case IDC_FLIPIMAGEH:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
//                         ImageSource *source = API->GetSceneImageSource(configData->lpName);
//                         if(source)
//                         {
//                             HWND hwndFlip = (HWND)lParam;
//                             BOOL bFlipImage = SendMessage(hwndFlip, BM_GETCHECK, 0, 0) == BST_CHECKED;
// 
//                             switch(LOWORD(wParam))
//                             {
//                                 case IDC_FLIPIMAGE:  source->SetInt(TEXT("flipImage"), bFlipImage); break;
//                                 case IDC_FLIPIMAGEH: source->SetInt(TEXT("flipImageHorizontal"), bFlipImage); break;
//                             }
//                         }

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["DeviceSourceID"].isNull())
						{
							SourceId << configData->data["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
						{
							HWND hwndFlip = (HWND)lParam;
                            BOOL bFlipImage = SendMessage(hwndFlip, BM_GETCHECK, 0, 0) == BST_CHECKED;

                            switch(LOWORD(wParam))
                            {
								case IDC_FLIPIMAGE:  Video->SetInt(TEXT("flipImage"), bFlipImage); break;
								case IDC_FLIPIMAGEH: Video->SetInt(TEXT("flipImageHorizontal"), bFlipImage); break;
                            }
						}
                    }
                    break;

                case IDC_POINTFILTERING:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
//                         ImageSource *source = API->GetSceneImageSource(configData->lpName);
//                         if(source)
//                         {
//                             HWND hwndPointFiltering = (HWND)lParam;
//                             BOOL bUsePointFiltering = SendMessage(hwndPointFiltering, BM_GETCHECK, 0, 0) == BST_CHECKED;
// 
//                             source->SetInt(TEXT("usePointFiltering"), bUsePointFiltering);
//                         }

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["DeviceSourceID"].isNull())
						{
							SourceId << configData->data["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
						{
							HWND hwndPointFiltering = (HWND)lParam;
                            BOOL bUsePointFiltering = SendMessage(hwndPointFiltering, BM_GETCHECK, 0, 0) == BST_CHECKED;

							Video->SetInt(TEXT("usePointFiltering"), bUsePointFiltering);
						}
                    }
                    break;

                case IDC_DELAY_EDIT:
                case IDC_TIMEOFFSET_EDIT:
                case IDC_OPACITY_EDIT:
                case IDC_BASETHRESHOLD_EDIT:
                case IDC_BLEND_EDIT:
                case IDC_SPILLREDUCTION_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(configData)
                        {
//                             ImageSource *source = API->GetSceneImageSource(configData->lpName);
// 
//                             if(source)
//                             {
//                                 HWND hwndVal = NULL;
//                                 switch(LOWORD(wParam))
//                                 {
//                                     case IDC_DELAY_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_DELAY); break;
//                                     case IDC_TIMEOFFSET_EDIT:       hwndVal = GetDlgItem(hwnd, IDC_TIMEOFFSET); break;
//                                     case IDC_OPACITY_EDIT:          hwndVal = GetDlgItem(hwnd, IDC_OPACITY); break;
//                                     case IDC_BASETHRESHOLD_EDIT:    hwndVal = GetDlgItem(hwnd, IDC_BASETHRESHOLD); break;
//                                     case IDC_BLEND_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_BLEND); break;
//                                     case IDC_SPILLREDUCTION_EDIT:   hwndVal = GetDlgItem(hwnd, IDC_SPILLREDUCTION); break;
//                                 }
// 
//                                 int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
//                                 switch(LOWORD(wParam))
//                                 {
//                                     case IDC_DELAY_EDIT:            source->SetInt(TEXT("bufferTime"), val); break;
//                                     case IDC_TIMEOFFSET_EDIT:       source->SetInt(TEXT("timeOffset"), val); break;
//                                     case IDC_OPACITY_EDIT:          source->SetInt(TEXT("opacity"), val); break;
//                                     case IDC_BASETHRESHOLD_EDIT:    source->SetInt(TEXT("keySimilarity"), val); break;
//                                     case IDC_BLEND_EDIT:            source->SetInt(TEXT("keyBlend"), val); break;
//                                     case IDC_SPILLREDUCTION_EDIT:   source->SetInt(TEXT("keySpillReduction"), val); break;
//                                 }
//                             }

							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["DeviceSourceID"].isNull())
							{
								SourceId << configData->data["DeviceSourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;
							if (Video)
							{
								HWND hwndVal = NULL;
								switch (LOWORD(wParam))
								{
								case IDC_DELAY_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_DELAY); break;
								case IDC_TIMEOFFSET_EDIT:       hwndVal = GetDlgItem(hwnd, IDC_TIMEOFFSET); break;
								case IDC_OPACITY_EDIT:          hwndVal = GetDlgItem(hwnd, IDC_OPACITY); break;
								case IDC_BASETHRESHOLD_EDIT:    hwndVal = GetDlgItem(hwnd, IDC_BASETHRESHOLD); break;
								case IDC_BLEND_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_BLEND); break;
								case IDC_SPILLREDUCTION_EDIT:   hwndVal = GetDlgItem(hwnd, IDC_SPILLREDUCTION); break;
								}

								int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
								switch (LOWORD(wParam))
								{
								case IDC_DELAY_EDIT:            Video->SetInt(TEXT("bufferTime"), val); break;
								case IDC_TIMEOFFSET_EDIT:       Video->SetInt(TEXT("timeOffset"), val); break;
								case IDC_OPACITY_EDIT:          Video->SetInt(TEXT("opacity"), val); break;
								case IDC_BASETHRESHOLD_EDIT:    Video->SetInt(TEXT("keySimilarity"), val); break;
								case IDC_BLEND_EDIT:            Video->SetInt(TEXT("keyBlend"), val); break;
								case IDC_SPILLREDUCTION_EDIT:   Video->SetInt(TEXT("keySpillReduction"), val); break;
								}
							}
                        }
                    }
                    break;

                case IDC_USEPREFERREDOUTPUT:
                    {
                       // BOOL bUsePreferredOutput = SendMessage(GetDlgItem(hwnd, IDC_USEPREFERREDOUTPUT), BM_GETCHECK, 0, 0) == BST_CHECKED;
                       // EnableWindow(GetDlgItem(hwnd, IDC_PREFERREDOUTPUT), bUsePreferredOutput);
                        break;
                    }
				case IDC_PREFERREDOUTPUT:
				{
											if (HIWORD(wParam) == CBN_SELCHANGE)
											{
												SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_STARTRECORD, 0), (LPARAM)GetDlgItem(hwnd, IDC_STARTRECORD));
											}
											break;
				}
                case IDC_REFRESH:
                    {
                        HWND hwndDeviceList = GetDlgItem(hwnd, IDC_DEVICELIST);
                        HWND hwndAudioDeviceList = GetDlgItem(hwnd, IDC_AUDIOLIST);

                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

                        FillOutListOfDevices(hwndDeviceList, CLSID_VideoInputDeviceCategory, &configData->deviceNameList, &configData->deviceIDList);
                        bAudioDevicesPresent = FillOutListOfDevices(hwndAudioDeviceList, CLSID_AudioInputDeviceCategory, &configData->audioNameList, &configData->audioIDList);

                        SendMessage(hwndDeviceList, CB_SETCURSEL, 0, 0);
                        ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_DEVICELIST, CBN_SELCHANGE), (LPARAM)hwndDeviceList);


                        if(bAudioDevicesPresent)
                        {
                            SendMessage(hwndAudioDeviceList, CB_SETCURSEL, 0, 0);
                            ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIOLIST, CBN_SELCHANGE), (LPARAM)hwndAudioDeviceList);
                        }
                        EnableWindow(hwndAudioDeviceList, bAudioDevicesPresent);
                        EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), bAudioDevicesPresent);

                        break;
                    }

                case IDC_DEINTERLACELIST:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);;
                        int confId = (int)SendMessage(GetDlgItem(hwnd, IDC_DEINTERLACELIST), CB_GETCURSEL, 0, 0);
                        
                        deinterlacingConfig.type        = configData->data["deinterlacingType"].asInt();
                        deinterlacingConfig.fieldOrder  = configData->data["deinterlacingFieldOrder"].asInt();
                        deinterlacingConfig.processor   = configData->data["deinterlacingGPU"].asInt();
                        if(deinterlacingConfig.type >= DEINTERLACING_TYPE_LAST)
                            deinterlacingConfig.type = DEINTERLACING_NONE;
						
                        DeinterlacerConfig& config = deinterlacerConfigs[confId];

                        const int deintProcBoth = (DEINTERLACING_PROCESSOR_CPU | DEINTERLACING_PROCESSOR_GPU);

                        HWND checkbox = GetDlgItem(hwnd, IDC_GPUDEINT);
                        EnableWindow(checkbox, config.processor == deintProcBoth);
                        if(config.processor != deintProcBoth && deinterlacingConfig.processor != config.processor)
                            deinterlacingConfig.processor = config.processor;
                        Button_SetCheck(checkbox, deinterlacingConfig.processor == DEINTERLACING_PROCESSOR_GPU);

                        const int deintFieldsBoth = (FIELD_ORDER_TFF | FIELD_ORDER_BFF);

                        HWND tff = GetDlgItem(hwnd, IDC_TFF),
                            bff = GetDlgItem(hwnd, IDC_BFF);
                        if(config.fieldOrder != deintFieldsBoth && deinterlacingConfig.fieldOrder != config.fieldOrder)
                            deinterlacingConfig.fieldOrder = config.fieldOrder;
                        else if(deinterlacingConfig.fieldOrder != config.fieldOrder && (config.fieldOrder & deinterlacingConfig.fieldOrder) == 0)
                            deinterlacingConfig.fieldOrder = config.fieldOrder & FIELD_ORDER_TFF ? FIELD_ORDER_TFF : FIELD_ORDER_BFF;
                        Button_SetCheck(tff, deinterlacingConfig.fieldOrder == FIELD_ORDER_TFF);
                        Button_SetCheck(bff, deinterlacingConfig.fieldOrder == FIELD_ORDER_BFF);
                        EnableWindow(tff, config.fieldOrder & FIELD_ORDER_TFF);
                        EnableWindow(bff, config.fieldOrder & FIELD_ORDER_BFF);
                    }
                    break;

                case IDC_DEVICELIST:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
					{
					    HWND hwndResolutions = GetDlgItem(hwnd, IDC_RESOLUTION);
                        SendMessage(hwndResolutions, CB_RESETCONTENT, 0, 0);

						ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

                        HWND hwndDevices = (HWND)lParam;
                        UINT id = (UINT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);
						if (id == CB_ERR || (NoExist && id == configData->deviceNameList.Num() - 1))
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_RESOLUTION), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_FPS), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_CONFIG), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
							//提示一下吧
							SetWindowText(GetDlgItem(hwnd, IDC_64BIT_WARNING), TEXT("摄像头异常无法获取参数和使用!"));
                        }
                        else
                        {
							//SetWindowText(GetDlgItem(hwnd, IDC_64BIT_WARNING), PluginStr("DeviceSelection.64BitWarning"));

                            HWND hwndAudioList = GetDlgItem(hwnd, IDC_AUDIOLIST);
							HWND hwndAudioRenderList = GetDlgItem(hwnd, IDC_AUDIORENDERLIST);

                            BOOL bCustomResolution = SendMessage(GetDlgItem(hwnd, IDC_CUSTOMRESOLUTION) , BM_GETCHECK, 0, 0) == BST_CHECKED;

                            EnableWindow(GetDlgItem(hwnd, IDC_RESOLUTION), bCustomResolution);
                            EnableWindow(GetDlgItem(hwnd, IDC_FPS), bCustomResolution);
                            EnableWindow(GetDlgItem(hwnd, IDC_CONFIG), TRUE);
                            EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);

                            IBaseFilter *filter = GetDeviceByValue(CLSID_VideoInputDeviceCategory,
                                                                   L"FriendlyName", configData->deviceNameList[id],
                                                                   L"DevicePath", configData->deviceIDList[id]);

                            if(filter)
                            {
                                //--------------------------------
                                // get video info
								
                                IPin *outputPin = GetOutputPin(filter, &MEDIATYPE_Video);
                                if(outputPin)
                                {
									Log::writeMessage(LOG_RTSPSERV,1,"FriendlyName: %s", WcharToAnsi(configData->deviceNameList[id].Array()).c_str());
                                    configData->ClearOutputList();
                                    GetOutputList(outputPin, configData->outputList);

                                    configData->GetResolutions(configData->resolutions);
                                    for(UINT i=0; i<configData->resolutions.Num(); i++)
                                    {
                                        SIZE &resolution = configData->resolutions[i];

                                        String strResolution;
                                        strResolution << IntString(resolution.cx) << TEXT("x") << IntString(resolution.cy);
                                        SendMessage(hwndResolutions, CB_ADDSTRING, 0, (LPARAM)strResolution.Array());
                                    }

                                    outputPin->Release();
                                    outputPin = NULL;
                                }

                                String strDeviceName = configData->deviceNameList[id];
                                bool bIsElworkaroundo = sstri(strDeviceName, TEXT("elgato")) != NULL; //HAHAHAHAHAHAHA HOW F***ING WONDERFUL. 自殺したい

                                IAMCrossbar *crossbar = bIsElworkaroundo ? NULL : GetFilterCrossbar(filter);
                                EnableWindow(GetDlgItem(hwnd, IDC_CROSSBAR), crossbar != NULL);
                                SafeRelease(crossbar);

                                outputPin = GetOutputPin(filter, &MEDIATYPE_Audio);
                                configData->bDeviceHasAudio = (outputPin != NULL);
                                SafeRelease(outputPin);

                                filter->Release();
                            }

                            HWND hwndUseBuffering = GetDlgItem(hwnd, IDC_USEBUFFERING);

                            if (sstri(configData->deviceNameList[id], TEXT("Elgato")) != NULL) {
                                SendMessage(hwndUseBuffering, BM_SETCHECK, BST_CHECKED, 0);
                                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_USEBUFFERING, BN_CLICKED), (LPARAM)hwndUseBuffering);
                            } else {
                                SendMessage(hwndUseBuffering, BM_SETCHECK, BST_UNCHECKED, 0);
                                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_USEBUFFERING, BN_CLICKED), (LPARAM)hwndUseBuffering);
                            }

                            //-------------------------------------------------

                            SIZE size;
                            UINT64 frameInterval;
                            if(GetClosestResolutionFPS(configData->outputList, size, frameInterval, true))
                            {
                                String strResolution;
                                strResolution << UIntString(size.cx) << TEXT("x") << UIntString(size.cy);

                                SendMessage(hwndResolutions, WM_SETTEXT, 0, (LPARAM)strResolution.Array());
                                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RESOLUTION, CBN_EDITCHANGE), (LPARAM)hwndResolutions);

                                HWND hwndFPS = GetDlgItem(hwnd, IDC_FPS);

								if (10000000.0 / double(frameInterval) <= 30)
								{
									SetWindowText(hwndFPS, FloatString(10000000.0 / double(frameInterval)));
								}
								else
								{
									SetWindowText(hwndFPS, FloatString(30));
								}
                            }
                            else
                            {
                                SendMessage(hwndResolutions, CB_SETCURSEL, 0, 0);
                                ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RESOLUTION, CBN_SELCHANGE), (LPARAM)hwndResolutions);
                            }

                            //-------------------------------------------------
                            // get audio devices

                            bAudioDevicesPresent = FillOutListOfDevices(hwndAudioList, CLSID_AudioInputDeviceCategory, &configData->audioNameList, &configData->audioIDList);

                            if (configData->bDeviceHasAudio) {
								CTSTR lpName = L"11111";//PluginStr("DeviceSelection.UseDeviceAudio");

                                SendMessage(hwndAudioList, CB_INSERTSTRING, 0, (LPARAM)lpName);
                                configData->audioNameList.Insert(0, lpName);
                                configData->audioIDList.Insert(0, NULL);
                            }

                            CTSTR pDisabled = L"禁用";
                            SendMessage(hwndAudioList, CB_INSERTSTRING, 0, (LPARAM)pDisabled);
                            configData->audioNameList.Insert(0, pDisabled);
                            configData->audioIDList.Insert(0, TEXT("Disabled"));

                            //EnableWindow(hwndAudioList, bAudioDevicesPresent);

                            if (!bAudioDevicesPresent)
                                EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), FALSE);

                            bool bHasAudio = configData->bDeviceHasAudio;

                            EnableWindow(GetDlgItem(hwnd, IDC_PLAYDESKTOPSOUND), bHasAudio);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTPUTSOUND),      bHasAudio);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET),       bHasAudio);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT),  bHasAudio);

                            if (bHasAudio)
                                SendMessage(hwndAudioList, CB_SETCURSEL, 1, 0);
                            else
                                SendMessage(hwndAudioList, CB_SETCURSEL, 0, 0);

                            ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIOLIST, CBN_SELCHANGE), (LPARAM)hwndAudioList);

							//-----------------------------------------------------------------
							// get render audio devices
							FillOutListOfDevices(hwndAudioRenderList, CLSID_AudioRendererCategory, &configData->audioRenderNameList, &configData->audioRenderIDList);
							CTSTR pDefault = L"默认";
							SendMessage(hwndAudioRenderList, CB_INSERTSTRING, 0, (LPARAM)pDefault);
							configData->audioRenderNameList.Insert(0, pDefault);
							configData->audioRenderIDList.Insert(0, TEXT("Default"));
							SendMessage(hwndAudioRenderList, CB_SETCURSEL, 0, 0);
                        }
                    }
                    break;

                case IDC_AUDIOLIST:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hwndDevices = (HWND)lParam;
                        UINT id = (UINT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);

                        if (id == CB_ERR || id == 0) {
                            EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO),      FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_PLAYDESKTOPSOUND), FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTPUTSOUND),      FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET),       FALSE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT),  FALSE);
                        } else {
                            ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
                            IBaseFilter *filter = GetDeviceByValue(CLSID_AudioInputDeviceCategory,
                                                                   L"FriendlyName", configData->audioNameList[id],
                                                                   L"DevicePath", configData->audioIDList[id]);
                            if (filter) {
                                EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), TRUE);

                                filter->Release();
                            } else {
                                EnableWindow(GetDlgItem(hwnd, IDC_CONFIGAUDIO), FALSE);
                            }

                            EnableWindow(GetDlgItem(hwnd, IDC_PLAYDESKTOPSOUND), TRUE);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTPUTSOUND),      TRUE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET),       TRUE);
                            EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT),  TRUE);
                        }
                    }
                    break;
				case IDC_AUDIORENDERLIST:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						HWND hwndDevices = (HWND)lParam;
						INT id = (INT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);

						if (id > 0) {
							ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
							IBaseFilter *filter = GetDeviceByValue(CLSID_AudioRendererCategory,
								L"FriendlyName", configData->audioRenderNameList[id],
								L"DevicePath", configData->audioRenderIDList[id]);
							if (filter)
							{
								filter->Release();
							}
							else
							{
								BLiveMessageBox(hwnd, PluginStr("DeviceSelection.UnsupportedRenderDevice"), NULL, 0);
							}
						}
					}
					break;
                case IDC_RESOLUTION:
                    if(HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

                        HWND hwndResolution = (HWND)lParam;
                        HWND hwndFPS        = GetDlgItem(hwnd, IDC_FPS);

                        SIZE resolution;
                        FPSInfo fpsInfo;

                        SendMessage(hwndFPS, CB_RESETCONTENT, 0, 0);
                        if(!GetResolution(hwndResolution, resolution, HIWORD(wParam) == CBN_SELCHANGE) || !configData->GetResolutionFPSInfo(resolution, fpsInfo))
                        {
                            SetWindowText(hwndFPS, TEXT("0"));
							EnableWindow(hwndFPS, false);
                            break;
                        }
						EnableWindow(hwndFPS, true);
                        //-------------------------------------------------------

                        double bestFPS = 0.0f;
                        UINT64 bestInterval = 0;
                        for(UINT i = 0; i < fpsInfo.supportedIntervals.Num(); i++)
                        {
                            double minFPS = 10000000.0/double(fpsInfo.supportedIntervals[i].maxFrameInterval);
                            double maxFPS = 10000000.0/double(fpsInfo.supportedIntervals[i].minFrameInterval);
							double realMaxFPS = maxFPS;
							if (maxFPS > 30)
							{
								maxFPS = 30;
							}

                            String strFPS;
                            if(CloseDouble(minFPS, maxFPS))
                                strFPS << FloatString(float(minFPS));
                            else
								strFPS << FloatString(float(minFPS)) << TEXT("-") << FloatString(float(realMaxFPS));

                            int id = (int)SendMessage(hwndFPS, CB_FINDSTRINGEXACT, -1, (LPARAM)strFPS.Array());
                            if(id == CB_ERR)
                                SendMessage(hwndFPS, CB_ADDSTRING, 0, (LPARAM)strFPS.Array());

                            if(bestFPS < minFPS)
                            {
                                bestFPS = minFPS;
                                bestInterval = fpsInfo.supportedIntervals[i].maxFrameInterval;
                            }
                            if(bestFPS < maxFPS)
                            {
                                bestFPS = maxFPS;
                                bestInterval = fpsInfo.supportedIntervals[i].minFrameInterval;
                            }
                        }

                        SetWindowText(hwndFPS, FloatString(float(bestFPS)));

                        //-------------------------------------------------------

                        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_FPS, CBN_EDITCHANGE), (LPARAM)GetDlgItem(hwnd, IDC_FPS));
                    }
                    break;

                case IDC_FPS:
                    if(HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

                        SIZE resolution;
                        if(!GetResolution(GetDlgItem(hwnd, IDC_RESOLUTION), resolution, FALSE))
                            break;

                        FPSInfo fpsInfo;
                        if(!configData->GetResolutionFPSInfo(resolution, fpsInfo))
                            break;

                        //--------------------------------------------

                        String strFPSVal = GetEditText((HWND)lParam);
                        if(schr(strFPSVal, '-'))
                        {
                            StringList tokens;
                            strFPSVal.GetTokenList(tokens, '-', FALSE);
                            if(tokens.Num())
                                strFPSVal = tokens.Last();
                            else
                                break;
                        }

                        if(!ValidFloatString(strFPSVal))
                            break;

                        float fps = strFPSVal.ToFloat();
                        INT64 interval = INT64(10000000.0/double(fps));

                        UINT64 bestInterval;
                        UINT64 bestDist = 0xFFFFFFFFFFFFFFFFLL;
                        for(UINT i = 0; i < fpsInfo.supportedIntervals.Num(); i++)
                        {
                            UINT64 maxDist = (UINT64)_abs64(INT64(fpsInfo.supportedIntervals[i].maxFrameInterval) - interval);
                            UINT64 minDist = (UINT64)_abs64(INT64(fpsInfo.supportedIntervals[i].minFrameInterval) - interval);

                            if(maxDist < bestDist)
                            {
                                bestDist = maxDist;
                                bestInterval = fpsInfo.supportedIntervals[i].maxFrameInterval;
                            }
                            if(minDist < bestDist)
                            {
                                bestDist = minDist;
                                bestInterval = fpsInfo.supportedIntervals[i].minFrameInterval;
                            }
                        }

                        //--------------------------------------------

                        HWND hwndPreferredList = GetDlgItem(hwnd, IDC_PREFERREDOUTPUT);
                        SendMessage(hwndPreferredList, CB_RESETCONTENT, 0, 0);

                        UINT preferredType = (UINT)configData->data["preferredType"].asUInt();

                        List<VideoOutputType> types;
                        if(GetVideoOutputTypes(configData->outputList, resolution.cx, resolution.cy, bestInterval, types))
                        {
                            int preferredID = -1;

                            for(UINT i=0; i<types.Num(); i++)
                            {
                                CTSTR lpName = EnumToName[(UINT)types[i]];

                                int id = (int)SendMessage(hwndPreferredList, CB_ADDSTRING, 0, (LPARAM)lpName);
                                SendMessage(hwndPreferredList, CB_SETITEMDATA, id, (LPARAM)types[i]);

								if (preferredType == -1)
								{
									switch ((UINT)types[i])
									{
									case VideoOutputType_RGB32:
									case VideoOutputType_RGB24:
									case VideoOutputType_MJPG:
										preferredType = (UINT)types[i];
										break;
									}
								}

                                if((UINT)types[i] == preferredType)
                                {
                                    SendMessage(hwndPreferredList, CB_SETCURSEL, id, 0);

									preferredID = id;

									if (wcscmp(EnumToName[(UINT)types[i]], L"I420") && wcscmp(EnumToName[(UINT)types[i]], L"YV12"))
									{
										SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_STARTRECORD, 0), (LPARAM)GetDlgItem(hwnd, IDC_STARTRECORD));
									}
                                }
                            }

							if (preferredID == -1)
							{
								bool bFind = false;
								for (UINT j = 0; j < types.Num(); j++)
								{
									CTSTR lpName = EnumToName[(UINT)types[j]];
									if (wcscmp(lpName, L"I420") == 0)
									{
										bFind = true;
										SendMessage(hwndPreferredList, CB_SETCURSEL, j, 0);
										break;
									}
								}

								if (!bFind)
								{
									for (UINT j = 0; j < types.Num(); j++)
									{
										CTSTR lpName = EnumToName[(UINT)types[j]];
										if (wcscmp(lpName, L"YV12") == 0)
										{
											bFind = true;
											SendMessage(hwndPreferredList, CB_SETCURSEL, j, 0);
											break;
										}
									}

									if (!bFind)
									{
										SendMessage(hwndPreferredList, CB_SETCURSEL, 0, 0);

										if (wcscmp(EnumToName[(UINT)types[0]], L"I420") && wcscmp(EnumToName[(UINT)types[0]], L"YV12"))
										{
											SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_STARTRECORD, 0), (LPARAM)GetDlgItem(hwnd, IDC_STARTRECORD));
										}
									}
								}
							}
                        }
                    }
                    break;

                case IDC_CONFIG:
                case IDC_CONFIGAUDIO:
                case IDC_CROSSBAR:
                    {
                        UINT id;
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
                        IBaseFilter *filter;

                        switch(LOWORD(wParam))
                        {
                            case IDC_CONFIG:
                                id = (UINT)SendMessage(GetDlgItem(hwnd, IDC_DEVICELIST), CB_GETCURSEL, 0, 0);
								if (id != CB_ERR) {

									std::stringstream SourceId;
									uint64_t VideoId = 0;
									if (!configData->data["DeviceSourceID"].isNull())
									{
										SourceId << configData->data["DeviceSourceID"].asString().c_str();
										SourceId >> VideoId;
									}

									IBaseFilter *BaseFilter = NULL;

									IBaseVideo *Video = (IBaseVideo*)VideoId;
									if (Video)
									{
										DeviceSource *IDevice = dynamic_cast<DeviceSource*>(Video);

										if (IDevice)
										{
											BaseFilter = IDevice->deviceFilter;
										}
									}

									OpenPropertyPagesByName(hwnd, configData->deviceNameList[id], configData->deviceIDList[id], CLSID_VideoInputDeviceCategory, BaseFilter);
								}
                                break;
                            case IDC_CONFIGAUDIO:
                                id = (UINT)SendMessage(GetDlgItem(hwnd, IDC_AUDIOLIST), CB_GETCURSEL, 0, 0);
                                if(id != CB_ERR) OpenPropertyPagesByName(hwnd, configData->audioNameList[id], configData->audioIDList[id], CLSID_AudioInputDeviceCategory);
                                break;

                            case IDC_CROSSBAR:
                                id = (UINT)SendMessage(GetDlgItem(hwnd, IDC_DEVICELIST), CB_GETCURSEL, 0, 0);
                                if (id == CB_ERR)
                                    break;

                                filter = GetDeviceByValue(CLSID_VideoInputDeviceCategory,
                                                          L"FriendlyName", configData->deviceNameList[id],
                                                          L"DevicePath", configData->deviceIDList[id]);

                                if (filter) {
                                    String strDeviceName = configData->deviceNameList[id];
                                    bool bIsElworkaroundo = sstri(strDeviceName, TEXT("elgato")) != NULL; //HAHAHAHAHAHAHA HOW F***ING WONDERFUL. 自殺したい

                                    IAMCrossbar *crossbar = bIsElworkaroundo ? NULL : GetFilterCrossbar(filter);
                                    if (crossbar) {
                                        OpenPropertyPages(hwnd, crossbar);
                                        crossbar->Release();
                                    }

                                    filter->Release();
                                }

                                break;
                        }
                        break;
                    }

                case IDOK:
                    {
                        UINT deviceID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_DEVICELIST), CB_GETCURSEL, 0, 0);
                        if(deviceID == CB_ERR)
                            break;

                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

                        SIZE resolution;
                        if(!GetResolution(GetDlgItem(hwnd, IDC_RESOLUTION), resolution, FALSE))
                        {
							BLiveMessageBox(hwnd, PluginStr("DeviceSelection.InvalidResolution"), NULL, 0);
                            break;
                        }

                        String strDevice = configData->deviceIDList[deviceID];
                        String strFPS = GetEditText(GetDlgItem(hwnd, IDC_FPS));
                        if(schr(strFPS, '-'))
                        {
                            StringList tokens;
                            strFPS.GetTokenList(tokens, '-', FALSE);
                            if(tokens.Num())
                                strFPS = tokens.Last();
                            else
                                break;
                        }

                        float fpsVal = 120.0f;
                        if(ValidFloatString(strFPS))
                            fpsVal = strFPS.ToFloat();

                        UINT64 frameInterval = UINT64(10000000.0/double(fpsVal));

                        if(strFPS == TEXT("0") || fpsVal == 0.0)
                        {
                            BLiveMessageBox(hwnd, PluginStr("DeviceSelection.UnsupportedResolution"), NULL, 0);
                            break;
                        }

						String strDeviceIDOld;

						if (configData->data["deviceID"].asString().c_str())
						{
							strDeviceIDOld = Asic2WChar(configData->data["deviceID"].asString().c_str()).c_str();
						}
					
						configData->data["device"] = WcharToAnsi((GetCBText(GetDlgItem(hwnd, IDC_DEVICELIST), deviceID)).Array()).c_str();
						configData->data["deviceName"] = WcharToAnsi(configData->deviceNameList[deviceID].Array()).c_str();

						if (!configData->deviceIDList[deviceID].IsEmpty())
							configData->data["deviceID"] = WcharToAnsi(configData->deviceIDList[deviceID].Array()).c_str();
						else
						{
							configData->data["deviceID"] = "";
						}

						if (configData->bCreating || strDeviceIDOld != strDevice)
						{
							if (CurrentDeviceExists(configData->data))
							{
								if (!strDeviceIDOld.IsEmpty())
									configData->data["deviceID"] = WcharToAnsi(strDeviceIDOld.Array()).c_str();

								BLiveMessageBox(hwnd, PluginStr("DeviceSelection.GlobalExists"), NULL, 0);
								break;
							}
                        }

						
                        //------------------------------------------

                        BOOL bFlip = SendMessage(GetDlgItem(hwnd, IDC_FLIPIMAGE), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bFlipHorizontal = SendMessage(GetDlgItem(hwnd, IDC_FLIPIMAGEH), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bCustomResolution = SendMessage(GetDlgItem(hwnd, IDC_CUSTOMRESOLUTION), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bUsePointFiltering = SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bPreserveSourceSize = SendMessage(GetDlgItem(hwnd, IDC_PRESERVESIZE), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        int deintId = (int)SendMessage(GetDlgItem(hwnd, IDC_DEINTERLACELIST), CB_GETCURSEL, 0, 0);
                        deinterlacingConfig = deinterlacerConfigs[deintId];
                        bool tff = SendMessage(GetDlgItem(hwnd, IDC_TFF), BM_GETCHECK, 0, 0) == BST_CHECKED,
                             bff = SendMessage(GetDlgItem(hwnd, IDC_BFF), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        deinterlacingConfig.fieldOrder = tff ? FIELD_ORDER_TFF : (bff ? FIELD_ORDER_BFF : FIELD_ORDER_NONE);
                        deinterlacingConfig.processor = SendMessage(GetDlgItem(hwnd, IDC_GPUDEINT), BM_GETCHECK, 0, 0) == BST_CHECKED ? DEINTERLACING_PROCESSOR_GPU : DEINTERLACING_PROCESSOR_CPU;

                        configData->data["customResolution"] =  bCustomResolution;
                        if(resolution.cx != 0 && resolution.cy != 0) {

							float x = 1.f;
							if (!configData->data["resolutionWidth"].isNull())
							{
								x = configData->data["resolutionWidth"].asDouble();
							}

							float y = 1.f;
							if (!configData->data["resolutionHeight"].isNull())
							{
								y = configData->data["resolutionHeight"].asDouble();
							}


                            configData->data["scaleFactor_x"] =  (float)resolution.cx / x;
                            configData->data["scaleFactor_y"] = (float)resolution.cy / y;
                        }
                        configData->data["resolutionWidth"] =  resolution.cx;
                        configData->data["resolutionHeight"] =  resolution.cy;
                        configData->data["frameInterval"] =  UINT(frameInterval);
                        configData->data["flipImage"] =  bFlip;
                        configData->data["flipImageHorizontal"] =  bFlipHorizontal;
                        configData->data["usePointFiltering"] =  bUsePointFiltering;
                        configData->data["gamma"] =  (int)SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);

                        configData->data["preserveSourceSize"]  = bPreserveSourceSize;
                        
                        configData->data["deinterlacingType"] =  deinterlacingConfig.type;
                        configData->data["deinterlacingFieldOrder"] =  deinterlacingConfig.fieldOrder;
                        configData->data["deinterlacingProcessor"] =  deinterlacingConfig.processor;
                        configData->data["deinterlacingDoublesFramerate"] =  deinterlacingConfig.doublesFramerate;

                        //------------------------------------------

						configData->data["ScanInterlace"] = SendMessage(GetDlgItem(hwnd, IDC_INTERLACE), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;

                        BOOL bUDMError;
                        UINT opacity = (UINT)SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_GETPOS32, 0, (LPARAM)&bUDMError);
                        if(bUDMError) opacity = 100;

                        configData->data["opacity"] =  opacity;

                        //------------------------------------------

                        UINT preferredType = -1;
                        int id = (int)SendMessage(GetDlgItem(hwnd, IDC_PREFERREDOUTPUT), CB_GETCURSEL, 0, 0);
                        if(id != -1)
                            preferredType = (UINT)SendMessage(GetDlgItem(hwnd, IDC_PREFERREDOUTPUT), CB_GETITEMDATA, id, 0);

                        BOOL bUsePreferredType = preferredType != -1 /*&& SendMessage(GetDlgItem(hwnd, IDC_USEPREFERREDOUTPUT), BM_GETCHECK, 0, 0) == BST_CHECKED*/;
                        configData->data["usePreferredType"] =  bUsePreferredType;
                        configData->data["preferredType"] =  preferredType;

                        bool bUseBuffering = SendMessage(GetDlgItem(hwnd, IDC_USEBUFFERING), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        configData->data["useBuffering"] =  bUseBuffering;

                        DWORD bufferTime = (DWORD)SendMessage(GetDlgItem(hwnd, IDC_DELAY), UDM_GETPOS32, 0, 0);
                        configData->data["bufferTime"] =  (UINT)bufferTime;

                        //------------------------------------------

                        int soundOutputType = 0;
                        UINT audioDeviceID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_AUDIOLIST), CB_GETCURSEL, 0, 0);

                        if (audioDeviceID != CB_ERR) {
                            if (bAudioDevicesPresent) {
                                String strAudioDevice = GetCBText(GetDlgItem(hwnd, IDC_AUDIOLIST), audioDeviceID);
                                configData->data["audioDevice"] =  WcharToAnsi(strAudioDevice.Array()).c_str();
								configData->data["audioDeviceName"] = WcharToAnsi(configData->audioNameList[audioDeviceID].Array()).c_str();
								String& IDD = configData->audioIDList[audioDeviceID];

								if (IDD.IsEmpty())
								{
									configData->data["audioDeviceID"] = "";
								}
								else
								{
									configData->data["audioDeviceID"] = WcharToAnsi(IDD.Array()).c_str();
								}
								
                            }

                            configData->data["forceCustomAudioDevice"] =  (!configData->bDeviceHasAudio || audioDeviceID > 1);

                            if(SendMessage(GetDlgItem(hwnd, IDC_OUTPUTSOUND), BM_GETCHECK, 0, 0) == BST_CHECKED)
                                soundOutputType = 1;
                            else if(SendMessage(GetDlgItem(hwnd, IDC_PLAYDESKTOPSOUND), BM_GETCHECK, 0, 0) == BST_CHECKED)
                                soundOutputType = 2;
                        }

						//----------------------------------
						// save audio render setting
						UINT audioRenderDeviceID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_AUDIORENDERLIST), CB_GETCURSEL, 0, 0);
						if (audioRenderDeviceID != CB_ERR) {
							configData->data["audioRenderDeviceName"] = WcharToAnsi(configData->audioRenderNameList[audioRenderDeviceID].Array()).c_str();
							configData->data["audioRenderDeviceID"] = WcharToAnsi(configData->audioRenderIDList[audioRenderDeviceID].Array()).c_str();
						}

                        configData->data["soundOutputType"] =  soundOutputType;

                        int soundTimeOffset = (int)SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_GETPOS32, 0, 0);
                        configData->data["soundTimeOffset"] = soundTimeOffset;

                        float fVol = GetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME));
                        configData->data["volume"] = fVol;
                        BOOL bUseAudioRender = SendMessage(GetDlgItem(hwnd, IDC_USEAUDIORENDER), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        configData->data["useAudioRender"] =  bUseAudioRender;

                        //------------------------------------------

                        BOOL bUseChromaKey = SendMessage(GetDlgItem(hwnd, IDC_USECHROMAKEY), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        DWORD color = CCGetColor(GetDlgItem(hwnd, IDC_COLOR));

                        UINT keySimilarity = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_GETPOS32, 0, (LPARAM)&bUDMError);
                        if(bUDMError) keySimilarity = 0;

                        UINT keyBlend = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_GETPOS32, 0, (LPARAM)&bUDMError);
                        if(bUDMError) keyBlend = 10;

                        int keySpillReduction = (int)SendMessage(GetDlgItem(hwnd, IDC_SPILLREDUCTION), UDM_GETPOS32, 0, (LPARAM)&bUDMError);
                        if(bUDMError) keySpillReduction = 0;

                        configData->data["useChromaKey"] =  bUseChromaKey;
                        configData->data["keyColor"] =  (UINT)color;
                        configData->data["keySimilarity"] =  keySimilarity;
                        configData->data["keyBlend"] =  keyBlend;
                        configData->data["keySpillReduction"] =  keySpillReduction;
						configData->data["UseRecorder"] = SendMessage(GetDlgItem(hwnd, IDC_STARTRECORD), BM_GETCHECK, 0, 0) == BST_CHECKED;

						TCHAR BiteText[200];
						GetWindowText(GetDlgItem(hwnd, IDC_RECORDBITRATE), BiteText, sizeof BiteText);
						configData->data["RecorderBitRate"] = String(BiteText).ToInt();
						GetWindowText(GetDlgItem(hwnd, IDC_RECORDPATH), BiteText, sizeof BiteText);
						configData->data["RecordPath"] = WcharToAnsi(BiteText).c_str();

                    }

                case IDCANCEL:
                    if(LOWORD(wParam) == IDCANCEL)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
//                         ImageSource *source = API->GetSceneImageSource(configData->lpName);
// 
//                         if(source)
//                         {
//                             source->SetInt(TEXT("bufferTime"),                      configData->data->GetInt(TEXT("bufferTime"), 0));
// 
//                             source->SetInt(TEXT("timeOffset"),                      configData->data->GetInt(TEXT("soundTimeOffset"), 0));
//                             source->SetFloat(TEXT("volume"),                        configData->data->GetFloat(TEXT("volume"), 1.0f));
// 
//                             source->SetInt(TEXT("flipImage"),                       configData->data->GetInt(TEXT("flipImage"), 0));
//                             source->SetInt(TEXT("flipImageHorizontal"),             configData->data->GetInt(TEXT("flipImageHorizontal"), 0));
//                             source->SetInt(TEXT("usePointFiltering"),               configData->data->GetInt(TEXT("usePointFiltering"), 0));
//                             source->SetInt(TEXT("opacity"),                         configData->data->GetInt(TEXT("opacity"), 100));
// 
//                             source->SetInt(TEXT("useChromaKey"),                    configData->data->GetInt(TEXT("useChromaKey"), 0));
//                             source->SetInt(TEXT("keyColor"),                        configData->data->GetInt(TEXT("keyColor"), 0xFFFFFFFF));
//                             source->SetInt(TEXT("keySimilarity"),                   configData->data->GetInt(TEXT("keySimilarity"), 0));
//                             source->SetInt(TEXT("keyBlend"),                        configData->data->GetInt(TEXT("keyBlend"), 80));
//                             source->SetInt(TEXT("keySpillReduction"),               configData->data->GetInt(TEXT("keySpillReduction"), 50));
//                             source->SetInt(TEXT("gamma"),                           configData->data->GetInt(TEXT("gamma"), 100));
//                             source->SetInt(TEXT("deinterlacingType"),               configData->data->GetInt(TEXT("deinterlacingType"), 0));
//                             source->SetInt(TEXT("deinterlacingFieldOrder"),         configData->data->GetInt(TEXT("deinterlacingFieldOrder"), 0));
//                             source->SetInt(TEXT("deinterlacingProcessor"),          configData->data->GetInt(TEXT("deinterlacingProcessor"), 0));
//                             source->SetInt(TEXT("deinterlacingDoublesFramerate"),   configData->data->GetInt(TEXT("deinterlacingDoublesFramerate"), 0));
// 
//                             source->SetInt(TEXT("preserveSourceSize"),              configData->data->GetInt(TEXT("preserveSourceSize"), 0));
//                             source->SetInt(TEXT("useAudioRender"),                  configData->data->GetInt(TEXT("useAudioRender"), 0));
//                         }

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["DeviceSourceID"].isNull())
						{
							SourceId << configData->data["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
						{
							Video->SetInt(TEXT("bufferTime"), configData->data["bufferTime"].asInt());

							Video->SetInt(TEXT("timeOffset"), configData->data["soundTimeOffset"].asInt());

							float Volum = 1.0f;
							if (!configData->data["volume"].isNull())
							{
								Volum = configData->data["volume"].asDouble();
							}
							Video->SetFloat(TEXT("volume"), Volum);

							Video->SetInt(TEXT("flipImage"), configData->data["flipImage"].asInt());
							Video->SetInt(TEXT("flipImageHorizontal"), configData->data["flipImageHorizontal"].asInt());
							Video->SetInt(TEXT("usePointFiltering"), configData->data["usePointFiltering"].asInt());

							int opacity = 100;

							if (!configData->data["opacity"].isNull())
							{
								opacity = configData->data["opacity"].asInt();
							}

							Video->SetInt(TEXT("opacity"), opacity);

							Video->SetInt(TEXT("useChromaKey"), configData->data["useChromaKey"].asInt());

							UINT keyColor = 0xFFFFFFFF;

							if (!configData->data["keyColor"].isNull())
							{
								keyColor = configData->data["keyColor"].asUInt();
							}

							Video->SetInt(TEXT("keyColor"), keyColor);
							Video->SetInt(TEXT("keySimilarity"), configData->data["keySimilarity"].asInt());

							keyColor = 80;
							if (!configData->data["keyBlend"].isNull())
							{
								keyColor = configData->data["keyBlend"].asInt();
							}
							Video->SetInt(TEXT("keyBlend"), keyColor);

							keyColor = 50;
							if (!configData->data["keySpillReduction"].isNull())
							{
								keyColor = configData->data["keySpillReduction"].asInt();
							}
							Video->SetInt(TEXT("keySpillReduction"), keyColor);

							keyColor = 100;
							if (!configData->data["keySpillReduction"].isNull())
							{
								keyColor = configData->data["keySpillReduction"].asInt();
							}
							Video->SetInt(TEXT("gamma"), keyColor);

							Video->SetInt(TEXT("deinterlacingType"), configData->data["deinterlacingType"].asInt());
							Video->SetInt(TEXT("deinterlacingFieldOrder"), configData->data["deinterlacingFieldOrder"].asInt());
							Video->SetInt(TEXT("deinterlacingProcessor"), configData->data["deinterlacingProcessor"].asInt());
							Video->SetInt(TEXT("deinterlacingDoublesFramerate"), configData->data["deinterlacingDoublesFramerate"].asInt());

							Video->SetInt(TEXT("preserveSourceSize"), configData->data["preserveSourceSize"].asInt());
							Video->SetInt(TEXT("useAudioRender"), configData->data["useAudioRender"].asInt());
						}
                    }

                    EndDialog(hwnd, LOWORD(wParam));
            }
    }

	return false;
}

static bool bFirst = true;

bool STDCALL ConfigureDShowSource(Json::Value &element, bool bCreating)
{

	if (bFirst)
	{
		InitColorControl(hinstMain);
		InitVolumeControl(hinstMain);
		InitVolumeMeter(hinstMain);
		
		pluginLocale = new LocaleStringLookup;

		if (!pluginLocale->LoadStringFile(DSHOWLOCALPATH))
			Log::writeError(LOG_RTSPSERV,1,"Could not open locale string file '%s'", WcharToAnsi(DSHOWLOCALPATH).c_str());

		bFirst = false;
	}

    ConfigDialogData configData(element);
//     configData.lpName = element->GetName();
// 
// 	if (!bCreating)
// 	{
// 		configData.lpName = element->GetString(TEXT("ActualName"), String(configData.lpName).Array());
// 	}

	configData.data = element;
	configData.bGlobalSource = true;// (scmpi(element->GetParent()->GetName(), TEXT("global sources")) == 0);
    configData.bCreating = bCreating;

	INT_PTR Ret = 0;

	if ((Ret = BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG), GetMainWindow(), ConfigureDialogProc, (LPARAM)&configData)) == IDOK)
    {
 //       bool bPreserveSourceSize = data->GetInt(TEXT("preserveSourceSize")) != 0;
        float scaleFactor_x = 1.0f, scaleFactor_y = 1.0f;
// 
//         if(bPreserveSourceSize) {
//             scaleFactor_x = data->GetFloat(TEXT("scaleFactor_x"), 1.0f);
//             scaleFactor_y = data->GetFloat(TEXT("scaleFactor_y"), 1.0f);
//         }

		element["cx"] = element["resolutionWidth"].asDouble() / scaleFactor_x;
		element["cy"] = element["resolutionHeight"].asDouble() / scaleFactor_y;

        return true;
    }

	DWORD Error = GetLastError();

    return false;
}

REGINST_CONFIGFUNC(DeviceSource, ConfigureDShowSource)


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

