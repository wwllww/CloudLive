/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/

#include <tchar.h>
#include "DShowPlugin.h"
#include <mmreg.h>

struct ResSize
{
    UINT cx;
    UINT cy;
};

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include "IVideoCaptureFilter.h"

DWORD STDCALL PackPlanarThread(ConvertData *data);
DWORD STDCALL CheckDeviceThread(LPVOID *pData);

#define NEAR_SILENT  3000
#define NEAR_SILENTf 3000.0




IMPLEMENT_DYNIC(DeviceSource, "视频捕捉源", "1.0.0.1")
DeviceSource::DeviceSource()
{
	bCapturing = false;
	m_pEvent = NULL;
	m_bCheckAudioDevice = false;
	hThreadCheckDevice = NULL;
	//m_pWaveOut          = NULL;
	hSampleThread = NULL;
	hStopSampleEvent = NULL;
	drawShader = NULL;
	m_hCheckExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	bRequestVolume = true;
	hAudioMutex = OSCreateMutex();
	bHasPre = false;

	texture = NULL;
	previousTexture = NULL;
	bFiltersLoaded = false;
	D3DRender = GetD3DRender();
	colorConvertShader = NULL;
	colorFieldConvertShader = NULL;
	RGBFieldShader = D3DRender->CreatePixelShaderFromFile(L"shaders/Field_RGB.pShader");

	lpImageBuffer = NULL;

	capture = NULL;
	graph = NULL;
	m_pEvent = NULL;
	audioOut = NULL;
	latestVideoSample = NULL;
	audioDeviceFilter = NULL;
	deviceFilter = NULL;
	audioFilter = NULL;
	enteredSceneCount = 0;
	m_bEnterScene = true;
	control = NULL;
	captureFilter = NULL;
	imageCX = imageCY = 0;
	oldDeinterlacerType = 0;
	hSampleMutex = NULL;
	colorMutex = NULL;
	ListCallBackMutex = NULL;
	hConvertThreads = NULL;
	convertData = NULL;
	bNeedCheck = true;
	bIsFieldSignal = false;
}
bool DeviceSource::Init(Value &data)
{
	fNewVol = 1.0f;
	if (!data["volume"].isNull())
		fNewVol = data["volume"].asDouble();

	if (!hSampleMutex)
		hSampleMutex = OSCreateMutex();

    if(!hSampleMutex)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: could not create sample mutex");
        return false;
    }

	if(!colorMutex)
		colorMutex = OSCreateMutex();

	if (!colorMutex)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: could not create colorMutex mutex");
		return false;
	}

	if (!ListCallBackMutex)
		ListCallBackMutex = OSCreateMutex();

	if (!ListCallBackMutex)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: could not create ListCallBackMutex mutex");
		return false;
	}

    int numThreads = MAX(OSGetTotalCores()-2, 1);
	if (!hConvertThreads)
	{
		hConvertThreads = (HANDLE*)Allocate_Bak(sizeof(HANDLE)*numThreads);
		zero(hConvertThreads, sizeof(HANDLE)*numThreads);
	}

	if (!convertData)
	{
		convertData = (ConvertData*)Allocate_Bak(sizeof(ConvertData)*numThreads);
		zero(convertData, sizeof(ConvertData)*numThreads);
	}

	this->data = data;

   Log::writeMessage(LOG_RTSPSERV, 1, "Using directshow input");

	UpdateSettings(data);
	return bCapturing;
}


#define SHADER_PATH TEXT("./shaders/")

String DeviceSource::ChooseShader(bool bNeedField)
{
	if (colorType == DeviceOutputType_RGB && !bUseChromaKey)
	{
		if (bIsFieldSignal)
		{
			String strShader;
			strShader << SHADER_PATH << L"Field_RGB.pShader";
			return strShader;
		}

		return String();
	}

    String strShader;
    strShader << SHADER_PATH;

	if (bNeedField)
	{
		strShader << L"Field_";
		bUseChromaKey = false;//这里如果bUseChromaKey = true的话有名字会有问题，强制为false
	}

    if(bUseChromaKey)
        strShader << TEXT("ChromaKey_");

	if (colorType == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
    else if(colorType == DeviceOutputType_YV12)
        strShader << TEXT("YVUToRGB.pShader");
    else if(colorType == DeviceOutputType_YVYU)
        strShader << TEXT("YVXUToRGB.pShader");
    else if(colorType == DeviceOutputType_YUY2)
        strShader << TEXT("YUXVToRGB.pShader");
    else if(colorType == DeviceOutputType_UYVY)
        strShader << TEXT("UYVToRGB.pShader");
    else if(colorType == DeviceOutputType_HDYC)
        strShader << TEXT("HDYCToRGB.pShader");
    else
	{
		strShader.Clear();
	}

    return strShader;
}

String DeviceSource::ChooseDeinterlacingShader()
{
    String shader;
    shader << SHADER_PATH << TEXT("Deinterlace_");

#ifdef _DEBUG
#define DEBUG__ _DEBUG
#undef _DEBUG
#endif
#define SELECT(x) case DEINTERLACING_##x: shader << String(TEXT(#x)).MakeLower(); break;
    switch(deinterlacer.type)
    {
        SELECT(RETRO)
        SELECT(BLEND)
        SELECT(BLEND2x)
        SELECT(LINEAR)
        SELECT(LINEAR2x)
        SELECT(YADIF)
        SELECT(YADIF2x)
        SELECT(_DEBUG)
    }
    return shader << TEXT(".pShader");
#undef SELECT
#ifdef DEBUG__
#define _DEBUG DEBUG__
#undef DEBUG__
#endif
}

const float yuv709Mat[16] = {0.182586f,  0.614231f,  0.062007f, 0.062745f,
                            -0.100644f, -0.338572f,  0.439216f, 0.501961f,
                             0.439216f, -0.398942f, -0.040274f, 0.501961f,
                             0.000000f,  0.000000f,  0.000000f, 1.000000f};

const float yuvMat[16] = {0.256788f,  0.504129f,  0.097906f, 0.062745f,
                         -0.148223f, -0.290993f,  0.439216f, 0.501961f,
                          0.439216f, -0.367788f, -0.071427f, 0.501961f,
                          0.000000f,  0.000000f,  0.000000f, 1.000000f};


void DeviceSource::SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType)
{
    expectedAudioType = audioMediaType->subtype;

    if(audioMediaType->formattype == FORMAT_WaveFormatEx)
    {
        WAVEFORMATEX *pFormat = reinterpret_cast<WAVEFORMATEX*>(audioMediaType->pbFormat);
        mcpy(&audioFormat, pFormat, sizeof(audioFormat));

		Log::writeMessage(LOG_RTSPSERV, 1, "    device audio info - bits per sample: %u, channels: %u, samples per sec: %u, block size: %u",
            audioFormat.wBitsPerSample, audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.nBlockAlign);

        //avoid local resampling if possible
        /*if(pFormat->nSamplesPerSec != 44100)
        {
            pFormat->nSamplesPerSec = 44100;
            if(SUCCEEDED(audioConfig->SetFormat(audioMediaType)))
            {
                Log(TEXT("    also successfully set samples per sec to 44.1k"));
                audioFormat.nSamplesPerSec = 44100;
            }
        }*/
    }
    else
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Audio format was not a normal wave format");
        soundOutputType = 0;
    }

    DeleteMediaType(audioMediaType);
}
//#include <winnt.h>
typedef TCHAR* PTCHAR;
//#include "streams.h"

//#include "wxdebug.h"

static IBaseFilter * FindRenderDevice( BSTR p_devicename, bool b_audio)
{
	IBaseFilter *p_base_filter = NULL;
	IMoniker *p_moniker = NULL;
	ULONG i_fetched;
	HRESULT hr;

	/* Create the system device enumerator */
	ICreateDevEnum *p_dev_enum = NULL;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **)&p_dev_enum);
	if (FAILED(hr))
	{
		Log::writeError(LOG_RTSPSERV, 1, "failed to create the device enumerator (0x%lx))", hr);
		return NULL;
	}

	/* Create an enumerator for the video render devices */
	IEnumMoniker *p_class_enum = NULL;
	if (!b_audio)
		hr = p_dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&p_class_enum, 0);
	else
		hr = p_dev_enum->CreateClassEnumerator(CLSID_AudioRendererCategory, &p_class_enum, 0);
	p_dev_enum->Release();
	if (FAILED(hr))
	{
		Log::writeError(LOG_RTSPSERV, 1, "failed to create the class enumerator (0x%lx)", hr);
		return NULL;
	}

	/* If there are no enumerators for the requested type, then
	* CreateClassEnumerator will succeed, but p_class_enum will be NULL */
	if (p_class_enum == NULL)
	{
		Log::writeError(LOG_RTSPSERV, 1, "no %s render device was detected", (b_audio ? "audio" : "video"));
		return NULL;
	}

	/* Enumerate the devices */

	/* Note that if the Next() call succeeds but there are no monikers,
	* it will return S_FALSE (which is not a failure). Therefore, we check
	* that the return code is S_OK instead of using SUCCEEDED() macro. */

	while (p_class_enum->Next(1, &p_moniker, &i_fetched) == S_OK)
	{
		/* Getting the property page to get the device name */
		IPropertyBag *p_bag;
		hr = p_moniker->BindToStorage(0, 0, IID_IPropertyBag,(void **)&p_bag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = p_bag->Read(L"FriendlyName", &var, NULL);
			p_bag->Release();
			if (SUCCEEDED(hr))
			{
				BSTR devname = var.bstrVal;
				
				if (p_devicename && _tcsstr(devname, p_devicename) != NULL)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "asked for %s, binding to %s", WcharToAnsi(p_devicename).c_str(), devname);
					/* NULL possibly means we don't need BindMoniker BindCtx ?? */
					hr = p_moniker->BindToObject(NULL, 0, IID_IBaseFilter, (void **)&p_base_filter);
					if (FAILED(hr))
					{
						Log::writeError(LOG_RTSPSERV, 1, "couldn't bind moniker to filter object (0x%lx)", hr);
						p_moniker->Release();
						p_class_enum->Release();
						return NULL;
					}
					p_moniker->Release();
					p_class_enum->Release();
					return p_base_filter;
				}
			}
		}
		p_moniker->Release();
	}
	p_class_enum->Release();
	return NULL;
}

bool DeviceSource::LoadFilters()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
	if (bCapturing || bFiltersLoaded)
		return false;

	bool bSucceeded = false;

	List<MediaOutputInfo> outputList;
	IAMStreamConfig *config = NULL;
	bool bAddedVideoCapture = false, bAddedAudioCapture = false, bAddedDevice = false;
	GUID expectedMediaType;
	IPin *devicePin = NULL, *audioPin = NULL;
	HRESULT err;

	deinterlacer.isReady = true;

	if (graph == NULL) {
		err = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_IFilterGraph, (void**)&graph);
		if (FAILED(err))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to build IGraphBuilder, result = %08lX", err);
			goto cleanFinish;
		}
	}

	if (capture == NULL) {
		err = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_ICaptureGraphBuilder2, (void**)&capture);
		if (FAILED(err))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to build ICaptureGraphBuilder2, result = %08lX", err);
			goto cleanFinish;
		}

		capture->SetFiltergraph(graph);
	}
	// 事件
	err = graph->QueryInterface(IID_IMediaEventEx, (void**)&m_pEvent);
	if (FAILED(err))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: QueryInterface(IID_IMediaEventEx, (void**)&pEvent) = %08lX", err);
		return false;
	}
	bUseThreadedConversion = false;//API->UseMultithreadedOptimizations() && (OSGetTotalCores() > 1);

	//------------------------------------------------
	// basic initialization vars

	bool bForceCustomAudio = data["forceCustomAudioDevice"].asInt() != 0;
    bUseAudioRender = data["useAudioRender"].asInt() != 0;

    bUseCustomResolution = data["customResolution"].asInt();
    strDevice = Asic2WChar(data["device"].asString()).c_str();
	strDeviceName = Asic2WChar(data["deviceName"].asString()).c_str();
	strDeviceID = Asic2WChar(data["deviceID"].asString()).c_str();
	if (!data["audioDevice"].isNull())
		strAudioDevice = Asic2WChar(data["audioDevice"].asString()).c_str();

	if (!data["audioDeviceName"].isNull())
		strAudioName = Asic2WChar(data["audioDeviceName"].asString()).c_str();

	if (!data["audioDeviceID"].isNull())
		strAudioID = Asic2WChar(data["audioDeviceID"].asString()).c_str();

	if (!data["audioRenderDeviceName"].isNull())
		strAudioRenderName = Asic2WChar(data["audioRenderDeviceName"].asString()).c_str();

	if (!data["audioRenderDeviceID"].isNull())
		strAudioRenderID = Asic2WChar(data["audioRenderDeviceID"].asString()).c_str();

    bFlipVertical = data["flipImage"].asInt() != 0;
    bFlipHorizontal = data["flipImageHorizontal"].asInt() != 0;
    bUsePointFiltering = data["usePointFiltering"].asInt() != 0;

    bool elgato = sstri(strDeviceName, L"elgato") != nullptr;

	opacity = 100;
	if (!data["opacity"].isNull())
	{
		opacity = data["opacity"].asDouble();
	}
	
    float volume = 1.0f;

	if (!data["volume"].isNull())
	{
		volume = data["volume"].asDouble();
	}

    bUseBuffering = data["useBuffering"].asInt() != 0;
    bufferTime = data["bufferTime"].asInt()*10000;

    //------------------------------------------------
    // chrom key stuff

    bUseChromaKey = data["useChromaKey"].asInt() != 0;
    keyColor = 0xFFFFFFFF;

	if (!data["keyColor"].isNull())
	{
		keyColor = data["keyColor"].asUInt();
	}

    keySimilarity = data["keySimilarity"].asInt();
    keyBlend = 80;

	if (!data["keyBlend"].isNull())
	{
		keyBlend = data["keyBlend"].asInt();
	}

    keySpillReduction = 50;

	if (!data["keySpillReduction"].isNull())
	{
		keySpillReduction = data["keySpillReduction"].asInt();
	}
    
    deinterlacer.type               = data["deinterlacingType"].asInt();
    deinterlacer.fieldOrder         = data["deinterlacingFieldOrder"].asInt();
    deinterlacer.processor          = data["deinterlacingProcessor"].asInt();
    deinterlacer.doublesFramerate   = data["deinterlacingDoublesFramerate"].asInt() != 0;
	deinterlacer.needsPreviousFrame = false;

    if(keyBaseColor.x < keyBaseColor.y && keyBaseColor.x < keyBaseColor.z)
        keyBaseColor -= keyBaseColor.x;
    else if(keyBaseColor.y < keyBaseColor.x && keyBaseColor.y < keyBaseColor.z)
        keyBaseColor -= keyBaseColor.y;
    else if(keyBaseColor.z < keyBaseColor.x && keyBaseColor.z < keyBaseColor.y)
        keyBaseColor -= keyBaseColor.z;

    //------------------------------------------------
    // get the device filter and pins

    if(strDeviceName.IsValid())
        deviceFilter = GetDeviceByValue(CLSID_VideoInputDeviceCategory, L"FriendlyName", strDeviceName, L"DevicePath", strDeviceID);
    else
    {
        if(!strDevice.IsValid())
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Invalid device specified");
            goto cleanFinish;
        }

        deviceFilter = GetDeviceByValue(CLSID_VideoInputDeviceCategory, L"FriendlyName", strDevice);
    }
    
    if(!deviceFilter)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not create device filter");
        goto cleanFinish;
    }

    devicePin = GetOutputPin(deviceFilter, &MEDIATYPE_Video);
    if(!devicePin)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not get device video pin");
        goto cleanFinish;
    }

    soundOutputType = data["soundOutputType"].asInt(); //0 is for backward-compatibility
    if (strAudioID.CompareI(TEXT("Disabled")))
        soundOutputType = 0;

    if(soundOutputType != 0)
    {
        if(!bForceCustomAudio)
        {
            err = capture->FindPin(deviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &audioPin);
            bDeviceHasAudio = SUCCEEDED(err);
        }
        else
            bDeviceHasAudio = false;

        if(!bDeviceHasAudio)
        {
            if(strDeviceName.IsValid())
            {
                audioDeviceFilter = GetDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", strAudioName, L"DevicePath", strAudioID);
				if (!audioDeviceFilter)
				{
					if (strAudioName.Array() && strAudioID.Array())
						Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Invalid audio device: name '%s', path '%s'", WcharToAnsi(strAudioName.Array()).c_str(), WcharToAnsi(strAudioID.Array()).c_str());
					else if (strAudioName.Array())
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Invalid audio device: name '%s'", WcharToAnsi(strAudioName.Array()).c_str());
					}
				}
            }
            else if(strAudioDevice.IsValid())
            {
                audioDeviceFilter = GetDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", strAudioDevice);
                if(!audioDeviceFilter)
					Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not create audio device filter");
            }

            if(audioDeviceFilter)
                err = capture->FindPin(audioDeviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &audioPin);
            else
                err = E_FAIL;
        }

        if(FAILED(err) || !audioPin)
        {
			Log::writeError(LOG_RTSPSERV, 1, "DShowPlugin: No audio pin, result = %lX", err);
            soundOutputType = 0;
        }
    }
    else
        bDeviceHasAudio = bForceCustomAudio = false;

    int soundTimeOffset = data["soundTimeOffset"].asInt();

    GetOutputList(devicePin, outputList);

    //------------------------------------------------
    // initialize the basic video variables and data

    if(FAILED(err = devicePin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not get IAMStreamConfig for device pin, result = %08lX", err);
        goto cleanFinish;
    }

    //renderCX = renderCY = newCX = newCY = 0;
    frameInterval = 0;

    UINT elgatoCX = 1280;
    UINT elgatoCY = 720;

    if(bUseCustomResolution)
    {
        /*renderCX = */newCX = data["resolutionWidth"].asInt();
        /*renderCY = */newCY = data["resolutionHeight"].asInt();
        frameInterval = data["frameInterval"].asInt();
    }
    else
    {
        SIZE size;
        size.cx = 0;
        size.cy = 0;

        // blackmagic/decklink devices will display a black screen if the resolution/fps doesn't exactly match.
        // they should rename the devices to blackscreen
        if (sstri(strDeviceName, L"blackmagic") != NULL || sstri(strDeviceName, L"decklink") != NULL ||
            !GetClosestResolutionFPS(outputList, size, frameInterval, true))
        {
            AM_MEDIA_TYPE *pmt;
            if (SUCCEEDED(config->GetFormat(&pmt))) {
                VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);

                // Use "preferred" format from the device
                size.cx = pVih->bmiHeader.biWidth;
                size.cy = pVih->bmiHeader.biHeight;
                frameInterval = pVih->AvgTimePerFrame;

                DeleteMediaType(pmt);
            } else {
                if (!outputList.Num()) {
					Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Not even an output list!  What the f***");
                    goto cleanFinish;
                }

                /* ..........elgato */
                size.cx = outputList[0].maxCX;
                size.cy = outputList[0].maxCY;
                frameInterval = outputList[0].minFrameInterval;
            }
        }

        /*renderCX = */newCX = size.cx;
        /*renderCY = */newCY = size.cy;
    }

    /* elgato always starts off at 720p and changes after. */
    if (elgato)
    {
        elgatoCX = renderCX;
        elgatoCY = renderCY;

        /*renderCX = */newCX = 1280;
        /*renderCY = */newCY = 720;
    }

	if (!newCX || !newCY || !frameInterval)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Invalid size/fps specified");
        goto cleanFinish;
    }

    preferredOutputType = (data["usePreferredType"].asInt() != 0) ? data["preferredType"].asUInt() : -1;

    bFirstFrame = true;

    //------------------------------------------------
    // get the closest media output for the settings used

	Log::writeError(LOG_RTSPSERV, 1, "DShowPlugin: Find the appropriate resolution %d X %d", newCX, newCY);

	if (!bNeedCheck)
	{
		//这里需要检测线程的不直接到cleanFinish
		bSucceeded = true;
		goto cleanFinish;
	}

	MediaOutputInfo *bestOutput = GetBestMediaOutput(outputList, newCX, newCY, preferredOutputType, frameInterval);
    if(!bestOutput)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not find appropriate resolution use outputList[0] instead!outputList = %d", outputList.Num());
        if (!outputList.Num()) {
            Log::writeError(LOG_RTSPSERV,1,"DShowPlugin: Could not find appropriate resolution to create device image source");
            goto cleanFinish;
        } else { /* エルガット＝自殺 */
            bestOutput = &outputList[0];
			//frameInterval = bestOutput->minFrameInterval;
			frameInterval = 333333;
			/*renderCX = */newCX = bestOutput->maxCX;
			/*renderCY = */newCY = bestOutput->maxCY;
        }
    }

    //------------------------------------------------
    // log video info

    {
        String strTest = FormattedString(TEXT("    device: %s,\r\n    device id %s,\r\n    chosen type: %s, usingFourCC: %s, res: %ux%u - %ux%u, frameIntervals: %llu-%llu\r\n    use buffering: %s - %u"),
            strDevice.Array(), strDeviceID.Array(),
            EnumToName[(int)bestOutput->videoType],
            bestOutput->bUsingFourCC ? TEXT("true") : TEXT("false"),
            bestOutput->minCX, bestOutput->minCY, bestOutput->maxCX, bestOutput->maxCY,
            bestOutput->minFrameInterval, bestOutput->maxFrameInterval,
	    bUseBuffering ? L"true" : L"false", bufferTime);

        BITMAPINFOHEADER *bmiHeader = GetVideoBMIHeader(bestOutput->mediaType);

        char fourcc[5];
        mcpy(fourcc, &bmiHeader->biCompression, 4);
        fourcc[4] = 0;

        if(bmiHeader->biCompression > 1000)
            strTest << FormattedString(TEXT(", fourCC: '%S'\r\n"), fourcc);
        else
            strTest << FormattedString(TEXT(", fourCC: %08lX\r\n"), bmiHeader->biCompression);

        if(!bDeviceHasAudio) strTest << FormattedString(TEXT("    audio device: %s,\r\n    audio device id %s,\r\n    audio time offset %d,\r\n"), strAudioDevice.Array(), strAudioID.Array(), soundTimeOffset);

		Log::writeMessage(LOG_RTSPSERV, 1, "------------------------------------------");
		Log::writeMessage(LOG_RTSPSERV, 1, "%s", WcharToAnsi(strTest.Array()).c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "Render %d X %d", newCX, newCY);
    }

    //------------------------------------------------
    // set up shaders and video output data

    expectedMediaType = bestOutput->mediaType->subtype;

    colorType = DeviceOutputType_RGB;
    if(bestOutput->videoType == VideoOutputType_I420)
        colorType = DeviceOutputType_I420;
    else if(bestOutput->videoType == VideoOutputType_YV12)
        colorType = DeviceOutputType_YV12;
    else if(bestOutput->videoType == VideoOutputType_YVYU)
        colorType = DeviceOutputType_YVYU;
    else if(bestOutput->videoType == VideoOutputType_YUY2)
        colorType = DeviceOutputType_YUY2;
    else if(bestOutput->videoType == VideoOutputType_UYVY)
        colorType = DeviceOutputType_UYVY;
    else if(bestOutput->videoType == VideoOutputType_HDYC)
        colorType = DeviceOutputType_HDYC;
    else
    {
        colorType = DeviceOutputType_RGB;
        expectedMediaType = MEDIASUBTYPE_RGB32;
    }

    //------------------------------------------------
    // set chroma details

    keyBaseColor = Color4().MakeFromRGBA(keyColor);
    Matrix4x4TransformVect(keyChroma, (colorType == DeviceOutputType_HDYC || colorType == DeviceOutputType_RGB) ? (float*)yuv709Mat : (float*)yuvMat, keyBaseColor);
    keyChroma *= 2.0f;

    //------------------------------------------------
    // configure video pin

    AM_MEDIA_TYPE outputMediaType;
    CopyMediaType(&outputMediaType, bestOutput->mediaType);

    VIDEOINFOHEADER *vih  = reinterpret_cast<VIDEOINFOHEADER*>(outputMediaType.pbFormat);
    BITMAPINFOHEADER *bmi = GetVideoBMIHeader(&outputMediaType);
	vih->AvgTimePerFrame  = frameInterval;
    bmi->biWidth          = newCX;
    bmi->biHeight         = newCY;
	bmi->biSizeImage      = newCX*newCY*(bmi->biBitCount >> 3);


    if(FAILED(err = config->SetFormat(&outputMediaType)))
    {
        if(err != E_NOTIMPL)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: SetFormat on device pin failed, result = %08lX", err);
            goto cleanFinish;
        }
    }

    FreeMediaType(outputMediaType);

    //------------------------------------------------
    // get audio pin configuration, optionally configure audio pin to 44100

    GUID expectedAudioType;

    if(soundOutputType == 1)
    {
        IAMStreamConfig *audioConfig;
        if(SUCCEEDED(audioPin->QueryInterface(IID_IAMStreamConfig, (void**)&audioConfig)))
        {
            AM_MEDIA_TYPE *audioMediaType;
            if(SUCCEEDED(err = audioConfig->GetFormat(&audioMediaType)))
            {
                SetAudioInfo(audioMediaType, expectedAudioType);
            }
            else if(err == E_NOTIMPL) //elgato probably
            {
                IEnumMediaTypes *audioMediaTypes;
                if(SUCCEEDED(err = audioPin->EnumMediaTypes(&audioMediaTypes)))
                {
                    ULONG i = 0;
                    if((err = audioMediaTypes->Next(1, &audioMediaType, &i)) == S_OK)
                        SetAudioInfo(audioMediaType, expectedAudioType);
                    else
                    {
						Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: audioMediaTypes->Next failed, result = %08lX", err);
                        soundOutputType = 0;
                    }

                    audioMediaTypes->Release();
                }
                else
                {
					Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: audioMediaTypes->Next failed, result = %08lX", err);
                    soundOutputType = 0;
                }
            }
            else
            {
				Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Could not get audio format, result = %08lX", err);
                soundOutputType = 0;
            }

            audioConfig->Release();
        }
        else {
            soundOutputType = 0;
        }
    }

    //------------------------------------------------
    // add video capture filter if any

    captureFilter = new CaptureFilter(this, MEDIATYPE_Video, expectedMediaType);

    if(FAILED(err = graph->AddFilter(captureFilter, NULL)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to add video capture filter to graph, result = %08lX", err);
        goto cleanFinish;
    }

    bAddedVideoCapture = true;

    //------------------------------------------------
    // add audio capture filter if any

    if(soundOutputType == 1)
    {
        audioFilter = new CaptureFilter(this, MEDIATYPE_Audio, expectedAudioType);
        if(!audioFilter)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "Failed to create audio capture filter");
            soundOutputType = 0;
        }
    }
    else if(soundOutputType == 2)
    {
//		audioFilter = FindRenderDevice(TEXT("Realtek Digital Output"), true);
//		audioFilter = FindRenderDevice(TEXT("扬声器"), true);
		audioFilter = FindRenderDevice(strAudioRenderName, true);
		if (audioFilter == NULL){
			if (bUseAudioRender) {
				if (FAILED(err = CoCreateInstance(CLSID_AudioRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&audioFilter)))
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: failed to create WaveOut Audio renderer, result = %08lX", err);
					soundOutputType = 0;
				}
			}
			else {
				if (FAILED(err = CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&audioFilter)))
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: failed to create DirectSound renderer, result = %08lX", err);
					soundOutputType = 0;
				}
			}
		}
        IBasicAudio *basicAudio;
        if(SUCCEEDED(audioFilter->QueryInterface(IID_IBasicAudio, (void**)&basicAudio)))
        {
            long lVol = long((double(volume)*NEAR_SILENTf)-NEAR_SILENTf);
            if(lVol <= -NEAR_SILENT)
                lVol = -10000;
            basicAudio->put_Volume(lVol);
            basicAudio->Release();
        }
    }

    if(soundOutputType != 0)
    {
        if(FAILED(err = graph->AddFilter(audioFilter, NULL)))
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to add audio capture filter to graph, result = %08lX", err);

        bAddedAudioCapture = true;
    }

    //------------------------------------------------
    // add primary device filter

    if(FAILED(err = graph->AddFilter(deviceFilter, NULL)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to add device filter to graph, result = %08lX", err);
        goto cleanFinish;
    }

    if(soundOutputType != 0 && !bDeviceHasAudio)
    {
        if(FAILED(err = graph->AddFilter(audioDeviceFilter, NULL)))
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to add audio device filter to graph, result = %08lX", err);
    }

    bAddedDevice = true;

    //------------------------------------------------
    // change elgato resolution
    if (elgato)
    {
        /* choose closest matching elgato resolution */
        if (!bUseCustomResolution)
        {
            UINT baseCX, baseCY;
            UINT closest = 0xFFFFFFFF;
            GetBaseSize(baseCX, baseCY);

            const ResSize resolutions[] = {{480, 360}, {640, 480}, {1280, 720}, {1920, 1080}};
            for (const ResSize &res : resolutions) {
                UINT val = (UINT)labs((long)res.cy - (long)baseCY);
                if (val < closest) {
                    elgatoCX = res.cx;
                    elgatoCY = res.cy;
                    closest = val;
                }
            }
        }

        IElgatoVideoCaptureFilter3 *elgatoFilter = nullptr;
        VIDEO_CAPTURE_FILTER_SETTINGS settings;

        if (SUCCEEDED(deviceFilter->QueryInterface(IID_IElgatoVideoCaptureFilter3, (void**)&elgatoFilter)))
        {
            if (SUCCEEDED(elgatoFilter->GetSettings(&settings)))
            {
                if (elgatoCY == 1080)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_1080;
                else if (elgatoCY == 480)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_480;
                else if (elgatoCY == 360)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_360;
                else
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_720;

                elgatoFilter->SetSettings(&settings);
            }

            elgatoFilter->Release();
        }
    }
	bool bConnected = false;
	// audio render first
	if (soundOutputType != 0)
	{
		if (elgato && bDeviceHasAudio)
		{
			bConnected = false;

			IPin *audioPin = GetOutputPin(deviceFilter, &MEDIATYPE_Audio);
			if (audioPin)
			{
				IPin* audioRendererPin = NULL;

				// FMB NOTE: Connect with first (= the only) pin of audio renderer
				IEnumPins* pIEnum = NULL;
				if (SUCCEEDED(err = audioFilter->EnumPins(&pIEnum)))
				{
					IPin* pIPin = NULL;
					pIEnum->Next(1, &audioRendererPin, NULL);
					SafeRelease(pIEnum);
				}

				if (audioRendererPin)
				{
					bConnected = SUCCEEDED(err = graph->ConnectDirect(audioPin, audioRendererPin, nullptr));
					audioRendererPin->Release();
				}
				audioPin->Release();
			}
		}
		else
		{
			IBaseFilter *aFilter = audioDeviceFilter;
			if (bDeviceHasAudio)
			{
				aFilter = deviceFilter;
			}

			IPin *audioPin = GetOutputPin(aFilter, &MEDIATYPE_Audio);
			if (audioPin)
			{
				IAMBufferNegotiation *pNeg;
				audioPin->QueryInterface(IID_IAMBufferNegotiation, (void **)&pNeg);
				// Set the buffer size based on selected settings  
				ALLOCATOR_PROPERTIES prop = { 0 };
				//HRESULT hrRet = pNeg->GetAllocatorProperties(&prop);
				prop.cBuffers = 500/40;
				prop.cbBuffer = audioFormat.nSamplesPerSec * 40 / (1000);
				prop.cbAlign = audioFormat.wBitsPerSample * audioFormat.nChannels /8;
				prop.cbPrefix = 0;

				HRESULT hr = pNeg->SuggestAllocatorProperties(&prop);
				pNeg->Release();
				Log::writeMessage(LOG_RTSPSERV, 1, "设置音频参数%d", prop.cbAlign);
     		}
			//m_strReanderName = BLiveGetRendAudioDevice(false, false);
/*			if (NULL == m_pWaveOut)
			{				
				if (!m_strReanderName.Compare(TEXT("Disable")))
				{
					m_pWaveOut = new AudioWaveOut;
				}				
			}
			if (NULL != m_pWaveOut)
			{
				AudioWaveOut *pWave = (AudioWaveOut *)m_pWaveOut;
				Log(L"音频渲染设备%s", m_strReanderName.Array());
				pWave->Initialize(m_strReanderName.Array(), audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.wBitsPerSample);
			}	*/		
			if (!bDeviceHasAudio)
				bConnected = SUCCEEDED(err = capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, audioDeviceFilter, NULL, audioFilter));
			else
				bConnected = SUCCEEDED(err = capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, deviceFilter, NULL, audioFilter));
		}

		if (!bConnected)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to connect the audio device pin to the audio capture pin, result = %08lX", err);
			soundOutputType = 0;
		}
	}
    //------------------------------------------------
    // connect all pins and set up the whole capture thing

    if (elgato)
    {
        bConnected = SUCCEEDED(err = graph->ConnectDirect(devicePin, captureFilter->GetCapturePin(), nullptr));
        if (!bConnected)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to connect the video device pin to the video capture pin, result = %08lX", err);
            goto cleanFinish;
        }
    }
    else
    {
        bConnected = SUCCEEDED(err = capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, deviceFilter, NULL, captureFilter));
        if(!bConnected)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to RenderStream the video device pin to the video capture pin, result = %08lX", err);
            if(FAILED(err = graph->Connect(devicePin, captureFilter->GetCapturePin())))
            {
				Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to connect the video device pin to the video capture pin, result = %08lX", err);
                goto cleanFinish;
            }
        }
    }

    if(FAILED(err = graph->QueryInterface(IID_IMediaControl, (void**)&control)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to get IMediaControl, result = %08lX", err);
        goto cleanFinish;
    }
//	EnumFilters(graph);
//	DumpGraph(graph, 0);

    if (bUseBuffering) {
        if (!(hStopSampleEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to create stop event", err);
            goto cleanFinish;
        }

        if (!(hSampleThread = OSCreateThread((XTHREAD)SampleThread, this))) {
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: Failed to create sample thread", err);
            goto cleanFinish;
        }
    }

// 	if (soundOutputType == 1 && (audioOut == NULL && enteredSceneCount))
//     {
//         audioOut = new DeviceAudioSource;
//         audioOut->Initialize(this);
// 
//         audioOut->SetAudioOffset(soundTimeOffset);
//         audioOut->SetVolume(volume);
//     }
// 	else if (soundOutputType == 1 && audioOut)
// 	{
// 		audioOut->Initialize(this);
// 
// 		audioOut->SetAudioOffset(soundTimeOffset);
// 		audioOut->SetVolume(volume);
// 	}

    bSucceeded = true;
	Log::writeMessage(LOG_RTSPSERV,1,"DeviceSource Name %s, FPS %u, collor %s, 分辨率 %d X %d",
		WcharToAnsi(strDeviceName.Array()).c_str(), INT64(10000000.0 / double(frameInterval)), EnumToName[bestOutput->videoType], newCX, newCY);
cleanFinish:
    SafeRelease(config);
    SafeRelease(devicePin);
    SafeRelease(audioPin);

    for(UINT i=0; i<outputList.Num(); i++)
        outputList[i].FreeData();

    if(!bSucceeded)
    {
        bCapturing = false;

        if(bAddedVideoCapture)
            graph->RemoveFilter(captureFilter);
        if(bAddedAudioCapture)
            graph->RemoveFilter(audioFilter);

        if(bAddedDevice)
        {
            if(!bDeviceHasAudio && audioDeviceFilter)
                graph->RemoveFilter(audioDeviceFilter);
            graph->RemoveFilter(deviceFilter);
        }

        SafeRelease(audioDeviceFilter);
        SafeRelease(deviceFilter);
        SafeRelease(captureFilter);
        SafeRelease(audioFilter);
        SafeRelease(control);
		SafeReleaseLogRef(m_pEvent);

        if (hSampleThread) {
            SetEvent(hStopSampleEvent);
            WaitForSingleObject(hSampleThread, INFINITE);
            CloseHandle(hSampleThread);
            hSampleThread = NULL;
        }

        if (hStopSampleEvent) {
            CloseHandle(hStopSampleEvent);
            hStopSampleEvent = NULL;
        }

        if(colorConvertShader)
        {
            delete colorConvertShader;
            colorConvertShader = NULL;
        }

        soundOutputType = 0;

        if(lpImageBuffer)
        {
            Free(lpImageBuffer);
            lpImageBuffer = NULL;
        }

        bReadyToDraw = true;
    }
    else
        bReadyToDraw = false;

    // Updated check to ensure that the source actually turns red instead of
    // screwing up the size when SetFormat fails.
//     if (renderCX <= 0 || renderCX >= 8192) { newCX = renderCX = 32; imageCX = renderCX; }
//     if (renderCY <= 0 || renderCY >= 8192) { newCY = renderCY = 32; imageCY = renderCY; }

	if (newCX <= 0 || newCX >= 8192) { newCX = renderCX = 32; imageCX = renderCX; }
	if (newCY <= 0 || newCY >= 8192) { newCY = renderCY = 32; imageCY = renderCY; }

	bFiltersLoaded = bLoadSucceed = bSucceeded;

// 	if (bSucceeded&&m_transform)
// 	{
// 		m_transform->SetResolution(renderCX, renderCY);
// 	}
	
	/*Log(TEXT("DeviceSource Name %s, id %s, FPS %u, collor %s, 分辨率 %u*%u, audio name %s, auido id %s"),
		strDeviceName.Array(), strDeviceID.Array(), INT64(10000000.0 / double(frameInterval)), EnumToName[bestOutput->videoType], renderCX, renderCY,
		strAudioName.Array(), strAudioID.Array());*/

	Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: %s end! bSucceeded %s",__FUNCTION__, bSucceeded ? "true" : "false");
    return bSucceeded;
}

void DeviceSource::UnloadFilters()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
    if (hSampleThread) {
        SetEvent(hStopSampleEvent);
        WaitForSingleObject(hSampleThread, INFINITE);
        CloseHandle(hSampleThread);
        CloseHandle(hStopSampleEvent);

        hSampleThread = NULL;
        hStopSampleEvent = NULL;
    }

//     if(texture)
//     {
//         delete texture;
//         texture = NULL;
//     }
//     if(previousTexture)
//     {
//         delete previousTexture;
//         previousTexture = NULL;
//     }

    KillThreads();

    if(bFiltersLoaded)
    {
        graph->RemoveFilter(captureFilter);
        graph->RemoveFilter(deviceFilter);
        if(!bDeviceHasAudio) graph->RemoveFilter(audioDeviceFilter);

        if(audioFilter)
            graph->RemoveFilter(audioFilter);

        SafeReleaseLogRef(captureFilter);
        SafeReleaseLogRef(deviceFilter);
        SafeReleaseLogRef(audioDeviceFilter);
        SafeReleaseLogRef(audioFilter);

        bFiltersLoaded = false;
    }

// 	OSEnterMutex(hAudioMutex);
//     if(audioOut)
//     {
//         API->RemoveAudioSource(audioOut);
//         delete audioOut;
//         audioOut = NULL;
//     }
// 	OSLeaveMutex(hAudioMutex);
// 	OSEnterMutex(colorMutex);
//     if(colorConvertShader)
//     {
//         delete colorConvertShader;
//         colorConvertShader = NULL;
//     }
// 	OSLeaveMutex(colorMutex);

    if(lpImageBuffer)
    {
        Free(lpImageBuffer);
        lpImageBuffer = NULL;
    }

    SafeReleaseLogRef(capture);
    SafeReleaseLogRef(graph);
	SafeReleaseLogRef(m_pEvent);

    SafeRelease(control);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}

void DeviceSource::Start()
{
    if(bCapturing || !control)
        return;

    HRESULT err;
    if(FAILED(err = control->Run()))
    {
		DWORD code = GetLastError();
		Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: control->Run failed, result = %08lX", err);
        return;
    }

	bCapturing = true;
    /*if (err == S_FALSE)
        AppWarning(L"Ook");*/
}

void DeviceSource::Stop()
{
	if (drawShader)
	{
		delete drawShader;
		drawShader = NULL;
	}

    if(!bCapturing)
        return;

    bCapturing = false;

// 	if (control)
// 	{
// 		control->Stop();
// 		FlushSamples();
// 	}
    //FlushSamples();
}

void DeviceSource::BeginScene()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	ChangeShader();

	ChangeSize(bLoadSucceed, true);
	drawShader = D3DRender->CreatePixelShaderFromFile(TEXT("./shaders/DrawTexture_ColorAdjust.pShader"));

    Start();
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void DeviceSource::EndScene()
{
// 	OSEnterMutex(hAudioMutex);
// 	if (audioOut)
// 	{
// 		API->RemoveAudioSource(audioOut);
// 		delete audioOut;
// 		audioOut = NULL;
// 	}
// 	OSLeaveMutex(hAudioMutex);
    Stop();
}

void DeviceSource::GlobalSourceLeaveScene()
{
	if (!enteredSceneCount)
		return;

	if (--enteredSceneCount)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "device %s :enteredSceneCount: %d, value : %d", strDeviceName.Array(), enteredSceneCount, /*(m_dataCallBack == NULL || (m_dataCallBack&&*/enteredSceneCount);
		return;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "device %s :enteredSceneCount: %d, value : %d", strDeviceName.Array(), enteredSceneCount, /*(m_dataCallBack == NULL || (m_dataCallBack&&*/enteredSceneCount);

// 	OSEnterMutex(hAudioMutex);
// 	if (audioOut)
// 	{
// 		API->RemoveAudioSource(audioOut);
// 		delete audioOut;
// 		audioOut = NULL;
// 	}
// 	OSLeaveMutex(hAudioMutex);

// 	if (soundOutputType == 1) {
// 		OSEnterMutex(hAudioMutex);
// 		if (audioOut)
// 			audioOut->SetVolume(0.0f);
// 		OSLeaveMutex(hAudioMutex);
// 	}
    if(soundOutputType == 2) {
        IBasicAudio *basicAudio;
        if(SUCCEEDED(audioFilter->QueryInterface(IID_IBasicAudio, (void**)&basicAudio)))
        {
            long lVol = long((double(0.0)*NEAR_SILENTf)-NEAR_SILENTf);
            if(lVol <= -NEAR_SILENT)
                lVol = -10000;
            basicAudio->put_Volume(lVol);
            basicAudio->Release();
        }
    }
	FlushSamples();
}

void DeviceSource::GlobalSourceEnterScene()
{
	if (enteredSceneCount++)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "device %s :enteredSceneCount: %d, value : %d", strDeviceName.Array(), enteredSceneCount, /*(m_dataCallBack == NULL || (m_dataCallBack&&*/enteredSceneCount);
		return;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "device %s :enteredSceneCount: %d, value : %d", strDeviceName.Array(), enteredSceneCount, /*(m_dataCallBack == NULL || (m_dataCallBack&&*/enteredSceneCount);

 //   float sourceVolume = data["volume"].asDouble();

    if(soundOutputType == 1) {
		OSEnterMutex(hAudioMutex);
		if (NULL != audioOut)
		{
			if (audioFormat.wBitsPerSample > 0)
				audioOut->Initialize(this);
		}   
		else
		{
			int soundTimeOffset = data["soundTimeOffset"].asInt();
			float volume = data["volume"].asDouble();

			audioOut = new DeviceAudioSource;
			if (audioFormat.wBitsPerSample > 0)
				audioOut->Initialize(this);

// 			audioOut->SetAudioOffset(soundTimeOffset);
// 			audioOut->SetVolume(volume);
		}
	   OSLeaveMutex(hAudioMutex);
    }

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}

DWORD DeviceSource::SampleThread(DeviceSource *source)
{
    HANDLE hSampleMutex = source->hSampleMutex;
    LONGLONG lastTime = GetQPC100NS(), bufferTime = 0, frameWait = 0, curBufferTime = source->bufferTime;
    LONGLONG lastSampleTime = 0;

    bool bFirstFrame = true;
    bool bFirstDelay = true;

    while (WaitForSingleObject(source->hStopSampleEvent, 2) == WAIT_TIMEOUT) {
        LONGLONG t = GetQPC100NS();
        LONGLONG delta = t-lastTime;
        lastTime = t;

        OSEnterMutex(hSampleMutex);

        if (source->samples.Num()) {
            if (bFirstFrame) {
                bFirstFrame = false;
                lastSampleTime = source->samples[0]->timestamp;
            }

            //wait until the requested delay has been buffered before processing packets
            if (bufferTime >= source->bufferTime) {
                frameWait += delta;

                //if delay time was adjusted downward, remove packets accordingly
                bool bBufferTimeChanged = (curBufferTime != source->bufferTime);
                if (bBufferTimeChanged) {
                    if (curBufferTime > source->bufferTime) {
						OSEnterMutex(source->hAudioMutex);
                        if (source->audioOut)
                            source->audioOut->FlushSamples();
						OSLeaveMutex(source->hAudioMutex);

                        LONGLONG lostTime = curBufferTime - source->bufferTime;
                        bufferTime -= lostTime;

                        if (source->samples.Num()) {
                            LONGLONG startTime = source->samples[0]->timestamp;

                            while (source->samples.Num()) {
                                CSampleData *sample = source->samples[0];

                                if ((sample->timestamp - startTime) >= lostTime)
                                    break;

                                lastSampleTime = sample->timestamp;

                                sample->Release();
                                source->samples.Remove(0);
                            }
                        }
                    }

                    curBufferTime = source->bufferTime;
                }

                while (source->samples.Num()) {
                    CSampleData *sample = source->samples[0];

                    LONGLONG timestamp = sample->timestamp;
                    LONGLONG sampleTime = timestamp - lastSampleTime;

                    //sometimes timestamps can go to shit with horrible garbage devices.
                    //so, bypass any unusual timestamp offsets.
                    if (sampleTime < -10000000 || sampleTime > 10000000) {
                        //OSDebugOut(TEXT("sample time: %lld\r\n"), sampleTime);
                        sampleTime = 0;
                    }

                    if (frameWait < sampleTime)
                        break;

                    if (sample->bAudio) {
						OSEnterMutex(source->hAudioMutex);
                        if (source->audioOut)
                            source->audioOut->ReceiveAudio(sample->lpData, sample->dataLength,source->fNewVol);
						OSLeaveMutex(source->hAudioMutex);

                        sample->Release();
                    } else {
                        SafeRelease(source->latestVideoSample);
                        source->latestVideoSample = sample;
                    }

                    source->samples.Remove(0);

                    if (sampleTime > 0)
                        frameWait -= sampleTime;

                    lastSampleTime = timestamp;
                }
            }
        }

        OSLeaveMutex(hSampleMutex);

        if (!bFirstFrame && bufferTime < source->bufferTime)
            bufferTime += delta;
    }

    return 0;
}

UINT DeviceSource::GetSampleInsertIndex(LONGLONG timestamp)
{
    UINT index;
    for (index=0; index<samples.Num(); index++) {
        if (samples[index]->timestamp > timestamp)
            return index;
    }

    return index;
}

void DeviceSource::KillThreads()
{
    int numThreads = MAX(OSGetTotalCores()-2, 1);
    for(int i=0; i<numThreads; i++)
    {
        if(hConvertThreads[i])
        {
            convertData[i].bKillThread = true;
            SetEvent(convertData[i].hSignalConvert);

            OSTerminateThread(hConvertThreads[i], 10000);
            hConvertThreads[i] = NULL;
        }

        convertData[i].bKillThread = false;

        if(convertData[i].hSignalConvert)
        {
            CloseHandle(convertData[i].hSignalConvert);
            convertData[i].hSignalConvert = NULL;
        }

        if(convertData[i].hSignalComplete)
        {
            CloseHandle(convertData[i].hSignalComplete);
            convertData[i].hSignalComplete = NULL;
        }
    }

	bFirstFrame = true;
}

void DeviceSource::ChangeSize(bool bSucceeded, bool bForce)
{
	if (!bForce && imageCX == newCX && imageCY == newCY && oldType == colorType && deinterlacer.type == oldDeinterlacerType)
        return;

    renderCX = newCX;
    renderCY = newCY;
	if (oldType != colorType)
	{
		ChangeShader();
		oldType = colorType;
	}
	
	oldDeinterlacerType = deinterlacer.type;

    switch(colorType) {
    case DeviceOutputType_RGB:
        lineSize = renderCX * 4;
        break;
    case DeviceOutputType_I420:
    case DeviceOutputType_YV12:
        lineSize = renderCX; //per plane
        break;
    case DeviceOutputType_YVYU:
    case DeviceOutputType_YUY2:
    case DeviceOutputType_UYVY:
    case DeviceOutputType_HDYC:
        lineSize = (renderCX * 2);
        break;
    }

    linePitch = lineSize;
    lineShift = 0;
    imageCX = renderCX;
    imageCY = renderCY;

    deinterlacer.imageCX = renderCX;
    deinterlacer.imageCY = renderCY;

    if(deinterlacer.doublesFramerate)
        deinterlacer.imageCX *= 2;

    switch(deinterlacer.type) {
    case DEINTERLACING_DISCARD:
        deinterlacer.imageCY = renderCY/2;
        linePitch = lineSize * 2;
        renderCY /= 2;
        break;

    case DEINTERLACING_RETRO:
        deinterlacer.imageCY = renderCY/2;
        if(deinterlacer.processor != DEINTERLACING_PROCESSOR_GPU)
        {
            lineSize *= 2;
            linePitch = lineSize;
            renderCY /= 2;
            renderCX *= 2;
        }
        break;

    case DEINTERLACING__DEBUG:
        deinterlacer.imageCX *= 2;
        deinterlacer.imageCY *= 2;
    case DEINTERLACING_BLEND2x:
    //case DEINTERLACING_MEAN2x:
    case DEINTERLACING_YADIF:
    case DEINTERLACING_YADIF2x:
        deinterlacer.needsPreviousFrame = true;
        break;
    }

    if(deinterlacer.type != DEINTERLACING_NONE && deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU)
    {
		deinterlacer.vertexShader.reset(D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader")));

		if (deinterlacer.type == DEINTERLACING_YADIF || deinterlacer.type == DEINTERLACING_YADIF2x)
		{
			deinterlacer.pixelShader.reset(D3DRender->CreatePixelShaderFromFile(ChooseDeinterlacingShader()));
		}
		else
		{
			//deinterlacer.pixelShaderAsync = CreatePixelShaderFromFileAsync(ChooseDeinterlacingShader());//异步接口没有抽出来
			deinterlacer.pixelShader.reset(D3DRender->CreatePixelShaderFromFile(ChooseDeinterlacingShader()));
		}
		
        deinterlacer.isReady = false;
    }

    KillThreads();

    int numThreads = MAX(OSGetTotalCores()-2, 1);
    for(int i=0; i<numThreads; i++)
    {
        convertData[i].width  = lineSize;
        convertData[i].height = renderCY;
        convertData[i].sample = NULL;
        convertData[i].hSignalConvert  = CreateEvent(NULL, FALSE, FALSE, NULL);
        convertData[i].hSignalComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
        convertData[i].linePitch = linePitch;
        convertData[i].lineShift = lineShift;

        if(i == 0)
            convertData[i].startY = 0;
        else
            convertData[i].startY = convertData[i-1].endY;

        if(i == (numThreads-1))
            convertData[i].endY = renderCY;
        else
            convertData[i].endY = ((renderCY/numThreads)*(i+1)) & 0xFFFFFFFE;
    }

    if(colorType == DeviceOutputType_YV12 || colorType == DeviceOutputType_I420)
    {
        for(int i=0; i<numThreads; i++)
            hConvertThreads[i] = OSCreateThread((XTHREAD)PackPlanarThread, convertData+i);
    }


    if(texture)
    {
        delete texture;
        texture = NULL;
    }
    if(previousTexture)
    {
        delete previousTexture;
        previousTexture = NULL;
    }

    //-----------------------------------------------------
    // create the texture regardless, will just show up as red to indicate failure
	BYTE *textureData = (BYTE*)Allocate_Bak(renderCX*renderCY * 4);

    if(colorType == DeviceOutputType_RGB) //you may be confused, but when directshow outputs RGB, it's actually outputting BGR
    {
        msetd(textureData, 0xFF000000, renderCX*renderCY*4);
		texture = D3DRender->CreateTexture(renderCX, renderCY, GS_BGR, textureData, FALSE, FALSE);
        if(bSucceeded && deinterlacer.needsPreviousFrame)
			previousTexture = D3DRender->CreateTexture(renderCX, renderCY, GS_BGR, textureData, FALSE, FALSE);
        if(bSucceeded && deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU)
			deinterlacer.texture.reset(D3DRender->CreateRenderTarget(deinterlacer.imageCX, deinterlacer.imageCY, GS_BGRA, FALSE));
    }
    else //if we're working with planar YUV, we can just use regular RGB textures instead
    {
        msetd(textureData, 0xFF000000, renderCX*renderCY*4);
		texture = D3DRender->CreateTexture(renderCX, renderCY, GS_RGB, textureData, FALSE, FALSE);
        if(bSucceeded && deinterlacer.needsPreviousFrame)
			previousTexture = D3DRender->CreateTexture(renderCX, renderCY, GS_RGB, textureData, FALSE, FALSE);
        if(bSucceeded && deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU)
			deinterlacer.texture.reset(D3DRender->CreateRenderTarget(deinterlacer.imageCX, deinterlacer.imageCY, GS_BGRA, FALSE));
    }

    if(bSucceeded && bUseThreadedConversion)
    {
        if(colorType == DeviceOutputType_I420 || colorType == DeviceOutputType_YV12)
        {
            LPBYTE lpData;
			if (S_OK == D3DRender->Map(texture, lpData, texturePitch, D3D11_MAP_WRITE_DISCARD))
				D3DRender->Unmap(texture);
            else
                texturePitch = renderCX*4;

			lpImageBuffer = (LPBYTE)Allocate_Bak(texturePitch*renderCY);
        }
    }

    Free(textureData);

    //bFiltersLoaded = bSucceeded;
}

void DeviceSource::ReceiveMediaSample(IMediaSample *sample, bool bAudio)
{
    if (!sample)
        return;

    if (/*bCapturing*/ true) {
        BYTE *pointer;

        if (!sample->GetActualDataLength())
            return;

        if (SUCCEEDED(sample->GetPointer(&pointer))) 
		{
            CSampleData *data = NULL;
            AM_MEDIA_TYPE *mt = nullptr;
			long nlen1 = sample->GetActualDataLength();

            if (sample->GetMediaType(&mt) == S_OK)
            {
                BITMAPINFOHEADER *bih = GetVideoBMIHeader(mt);
                newCX = bih->biWidth;
                newCY = bih->biHeight;
                DeleteMediaType(mt);
            }

            if (bUseBuffering || !bAudio) {
// 				if (bUseBuffering)
// 				{
					data = new CSampleData;
					data->lpData = (LPBYTE)Allocate_Bak(sample->GetActualDataLength());
					memcpy(data->lpData, pointer, sample->GetActualDataLength());
// 				}
// 				else
// 				{
// 					data = new CSampleData(sample);
// 					data->lpData = pointer;// (LPBYTE)Allocate_Bak(data->dataLength);//pointer; //
// 				}
                data->bAudio = bAudio;
                data->dataLength = sample->GetActualDataLength();
				
                data->cx = newCX;
                data->cy = newCY;
				data->colorType = colorType;
				data->bFieldSignal = bIsFieldSignal;

                LONGLONG stopTime;
                sample->GetTime(&stopTime, &data->timestamp);
            }

			OSEnterMutex(ListCallBackMutex);//为了录制加了这个锁
			if (m_ListCallBack.Num() > 0)
			{
				for (int i = 0; i < m_ListCallBack.Num(); ++i)
				{
					__DataCallBack &OneCallBack = m_ListCallBack[i];
					CPObject *Object = reinterpret_cast<CPObject*>(OneCallBack.Context);
					if (bAudio && strcmp(Object->GainClassName(),"SharedDevice") != 0)
					{

						CSampleData      *audioSample = NULL;

						audioSample = new CSampleData;
						audioSample->lpData = (LPBYTE)Allocate_Bak(sample->GetActualDataLength());
						memcpy(audioSample->lpData, pointer, sample->GetActualDataLength());

						audioSample->bAudio = bAudio;
						audioSample->dataLength = sample->GetActualDataLength();
						audioSample->pAudioFormat = (void*)&audioFormat;
						audioSample->UserData = &this->data;
						audioSample->colorType = colorType;
						LONGLONG stopTime;
						sample->GetTime(&stopTime, &audioSample->timestamp);
						OneCallBack.CallBack(OneCallBack.Context, audioSample);

						audioSample->Release();
					}
					else if (!bAudio)  //视频
					{
						data->UserData = &this->data;
						data->bDisableAudio = soundOutputType == 0;
						OneCallBack.CallBack(OneCallBack.Context, data);
					}
				}
			}
			OSLeaveMutex(ListCallBackMutex);

			OSEnterMutex(hSampleMutex);
            if (bUseBuffering && !bAudio) {
                UINT id = GetSampleInsertIndex(data->timestamp);
                samples.Insert(id, data);
            } else if (bAudio) {
				long nlen = sample->GetActualDataLength();

				OSEnterMutex(hAudioMutex);
				// 有音频数据到来，说明要坚持音频				
				m_qwrdAudioTime = GetQPCMS();				
// 				if (audioOut && bCapturing && enteredSceneCount)
// 					audioOut->ReceiveAudio(pointer, nlen,fNewVol);
				OSLeaveMutex(hAudioMutex);
				
            } else {
//				if (enteredSceneCount && bCapturing)
				{
					SafeRelease(latestVideoSample);
					latestVideoSample = data;
				}
// 				else
// 				{
// 					SafeRelease(data);
// 				}
            }

            OSLeaveMutex(hSampleMutex);
        }
    }
}

static DWORD STDCALL PackPlanarThread(ConvertData *data)
{
    do {
        WaitForSingleObject(data->hSignalConvert, INFINITE);
        if(data->bKillThread) break;

        PackPlanar(data->output, data->input, data->width, data->height, data->pitch, data->startY, data->endY, data->linePitch, data->lineShift);
        data->sample->Release();

        SetEvent(data->hSignalComplete);
    }while(!data->bKillThread);

    return 0;
}
static DWORD STDCALL CheckDeviceThread(LPVOID *pData)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!,ThreadID = %d", __FUNCTION__,GetCurrentThreadId());
	DeviceSource *pThis = (DeviceSource *)pData;
	return pThis->CheckDevice();	
}
void DeviceSource::Preprocess()
{
	if (!bCapturing /*|| enteredSceneCount == 0*/)
	{
		return;
	}
    //----------------------------------------
	OSEnterMutex(colorMutex);
    if(bRequestVolume)
    {
		OSEnterMutex(hAudioMutex);
        if(audioOut)
            audioOut->SetVolume(fNewVol);
        else if(audioFilter)
        {
            IBasicAudio *basicAudio;
            if(SUCCEEDED(audioFilter->QueryInterface(IID_IBasicAudio, (void**)&basicAudio)))
            {
                long lVol = long((double(fNewVol)*NEAR_SILENTf)-NEAR_SILENTf);
                if(lVol <= -NEAR_SILENT)
                    lVol = -10000;
                basicAudio->put_Volume(lVol);
                basicAudio->Release();
            }
        }
		OSLeaveMutex(hAudioMutex);
        bRequestVolume = false;
    }

    //----------------------------------------
    CSampleData *lastSample = NULL;

    OSEnterMutex(hSampleMutex);

    lastSample = latestVideoSample;
    latestVideoSample = NULL;

    OSLeaveMutex(hSampleMutex);

    //----------------------------------------

    int numThreads = MAX(OSGetTotalCores()-2, 1);


	
    if(lastSample)
    {
        /*REFERENCE_TIME refTimeStart, refTimeFinish;
        lastSample->GetTime(&refTimeStart, &refTimeFinish);

        static REFERENCE_TIME lastRefTime = 0;
        Log(TEXT("refTimeStart: %llu, refTimeFinish: %llu, offset = %llu"), refTimeStart, refTimeFinish, refTimeStart-lastRefTime);
        lastRefTime = refTimeStart;*/

        if(previousTexture)
        {
            Texture *tmp = texture;
            texture = previousTexture;
            previousTexture = tmp;
        }
		deinterlacer.curField = deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU ? false : (deinterlacer.fieldOrder == FIELD_ORDER_BFF);
        deinterlacer.bNewFrame = true;
 
        if(colorType == DeviceOutputType_RGB)
        {
            if(texture)
            {
                ChangeSize();

				D3DRender->SetImage(texture, lastSample->lpData, GS_IMAGEFORMAT_BGRX, linePitch);
                bReadyToDraw = true;
            }
        }
        else if(colorType == DeviceOutputType_I420 || colorType == DeviceOutputType_YV12)
        {
            if(bUseThreadedConversion)
            {
                if(!bFirstFrame)
                {
                    List<HANDLE> events;
                    for(int i=0; i<numThreads; i++)
                        events << convertData[i].hSignalComplete;

                    WaitForMultipleObjects(numThreads, events.Array(), TRUE, INFINITE);
					D3DRender->SetImage(texture, lpImageBuffer, GS_IMAGEFORMAT_RGBX, texturePitch);

                    bReadyToDraw = true;
                }
                else
                    bFirstFrame = false;

                ChangeSize();

                for(int i=0; i<numThreads; i++)
                    lastSample->AddRef();

                for(int i=0; i<numThreads; i++)
                {
                    convertData[i].input     = lastSample->lpData;
                    convertData[i].sample    = lastSample;
                    convertData[i].pitch     = texturePitch;
                    convertData[i].output    = lpImageBuffer;
                    convertData[i].linePitch = linePitch;
                    convertData[i].lineShift = lineShift;
                    SetEvent(convertData[i].hSignalConvert);
                }
            }
            else
            {
                LPBYTE lpData;
                UINT pitch;

                ChangeSize();

				if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
                {
                    PackPlanar(lpData, lastSample->lpData, renderCX, renderCY, pitch, 0, renderCY, linePitch, lineShift);
					D3DRender->Unmap(texture);
                }

                bReadyToDraw = true;
            }
        }
        else if(colorType == DeviceOutputType_YVYU || colorType == DeviceOutputType_YUY2)
        {
            LPBYTE lpData;
            UINT pitch;

            ChangeSize();

			if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
            {
                Convert422To444(lpData, lastSample->lpData, pitch, true);
				D3DRender->Unmap(texture);
            }

            bReadyToDraw = true;
        }
        else if(colorType == DeviceOutputType_UYVY || colorType == DeviceOutputType_HDYC)
        {
            LPBYTE lpData;
            UINT pitch;

            ChangeSize();

			if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
            {
                Convert422To444(lpData, lastSample->lpData, pitch, false);
				D3DRender->Unmap(texture);
            }

            bReadyToDraw = true;
        }

        lastSample->Release();

		bool bSync = true;//false
// 		if (deinterlacer.type != DEINTERLACING_NONE)
// 			bSync = (deinterlacer.type == DEINTERLACING_YADIF || deinterlacer.type == DEINTERLACING_YADIF2x);

        if (bReadyToDraw &&
            deinterlacer.type != DEINTERLACING_NONE &&
            deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU &&
            deinterlacer.texture.get() &&
			bSync ? deinterlacer.pixelShader.get():deinterlacer.pixelShaderAsync.Shader())
        {
			D3DRender->SetRenderTarget(deinterlacer.texture.get());

			Shader *oldVertShader = D3DRender->GetCurrentVertexShader();
			D3DRender->LoadVertexShader(deinterlacer.vertexShader.get());
            
			Shader *oldShader = D3DRender->GetCurrentPixelShader();
			D3DRender->LoadPixelShader(bSync ? deinterlacer.pixelShader.get() : deinterlacer.pixelShaderAsync.Shader());

			HANDLE hField =  (bSync ? deinterlacer.pixelShader.get() : deinterlacer.pixelShaderAsync.Shader())->GetParameterByName(TEXT("field_order"));
            if(hField)
				(bSync ? deinterlacer.pixelShader.get() : deinterlacer.pixelShaderAsync.Shader())->SetBool(hField, deinterlacer.fieldOrder == FIELD_ORDER_BFF);
            
			D3DRender->Ortho(0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, -100.0f, 100.0f);
			D3DRender->SetViewport(0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY));

            if(previousTexture)
				D3DRender->LoadTexture(previousTexture, 1);

			D3DRender->DrawSpriteEx(texture, 0xFFFFFFFF, 0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, 0.0f, 1.0f, 1.0f);

            if(previousTexture)
				D3DRender->LoadTexture(nullptr, 1);

			D3DRender->LoadPixelShader(oldShader);
			D3DRender->LoadVertexShader(oldVertShader);
            deinterlacer.isReady = true;
        }
    }

	OSLeaveMutex(colorMutex);
}

void DeviceSource::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	if (!bCapturing)
		return;

	OSEnterMutex(colorMutex);
	if (texture && bReadyToDraw && deinterlacer.isReady)
    {
		SamplerState *sampler = NULL;
		//=========================
		Vect2 lPos = pos;
		Vect2 lSize = size;
		if (!bScaleFull) //等比例处理
		{
			int scaleDelte = newCX*size.y - newCY*size.x;

			if (scaleDelte > 0)
			{
				//以宽为主
				int Ry = size.x *  newCY / newCX;

				lPos.y += (size.y - Ry) / 2;
				lSize.y = Ry;
			}
			else if (scaleDelte < 0)
			{
				//以高为主
				int w = size.y * newCX / newCY;

				lPos.x += (size.x - w) / 2;
				lSize.x = w;
			}
		}
		//=========================

        bool bFlip = bFlipVertical;

        if(colorType != DeviceOutputType_RGB)
            bFlip = !bFlip;

        float x, x2;
        if(bFlipHorizontal)
        {
			x2 = lPos.x;
			x = x2 + lSize.x;
        }
        else
        {
			x = lPos.x;
			x2 = x + lSize.x;
        }

		float y = lPos.y;
		float y2 = y + lSize.y;

		if (!bFlip)
		{
			y2 = lPos.y;
			y = y2 + lSize.y;
		}

        float fOpacity = float(opacity)*0.01f;
        DWORD opacity255 = DWORD(fOpacity*255.0f);

        if(bUsePointFiltering) {
            SamplerInfo samplerinfo;
            samplerinfo.filter = GS_FILTER_POINT;
			sampler = D3DRender->CreateSamplerState(samplerinfo);
			D3DRender->LoadSamplerState(sampler, 0);
        }

		if (!FilterTexture)
		{
			Shader *oldShader = D3DRender->GetCurrentPixelShader();

			gamma = data["gamma"].asInt();
			float fGamma = float(-(gamma - 100) + 100) * 0.01f;

			if (Vect2(x2,y2) == GetSize() || !bIsFieldSignal)
			{
				if (colorConvertShader)
				{
					D3DRender->LoadPixelShader(colorConvertShader);

					if (bUseChromaKey)
					{
						float fSimilarity = float(keySimilarity) / 1000.0f;
						float fBlendVal = float(max(keyBlend, 1) / 1000.0f);
						float fSpillVal = (float(max(keySpillReduction, 1)) / 1000.0f);

						Vect2 pixelSize = 1.0f / GetSize();

						colorConvertShader->SetColor(colorConvertShader->GetParameterByName(TEXT("keyBaseColor")), Color4(keyBaseColor));
						colorConvertShader->SetColor(colorConvertShader->GetParameterByName(TEXT("chromaKey")), Color4(keyChroma));
						colorConvertShader->SetVector2(colorConvertShader->GetParameterByName(TEXT("pixelSize")), pixelSize);
						colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("keySimilarity")), fSimilarity);
						colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("keyBlend")), fBlendVal);
						colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("keySpill")), fSpillVal);
					}

					colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
				}
			}
			else if (bIsFieldSignal && colorFieldConvertShader)
			{
				D3DRender->LoadPixelShader(colorFieldConvertShader);
				colorFieldConvertShader->SetFloat(colorFieldConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
			}

			Texture *tex = (deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU && deinterlacer.texture.get()) ? deinterlacer.texture.get() : texture;
			if (deinterlacer.doublesFramerate)
			{
				if (!deinterlacer.curField)
					D3DRender->DrawSpriteEx(tex, (opacity255 << 24) | 0xFFFFFF, x, y, x2, y2, 0.f, 0.0f, 0.5f, 1.f);
				else
					D3DRender->DrawSpriteEx(tex, (opacity255 << 24) | 0xFFFFFF, x, y, x2, y2, .5f, 0.0f, 1.f, 1.f);
			}
			else
				D3DRender->DrawSprite(tex, (opacity255 << 24) | 0xFFFFFF, x, y, x2, y2);

			if (colorConvertShader || fGamma != 1.0f)
				D3DRender->LoadPixelShader(oldShader);
		}
		else
		{
			Shader *oldShader = NULL;
			if (bIsFieldSignal && Vect2(x2, y2) != GetSize())
			{
				oldShader = D3DRender->GetCurrentPixelShader();
				if (RGBFieldShader)
				{
					D3DRender->LoadPixelShader(RGBFieldShader);
				}
			}

			if (x > x2 || y > y2)
			{
				if (x > x2 && y > y2)
				{
					D3DRender->DrawSprite(FilterTexture, (opacity255 << 24) | 0xFFFFFF, x2, y2, x, y);
				}
				else if (x > x2)
				{
					D3DRender->DrawSprite(FilterTexture, (opacity255 << 24) | 0xFFFFFF, x2, y, x, y2);
				}
				else
				{
					D3DRender->DrawSprite(FilterTexture, (opacity255 << 24) | 0xFFFFFF, x, y2, x2, y);
				}
			}
			else
			{
				D3DRender->DrawSprite(FilterTexture, (opacity255 << 24) | 0xFFFFFF, x, y, x2, y2);
			}

			if (bIsFieldSignal && Vect2(x2, y2) != GetSize())
			{
				D3DRender->LoadPixelShader(oldShader);
			}
		}

		if (deinterlacer.bNewFrame)
        {
            //deinterlacer.curField = !deinterlacer.curField;
            deinterlacer.bNewFrame = false; //prevent switching from the second field to the first field
        }

        if(bUsePointFiltering) delete(sampler);

       
    }
	OSLeaveMutex(colorMutex);
}

bool DeviceExist(String& devece, GUID matchGUID)
{
	StringList deviceNameList;
	StringList deviceIDList;
	FillOutListOfDevices(NULL, matchGUID, &deviceNameList, &deviceIDList);

	bool find = false;
	int num = deviceNameList.Num();
	for (int i = 0; i < num; ++i)
	{
		if (devece == deviceNameList[i])
		{
			find = true;
		}
	}

	return find;
}

void DeviceSource::UpdateSettings(Value &data)
{	
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!",__FUNCTION__);
	this->data = data;

    String strNewDevice         = Asic2WChar(data["device"].asString()).c_str();
	m_bCheckAudioDevice         = false; // 更新配置，默认不检测音频
	m_qwrdAudioTime             = 0;
	bool VideoDeviceExist = true;
	if (!DeviceExist(strNewDevice, CLSID_VideoInputDeviceCategory)) //视频设备不存在
	{
// 		String info(L"视频设备 ");
// 		info += (strNewDevice + L" 不存在");
// 		AddCreateInfo(info);
		VideoDeviceExist = false;
	}

    String strNewAudioDevice    = Asic2WChar(data["audioDevice"].asString()).c_str();
	String Disable = L"禁用";
	bool nqe = strNewAudioDevice != Disable;
	UINT newSoundOutputType = data["soundOutputType"].asInt();

	if (!data["audioDeviceID"].isNull())
		strAudioID = Asic2WChar(data["audioDeviceID"].asString()).c_str();
	if (strAudioID.CompareI(TEXT("Disabled")))
		newSoundOutputType = 0;

	if (VideoDeviceExist && newSoundOutputType == 1 && nqe && !DeviceExist(strNewAudioDevice, CLSID_AudioInputDeviceCategory))
	{
// 		String info(L"音频设备 ");
// 		info += (strNewAudioDevice + L" 不存在");
// 		AddCreateInfo(info);
		data["soundOutputType"] =  0;
		newSoundOutputType = 0;
	}

    UINT64 newFrameInterval     = data["frameInterval"].asUInt();
    UINT newCX1                  = data["resolutionWidth"].asInt();
    UINT newCY1                  = data["resolutionHeight"].asInt();
    BOOL bNewCustom             = data["customResolution"].asInt();
    UINT newPreferredType       = data["usePreferredType"].asUInt() != 0 ? data["preferredType"].asUInt() : -1;

    bool bNewUseBuffering       = data["useBuffering"].asInt() != 0;
    bool bNewUseAudioRender     = data["useAudioRender"].asInt() != 0;
// 	UINT newGamma               = 100;
// 
// 	if (!data["gamma"].isNull())
// 	{
// 		newGamma = data["gamma"].asInt();
// 	}

    int newDeintType            = data["deinterlacingType"].asInt();
    int newDeintFieldOrder      = data["deinterlacingFieldOrder"].asInt();
    int newDeintProcessor       = data["deinterlacingProcessor"].asInt();

    UINT64 frameIntervalDiff = 0;
    bool bCheckSoundOutput = true;

    if(newFrameInterval > frameInterval)
        frameIntervalDiff = newFrameInterval - frameInterval;
    else
        frameIntervalDiff = frameInterval - newFrameInterval;

	if (strNewAudioDevice == L"禁用" && strAudioDevice == L"禁用")
        bCheckSoundOutput = false;

    bool eglato = sstri(strNewDevice.Array(), L"elgato") != nullptr;

	if (eglato /*|| bFlip*/ || !bCapturing || (bNewUseAudioRender != bUseAudioRender && bCheckSoundOutput) ||
       (newSoundOutputType != soundOutputType && bCheckSoundOutput) || imageCX != newCX1 || imageCY != newCY1 ||
       frameIntervalDiff >= 10 || newPreferredType != preferredOutputType ||
       !strDevice.CompareI(strNewDevice) || !strAudioDevice.CompareI(strNewAudioDevice) ||
       bNewCustom != bUseCustomResolution || bNewUseBuffering != bUseBuffering || newDeintType != deinterlacer.type ||
       newDeintFieldOrder != deinterlacer.fieldOrder || newDeintProcessor != deinterlacer.processor)
    {
		// 退出检测线程
		m_bCheckExit = true;
		if (NULL != hThreadCheckDevice)
		{
			SetEvent(m_hCheckExitEvent);
			if (WaitForSingleObject(hThreadCheckDevice, 4000) == WAIT_TIMEOUT)
			{
				TerminateThread(hThreadCheckDevice, 0);
			}
			CloseHandle(hThreadCheckDevice);
		}		
		hThreadCheckDevice = NULL;	

		// 重启设备
		ReStartCaptrue();

		if (bNeedCheck)
		{
			// 创建检测线程
			m_bCheckExit = false;
			ResetEvent(m_hCheckExitEvent);
			hThreadCheckDevice = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckDeviceThread, this, 0, NULL);
		}

    }


	if (bIsFieldSignal != data["ScanInterlace"].asUInt())
	{
		//让!bNeedCheck进来
		bIsFieldSignal = data["ScanInterlace"].asUInt();

		if (bNeedCheck)
		{
			OSEnterMutex(colorMutex);

			ChangeShader();

			OSLeaveMutex(colorMutex);
		}

	}


	float sourceVolume = data["volume"].asDouble();
	OSEnterMutex(hAudioMutex);
	if (!audioOut)
	{
		audioOut = new DeviceAudioSource;
	}
	if (soundOutputType == 1)
	{
		int soundTimeOffset = data["soundTimeOffset"].asInt();
		if(audioFormat.wBitsPerSample > 0)
			audioOut->Initialize(this);

		audioOut->SetAudioOffset(soundTimeOffset);
		audioOut->SetVolume(sourceVolume);
	}

	OSLeaveMutex(hAudioMutex);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}

void DeviceSource::SetInt(CTSTR lpName, int iVal)
{
	if (scmpi(lpName, TEXT("bNeedCheckThread")) == 0)
	{
		bNeedCheck = iVal == 1;
		return;
	}

    if(bCapturing)
    {
        if(scmpi(lpName, TEXT("useChromaKey")) == 0)
        {
            bool bNewVal = iVal != 0;
            if(bUseChromaKey != bNewVal)
            {
                bUseChromaKey = bNewVal;
				OSEnterMutex(colorMutex);

				ChangeShader();

				OSLeaveMutex(colorMutex);
            }
        }
        else if(scmpi(lpName, TEXT("flipImage")) == 0)
        {
            bFlipVertical = iVal != 0;
        }
        else if(scmpi(lpName, TEXT("flipImageHorizontal")) == 0)
        {
            bFlipHorizontal = iVal != 0;
        }
        else if(scmpi(lpName, TEXT("usePointFiltering")) == 0)
        {
            bUsePointFiltering = iVal != 0;
        }
        else if(scmpi(lpName, TEXT("keyColor")) == 0)
        {
            keyColor = (DWORD)iVal;

            keyBaseColor = Color4().MakeFromRGBA(keyColor);
            Matrix4x4TransformVect(keyChroma, (colorType == DeviceOutputType_HDYC || colorType == DeviceOutputType_RGB) ? (float*)yuv709Mat : (float*)yuvMat, keyBaseColor);
            keyChroma *= 2.0f;

            if(keyBaseColor.x < keyBaseColor.y && keyBaseColor.x < keyBaseColor.z)
                keyBaseColor -= keyBaseColor.x;
            else if(keyBaseColor.y < keyBaseColor.x && keyBaseColor.y < keyBaseColor.z)
                keyBaseColor -= keyBaseColor.y;
            else if(keyBaseColor.z < keyBaseColor.x && keyBaseColor.z < keyBaseColor.y)
                keyBaseColor -= keyBaseColor.z;
        }
        else if(scmpi(lpName, TEXT("keySimilarity")) == 0)
        {
            keySimilarity = iVal;
        }
        else if(scmpi(lpName, TEXT("keyBlend")) == 0)
        {
            keyBlend = iVal;
        }
        else if(scmpi(lpName, TEXT("keySpillReduction")) == 0)
        {
            keySpillReduction = iVal;
        }
        else if(scmpi(lpName, TEXT("opacity")) == 0)
        {
            opacity = iVal;
        }
        else if(scmpi(lpName, TEXT("timeOffset")) == 0)
        {
			OSEnterMutex(hAudioMutex);
            if(audioOut)
                audioOut->SetAudioOffset(iVal);
			OSLeaveMutex(hAudioMutex);
        }
        else if(scmpi(lpName, TEXT("bufferTime")) == 0)
        {
            bufferTime = iVal*10000;
        }
    }
}

void DeviceSource::SetFloat(CTSTR lpName, float fValue)
{
    if(!bCapturing)
        return;

    if(scmpi(lpName, TEXT("volume")) == 0)
    {
        fNewVol = fValue;
        bRequestVolume = true;
    }
}

DeviceSource::~DeviceSource()
{
	// 去掉检测线程
	m_bCheckExit = true;
	SetEvent(m_hCheckExitEvent);

	//反注册
	//String strNewDevice = data->GetString(TEXT("device"));
	//m_transform = NULL;
	//RemoveVideoCapture(strDeviceName, strDeviceID);
	 
	Stop();
	
	if (control)
	{
		enteredSceneCount = 0;
		control->Stop();
		SafeRelease(latestVideoSample);
	}
	UnloadFilters();
	//	fclose(fdumppcm);
	FlushSamples();

	if (NULL != hThreadCheckDevice)
	{
		if (WaitForSingleObject(hThreadCheckDevice, 4000) == WAIT_TIMEOUT)
		{
			TerminateThread(hThreadCheckDevice,0);
		}
		CloseHandle(hThreadCheckDevice);
	}
	hThreadCheckDevice = NULL;

	SafeReleaseLogRef(capture);
	SafeReleaseLogRef(graph);
	SafeReleaseLogRef(m_pEvent);

	OSEnterMutex(hAudioMutex);
	if (audioOut)
	{
		//API->RemoveAudioSource(audioOut);
		delete audioOut;
		audioOut = NULL;
	}
	OSLeaveMutex(hAudioMutex);

	if (hConvertThreads)
		Free(hConvertThreads);

	if (convertData)
		Free(convertData);

	if (hSampleMutex)
		OSCloseMutex(hSampleMutex);

	if (colorMutex)
		OSCloseMutex(colorMutex);

	if (ListCallBackMutex)
		OSCloseMutex(ListCallBackMutex);

	if (hAudioMutex)
		OSCloseMutex(hAudioMutex);

	if (m_hCheckExitEvent)
	{
		OSCloseEvent(m_hCheckExitEvent);
		m_hCheckExitEvent = NULL;
	}

	if (colorConvertShader)
		delete colorConvertShader;
	colorConvertShader = NULL;

	if (colorFieldConvertShader)
		delete colorFieldConvertShader;
	colorFieldConvertShader = NULL;

	if (RGBFieldShader)
		delete RGBFieldShader;
	RGBFieldShader = NULL;
}
bool  CheckDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName);

DWORD DeviceSource::CheckDevice()
{	
	while (!m_pEvent && !m_bCheckExit)
	{
		DWORD result = WaitForSingleObject(m_hCheckExitEvent, 1000);
		if (WAIT_OBJECT_0 == result)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!,ThreadID = %d", __FUNCTION__, GetCurrentThreadId());
			return 0;
		}

		if (!DeviceExist(strDeviceName, CLSID_VideoInputDeviceCategory)) //视频设备不存在
		{
			continue;
		}

		OSEnterMutex(colorMutex);

		if (control)
		{
			control->Stop();
			FlushSamples();
		}

		Stop();

		LoadFilters();

		//text;
		//===================
		if (enteredSceneCount)
		{
			
			ChangeShader();

			//ChangeSize(bLoadSucceed, true);

			if (drawShader)
			{
				delete drawShader;
				drawShader = NULL;
			}

			drawShader = D3DRender->CreatePixelShaderFromFile(TEXT("./shaders/DrawTexture_ColorAdjust.pShader"));
			
		}
		//==================

		if (control)
		{
			HRESULT err;
			if (FAILED(err = control->Run()))
			{
				DWORD code = GetLastError();
				Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: control->Run failed, result = %08lX", err);
			}
			else
			{
				bCapturing = true;
			}
		}

		OSLeaveMutex(colorMutex);
	}


	// 记住设备重启时间
	OSEnterMutex(hAudioMutex);
	m_qwrdDeviceResetTime = GetQPCMS();
	OSLeaveMutex(hAudioMutex);

	HANDLE hEventArray[2];
	hEventArray[0] = NULL;
	hEventArray[1] = m_hCheckExitEvent;
	long  evCode;
	LONG_PTR  param1, param2;
	BOOLEAN bDone = FALSE;
	HRESULT hr = S_OK;	

	while (!m_bCheckExit)
	{
		if (NULL == hEventArray[0])
		{
			hr = m_pEvent->GetEventHandle((OAEVENT*)&hEventArray[0]);
		}

		DWORD result = WaitForMultipleObjects(2, hEventArray, FALSE, 1000);

		if (WAIT_OBJECT_0 == result)
		{
			while (hr = m_pEvent->GetEvent(&evCode, &param1, &param2, 0), SUCCEEDED(hr))
			{
				if (m_bCheckExit)
				{
					break;
				}
				//printf("Event code:%#04x\n   Params:%d,%d\n", evCode, param1, param2);
				
				hr = m_pEvent->FreeEventParams(evCode, param1, param2);

				// 处理丢失时间
				if (EC_DEVICE_LOST == evCode)
				{
					IBaseFilter  *Filter = (IBaseFilter  *)param1;
					String strLog=L"";					
					if (Filter == audioDeviceFilter)
					{
						strLog = L"音频";
						//AppWarning(L"音频设备丢失，请检测硬件");
					}
					else
					{
						strLog = L"视频";
						//AppWarning(L"视频设备丢失，请检测硬件");
					}
					if (1 == param2)
					{
						strLog += L"热插入，请检测硬件";
						if (m_bCheckExit)
						{
							break;
						}
						Sleep(1000);
						ReStartCaptrue();
						hEventArray[0] = NULL;
					}
					else
					{
						strLog += L"热拔出，请检测硬件";
					}
					Log::writeMessage(LOG_RTSPSERV, 1, "%s", WcharToAnsi(strLog.Array()).c_str());
					if (NULL == hEventArray[0])
					{
						break;
					}
				}
				else if (EC_CLOCK_CHANGED)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "DESHOW底层准备完毕");
				}
				else
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "DESHOW底层通知事件，时间代码:%#04x\n   Params:%d,%d\n", evCode, param1, param2);
				}

			}
		}
		else if (WAIT_TIMEOUT == result)
		{
			if (m_bCheckExit)
			{
				break;
			}

			continue;
		}
		else 
		{
			if (m_bCheckExit || !soundOutputType)
			{
				continue;;
			}		
			QWORD qdwrodTimeNow = GetQPCMS();
			QWORD qdwrodTimeLong = 0;
			bool  bNeedCheck    = false;
			OSEnterMutex(hAudioMutex);

			// 判断音频是否10s没有数据了
			bNeedCheck = qdwrodTimeNow - m_qwrdAudioTime > 10 * 1000;

			// 判断设备重启时间
			if (bNeedCheck)
			{
				bNeedCheck     = (qdwrodTimeNow - m_qwrdDeviceResetTime) > 10 * 1000;
				qdwrodTimeLong = qdwrodTimeNow - m_qwrdDeviceResetTime;
			}
			
			// 判断设备是否在线
			if (bNeedCheck)
			{				
				bNeedCheck = CheckDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", strAudioDevice);
				m_qwrdAudioTime = GetQPCMS();
			}
			OSLeaveMutex(hAudioMutex);

			if (m_bCheckExit)
			{
				break;
			}

			// 重启设备
			if (bNeedCheck)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "超过%llu毫秒没有收到音频数据，重启", qdwrodTimeLong);
				Sleep(1000);
				ReStartCaptrue();
				hEventArray[0] = NULL;
			}
		}
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!,ThreadID = %d", __FUNCTION__, GetCurrentThreadId());
	return 0;
}

bool DeviceSource::ReStartCaptrue()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
	Stop();
	if (bNeedCheck)
	{
		if (control)
		{
			control->Stop();
			FlushSamples();
		}
	}
	UnloadFilters();
	LoadFilters();

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:Using directshow input");
	OSEnterMutex(colorMutex);

	ChangeShader();

	drawShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders\\DrawTexture_ColorAdjust.pShader"));
	if (bNeedCheck)
		Start();

	OSLeaveMutex(colorMutex);
	// 记住设备重启时间
	OSEnterMutex(hAudioMutex);
	m_qwrdDeviceResetTime = GetQPCMS();
	OSLeaveMutex(hAudioMutex);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
	return true;
}

const char* DeviceSource::GetAduioClassName() const
{
	return NULL;//"DeviceAudioSource";
}

IBaseAudio * DeviceSource::GetAudioRender()
{
		return NULL;
}

void DeviceSource::SetHasPreProcess(bool bHasPre)
{
	this->bHasPre = bHasPre;
}

bool DeviceSource::GetHasPreProcess() const
{
	return bHasPre;
}


void DeviceSource::RegisterDataCallBack(void *Context, DataCallBack pCb)
{
	__DataCallBack DataBack;
	DataBack.Context = Context;
	DataBack.CallBack = pCb;
	OSEnterMutex(ListCallBackMutex);
	m_ListCallBack.Add(DataBack);
	OSLeaveMutex(ListCallBackMutex);
}

void DeviceSource::UnRegisterDataCallBack(void *Context)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin m_ListCallBack.Num() = %d", __FUNCTION__, m_ListCallBack.Num());
	OSEnterMutex(ListCallBackMutex);
	for (int i = 0; i < m_ListCallBack.Num();)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		if (OneCallBack.Context == Context)
		{
			m_ListCallBack.Remove(i);
			//break;
		}
		else
		{
			++i;
		}
	}
	OSLeaveMutex(ListCallBackMutex);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end m_ListCallBack.Num() = %d", __FUNCTION__, m_ListCallBack.Num());
}

void DeviceSource::SetCanEnterScene(bool bCanEnter)
{
	m_bEnterScene = bCanEnter;
}

bool DeviceSource::CanEnterScene() const
{
	return m_bEnterScene;
}

const char* DeviceSource::GetDeviceName() const
{
	if (data["deviceName"].isNull())
		return "";
	return data["deviceName"].asCString();
}

const char* DeviceSource::GetDeviceID() const
{
	if (data["deviceID"].isNull())
		return "";
	return data["deviceID"].asCString();
}

bool DeviceSource::IsFieldSignal() const
{
	return bIsFieldSignal;
}

void DeviceSource::ChangeShader()
{
	String strShader = ChooseShader(false);

	if (strShader.IsValid() && (!strShaderOld.Compare(strShader)))
	{
		if (colorConvertShader)
		{
			delete colorConvertShader;
			colorConvertShader = NULL;
		}

		if (colorFieldConvertShader)
		{
			delete colorFieldConvertShader;
			colorFieldConvertShader = NULL;
		}

		colorConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);
		strShaderOld = strShader;


		strShader = ChooseShader();
		colorFieldConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);

	}
	else if (colorType == DeviceOutputType_RGB && !bIsFieldSignal)
	{
		if (colorConvertShader)
		{
			delete colorConvertShader;
			colorConvertShader = NULL;
		}

		if (colorFieldConvertShader)
		{
			delete colorFieldConvertShader;
			colorFieldConvertShader = NULL;
		}
		strShaderOld.Clear();
	}
}

