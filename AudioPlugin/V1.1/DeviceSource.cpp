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
#include "AudioPlugin.h"
typedef TCHAR* PTCHAR;
//#include "streams.h"

#include "IVideoCaptureFilter.h"

IMPLEMENT_DYNIC(DSource, "音频捕捉源", "1.0.0.1")


DSource::DSource()
{
	bHasPre = false;
	enteredSceneCount = 0;
	hThreadCheckDevice = NULL;
	bCanEnter = true;
	bFiltersLoaded = false;
	audioDeviceFilter = NULL;
	audioFilter = NULL;
	audioOut = NULL;

}

bool DSource::Init(Value &data)
{
	bCapturing = false;
	bRequestVolume = true;
    hSampleMutex = OSCreateMutex();
    if(!hSampleMutex)
    {
        AppWarning(TEXT("AuidoPlugin: could not create sample mutex"));
        return false;
    }

	m_hCheckExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_pEvent = NULL;
	control = NULL;

	HRESULT err;
	err = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_IFilterGraph, (void**)&graph);
	if (FAILED(err))
	{
		AppWarning(TEXT("DShowPlugin: Failed to build IGraphBuilder, result = %08lX"), err);
		return false;
	}

	err = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_ICaptureGraphBuilder2, (void**)&capture);
	if (FAILED(err))
	{
		AppWarning(TEXT("DShowPlugin: Failed to build ICaptureGraphB888uilder2, result = %08lX"), err);
		return false;
	}

	capture->SetFiltergraph(graph);

	// 事件
	err = graph->QueryInterface(IID_IMediaEventEx, (void**)&m_pEvent);
	if (FAILED(err))
	{
		AppWarning(TEXT("DShowPlugin: QueryInterface(IID_IMediaEventEx, (void**)&pEvent) = %08lX"), err);
		return false;
	}

    this->data = data;
	fNewVol = 1.0f;
	//if (!data["volume"].isNull())
	//	fNewVol = data["volume"].asDouble();
	UpdateSettings(data);
    Log(TEXT("Using directshow input"));

    return true;
}

DSource::~DSource()
{
	// 退出检测线程
	SetEvent(m_hCheckExitEvent);
	if (NULL != hThreadCheckDevice)
	{
		OSTerminateThread(hThreadCheckDevice, 4000);
	}
	hThreadCheckDevice = NULL;

    Stop();
	UnloadAudioInputDevice();

	if (audioOut)
	{
		delete audioOut;
		audioOut = NULL;
	}

	if (hSampleMutex)
	{
		OSCloseMutex(hSampleMutex);
		hSampleMutex = NULL;
	}
}

void DSource::SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType)
{
	expectedAudioType = audioMediaType->subtype;

	if (audioMediaType->formattype == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX *pFormat = reinterpret_cast<WAVEFORMATEX*>(audioMediaType->pbFormat);
		mcpy(&audioFormat, pFormat, sizeof(audioFormat));

		Log(TEXT("    device audio info - bits per sample: %u, channels: %u, samples per sec: %u, block size: %u"),
			audioFormat.wBitsPerSample, audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.nBlockAlign);
	}
	else
	{
		AppWarning(TEXT("DShowPlugin: Audio format was not a normal wave format"));
		soundOutputType = 0;
	}

	DeleteMediaType(audioMediaType);
}


bool DSource::LoadAudioInputDevice()
{
//============================
    bool bDeviceHasAudio = false;
	bool bSucceeded = false;
	bool  bAddedAudioCapture = false, bAddedDevice = false;

	IPin  *audioPin = NULL;
	HRESULT err;

	if (graph == NULL) {
		err = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_IFilterGraph, (void**)&graph);
		if (FAILED(err))
		{
			AppWarning(TEXT("DShowPlugin: Failed to build IGraphBuilder, result = %08lX"), err);
			goto cleanFinish;
		}
	}

	if (capture == NULL) {
		err = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, (REFIID)IID_ICaptureGraphBuilder2, (void**)&capture);
		if (FAILED(err))
		{
			AppWarning(TEXT("DShowPlugin: Failed to build ICaptureGraphBuilder2, result = %08lX"), err);
			goto cleanFinish;
		}

		capture->SetFiltergraph(graph);
	}
	// 事件
	err = graph->QueryInterface(IID_IMediaEventEx, (void**)&m_pEvent);
	if (FAILED(err))
	{
		AppWarning(TEXT("DShowPlugin: QueryInterface(IID_IMediaEventEx, (void**)&pEvent) = %08lX"), err);
		return false;
	}

	strAudioName = L"";
	if (!data["audioDeviceName"].isNull())
		strAudioName = Asic2WChar(data["audioDeviceName"].asString().c_str()).c_str();

	strAudioID = L"";
	if (!data["audioDeviceID"].isNull())
		strAudioID = Asic2WChar(data["audioDeviceID"].asString().c_str()).c_str();

	float volume = 1.f;

	if (!data["volume"].isNull())
		volume = data["volume"].asDouble();

	soundOutputType = 1;

	if (strAudioName.CompareI(TEXT("禁用")))
		soundOutputType = 0;

	if (soundOutputType != 0)
	{
		if (!bDeviceHasAudio)
		{
			if (strAudioName.IsValid())
			{
				audioDeviceFilter = GetDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", strAudioName, L"DevicePath", strAudioID);
				if (!audioDeviceFilter)
					AppWarning(TEXT("DShowPlugin: Invalid audio device: name '%s', path '%s'"), strAudioName.Array(), strAudioID.Array());
			}

			if (audioDeviceFilter)
				err = capture->FindPin(audioDeviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &audioPin);
			else
				err = E_FAIL;
		}

		if (FAILED(err) || !audioPin)
		{
			Log(TEXT("DShowPlugin: No audio pin, result = %lX"), err);
			soundOutputType = 0;
		}
	}
	else
		bDeviceHasAudio = false;

	int soundTimeOffset = data["soundTimeOffset"].asInt();

	GUID expectedAudioType;

	if (soundOutputType == 1)
	{
		IAMStreamConfig *audioConfig;
		if (SUCCEEDED(audioPin->QueryInterface(IID_IAMStreamConfig, (void**)&audioConfig)))
		{
			AM_MEDIA_TYPE *audioMediaType;
			if (SUCCEEDED(err = audioConfig->GetFormat(&audioMediaType)))
			{
				SetAudioInfo(audioMediaType, expectedAudioType);
			}
			else
			{
				AppWarning(TEXT("DShowPlugin: Could not get audio format, result = %08lX"), err);
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
	// add audio capture filter if any

	if (soundOutputType == 1)
	{
		audioFilter = new CaptureFilter(this, MEDIATYPE_Audio, expectedAudioType);
		if (!audioFilter)
		{
			AppWarning(TEXT("Failed to create audio capture filter"));
			soundOutputType = 0;
		}
	}

	if (soundOutputType != 0)
	{
		if (FAILED(err = graph->AddFilter(audioFilter, NULL)))
			AppWarning(TEXT("DShowPlugin: Failed to add audio capture filter to graph, result = %08lX"), err);

		bAddedAudioCapture = true;
	}

	//------------------------------------------------
	// add primary device filter

	if (soundOutputType != 0 && !bDeviceHasAudio)
	{
		if (FAILED(err = graph->AddFilter(audioDeviceFilter, NULL)))
			AppWarning(TEXT("DShowPlugin: Failed to add audio device filter to graph, result = %08lX"), err);
	}

	bAddedDevice = true;


	bool bConnected = false;
	// audio render first
	if (soundOutputType != 0)
	{
		{
			IBaseFilter *aFilter = audioDeviceFilter;

			IPin *audioPin = GetOutputPin(aFilter, &MEDIATYPE_Audio);
			DWORD code = GetLastError();
			if (audioPin)
			{
				IAMBufferNegotiation *pNeg;
				audioPin->QueryInterface(IID_IAMBufferNegotiation, (void **)&pNeg);
				// Set the buffer size based on selected settings  
				ALLOCATOR_PROPERTIES prop = { 0 };
				//HRESULT hrRet = pNeg->GetAllocatorProperties(&prop);
				prop.cBuffers = 500 / 40;
				prop.cbBuffer = audioFormat.nSamplesPerSec * 40 / (1000);
				prop.cbAlign = audioFormat.wBitsPerSample * audioFormat.nChannels / 8;
				prop.cbPrefix = 0;

				HRESULT hr = pNeg->SuggestAllocatorProperties(&prop);
				pNeg->Release();
				Log(L"设置音频参数%d", prop.cbAlign);
			}

			if (!bDeviceHasAudio)
				bConnected = SUCCEEDED(err = capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, audioDeviceFilter, NULL, audioFilter));
		}

		if (!bConnected)
		{
			AppWarning(TEXT("DShowPlugin: Failed to connect the audio device pin to the audio capture pin, result = %08lX"), err);
			soundOutputType = 0;
		}
	}
	//------------------------------------------------
	// connect all pins and set up the whole capture thing

	if (FAILED(err = graph->QueryInterface(IID_IMediaControl, (void**)&control)))
	{
		AppWarning(TEXT("DShowPlugin: Failed to get IMediaControl, result = %08lX"), err);
		goto cleanFinish;
	}

	//	EnumFilters(graph);
	//DumpGraph(graph, 0);
	
	if (audioOut == NULL)
	{
		audioOut = new DAudioSource;
		if (soundOutputType == 1)
		{
			if (audioFormat.wBitsPerSample > 0)
				audioOut->Initialize(this);

			audioOut->SetAudioOffset(soundTimeOffset);
			audioOut->SetVolume(volume);
		}
	}
	else if (soundOutputType == 1)
	{
		if (audioFormat.wBitsPerSample > 0)
			audioOut->Initialize(this);

		audioOut->SetAudioOffset(soundTimeOffset);
		audioOut->SetVolume(volume);
	}
	bSucceeded = true;

	cleanFinish:

	SafeRelease(audioPin);

	if (!bSucceeded)
	{
		bCapturing = false;

		if (bAddedAudioCapture)
			graph->RemoveFilter(audioFilter);

		if (bAddedDevice)
		{
			if (!bDeviceHasAudio && audioDeviceFilter)
				graph->RemoveFilter(audioDeviceFilter);
		}

		SafeRelease(audioDeviceFilter);
		SafeRelease(audioFilter);
		SafeRelease(control);

		soundOutputType = 0;
	}

	bFiltersLoaded = bSucceeded;
//============================
	 return bSucceeded;
}

void DSource::UnloadAudioInputDevice()
{
	if (bFiltersLoaded)
	{
		graph->RemoveFilter(audioDeviceFilter);

		if (audioFilter)
			graph->RemoveFilter(audioFilter);

		SafeReleaseLogRef(audioDeviceFilter);
		SafeReleaseLogRef(audioFilter);

		bFiltersLoaded = false;
	}

	//OSEnterMutex(hSampleMutex);
	//if (audioOut)
	//{
	//	API->RemoveAudioSource(audioOut);
	//	delete audioOut;
	//	audioOut = NULL;
	//}
	//OSLeaveMutex(hSampleMutex);

	SafeReleaseLogRef(capture);
	SafeReleaseLogRef(graph);
	SafeReleaseLogRef(m_pEvent);
	SafeRelease(control);
}

void DSource::Start()
{
	if (bCapturing || !control)
		return;

	bCapturing = true;

	HRESULT err;
	if (FAILED(err = control->Run()))
	{
		AppWarning(TEXT("DShowPlugin: control->Run failed, result = %08lX"), err);
		return;
	}
}

void DSource::Stop()
{
    if(!bCapturing)
        return;

    bCapturing = false;
}

void DSource::BeginScene()
{
    Start();
}

void DSource::EndScene()
{
    Stop();
}

void DSource::GlobalSourceLeaveScene()
{
    if (!enteredSceneCount)
        return;
    if (--enteredSceneCount)
        return;

	//OSEnterMutex(hSampleMutex);
	//if (audioOut)
	//{
	//	API->RemoveAudioSource(audioOut);
	//	delete audioOut;
	//	audioOut = NULL;
	//}

	//OSLeaveMutex(hSampleMutex);
}

void DSource::GlobalSourceEnterScene()
{
    if (enteredSceneCount++)
        return;

    float sourceVolume = data["volume"].asDouble();

	OSEnterMutex(hSampleMutex);
	if (audioOut) {
		audioOut->Initialize(this);
        audioOut->SetVolume(1.0f);
    }
	OSLeaveMutex(hSampleMutex);
}

void DSource::ReceiveMediaSample(IMediaSample *sample, bool bAudio)
{
	if (!bAudio)
	{
		return;
	}

	BYTE *pointer;

	if (!sample->GetActualDataLength())
		return;

	if (SUCCEEDED(sample->GetPointer(&pointer)))
	{
		CSampleData *data = NULL;

		OSEnterMutex(hSampleMutex);

		m_qwrdAudioTime = GetQPCMS();

		long nlen = sample->GetActualDataLength();
		if (audioOut && bCapturing)
			audioOut->ReceiveAudio(pointer, nlen, enteredSceneCount != 0);


		CSampleData Audio;
		Audio.bAudio = true;
		Audio.lpData = pointer;
		Audio.dataLength = nlen;

		for (int i = 0; i < m_ListCallBack.Num(); ++i)
		{
			__DataCallBack &OneCallBack = m_ListCallBack[i];
			Audio.pAudioFormat = (void*)&audioFormat;
			if (OneCallBack.CallBack)
				OneCallBack.CallBack(OneCallBack.Context, &Audio);
		}

		Audio.lpData = NULL;

		OSLeaveMutex(hSampleMutex);
	}
}

void DSource::Preprocess()
{
	if (bCapturing && bRequestVolume)
    {
		OSEnterMutex(hSampleMutex);
        if(audioOut)
            audioOut->SetVolume(fNewVol);
		OSLeaveMutex(hSampleMutex);

        bRequestVolume = false;
    }
}

void DSource::Render(const Vect2 &pos, const Vect2 &size, Texture*texture, bool bScaleFull, bool bIsLiveC){ }

DWORD STDCALL DSource::CheckDeviceThread(LPVOID *pData)
{
	DSource *pThis = (DSource *)pData;
	return pThis->CheckDevice();
}

//重连机制
DWORD DSource::CheckDevice()
{
	HANDLE hEventArray[2];
	HRESULT hr = m_pEvent->GetEventHandle((OAEVENT*)&hEventArray[0]);
	hEventArray[1] = m_hCheckExitEvent;
	long  evCode;
	int64_t  param1, param2;
	BOOLEAN bDone = FALSE;
	long SleepTime = 3000;

	while (true)
	{
		DWORD result = WaitForMultipleObjects(2, hEventArray, FALSE, SleepTime);

		if (WAIT_OBJECT_0 == result)
		{
			while (hr = m_pEvent->GetEvent(&evCode, &param1, &param2, 0), SUCCEEDED(hr))
			{
				hr = m_pEvent->FreeEventParams(evCode, param1, param2);

				// 处理丢失时间
				if (EC_CLOCK_CHANGED == evCode)
				{
					Log(L"DESHOW底层准备完毕");
				}
				else
				{
					Log(L"DESHOW底层通知事件，时间代码:%#04x\n   Params:%d,%d\n", evCode, param1, param2);
				}
			}
		}
		else if (WAIT_TIMEOUT == result)
		{
			//// 判断音频是否10s没有数据了
			QWORD qdwrodTimeNow = GetQPCMS();
			DWORD RealSleep = qdwrodTimeNow - m_qwrdAudioTime;
			SleepTime = 3000 - RealSleep;
			if (SleepTime < 500)
			{
				SleepTime = 500;
			}

			bool bNeedCheck = (RealSleep >= 2 * 1000);
			if (bNeedCheck && CheckDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", strAudioName))
			{
				ReStartCaptrue();
			
				SleepTime = 3000;
				hr = m_pEvent->GetEventHandle((OAEVENT*)&hEventArray[0]);
			}
			else if (bNeedCheck && audioOut)
			{
				audioOut->ResetAudioDB();
			}

		}
		else if ((WAIT_OBJECT_0 + 1)== result)
		{
			break;
		}
	}

	return 0;
}

void DSource::UpdateSettings(Value &data)
{
	this->data = data;
	m_qwrdAudioTime = GetQPCMS();
	//m_bCheckAudioDevice = false; // 更新配置，默认不检测音频
	soundOutputType = 1; 
	// 退出检测线程
	SetEvent(m_hCheckExitEvent);
	if (NULL != hThreadCheckDevice)
	{
		OSTerminateThread(hThreadCheckDevice, 4000);
	}
	hThreadCheckDevice = NULL;

	// 重启设备
	ReStartCaptrue();

	// 创建检测线程
	ResetEvent(m_hCheckExitEvent);
	hThreadCheckDevice = OSCreateThread((XTHREAD)CheckDeviceThread, this);

	if (audioOut)
		audioOut->ResetAudioDB();
}

bool DSource::ReStartCaptrue()
{
	if (control)
	{
		control->Stop();
	}

	bool bWasCapturing = bCapturing;
	Stop();

	UnloadAudioInputDevice();
	LoadAudioInputDevice();

	if (bWasCapturing)
	{
		Start();
	}

	return true;
}

void DSource::SetInt(CTSTR lpName, int iVal)
{
    if(bCapturing)
    {
        if(scmpi(lpName, TEXT("timeOffset")) == 0)
        {
			OSEnterMutex(hSampleMutex);
            if(audioOut)
                audioOut->SetAudioOffset(iVal);
			OSLeaveMutex(hSampleMutex);
        }
    }
}

void DSource::SetFloat(CTSTR lpName, float fValue)
{
    if(!bCapturing)
        return;

//     if(scmpi(lpName, TEXT("volume")) == 0)
//     {
//         fNewVol = fValue;
//         bRequestVolume = true;
//     }
}

void DSource::SetHasPreProcess(bool bHasPre)
{
	this->bHasPre;
}

bool DSource::GetHasPreProcess() const
{
	return bHasPre;
}

void DSource::SetCanEnterScene(bool bCanEnter)
{
	this->bCanEnter = bCanEnter;
}

bool DSource::CanEnterScene() const
{
	return bCanEnter;
}

const char* DSource::GetAduioClassName() const
{
	return "DAudioSource";

}

IBaseAudio * DSource::GetAudioRender()
{
	return audioOut;
}

void DSource::RegisterDataCallBack(void *Context, DataCallBack pCb)
{
	__DataCallBack DataBack;
	DataBack.Context = Context;
	DataBack.CallBack = pCb;
	OSEnterMutex(hSampleMutex);
	m_ListCallBack.Add(DataBack);
	OSLeaveMutex(hSampleMutex);
}

void DSource::UnRegisterDataCallBack(void *Context)
{
	OSEnterMutex(hSampleMutex);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		if (OneCallBack.Context == Context)
		{
			m_ListCallBack.Remove(i);
			break;
		}
	}
	OSLeaveMutex(hSampleMutex);
}


