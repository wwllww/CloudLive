#include "PipeVideoPlugin.h"
#include "MediaInfoStuff.h"
#include "CaptureFilter.h"
#include "PCMRecord.h"

#define INP_BUFFER_SIZE (2 * 1024)

extern IBaseFilter* GetExceptionDevice(CTSTR lpGUID);
const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 } };
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
			for (int i = 0; i < count; i++)
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

PCMRecord::PCMRecord(std::wstring auidoName, void* data)
{
	m_auidoName = auidoName;
	clientData = data;
	m_callBack = NULL;
	bCapturing = false;
	bFiltersLoaded = false;
	hThreadCheckDevice = NULL;
	m_hCheckExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

PCMRecord::~PCMRecord()
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
}

int PCMRecord::InitParam(int nSamplesRate, int nAudioChannel, int nBitPerSample)
{
	waveform.nSamplesPerSec = nSamplesRate;
	waveform.wBitsPerSample = nBitPerSample;
	waveform.nChannels = nAudioChannel;
	waveform.nAvgBytesPerSec = nSamplesRate * sizeof(unsigned short);
	waveform.nBlockAlign = waveform.nChannels * waveform.wBitsPerSample / 8;
	waveform.cbSize = 0;       //PCM编码时，此处为0
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  waveform.nChannels  = %d.nSamplesRate = %d.", __LINE__, String(__FUNCTION__).Array(), waveform.nChannels, nSamplesRate);
// 	//创建线程处理函数
// 	M_PCMThread = ::CreateThread(NULL, 0, PCMCallThread, this, 0, &M_PCMThreadID);


	ReStartCaptrue();

	hThreadCheckDevice = OSCreateThread((XTHREAD)CheckDeviceThread, this);

	if (bFiltersLoaded)
	{
		return 0;
	}

	return -1;
}

void PCMRecord::SetCallback(AudioDataCallback callback)
{
	m_mutex.lock();
	m_callBack = callback;
	m_mutex.unlock();
}

void PCMRecord::RecordStart()     //录音准备
{
	if (bCapturing)
		return;
	Start();
}

void PCMRecord::RecordStop()
{
	Stop();
}

void PCMRecord::Start()
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
	m_qwrdAudioTime = GetQPCMS();
}


void PCMRecord::Stop()
{
	if (!bCapturing)
		return;

	if (control)
	{
		control->Stop();
	}

	bCapturing = false;
}

void PCMRecord::SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType, IAMStreamConfig *audioConfig)
{
	expectedAudioType = audioMediaType->subtype;

	if (audioMediaType->formattype == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX *pFormat = reinterpret_cast<WAVEFORMATEX*>(audioMediaType->pbFormat);
		mcpy(&audioFormat, pFormat, sizeof(audioFormat));

		pFormat->nSamplesPerSec = waveform.nSamplesPerSec;
		pFormat->wBitsPerSample = waveform.wBitsPerSample;
		pFormat->nChannels = waveform.nChannels;
		pFormat->nAvgBytesPerSec = waveform.nAvgBytesPerSec;
		pFormat->nBlockAlign = waveform.nBlockAlign;

		if (SUCCEEDED(audioConfig->SetFormat(audioMediaType)))
		{
			mcpy(&audioFormat, pFormat, sizeof(audioFormat));

			Log::writeMessage(LOG_RTSPSERV, 1, " audioConfig->SetFormat  SUCCEEDED  device audio info - bits per sample: %u, channels: %u, samples per sec: %u, block size: %u",
				audioFormat.wBitsPerSample, audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.nBlockAlign);
		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "  audioConfig->SetFormat Failed  device audio info - bits per sample: %u, channels: %u, samples per sec: %u, block size: %u",
				audioFormat.wBitsPerSample, audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.nBlockAlign);
		}


	
	}
	else
	{
		AppWarning(TEXT("DShowPlugin: Audio format was not a normal wave format"));
		soundOutputType = 0;
	}

	DeleteMediaType(audioMediaType);
}

bool PCMRecord::ReStartCaptrue()
{
	bool bWasCapturing = bCapturing;
	Stop();

	UnloadAudioInputDevice();
	LoadAudioInputDevice();

	if (bWasCapturing)
	{
		Start();
		m_qwrdAudioTime = GetQPCMS();
	}

	return true;
}

DWORD PCMRecord::CheckDevice()
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
			if (bNeedCheck && CheckDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", m_auidoName.c_str()))
			{
				ReStartCaptrue();

				SleepTime = 3000;
				hr = m_pEvent->GetEventHandle((OAEVENT*)&hEventArray[0]);
			}
		}
		else if ((WAIT_OBJECT_0 + 1) == result)
		{
			break;
		}
	}

	return 0;
}

DWORD STDCALL PCMRecord::CheckDeviceThread(LPVOID *pData)
{
	PCMRecord *pThis = (PCMRecord *)pData;
	return pThis->CheckDevice();
}

void PCMRecord::ReceiveMediaSample(IMediaSample *sample, bool bAudio)
{
	if (!bAudio || !bCapturing)
	{
		return;
	}

	BYTE *pointer;

	if (!sample->GetActualDataLength())
		return;

	if (SUCCEEDED(sample->GetPointer(&pointer)))
	{
		m_qwrdAudioTime = GetQPCMS();
		m_mutex.lock();
		if (m_callBack)
		{
			m_callBack((LPSTR)pointer, sample->GetActualDataLength(), audioFormat, clientData);
		}
		m_mutex.unlock();
	}
}

void PCMRecord::UnloadAudioInputDevice()
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

	SafeReleaseLogRef(capture);
	SafeReleaseLogRef(graph);
	SafeReleaseLogRef(m_pEvent);
	SafeRelease(control);
}

bool PCMRecord::LoadAudioInputDevice()
{
	//= == == == == == == == == == == == == == =
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

	soundOutputType = 1;

	if (!bDeviceHasAudio)
	{
		if (!m_auidoName.empty())
		{
			audioDeviceFilter = GetDeviceByValue(CLSID_AudioInputDeviceCategory, L"FriendlyName", m_auidoName.c_str(), L"DevicePath", NULL);
			if (!audioDeviceFilter)
				Log::writeError(LOG_RTSPSERV, 1, "DShowPlugin: Invalid audio device: name '%s'", WcharToAnsi(m_auidoName.c_str()).c_str());
		}

		if (audioDeviceFilter)
			err = capture->FindPin(audioDeviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &audioPin);
		else
			err = E_FAIL;

		if (FAILED(err) || !audioPin)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "DShowPlugin: No audio pin, result = %lX", err);
			soundOutputType = 0;
		}

		GUID expectedAudioType;

		if (soundOutputType == 1)
		{
			IAMStreamConfig *audioConfig;
			if (SUCCEEDED(audioPin->QueryInterface(IID_IAMStreamConfig, (void**)&audioConfig)))
			{
				AM_MEDIA_TYPE *audioMediaType;
				if (SUCCEEDED(err = audioConfig->GetFormat(&audioMediaType)))
				{
					SetAudioInfo(audioMediaType, expectedAudioType, audioConfig);
				}
				else
				{
					Log::writeError(LOG_RTSPSERV, 1, "DShowPlugin: Could not get audio format, result = %08lX", err);
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
				Log::writeError(LOG_RTSPSERV, 1, "PCMRecord Failed to create audio capture filter");
				soundOutputType = 0;
			}
		}

		if (soundOutputType != 0)
		{
			if (FAILED(err = graph->AddFilter(audioFilter, NULL)))
				Log::writeError(LOG_RTSPSERV, 1, "PCMRecord DShowPlugin: Failed to add audio capture filter to graph, result = %08lX", err);

			bAddedAudioCapture = true;
		}

		//------------------------------------------------
		// add primary device filter

		if (soundOutputType != 0 && !bDeviceHasAudio)
		{
			if (FAILED(err = graph->AddFilter(audioDeviceFilter, NULL)))
				Log::writeError(LOG_RTSPSERV, 1, "PCMRecord DShowPlugin: Failed to add audio device filter to graph, result = %08lX", err);
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
					Log::writeMessage(LOG_RTSPSERV, 1, "PCMRecord 设置音频参数%d", prop.cbAlign);
				}

				if (!bDeviceHasAudio)
					bConnected = SUCCEEDED(err = capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, audioDeviceFilter, NULL, audioFilter));
			}

			if (!bConnected)
			{
				Log::writeError(LOG_RTSPSERV, 1, "PCMRecord DShowPlugin: Failed to connect the audio device pin to the audio capture pin, result = % 08lX", err);
				soundOutputType = 0;
			}
		}
		//------------------------------------------------
		// connect all pins and set up the whole capture thing

		if (FAILED(err = graph->QueryInterface(IID_IMediaControl, (void**)&control)))
		{
			Log::writeError(LOG_RTSPSERV, 1, "PCMRecord DShowPlugin: Failed to get IMediaControl, result = %08lX", err);
			goto cleanFinish;
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
}

std::wstring PCMRecord::GetAuidoName() const
{
	return m_auidoName;
}



