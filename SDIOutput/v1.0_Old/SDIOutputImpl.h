
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

class OutputCallback;
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
// 	OutputCallback*             pOutputCallback;
	IDeckLinkMutableVideoFrame*	pDLVideoFrame;

	BMDTimeValue				frameDuration;
	BMDTimeScale				frameTimescale;
	unsigned __int32			uiFPS;
	unsigned __int32			uiTotalFrames;
	unsigned char*				pConvertBuf;

	SDIOUT_COLORFORMAT colorFormat;
	SDIOUT_DISPLAYMODE displayMode;
	int nPreSrcWidth;
	int nPreSrcHeight;
	int nPreSrcChannelCount;
	int nPreSrcSampleCount;

	int nBufferTime;
	SRC_STATE *resampler;
	float* pResampleBuffer;
	float* convertBufferFloat;
	short* convertBufferShort;
	short* doubleChannelBuffer;

	ChannelInfo()
	{
		nDeviceID = 0;
		uiFrameWidth = 0;
		uiFrameHeigh = 0;
		pDL = NULL;
		pDLOutput = NULL;
// 		pOutputCallback = NULL;
		pDLVideoFrame = NULL;
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
		nBufferTime = 0;
		pResampleBuffer = NULL;
		convertBufferFloat = NULL;
		convertBufferShort = NULL;
		doubleChannelBuffer = NULL;
		resampler = NULL;
		InitializeCriticalSection(&pMutex);
	}
	~ChannelInfo()
	{
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

	void ResetFrame(int nDeviceID);
	void SetPreroll(int nDeviceID, int bufferTime);

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
	bool Restart(bool bAudio, ChannelInfo* pChannelInfo, int nChannel, int nSample, bool bParaChange, SDIOUT_COLORFORMAT colorFormat, int nWidth, int nHeight);
};

// static long long t5, t6, detalCallback;
// 
// class OutputCallback : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback
// {
// public:
// 	OutputCallback(IDeckLinkOutput* deckLinkOutput, std::list<ChannelInfo*>& listInfo, int nDeviceID)
// 	{
// 		m_deckLinkOutput = deckLinkOutput;
// 		m_deckLinkOutput->AddRef();
// 		listChannel = listInfo;
// 		curDeviceID = nDeviceID;
// 	}
// 	virtual ~OutputCallback(void)
// 	{
// 		m_deckLinkOutput->Release();
// 	}
// 	HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result)
// 	{
// 		IDeckLinkDisplayModeIterator*		pDLDisplayModeIterator = NULL;
// 		IDeckLinkDisplayMode*				pDLDisplayMode = NULL;
// 
// // 		_BMDDisplayMode DisplayMode = getDisplayMode(kDisplayModeMappings, (SDIOUT_DISPLAYMODE)7);
// // 		if (m_deckLinkOutput->GetDisplayModeIterator(&pDLDisplayModeIterator) == S_OK)
// // 		{
// // 			while (pDLDisplayModeIterator->Next(&pDLDisplayMode) == S_OK)
// // 			{
// // 				if (pDLDisplayMode->GetDisplayMode() == bmdModeHD1080i50)
// // 				{
// // 					BMDFieldDominance filedDominance = pDLDisplayMode->GetFieldDominance();
// // 					int a = 0;
// // 					break;
// // 				}
// // 			}
// // 			if (pDLDisplayModeIterator)
// // 			{
// // 				delete pDLDisplayModeIterator;
// // 				pDLDisplayModeIterator = NULL;
// // 			}
// // 		}
// // 
// // 		if (count_cb == 0)
// // 		{
// // 			t3 = GetTickCount64();
// // 		}
// // 		count_cb++;
// // 		unsigned int a = 0;
// // 		m_deckLinkOutput->GetBufferedVideoFrameCount(&a);
// // 		if (count_cb == 100)
// // 		{	
// // // 			void*	pFrame;
// // // 			completedFrame->GetBytes(&pFrame);
// // 
// // // 			static FILE *fp1, *fp2 = NULL;
// // // 			fp1 = fopen("SDI1.rgb", "wb");
// // 			//int width = 1920, height = 1080;			//char BMPHeader[54];
// // 			//char* pBMPFileHeader = BMPHeader;
// // 			//char* pBMPInfoHeader = pBMPFileHeader + 14;
// // 			//BITMAPFILEHEADER tBMPFileHeader;
// // 			//BITMAPINFOHEADER tBMPInfoHeader;
// // 			//memset(BMPHeader, 0, 54);
// // 			//int nDstDataLen = width * height * 4;
// // 			//*((short*)(pBMPFileHeader + 0)) = 0x4d42; //"BM"
// // 			//*((int*)(pBMPFileHeader + 2)) = nDstDataLen + 54;
// // 			//*((int*)(pBMPFileHeader + 10)) = 54;
// // 
// // 			//*((int*)(pBMPInfoHeader + 0)) = 40;
// // 			//*((int*)(pBMPInfoHeader + 4)) = width;
// // 			//*((int*)(pBMPInfoHeader + 8)) = height;
// // 			//*((short*)(pBMPInfoHeader + 12)) = 1;
// // 			//*((short*)(pBMPInfoHeader + 14)) = 32;
// // 			//*((int*)(pBMPInfoHeader + 16)) = 0;
// // 			//*((int*)(pBMPInfoHeader + 26)) = nDstDataLen;
// // 
// // 			//fwrite(BMPHeader, 1, 54, fp1);			//fwrite(pFrame, 1, 1920 * 1080 * 4, fp1);
// // 			t4 = GetTickCount64();
// // 			detal_cb = t4 - t3;
// // 			count_cb = 0;
// // 		}
// // 		Log::writeMessage(LOG_SDI, 1, "%d", count_cb);
// // 		Log::writeMessage(LOG_SDI, 1, "%d", result);
// // 		Log::writeMessage(LOG_SDI, 1, "%d", a);
// 
// 		// When a video frame completes,reschedule another frame
// 		std::list<ChannelInfo*>::iterator pos = listChannel.begin();
// 		std::list<ChannelInfo*>::iterator end = listChannel.end();
// 		for (; pos != end; ++pos)
// 		{
// 			if ((*pos)->nDeviceID == curDeviceID)
// 			{
// 				EnterCriticalSection(&(*pos)->pMutex);
// 				m_deckLinkOutput->ScheduleVideoFrame(completedFrame, (*pos)->uiTotalFrames * (*pos)->frameDuration, (*pos)->frameDuration, (*pos)->frameTimescale);
// 				(*pos)->uiTotalFrames++;
// 
// 				t6 = GetTickCount64();
// 				detalCallback = t6 - t5;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice detalCallback = %d!", detalCallback);
// 				t5 = GetTickCount64();
// 
// 				LeaveCriticalSection(&(*pos)->pMutex);
// 				break;
// 			}
// 		}
// 		return S_OK;
// 	}
// 
// 	HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped(void)
// 	{
// 		return S_OK;
// 	}
// 
// 	HRESULT STDMETHODCALLTYPE	RenderAudioSamples(BOOL preroll)
// 	{
// 		return S_OK;
// 	}
// 
// 	HRESULT	STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
// 	ULONG STDMETHODCALLTYPE AddRef() { return 1; }
// 	ULONG STDMETHODCALLTYPE Release() { return 1; }
// private:
// 	IDeckLinkOutput*	   m_deckLinkOutput;
// 	std::list<ChannelInfo*> listChannel;
// 	int	curDeviceID;
// 
//     int count_cb = 0;
// 	long long t3, t4, detal_cb;
// 
// };

#endif      
