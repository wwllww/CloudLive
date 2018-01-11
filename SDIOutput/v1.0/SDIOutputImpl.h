
#ifndef __SDI_h__
#define __SDI_h__

#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <objbase.h>
#include <comutil.h>
#include <iostream>
#include <map>
#include <stdio.h>

#include "DeckLinkAPI_h.h"
#include "SDIOutput.h"
#include "samplerate.h"
#include "concurrent_queue.h"

class OutputCallback;

#define FRAMECOUNT 4
#define SAFE_RELEASE(punk)  if ((punk) != NULL)  { (punk)->Release(); (punk) = NULL; }

struct ModeMapping
{
	SDIOUT_DISPLAYMODE modeIndex;
	_BMDDisplayMode    mode;
};

static ModeMapping kDisplayModeMappings[] =
{
	{ NTSC, bmdModeNTSC },
	{ PAL, bmdModePAL },

	{ HD1080p2398, bmdModeHD1080p2398 },
	{ HD1080p24, bmdModeHD1080p24 },
	{ HD1080p25, bmdModeHD1080p25 },
	{ HD1080p2997, bmdModeHD1080p2997 },
	{ HD1080p30, bmdModeHD1080p30 },
	{ HD1080i50, bmdModeHD1080i50 },
	{ HD1080i5994, bmdModeHD1080i5994 },
	{ HD1080i6000, bmdModeHD1080i6000 },
	{ HD1080p50, bmdModeHD1080p50 },
	{ HD1080p5994, bmdModeHD1080p5994 },
	{ HD1080p60, bmdModeHD1080p6000 },

	{ HD720p50, bmdModeHD720p50 },
	{ HD720p5994, bmdModeHD720p5994 },
	{ HD720p60, bmdModeHD720p60 }
};

extern BMDTimeValue	g_hardwareTime;
extern BMDTimeValue	g_timeInFrame;
extern BMDTimeValue	g_ticksPerFrame;

extern enum SDIOUT_DISPLAYMODE;
extern enum SDIOUT_COLORFORMAT;

struct ChannelInfo
{
	int							nDeviceID;
	CRITICAL_SECTION	        pMutex;
	unsigned __int32			uiFrameWidth;
	unsigned __int32			uiFrameHeigh;

	IDeckLink*					pDL;
	IDeckLinkOutput*			pDLOutput;
	OutputCallback*             pOutputCallback;
	tbb::concurrent_queue<unsigned char*> m_BufferList;
	unsigned char*				pPreBuffer;

	BMDTimeValue				frameDuration;
	BMDTimeScale				frameTimescale;
	unsigned __int32			uiFPS;
	unsigned __int32			uiTotalFrames;
	unsigned char*				pConvertBuf;

	LARGE_INTEGER				nLastTime1;
	LARGE_INTEGER				nLastTime2;

	SDIOUT_COLORFORMAT colorFormat;
	SDIOUT_DISPLAYMODE displayMode;
	int nPreSrcWidth;
	int nPreSrcHeight;
	int nPreSrcChannelCount;
	int nPreSrcSampleCount;
	int nPreSrcBytePerSample;
	int nPreSrcAudioLength;

	int nBufferTime;
	SRC_STATE *resampler;
	float* pResampleBuffer;
	float* convertBufferFloat;
	short* convertBufferShort;
	short* doubleChannelBuffer;

	bool bStop = false;
	long ref = 0;
	void AddRef()
	{
		InterlockedIncrement(&ref);
	}
	void Release()
	{
		InterlockedDecrement(&ref);
	}

	ChannelInfo()
	{
		nDeviceID = 0;
		uiFrameWidth = 0;
		uiFrameHeigh = 0;
		pDL = NULL;
		pDLOutput = NULL;
		pOutputCallback = NULL;
		frameTimescale = 0;
		frameDuration = 0;
		uiFPS = 0;
		uiTotalFrames = 0;
		pConvertBuf = NULL;
		colorFormat = ColorFormat_I420;
		displayMode = HD1080i50;
		nPreSrcWidth = 0;
		nPreSrcHeight = 0;
		nPreSrcChannelCount = 0;
		nPreSrcSampleCount = 0;
		nPreSrcBytePerSample = 0;
		nPreSrcAudioLength = 0;
		nBufferTime = 0;
		pResampleBuffer = NULL;
		convertBufferFloat = NULL;
		convertBufferShort = NULL;
		doubleChannelBuffer = NULL;
		resampler = NULL;
		pPreBuffer = NULL;
		InitializeCriticalSection(&pMutex);
	}
	~ChannelInfo()
	{
// 		Log::writeMessage(LOG_SDI, 1, "~ChannelInfo%d!", nDeviceID);
	}
};

class SDIOutput
{
private:
	std::list<ChannelInfo*>		listChannelInfo;
	DeviceInfo* m_pDevicelist;
	int			m_nDeviceCount;
	DisplayMode* m_pDisplayModeList;
	int			m_nModeCount;

	mapProperty m_mapProperty;
	mapOutDevicePara m_mapOutDevicePara;

	CRITICAL_SECTION lock;

	void SetPreroll(int nDeviceID, int bufferTime, SDIOUT_COLORFORMAT nColorFormat);

public:
	SDIOutput();
	~SDIOutput();

	int SDI_Init();
	int SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt);
	int SDI_GetDisplayModeList(DisplayMode** pModeList, int* nModeCount);
	int SDI_ReleaseDeviceList();
	int SDI_ReleaseDisplayModeList();
	int SDI_SetDeviceProperty(int deviceID, bool bInput);
	int SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode);
	int SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE mode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime);
	int SDI_RenderDevice(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT colorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM);
	int SDI_StopOut(int nDeviceID);
	mapProperty&  SDI_GetPropertyMap();
	mapOutDevicePara& SDI_GetOutDeviceParaMap();
	bool Restart(bool bAudio, ChannelInfo* pChannelInfo, int nChannel, int nSample, int nBytePerSample, int nAudioLength, bool bParaChange, SDIOUT_COLORFORMAT colorFormat, int nWidth, int nHeight);
	int SDI_Configuration();
};

static LARGE_INTEGER nFreq;
static long long t5, t6, detalCallback;
static BMDTimeValue	hardware2;
static BMDTimeValue	hardware;

class OutputCallback : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback
{
public:
	OutputCallback(IDeckLinkOutput* deckLinkOutput, std::list<ChannelInfo*>* listInfo, int nDeviceID)
	{
		m_deckLinkOutput = deckLinkOutput;
// 		m_deckLinkOutput->AddRef();
		listChannel = listInfo;
		curDeviceID = nDeviceID;
	}
	virtual ~OutputCallback(void)
	{
// 		m_deckLinkOutput->Release();
	}
	HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result)
	{
		IDeckLinkDisplayModeIterator*		pDLDisplayModeIterator = NULL;
		IDeckLinkDisplayMode*				pDLDisplayMode = NULL;

		// When a video frame completes,reschedule another frame
		std::list<ChannelInfo*>::iterator pos = listChannel->begin();
		std::list<ChannelInfo*>::iterator end = listChannel->end();
		for (; pos != end; ++pos)
		{
			if ((*pos) && (*pos)->nDeviceID == curDeviceID && (!listChannel->empty()))
			{
// 				EnterCriticalSection(&(*pos)->pMutex);

// 				unsigned int nCount = 0;
// 				(*pos)->pDLOutput->GetBufferedVideoFrameCount(&nCount);

				int Width = (*pos)->uiFrameWidth;
				int Height = (*pos)->uiFrameHeigh;

				unsigned char* pBuffer = nullptr;
				if ((*pos) && (!((*pos)->m_BufferList.empty())))
				{
					void*	pFrame = nullptr;
					if (completedFrame)
						completedFrame->GetBytes((void**)&pFrame);
					(*pos)->m_BufferList.try_pop(pBuffer);

					(*pos)->Release();

// 					Log::writeMessage(LOG_SDI, 1, "ScheduledFrameCompleted%d Release=%d", curDeviceID, (*pos)->ref);

					if ((*pos)->colorFormat == ColorFormat_RGBA32REVERSE || (*pos)->colorFormat == ColorFormat_RGB32)
					{
						int size = Width * Height * 4;
						if (pFrame && pBuffer)
							memcpy(pFrame, pBuffer, size);
						if (NULL == (*pos)->pPreBuffer)
							(*pos)->pPreBuffer = new unsigned char[size];
						if ((*pos)->pPreBuffer && pBuffer)
							memcpy((*pos)->pPreBuffer, pBuffer, size);
					}
					if ((*pos)->colorFormat == ColorFormat_UYVY || (*pos)->colorFormat == ColorFormat_YUY2
						|| (*pos)->colorFormat == ColorFormat_I420 || (*pos)->colorFormat == ColorFormat_YV12)
					{
						int size = Width * Height * 2;
						if (pFrame && pBuffer)
							memcpy(pFrame, pBuffer, size);
						if (NULL == (*pos)->pPreBuffer)
							(*pos)->pPreBuffer = new unsigned char[size];
						if ((*pos)->pPreBuffer && pBuffer)
							memcpy((*pos)->pPreBuffer, pBuffer, size);
					}
					if (pBuffer)
					{
						delete[] pBuffer;
						pBuffer = nullptr;
					}
				}
				else
				{
					void*	pFrame = nullptr;
					if (completedFrame)
						completedFrame->GetBytes((void**)&pFrame);
					if ((*pos) && (*pos)->pPreBuffer && pFrame && completedFrame)
					{
						if ((*pos)->colorFormat == ColorFormat_RGBA32REVERSE || (*pos)->colorFormat == ColorFormat_RGB32)
						{
							if (completedFrame->GetWidth() == Width && completedFrame->GetHeight() == Height)
								memcpy(pFrame, (*pos)->pPreBuffer, Width * Height * 4);
							else
							{
								Log::writeMessage(LOG_SDI, 1, "completedFrame Width %d,completedFrame Height %d,Width %d,Height %d", completedFrame->GetWidth(), completedFrame->GetHeight(),Width,Height);
							}
						}
						if ((*pos)->colorFormat == ColorFormat_UYVY || (*pos)->colorFormat == ColorFormat_YUY2
							|| (*pos)->colorFormat == ColorFormat_I420 || (*pos)->colorFormat == ColorFormat_YV12)
						{
							if (completedFrame->GetWidth() == Width && completedFrame->GetHeight() == Height)
									memcpy(pFrame, (*pos)->pPreBuffer, Width * Height * 2);
							else
							{
								Log::writeMessage(LOG_SDI, 1, "completedFrame Width %d,completedFrame Height %d,Width %d,Height %d", completedFrame->GetWidth(), completedFrame->GetHeight(), Width, Height);
							}
						}
					}
// 					Log::writeMessage(LOG_SDI, 1, "ScheduledFrameCompleted%d Else=%d", curDeviceID, (*pos)->ref);
				}

// 				QueryPerformanceFrequency(&nFreq);
// 				QueryPerformanceCounter(&(*pos)->nLastTime2);
// 				float fInterval = ((*pos)->nLastTime2.QuadPart - (*pos)->nLastTime1.QuadPart) * 1000;
// 				detalCallback = fInterval / (float)nFreq.QuadPart;
// 				Log::writeMessage(LOG_SDI, 1, "OutputCallback%d间隔=%d, 自己缓冲帧数=%d, SDI缓冲帧数=%d，ret=%d!", curDeviceID, detalCallback, (*pos)->m_BufferList.unsafe_size(), nCount, result);
// 				QueryPerformanceCounter(&(*pos)->nLastTime1);

				if (!(*pos)->bStop)
				{
// 					Log::writeMessage(LOG_SDI, 1, "ScheduleVideoFrame%d!引用计数=%d", curDeviceID, (*pos)->ref);

					m_deckLinkOutput->ScheduleVideoFrame(completedFrame, (*pos)->uiTotalFrames * (*pos)->frameDuration, (*pos)->frameDuration, (*pos)->frameTimescale);
					(*pos)->uiTotalFrames++;
				}

// 				t6 = GetTickCount64();
// 				detalCallback = t6 - t5;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice detalCallback = %d!", detalCallback);
// 				t5 = GetTickCount64();

// 				BMDTimeValue	timeInFrame;
// 				BMDTimeValue	ticksPerFrame;
// 				m_deckLinkOutput->GetHardwareReferenceClock((*pos)->frameTimescale, &hardware, &timeInFrame, &ticksPerFrame);
// 				detalCallback = (hardware - hardware2) * ticksPerFrame / (*pos)->frameTimescale;
// 				Log::writeMessage(LOG_SDI, 1, "OutputCallback_间隔=%d_ret=%d!", detalCallback, result);
// 				m_deckLinkOutput->GetHardwareReferenceClock((*pos)->frameTimescale, &hardware2, &timeInFrame, &ticksPerFrame);

// 				LeaveCriticalSection(&(*pos)->pMutex);
				break;
			}
		}
		return S_OK;
	}

	HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped(void)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	RenderAudioSamples(BOOL preroll)
	{
		return S_OK;
	}

	HRESULT	STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE AddRef() { return 1; }
	ULONG STDMETHODCALLTYPE Release() { return 1; }
private:
	IDeckLinkOutput*	   m_deckLinkOutput;
	std::list<ChannelInfo*>* listChannel = nullptr;
	int	curDeviceID;

    int count_cb = 0;
	long long t3, t4, detal_cb;
};

#endif      
