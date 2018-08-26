/**
* copyright : zhanghaitian@butel
*/

#include "VideoSource.h"
#include "VideoSourceConfig.h"
#include "MediaAudio.h"
#include <process.h>

/****************************************************/
/*Function : VolumeCaculate
Action : Calculate volume
Parameters ::
buf : Input&Output, Audio data buffer;
size: Input, Length of buf;
vol : Input, Level of Volume;
*/
/****************************************************/

__int64 GetSysTickCount64()
{
	static LARGE_INTEGER TicksPerSecond = { 0 };
	LARGE_INTEGER Tick;
	if (!TicksPerSecond.QuadPart)
	{
		QueryPerformanceFrequency(&TicksPerSecond);
	}

	QueryPerformanceCounter(&Tick);

	__int64 Seconds = Tick.QuadPart / TicksPerSecond.QuadPart;
	__int64 LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart*Seconds);
	__int64 MillSeconds = LeftPart * 1000 / TicksPerSecond.QuadPart;
	__int64 Ret = Seconds * 1000 + MillSeconds;
	return Ret;
};

static void VolumeCaculate(char* buf, UINT32 size, double vol)//buf为需要调节音量的音频数据块首地址指针，size为长度，uRepeat为重复次数，通常设为1，vol为增益倍数,可以小于1
{
	if (!size)
	{
		return;
	}
	for (int i = 0; i < size;)
	{
		signed long minData = -0x8000; //如果是8bit编码这里变成-0x80
		signed long maxData = 0x7FFF;//如果是8bit编码这里变成0xFF

		signed short wData = buf[i + 1];
		wData = MAKEWORD(buf[i], buf[i + 1]);
		signed long dwData = wData;

		dwData = dwData * vol;
		if (dwData < -0x8000)
		{
			dwData = -0x8000;
		}
		else if (dwData > 0x7FFF)
		{
			dwData = 0x7FFF;
		}
		wData = LOWORD(dwData);
		buf[i] = LOBYTE(wData);
		buf[i + 1] = HIBYTE(wData);
		i += 2;
	}
}

IMPLEMENT_DYNIC(VideoSource, "点播源", "1.0.0.1");

VideoSource::VideoSource()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,Using Demand Video Source, This = %x"), __LINE__, __FUNCTION__, this);

	//初始化锁，注册回调使用
	InitializeCriticalSection(&m_secDataLock);
	InitializeCriticalSection(&m_secAudioDataLock);
	InitializeCriticalSection(&m_secTextureDataLock);
	InitializeCriticalSection(&m_secCallBackLock);
	ZeroMemory(&m_wavAudioFormat, sizeof WAVEFORMATEX);
}

bool VideoSource::Init(Value &JsonParam)
{
	//创建SDK实例
	m_hMediaProcess = mp_create(4);
	if (!m_hMediaProcess)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,m_hMediaProcess 创建失败，返回错误. This = %x"), __LINE__, __FUNCTION__, this);
		return false;
	}

	//初始化媒体数据使用变量
	m_height = 0;
	m_width = 0;
	m_nMediaDuration = 0;
	m_pAudioSample = NULL;
	m_pLatestVideoSample = NULL;
	m_pDemandMediaAudio = new CDemandMediaAudio;

	//渲染使用
	m_nEnteredSceneCount = 0;
	m_vectVideoSize = Vect2(640, 360);
	m_pTexture = NULL;
	m_pColorConvertShader = NULL;
	m_pColorFieldConvertShader = NULL;
	memset(m_playPath, 0, PATH_MAX_);
	Captureing = false;
	m_pD3DRender = GetD3DRender();
	m_MediaState = MediaStop;
	m_pDeinterlacer = NULL;//new CDeinterlacer;
	m_pRGBFieldShader = m_pD3DRender->CreatePixelShaderFromFile(L"shaders/Field_RGB.pShader");

	//音视频同步使用
	m_hPauseSyncThreadEvent = CreateEvent(NULL, true, false, NULL);
	m_hSyncThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThread, this, 0, NULL);
	if (!m_hSyncThread)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,同步线程创建失败，返回错误. This = %x"), __LINE__, __FUNCTION__, this);
		return false;
	}

	m_hMsgProcessThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MsgProcessThread, this, 0, NULL);
	if (!m_hMsgProcessThread)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,消息线程创建失败，返回错误. This = %x"), __LINE__, __FUNCTION__, this);
		return false;
	}

	m_pConfig = new VideoSourceConfig(JsonParam);
	m_DeinterConfig.processor = DEINTERLACING_PROCESSOR_GPU;
	m_DeinterConfig.doublesFramerate = false;
	m_DeinterConfig.fieldOrder = FIELD_ORDER_TFF | FIELD_ORDER_BFF;
	m_DeinterConfig.type = DEINTERLACING_YADIF;

	UpdateSettings(JsonParam);

	return true;
}

VideoSource::~VideoSource()
{ 
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s  VideoSource Demand Destructor,This = %x"), __LINE__, __FUNCTION__,this);
	m_bExit = true;

// 	ColseFile();
	
	if (m_hMsgProcessThread)
	{
		//发送退出消息
		VideoMessageType MessageType = VideoExit;
		m_pMessageQueue.push(MessageType);
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s 发送退出消息循环线程消息! This = %x"), __LINE__, __FUNCTION__, this);
		WaitForSingleObject(m_hMsgProcessThread, 5000);
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s VideoSource 成功释放消息线程! This = %x"), __LINE__, __FUNCTION__, this);
		}
		OSCloseThread(m_hMsgProcessThread);
	}

	if (m_MediaState = MediaStop)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,设置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		SetEvent(m_hPauseSyncThreadEvent);
	}

	if (m_hSyncThread)
	{
		WaitForSingleObject(m_hSyncThread, 5000);
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s VideoSource 成功释放同步线程! This = %x"), __LINE__, __FUNCTION__,this);
		}
		OSCloseThread(m_hSyncThread);
	}

	if (m_hMediaProcess)
	{
		mp_destroy(m_hMediaProcess);
		m_hMediaProcess = NULL;
	}

	if (m_pDemandMediaAudio)
		delete m_pDemandMediaAudio;

    if (m_pTexture) {
        delete m_pTexture;
        m_pTexture = nullptr;
    }
	
    delete m_pConfig;
    m_pConfig = nullptr;
	
	DestoryCSampleData();

	if (m_pDeinterlacer)
		delete m_pDeinterlacer;

	if (m_pColorConvertShader)
		delete m_pColorConvertShader;
	m_pColorConvertShader = NULL;

	if (m_pColorFieldConvertShader)
		delete m_pColorFieldConvertShader;
	m_pColorFieldConvertShader = NULL;

	if (m_pRGBFieldShader)
		delete m_pRGBFieldShader;
	m_pRGBFieldShader = NULL;

	if (m_hPauseSyncThreadEvent)
	{
		CloseHandle(m_hPauseSyncThreadEvent);
	}
	
	DeleteCriticalSection(&m_secDataLock);
	DeleteCriticalSection(&m_secAudioDataLock);
	DeleteCriticalSection(&m_secTextureDataLock);
	DeleteCriticalSection(&m_secCallBackLock);
}

unsigned VideoSource::VideoFormatCallback(unsigned *width, unsigned *height)
{	
	EnterCriticalSection(&m_secTextureDataLock);
	m_height = *height;
	m_width = *width;
	if (Captureing)
	{
		DWORD Width = 0, Height = 0;
		m_pD3DRender->GetTextureWH(m_pTexture, Width, Height);
		if (!m_pTexture || Width != *width || Height != *height) 
		{
			if (m_pTexture) 
			{
				delete m_pTexture;
				m_pTexture = nullptr;
			}
			if (!m_pTexture)
			{
				LPBYTE pData = (LPBYTE)Allocate_Bak(m_width*m_height * 4);
				if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
				{
					msetd(pData, 0x00808000, m_width*m_height * 4);
					m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_RGBA, pData, FALSE, FALSE);
				}
				else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
				{
					msetd(pData, 0x00000000, m_width*m_height * 4);
					m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_BGRA, pData, FALSE, FALSE);
				}
				else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
				{
					msetd(pData, 0x00000000, m_width*m_height * 4);
					m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_BGR, pData, FALSE, FALSE);
				}
				Free(pData);
			}
		}
	}   
	LeaveCriticalSection(&m_secTextureDataLock);

    m_nMediaWidthOffset = 0;
    m_nMediaHeightOffset = 0;
    
    m_nMediaWidth = *width;
    m_nMediaHeight = *height;
    
    if (!m_pConfig->isStretching) {
        float srcAspect = (float)*width / (float)*height;
        float dstAspect = (float)m_pConfig->width / (float)m_pConfig->height;

        if (srcAspect > dstAspect) {
            if(m_pConfig->width != (*width) ) { //don't scale if size equal
                m_nMediaWidth  = m_pConfig->width;
                m_nMediaHeight = static_cast<unsigned>(m_nMediaWidth / srcAspect + 0.5);
            }
            m_nMediaHeightOffset = (m_pConfig->height - m_nMediaHeight) / 2;
        } else {
            if( m_pConfig->height != (*height) ) { //don't scale if size equal
                m_nMediaHeight = m_pConfig->height;
                m_nMediaWidth  = static_cast<unsigned>(m_nMediaHeight * srcAspect + 0.5);
            }
            m_nMediaWidthOffset = (m_pConfig->width - m_nMediaWidth) / 2;
        }
    } else {
        m_nMediaWidth = m_pConfig->width;
        m_nMediaHeight = m_pConfig->height;
    }
    m_vectPreviousRenderSize.x = m_vectPreviousRenderSize.y = 0;
    return 1;
}

void VideoSource::GlobalSourceEnterScene()
{
	if (m_nEnteredSceneCount++)
		return;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,点播源进入场景! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	if (m_pConfig && m_pConfig->isSenceChangeStartPlay)
	{
		VideoMessageType MessageType = VideoPlay;
		m_pMessageQueue.push(MessageType);
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,投递一个消息到消息队列，消息类型:%d.! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, MessageType, GetCurrentThreadId(), this);
	}
	else
	{
		PlayMedia();
	}
	AudioParam Param;
	Param.iBitPerSample = m_wavAudioFormat.wBitsPerSample;
	Param.iChannel = m_wavAudioFormat.nChannels;
	Param.iSamplesPerSec = m_wavAudioFormat.nSamplesPerSec;
	EnterLiveVideoSection();
	if (Param.iBitPerSample > 0)
		m_pDemandMediaAudio->ResetAudioParam(Param);
	LeaveLiveVideoSection();
}

void VideoSource::GlobalSourceLeaveScene()
{
	if (!m_nEnteredSceneCount)
		return;
	if (--m_nEnteredSceneCount)
		return;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,点播源离开场景! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	PauseMedia();
	if (m_pConfig && m_pConfig->isSenceChangeStartPlay)
	{
		VideoMessageType MessageType = VideoReset;
		m_pMessageQueue.push(MessageType);
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,投递一个消息到消息队列，消息类型:%d.! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, MessageType, GetCurrentThreadId(), this);
	}
}

void VideoSource::Preprocess()
{
	if (!Captureing)
	{
		return;
	}
	
	CSampleData *lastSample = NULL;
	MPFrameInfo * lastMPFrameInfo = NULL;
	EnterCriticalSection(&m_secDataLock);
	lastSample = m_pLatestVideoSample;
	m_pLatestVideoSample = NULL;
	lastMPFrameInfo = m_pLastMPFrameInfo;
	m_pLastMPFrameInfo = NULL;
	LeaveCriticalSection(&m_secDataLock);
	
	LPBYTE lpData;
	UINT pitch;

	EnterCriticalSection(&m_secTextureDataLock);
	if (Captureing&&lastSample&&m_pTexture)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			if (S_OK == m_pD3DRender->Map(m_pTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				if (lastSample->cx == m_width && lastSample->cy == m_height && lastSample->dataLength == m_width * m_height * 3 / 2)
					PackPlanar(lpData, lastSample->lpData, m_width, m_height, pitch, 0, m_height, m_width, 0);
				m_pD3DRender->Unmap(m_pTexture);
			}
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt) {
			if (lastSample->cx == m_width && lastSample->cy == m_height && lastSample->dataLength == m_width * m_height * 4)
				m_pD3DRender->SetImage(GetTexture(), lastSample->lpData, GS_IMAGEFORMAT_BGRA, m_width * 4);
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			if (lastSample->cx == m_width && lastSample->cy == m_height && lastSample->dataLength == m_width * m_height * 3)
				m_pD3DRender->SetImage(GetTexture(), lastSample->lpData, GS_IMAGEFORMAT_BGR, m_width * 3);
		}

		if (m_pDeinterlacer)
		{
			if (lastSample->cx == m_width && lastSample->cy == m_height)
				m_pDeinterlacer->SetImage(m_pTexture, &m_DeinterConfig, m_width, m_height, m_ColorType);
		}
		UINT64 nHigeCheckNum = lastSample->CheckNum & 0xFF00000000000000;
		if (nHigeCheckNum != 0) //最高字节不为0
		{
			lastSample->Release();
		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 最高位校验值为0，不释放: uCurrentCheckNum = 0x%p,lastSample->CheckNum =0x%p.nHigeCurrrntNum = 0x%p.nHigeCheckNum = 0x%p.lastSample->lpData :0x%p.lastSample :0x%p.ThreadID = %d,This = 0x%p"),
				__LINE__, __FUNCTION__, 0, lastSample->CheckNum, 0, nHigeCheckNum, lastSample->lpData, lastSample, GetCurrentThreadId(), this);
		}
	}
	LeaveCriticalSection(&m_secTextureDataLock);

	EnterCriticalSection(&m_secDataLock);
	if (lastMPFrameInfo)
	{
		ReleaseFrame(m_hMediaProcess, lastMPFrameInfo, 1);
		lastMPFrameInfo = NULL;
	}
	LeaveCriticalSection(&m_secDataLock);
}

void VideoSource::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	if (!Captureing || !m_pMPMediaInfo.has_video)
	{
		return;
	}
	if (m_vectPreviousRenderSize != size || m_bScaleFull != bScaleFull) {

		m_bScaleFull = bScaleFull;
		m_nMediaHeightOffset = 0;
		m_nMediaWidthOffset = 0;

		if (bScaleFull)
		{
			m_nMediaWidth = size.x;
			m_nMediaHeight = size.y;
		}
		else
		{
			float srcAspect = (float)m_width / (float)m_height;
			float dstAspect = (float)size.x / (float)size.y;
			if (srcAspect > dstAspect) {

				m_nMediaWidth = size.x;
				m_nMediaHeight = static_cast<unsigned>(m_nMediaWidth / srcAspect + 0.5);
				m_nMediaHeightOffset = (size.y - m_nMediaHeight) / 2;
			}
			else {
				m_nMediaHeight = size.y;
				m_nMediaWidth = static_cast<unsigned>(m_nMediaHeight * srcAspect + 0.5);
				m_nMediaWidthOffset = (size.x - m_nMediaWidth) / 2;
			}
		}
		m_vectMediaOffset.x = (float)m_nMediaWidthOffset;
		m_vectMediaOffset.y = (float)m_nMediaHeightOffset;

		m_vectMediaSize.x = (float)m_nMediaWidth;
		m_vectMediaSize.y = (float)m_nMediaHeight;

		m_vectMediaSize += m_vectMediaOffset;
		
		m_vectPreviousRenderSize.x = size.x;
		m_vectPreviousRenderSize.y = size.y;

	}	
	
	EnterCriticalSection(&m_secTextureDataLock);
	if (m_pTexture&&Captureing&&m_bReadyDraw) {

		if (m_pDeinterlacer)
		{
			m_pDeinterlacer->RenderTexture(Vect2(pos.x + m_vectMediaOffset.x, pos.y + m_vectMediaOffset.y), Vect2(pos.x + m_vectMediaSize.x, pos.y + m_vectMediaSize.y));
		}
		else
		{
			if (FilterTexture)
			{
				Shader *oldShader = NULL;
				if (m_bIsFieldSignal && m_vectMediaSize != m_vectVideoSize)
				{
					oldShader = m_pD3DRender->GetCurrentPixelShader();
					if (m_pRGBFieldShader)
					{
						m_pD3DRender->LoadPixelShader(m_pRGBFieldShader);
					}
				}

				m_pD3DRender->DrawSprite(FilterTexture, (opacity255 << 24) | 0xFFFFFF, pos.x + m_vectMediaOffset.x, pos.y + m_vectMediaOffset.y, pos.x + m_vectMediaSize.x, pos.y + m_vectMediaSize.y);

				if (m_bIsFieldSignal && m_vectMediaSize != m_vectVideoSize)
				{
					m_pD3DRender->LoadPixelShader(oldShader);
				}
			}
			else
			{
				if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
				{
					Shader *oldShader = m_pD3DRender->GetCurrentPixelShader();
					float fGamma = 1.0f;

					if (m_vectMediaSize == m_vectVideoSize || !m_bIsFieldSignal)
					{
						if (m_pColorConvertShader)
						{
							m_pD3DRender->LoadPixelShader(m_pColorConvertShader);
							m_pColorConvertShader->SetFloat(m_pColorConvertShader->GetParameterByName(TEXT("gamma")), fGamma);
						}
					}
					else if (m_bIsFieldSignal && m_pColorFieldConvertShader)
					{
						m_pD3DRender->LoadPixelShader(m_pColorFieldConvertShader);
						m_pColorFieldConvertShader->SetFloat(m_pColorFieldConvertShader->GetParameterByName(TEXT("gamma")), fGamma);
					}

					if (m_pTexture)
						m_pD3DRender->DrawSprite(m_pTexture, (opacity255 << 24) | 0xFFFFFF, pos.x + m_vectMediaOffset.x, pos.y + m_vectMediaOffset.y, pos.x + m_vectMediaSize.x, pos.y + m_vectMediaSize.y);

					m_pD3DRender->LoadPixelShader(oldShader);
				}
				else
				{
					m_pD3DRender->DrawSprite(m_pTexture, (opacity255 << 24) | 0xFFFFFF, pos.x + m_vectMediaOffset.x, pos.y + m_vectMediaOffset.y, pos.x + m_vectMediaSize.x, pos.y + m_vectMediaSize.y);
				}
			}
			
		}
		
    }
	LeaveCriticalSection(&m_secTextureDataLock);
}

const char* VideoSource::GetAduioClassName() const
{
	return "VideoSource";
}

void VideoSource::UpdateSettings(Value &JsonParam)
{
	//函数进入日志
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,UpdateSettings Execute！ Open New File! This = %x"), __LINE__, __FUNCTION__,this);

	//判断需要打开的文件
	m_nfileLoopPlayUseTsp = 0;
	bool bisSame = false;
	if (!JsonParam["DeskTopSetting"].isNull())
	{
		UINT Index = JsonParam["CurrentIndex"].asUInt();
		if (Index >= m_pConfig->playlist.Num())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Index %d is too big,m_pConfig->playlist.Num is %d", __FUNCTION__, Index, m_pConfig->playlist.Num());
			Index = 0;
		}
		
		m_pConfig->element["CurrentIndex"] = Index;
		m_pConfig->CurrentIndex = Index;
		m_pConfig->isHaveSelect = true;
		Log::writeMessage(LOG_RTSPSERV, 1, "%s DeskTopSetting m_pConfig->playlist.Num is %d", __FUNCTION__, m_pConfig->playlist.Num());
	}
	else
	{
		m_pConfig->Reload();
	}
	
	if (m_pConfig->playlist.Num() == 0)
	{
		if (MediaPalying == m_MediaState)                              //当前状态为播放先暂停线程
		{
			m_MediaState = MediaStop;
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
			ResetEvent(m_hPauseSyncThreadEvent);
			Sleep(20);
		}
		ColseFile();
		memset(m_playPath, 0, PATH_MAX_);
		m_nMediaDuration = 0;
		return;
	}

	if (!m_pConfig->isHaveSelect)
	{
		for (int iIndex = 0; iIndex < m_pConfig->playlist.Num(); iIndex++)
		{
			if (!strcmp(WcharToAnsi(m_pConfig->playlist[iIndex].Array()).c_str(), m_playPath))
			{
				m_pConfig->CurrentIndex = iIndex;
				m_pConfig->Save();
				m_pConfig->Reload();
				break;
			}
			
		}
	}
	unsigned int i = m_pConfig->CurrentIndex;
	if (strcmp(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), m_playPath)) //不是当前播放文件
	{
		if (i < m_pConfig->playlist.Num())
		{
			memset(m_playPath, 0, PATH_MAX_);
			int nLen = strlen(WcharToAnsi(m_pConfig->playlist[i].Array()).c_str());
			if (nLen > PATH_MAX_)
				nLen = PATH_MAX_;
			memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[i].Array()).c_str(), nLen);
		}
	}
	else
	{
		bisSame = true;
	}

	if (m_bIsFieldSignal != m_pConfig->bIsScanInterlace)
	{
		m_bIsFieldSignal = m_pConfig->bIsScanInterlace;
		EnterCriticalSection(&m_secTextureDataLock);
		ChangeShader();
		LeaveCriticalSection(&m_secTextureDataLock);
	}
	
	if (bisSame)
	{
		return;
	}
	m_nPts = 0;
	memset(m_pPlayParam, 0, PATH_MAX_);
	sprintf_s(m_pPlayParam, "-re -f YUV -ss  %d", m_nPts);
	OpenStartFile();
}

bool VideoSource::ChangePos()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath, this);

	if (m_nPts == m_nMediaDuration && m_nPts > 3600 * 1000)
	{
		m_nPts = m_nPts + m_nFirstTsTimeStamp;
		m_nPts -= 30000;
	}
	else if (m_nPts == m_nMediaDuration && m_nPts > 10000)
	{
		m_nPts = m_nPts + m_nFirstTsTimeStamp;
		m_nPts -= 10000;
	}
	else
	{
		m_nPts = m_nPts + m_nFirstTsTimeStamp;
	}

	if (MediaPalying == m_MediaState)                              //当前状态为播放先暂停线程
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		ResetEvent(m_hPauseSyncThreadEvent);
		Sleep(20);                                                  //关闭文件前一定要停止线程，睡眠20ms等待线程挂起
	}
	m_bSeekFlag = true;
	m_bSeekClearVideoLastFrame = true;
	mp_seek(m_hMediaProcess, m_nPts);
	EnterCriticalSection(&m_secDataLock);
	SafeRelease(m_pCurrentVideoSample);
	m_pCurrentVideoSample = NULL;
	SafeRelease(m_pCurrentAudioSample);
	m_pCurrentAudioSample = NULL;
	m_bFirstFrame = true;
	LeaveCriticalSection(&m_secDataLock);

	m_nSyncTimeStart = 0;
	m_bFileIsError = false;                                                              //文件打开成功可以正常播放
	SetEvent(m_hPauseSyncThreadEvent);
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,设置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	return true;
}

bool VideoSource::ChangePlay()
{
	m_nfileLoopPlayUseTsp = 0;
	if (!m_bisLastFileStop || m_pConfig->isPlaylistLooping)
	{
		m_nSyncTimeStart = 0;
		if (MediaStop == m_MediaState)
		{
			m_MediaState = MediaPalying;
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,设置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
			SetEvent(m_hPauseSyncThreadEvent);
			Sleep(20);
		}
	}
	else
	{
		m_bisOpen = true;
		m_bisLastFileStop = false;
		ChangeNext();   //特定用于禁止循环且播放置最后一个文件末尾
	}

	return true;
}

bool VideoSource::ChangeReset()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,执行从头播放操作! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	m_nfileLoopPlayUseTsp = 0;
	if (m_pConfig->playlist.Num() == 0 || m_bFileIsError)
	{
		return false;
	}

	m_nPts = 0;
	memset(m_pPlayParam, 0, PATH_MAX_);
	sprintf_s(m_pPlayParam, "-re -f YUV -ss  %d", m_nPts);

	return OpenStartFile();
}

bool VideoSource::ChangeNext(bool bLoop)
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath,this);
	if (m_pConfig->playlist.Num() == 0)  //全部删除文件
	{
		memset(m_playPath, 0, PATH_MAX_);
		return false;
	}

	if (m_pConfig->isFileLoop && m_pConfig->isPlaylistLooping)  //需要循环并且为单个文件循环
	{
		//继续播放当前文件
	}
	else if (m_pConfig->isPlaylistLooping && m_pConfig->isListLoop)   //需要循环并且为列表文件循环
	{
		char currentPath[PATH_MAX_] = { 0 };
		{
			memset(currentPath, 0, PATH_MAX_);
			memcpy(currentPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));

			int ret = strcmp(currentPath, m_playPath);
			if (ret == 0)
			{
				if (m_pConfig->CurrentIndex == m_pConfig->playlist.Num() - 1) //当前播放为最后一个文件，下次播放第一个
				{
					m_pConfig->CurrentIndex = 0;
					memset(m_playPath, 0, PATH_MAX_);
					int nLen = strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str());
					if (nLen > PATH_MAX_)
						nLen = PATH_MAX_;
					memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));
				}
				else               //播放下一个文件
				{
					m_pConfig->CurrentIndex = m_pConfig->CurrentIndex + 1;
					memset(m_playPath, 0, PATH_MAX_);
					int nLen = strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str());
					if (nLen > PATH_MAX_)
						nLen = PATH_MAX_;
					memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));
					
				}
			}
		}
	}
	else if (!m_pConfig->isPlaylistLooping)        //不循环播放，顺序播放所有文件，播完最后一个停止
	{
		char currentPath[PATH_MAX_] = { 0 };
		{
			memset(currentPath, 0, PATH_MAX_);
			memcpy(currentPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));

			int ret = strcmp(currentPath, m_playPath);
			if (ret == 0)
			{
				if (m_pConfig->CurrentIndex == m_pConfig->playlist.Num() - 1) //当前播放为最后一个文件，不再播放直接返回
				{
					if (m_bisOpen)
					{
						m_bisOpen = false;
						goto Label;
					}
					else
					{
						m_MediaState = MediaStop;
						m_bisLastFileStop = true;
						return false;
					}
				}
				else               //播放下一个文件
				{
					m_pConfig->CurrentIndex = m_pConfig->CurrentIndex + 1;
					memset(m_playPath, 0, PATH_MAX_);
					int nLen = strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str());
					if (nLen > PATH_MAX_)
						nLen = PATH_MAX_;
					memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));
				}
			}
		}
	}

Label:
	m_pConfig->Save();
	m_MediaState = MediaPalying;  
	m_bisAutoNextFile = true;
	m_nPts = 0;
	memset(m_pPlayParam, 0, PATH_MAX_);
	sprintf_s(m_pPlayParam, "-re -f YUV -ss  %d", m_nPts);

	if (bLoop)
		return OpenStartFile();

	return false;
}

bool VideoSource::ChangePrev()
{
	//不能是第一个文件，因为第一个文件禁止向前
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s,This = %x"), __LINE__, __FUNCTION__, this);

	//只要能够执行就认为文件数量大于当前索引减一
	m_pConfig->CurrentIndex--;
	memset(m_playPath, 0, PATH_MAX_);
	int nLen = strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str());
	if (nLen > PATH_MAX_)
		nLen = PATH_MAX_;
	memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));
	m_pConfig->Save();
	m_nPts = 0;
	memset(m_pPlayParam, 0, PATH_MAX_);
	sprintf_s(m_pPlayParam, "-re -f YUV -ss  %d", m_nPts);

	return OpenStartFile();
}

bool VideoSource::ChangeNextUI()
{
	//不可能是最后一个
	m_nfileLoopPlayUseTsp = 0;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s"), __LINE__, __FUNCTION__, m_playPath);
	m_pConfig->CurrentIndex++;
	memset(m_playPath, 0, PATH_MAX_);
	int nLen = strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str());
	if (nLen > PATH_MAX_)
		nLen = PATH_MAX_;
	memcpy(m_playPath, WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(m_pConfig->playlist[m_pConfig->CurrentIndex].Array()).c_str()));
	m_pConfig->Save();
	m_nPts = 0;
	memset(m_pPlayParam, 0, PATH_MAX_);
	sprintf_s(m_pPlayParam, "-re -f YUV -ss  %d", m_nPts);

	return OpenStartFile();
}

Vect2 VideoSource::GetSize() const 
{
	if (Vect2(0,0) != m_vectVideoSize)
		 return m_vectVideoSize;
	else
	{
		return Vect2(1280, 720);
	}
}

void VideoSource::BeginScene()
{
	EnterCriticalSection(&m_secTextureDataLock);
	Captureing = true;
	
	ChangeShader();

	DWORD nWidth = 0, nHeigth = 0;
	if (m_pTexture)
		m_pD3DRender->GetTextureWH(m_pTexture, nWidth, nHeigth);

	if (nWidth != m_width || nHeigth != m_height) {
		if (m_pTexture) {
			delete m_pTexture;
			m_pTexture = nullptr;
		}
	}
	if (!m_pTexture) 
	{
		LPBYTE pData = (LPBYTE)Allocate_Bak(m_width*m_height * 4);
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			msetd(pData, 0x00808000, m_width*m_height * 4);
			m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_RGBA, pData, FALSE, FALSE);
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			msetd(pData, 0x00000000, m_width*m_height * 4);
			m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_BGRA, pData, FALSE, FALSE);
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			msetd(pData, 0x00000000, m_width*m_height * 4);
			m_pTexture = m_pD3DRender->CreateTexture(m_width, m_height, GS_BGR, pData, FALSE, FALSE);
		}
		Free(pData);
	}
	LeaveCriticalSection(&m_secTextureDataLock);
}

void VideoSource::EndScene()
{
	EnterCriticalSection(&m_secTextureDataLock);
	Captureing = false;
	if (m_pTexture)
	{
		delete m_pTexture;
		m_pTexture = nullptr;
	}	
	LeaveCriticalSection(&m_secTextureDataLock);
}

void VideoSource::InitCSampleData()
{
	InitAudioCSampleData();
}

void VideoSource::DestoryCSampleData()
{
	DestoryAuidoCSampleData();
}

void VideoSource::InitAudioCSampleData()
{
	if (m_pAudioSample == NULL)
	{
		m_pAudioSample = new CSampleData;
		if (!m_pAudioSample)
			return;
		m_pAudioSample->bAudio = true;
		m_pAudioSample->dataLength = 0;
		m_pAudioSample->lpData = NULL;
		m_pAudioSample->pAudioFormat = &m_wavAudioFormat;
	}
}

void VideoSource::DestoryAuidoCSampleData()
{
	if (m_pAudioSample)
	{
		m_pAudioSample->Release();
		m_pAudioSample = NULL;
	}
}

bool STDCALL SleepToNS(QWORD qwNSTime)
{
	QWORD t = GetQPCNS();

	if (t >= qwNSTime)
		return false;

	unsigned int milliseconds = (unsigned int)((qwNSTime - t) / 1000000);
	if (milliseconds > 1) //also accounts for windows 8 sleep problem
	{
		//trap suspicious sleeps that should never happen
		if (milliseconds > 10000)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("Tried to sleep for %u seconds, that can't be right! Triggering breakpoint."), milliseconds);
			DebugBreak();
		}
		OSSleep(milliseconds);
	}

	for (;;)
	{
		t = GetQPCNS();
		if (t >= qwNSTime)
			return true;
		Sleep(1);
	}
}

String VideoSource::ChooseShader(bool bNeedField)
{
	if (m_ColorType == DeviceOutputType_RGB || m_ColorType == DeviceOutputType_RGB24)
		return String();

	String strShader;
	strShader << SHADER_PATH;

	if (bNeedField)
		strShader << L"Field_";

	if (m_ColorType == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
	else if (m_ColorType == DeviceOutputType_YV12)
		strShader << TEXT("YVUToRGB.pShader");
	else if (m_ColorType == DeviceOutputType_YVYU)
		strShader << TEXT("YVXUToRGB.pShader");
	else if (m_ColorType == DeviceOutputType_YUY2)
		strShader << TEXT("YUXVToRGB.pShader");
	else if (m_ColorType == DeviceOutputType_UYVY)
		strShader << TEXT("UYVToRGB.pShader");
	else if (m_ColorType == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
	{
		strShader.Clear();
	}

	return strShader;
}

void VideoSource::AudioPlayBack(const void *samples, unsigned int count, int64_t pts)
{
	int i = 0;
}

bool VideoSource::PlayMedia()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,执行播放操作! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	if (m_pConfig->playlist.Num() == 0 || m_bFileIsError)
	{
		if (MediaPalying == m_MediaState)
		{
			m_MediaState = MediaStop;
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
			ResetEvent(m_hPauseSyncThreadEvent);
		}
		return false;
	}
	return ChangePlay();
}

bool VideoSource::ReSetMedia()
{
	bool result = false;
	if (m_nMediaDuration)
	{
		result = ChangeReset();
	}

	return result;
}

bool VideoSource::PauseMedia()
{
	if (MediaPalying == m_MediaState)
	{
		m_MediaState = MediaStop;
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		ResetEvent(m_hPauseSyncThreadEvent);
		Sleep(20);
	}
	
	return true;
}

bool VideoSource::SetStreamPos(UINT nPos)
{
	bool bRet = false;
	if (m_nMediaDuration)
	{
		m_nPts = nPos;
		bRet = ChangePos();
	}
	return bRet;
}

bool VideoSource::PlayNextFile()
{
	bool ret = ChangeNextUI();
	while (!ret && (m_pConfig->CurrentIndex != m_pConfig->playlist.Num() - 1))    //最后一个文件
	{
		ret = ChangeNextUI();
	}
	if (!ret && m_pConfig->CurrentIndex == m_pConfig->playlist.Num() - 1)//最后一个文件
	{
		bool retNext = ChangeNext();
		if (!ret && !m_bisAutoNextFile)    //播放第一个文件仍然是无法打开的
		{
			int LoopIndex = 0;//尝试次数
			while (!ret && (m_pConfig->CurrentIndex != m_pConfig->playlist.Num() - 1 && LoopIndex <= m_pConfig->playlist.Num()-1))    //最后一个文件
			{
				ret = ChangeNextUI();
			}
		}
	}
	return ret;
}

bool VideoSource::PlayPrevFile()
{
	 bool ret = ChangePrev();
	 while (!ret && (m_pConfig->CurrentIndex != 0))     //第一个文件
	 {
		 ret = ChangePrev();
	 }
	 return ret;
}

bool VideoSource::GetStreamPos(UINT &nPos)
{
	if (m_pConfig->playlist.Num() == 0)
	{
		nPos = 0;
		return true;
	}
	if (!m_nMediaDuration)
	{
		nPos = 0;
		return true;
	}
	//判断原则，视频播放完成，自动播放下一个文件，完成标准，当前时间戳等于上一次的时间戳且不等于0并且和总时长相差不到一秒
	if ((m_nPtsLast == m_nPts && m_nPts + 1000 > m_nMediaDuration) || (m_nPtsLast == m_nPts && m_nfileLoopPlayUseTsp + 1000 > m_nMediaDuration))
	{
		//注意最后一帧视频可能不是和总时长在同一个秒数位，这种情况把最后时间戳设置为文件时长
		m_nPtsLast = m_nPts = m_nMediaDuration;
	}
	if (m_nPtsLast == m_nPts && m_nPtsLast != 0 && m_nPts + 1000 > m_nMediaDuration)
	{	
		if (MediaPalying == m_MediaState)
		{
			m_nPts = 0;
			m_nfileLoopPlayUseTsp = 0;
			bool ret = ChangeNext();
			int iLoopIndex = 0; //寻找播放文件的次数不能大于循环列表个数，否则说明全是错误文件
			while (!ret && !m_bisAutoNextFile && iLoopIndex <= m_pConfig->playlist.Num())
			{
				iLoopIndex++;
				ret = ChangeNext();
			}
		}
	}
	if (m_nPts > m_nMediaDuration)
	{
		m_nPts -= m_nFirstTsTimeStamp;
	}
	m_nPtsLast = m_nPts;
	nPos = m_nPts;
	if (nPos > m_nMediaDuration)
	{
		nPos = m_nMediaDuration;
	}

	return true;
}

int64_t VideoSource::GetMediaDuration()
{
		return m_nMediaDuration;
}

void VideoSource::GetPlayList(StringList & PlayList)
{
	if (m_pConfig){
		for (unsigned int i = 0; i < m_pConfig->playlist.Num(); i++) {
			PlayList.Add(m_pConfig->playlist[i]);
		}
	} else {
		PlayList.Clear();
	}
}

IBaseAudio * VideoSource::GetAudioRender()
{
	return m_pDemandMediaAudio;
}

bool VideoSource::GetStreamInfo(Value &JsonInfo)
{
	JsonInfo["DemandFileDuration"] = (int)m_nMediaDuration;
	Value &PlayArry = JsonInfo["playlist"];
	PlayArry.resize(m_pConfig->playlist.Num());
	for (int i = 0; i < m_pConfig->playlist.Num(); ++i)
	{
		PlayArry[i] = WcharToAnsi(m_pConfig->playlist[i].Array()).c_str();
	}
	JsonInfo["CurrentIndex"] = m_pConfig->CurrentIndex;
	JsonInfo["IsLoop"] = m_pConfig->isListLoop;
	JsonInfo["IsListLoop"] = m_pConfig->isPlaylistLooping;
	JsonInfo["IsFileLoop"] = m_pConfig->isFileLoop;
	JsonInfo["MediaStatus"] = m_MediaState;

	if (m_MediaState != MediaPalying)
	{
		if (m_pDemandMediaAudio)
			m_pDemandMediaAudio->ResetAudioDB();
	}

	return true;
}

/**************************************************
修改文件播放状态函数
***************************************************/
bool VideoSource::OperaterStream(DBOperation OperType)
{
	if (MediaUnknow == m_MediaState)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作插件未初始化 OperType =  %d.", __FUNCTION__, __LINE__, OperType);
		return false;
	}
	switch (OperType)
	{
	case None:
	{
				 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 未初始化 OperType =  %d.", __FUNCTION__, __LINE__, OperType);
				 return false;
	}

	case Pause:
	{
				  Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 暂停OperType =  %d.", __FUNCTION__, __LINE__, OperType);
				  return PauseMedia();
	}

	case Play:
	{
				 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
				 return PlayMedia();

	}

	case ReStart:
	{
					Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 重新播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					PlayMedia();
					return ReSetMedia();
	}

	case PlayNext:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 播放下一个文件OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					 if (m_pConfig->playlist.Num() && m_pConfig->CurrentIndex < m_pConfig->playlist.Num() - 1 && m_pConfig->CurrentIndex >= 0)
					 {
						 return PlayNextFile();
					 }
					 break;
	}
	case PlayPrev:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 播放上一个文件OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					 if (m_pConfig->playlist.Num() && m_pConfig->CurrentIndex > 0 && m_pConfig->CurrentIndex < m_pConfig->playlist.Num())
					 {
						 return PlayPrevFile();
					 }
					 break;
	}
	default:
	{
			   Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 无效OperType =  %d.", __FUNCTION__, __LINE__, OperType);
			   break;
	}
	}
	return false;
}

void VideoSource::SetHasSwitchSences(bool bSwitch)
{
	m_bSwitch = bSwitch;
}

void VideoSource::SwitchSences()
{
	if (m_bSwitch && m_nEnteredSceneCount >= 1)
	{
		if (m_pConfig && m_pConfig->isSenceChangeStartPlay)
		{
			ChangeReset();
		}
	}
}

void VideoSource::SetCanEnterScene(bool bCanEnter)
{
	m_bEnterScene = bCanEnter;
}

bool VideoSource::CanEnterScene() const
{
	return m_bEnterScene;
}

void VideoSource::RegisterDataCallBack(void *Context, DataCallBack pCb)
{
	__DataCallBack DataBack;
	DataBack.Context = Context;
	DataBack.CallBack = pCb;
	EnterCriticalSection(&m_secCallBackLock);
	m_ListCallBack.Add(DataBack);
	LeaveCriticalSection(&m_secCallBackLock);
}

void VideoSource::UnRegisterDataCallBack(void *Context)
{
	EnterCriticalSection(&m_secCallBackLock);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		if (OneCallBack.Context == Context)
		{
			m_ListCallBack.Remove(i);
			break;
		}
	}
	LeaveCriticalSection(&m_secCallBackLock);
}

void VideoSource::SetHasPreProcess(bool bPre)
{
	m_bHasPreProcess = bPre;
}

bool VideoSource::GetHasPreProcess() const
{
	return m_bHasPreProcess;
}

void VideoSource::OpenFileFailed()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LINE = %d,FUNC = %s,打开文件失败 FILE = %s", __LINE__, __FUNCTION__, m_playPath);
	HintInfo *pHintInfo = new HintInfo;

	if (!pHintInfo)
		return;

	pHintInfo->FileName = m_playPath;
	pHintInfo->Index = m_nFileIndex;
	m_nFileIndex++;
	if (m_nFileIndex > 2)
	{
		m_nFileIndex = 0;
	}
	OSCreateThread((XTHREAD)VideoSource::FileHintThread, pHintInfo);
}

void VideoSource::ColseFile()
{
	try
	{
		EnterCriticalSection(&m_secDataLock);
		if (m_pLastMPFrameInfo)
		{
			ReleaseFrame(m_hMediaProcess, m_pLastMPFrameInfo, 1);
			m_pLastMPFrameInfo = NULL;
		}
		if (m_pCurrentMPFrameInfo)
		{
			ReleaseFrame(m_hMediaProcess, m_pCurrentMPFrameInfo, 1);
			m_pCurrentMPFrameInfo = NULL;
		}
		LeaveCriticalSection(&m_secDataLock);

		if (m_hMediaProcess)
			mp_close(m_hMediaProcess);
	}
	catch (...)
	{

	}
}

bool VideoSource::IsFieldSignal() const
{
	return m_bIsFieldSignal;
}

void VideoSource::ChangeShader()
{
	String strShader = ChooseShader(false);

	if (strShader.IsValid() && (!m_strShaderOld.Compare(strShader)))
	{
		if (m_pColorConvertShader)
		{
			delete m_pColorConvertShader;
			m_pColorConvertShader = NULL;
		}

		if (m_pColorFieldConvertShader)
		{
			delete m_pColorFieldConvertShader;
			m_pColorFieldConvertShader = NULL;
		}

		m_pColorConvertShader = m_pD3DRender->CreatePixelShaderFromFile(strShader);
		m_strShaderOld = strShader;


		strShader = ChooseShader();
		m_pColorFieldConvertShader = m_pD3DRender->CreatePixelShaderFromFile(strShader);

	}
	else if ((m_ColorType == DeviceOutputType_RGB || m_ColorType == DeviceOutputType_RGB24) && !m_bIsFieldSignal)
	{
		if (m_pColorConvertShader)
		{
			delete m_pColorConvertShader;
			m_pColorConvertShader = NULL;
		}

		if (m_pColorFieldConvertShader)
		{
			delete m_pColorFieldConvertShader;
			m_pColorFieldConvertShader = NULL;
		}
		m_strShaderOld.Clear();
	}
}

void VideoSource::PlayCallBackAudio(LPBYTE lpData, UINT len)
{
	CSampleData Audio;
	Audio.bAudio = true;
	Audio.lpData = lpData;
	Audio.dataLength = len;

	m_pDemandMediaAudio->GetDb(Audio.LeftDb, Audio.RightDb);

	WAVEFORMATEX FormatAudio = m_wavAudioFormat;

	if (FormatAudio.nChannels > 2)
	{
		FormatAudio.nChannels = 2;
		FormatAudio.wBitsPerSample = m_wavAudioFormat.wBitsPerSample * 2;
	}

	EnterCriticalSection(&m_secCallBackLock);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		Audio.pAudioFormat = (void*)&FormatAudio;
		if (OneCallBack.CallBack)
			OneCallBack.CallBack(OneCallBack.Context, &Audio);
	}
	LeaveCriticalSection(&m_secCallBackLock);

	Audio.lpData = NULL;//不是动态申请的置NULL
}

bool VideoSource::InitQPCTimer()
{
	if (m_bSeekFlag)
	{
		if (m_pMPMediaInfo.has_audio)
		{
			if (m_pCurrentAudioSample)
			{
				if (m_pCurrentVideoSample)
				{
					if (m_bSeekClearVideoLastFrame)
					{
						m_bSeekClearVideoLastFrame = false;
						SafeRelease(m_pCurrentVideoSample);
						m_pCurrentVideoSample = NULL;
					}
				}
				else
				{
					m_bSeekClearVideoLastFrameSecond = true;
				}
				m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentAudioSample->timestamp;
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Seek时获取初始化时间_音频: m_pCurrentAudioSample->timestamp = %d，同步起点m_nSyncTimeStart = %d"), __LINE__, __FUNCTION__, m_pCurrentAudioSample->timestamp, m_nSyncTimeStart);
				m_bSeekFlag = false;
				return true;
			}
		}
		if (m_pMPMediaInfo.has_video)
		{
			if (m_pCurrentVideoSample)
			{
				if (m_bSeekClearVideoLastFrame)
				{
					m_bSeekClearVideoLastFrame = false;
					SafeRelease(m_pCurrentVideoSample);
					m_pCurrentVideoSample = NULL;
					return false;
				}
				m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentVideoSample->timestamp;
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Seek时获取初始化时间_视频: m_pCurrentVideoSample->timestamp = %d, 同步起点m_nSyncTimeStart = %d"), __LINE__, __FUNCTION__, m_pCurrentVideoSample->timestamp, m_nSyncTimeStart);
				m_bSeekFlag = false;
				return true;
			}
		}
		return false;
	}
	//同时有音视频
	if (m_pMPMediaInfo.has_audio&&m_pMPMediaInfo.has_video)
	{
		if (m_pCurrentVideoSample && m_pCurrentAudioSample)
		{
			if (m_pCurrentVideoSample->timestamp > m_pCurrentAudioSample->timestamp) //先渲染音频
			{
				if (m_nFirstTsTimeStamp == -1)
				{
					m_nFirstTsTimeStamp = m_pCurrentAudioSample->timestamp;
				}
				m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentAudioSample->timestamp;
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取初始化时间_音频: m_pCurrentAudioSample->timestamp = %d，同步起点m_nSyncTimeStart = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentAudioSample->timestamp, m_nSyncTimeStart, GetCurrentThreadId(), this);
			}
			else
			{
				if (m_nFirstTsTimeStamp == -1)
				{
					m_nFirstTsTimeStamp = m_pCurrentVideoSample->timestamp;
				}
				m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentVideoSample->timestamp;
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取初始化时间_视频: m_pCurrentVideoSample->timestamp = %d, 同步起点m_nSyncTimeStart = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentVideoSample->timestamp, m_nSyncTimeStart, GetCurrentThreadId(), this);
			}
			return true;
		}
	}
	else if (m_pMPMediaInfo.has_audio)
	{
		if (m_pCurrentAudioSample)
		{
			if (m_nFirstTsTimeStamp == -1)
			{
				m_nFirstTsTimeStamp = m_pCurrentAudioSample->timestamp;
			}
			m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentAudioSample->timestamp;
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取初始化时间: m_pCurrentAudioSample->timestamp = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentAudioSample->timestamp, GetCurrentThreadId(), this);
			return true;
		}		
	}
	else if (m_pMPMediaInfo.has_video)
	{
		if (m_pCurrentVideoSample)
		{
			if (m_nFirstTsTimeStamp == -1)
			{
				m_nFirstTsTimeStamp = m_pCurrentVideoSample->timestamp;
			}
			m_nSyncTimeStart = GetSysTickCount64() - m_pCurrentVideoSample->timestamp;
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取初始化时间: m_pCurrentVideoSample->timestamp = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentVideoSample->timestamp, GetCurrentThreadId(), this);
			return true;
		}	
	}
	return false;
}

void VideoSource::SyncVideoFrame()
{
	//计时器没有初始化，直接返回
	if (m_nSyncTimeStart == 0 || !m_pMPMediaInfo.has_video)
		return;
	//判断当前帧是否需要渲染
	EnterCriticalSection(&m_secDataLock);
	if (m_pCurrentVideoSample)
	{
		if (m_bSeekClearVideoLastFrameSecond)
		{
			m_bSeekClearVideoLastFrameSecond = false;
			SafeRelease(m_pCurrentVideoSample);
			m_pCurrentVideoSample = NULL;
			LeaveCriticalSection(&m_secDataLock);
			return;
		}

// 		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,一帧视频: m_nSyncTimeStart = %d, m_pCurrentVideoSample->timestamp = %d, 和 = %d, GetSysTickCount64 = %d."), __LINE__, __FUNCTION__, m_nSyncTimeStart, m_pCurrentVideoSample->timestamp, m_nSyncTimeStart + m_pCurrentVideoSample->timestamp, GetSysTickCount64());
		if (m_nSyncTimeStart + m_pCurrentVideoSample->timestamp <= GetSysTickCount64()) //符合渲染条件
		{
// 			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,渲染一帧视频: m_nSyncTimeStart = %d, m_pCurrentVideoSample->timestamp = %d, 和 = %d, GetSysTickCount64 = %d."), __LINE__, __FUNCTION__, m_nSyncTimeStart, m_pCurrentVideoSample->timestamp, m_nSyncTimeStart + m_pCurrentVideoSample->timestamp, GetSysTickCount64());
			m_pCurrentVideoSample->bFieldSignal = m_bIsFieldSignal;
			EnterCriticalSection(&m_secCallBackLock);
			for (int i = 0; i < m_ListCallBack.Num(); ++i)
			{
				__DataCallBack &OneCallBack = m_ListCallBack[i];
				if (OneCallBack.CallBack)
					OneCallBack.CallBack(OneCallBack.Context, m_pCurrentVideoSample);
			}
			LeaveCriticalSection(&m_secCallBackLock);
			
			if (m_pLatestVideoSample)
			{
				SafeRelease(m_pLatestVideoSample);
				if (m_pLastMPFrameInfo)
				{
					ReleaseFrame(m_hMediaProcess, m_pLastMPFrameInfo, 1);
					m_pLastMPFrameInfo = NULL;
				}			
			}
			if (m_pCurrentVideoSample->timestamp - m_nFirstTsTimeStamp > m_nPts)
			{
				m_nfileLoopPlayUseTsp = m_nPts = m_pCurrentVideoSample->timestamp - m_nFirstTsTimeStamp;
			}
			m_pLatestVideoSample = m_pCurrentVideoSample;
			m_pLastMPFrameInfo = m_pCurrentMPFrameInfo;
			m_pCurrentVideoSample = NULL;
			m_pCurrentMPFrameInfo = NULL;
			if (m_bFirstFrame)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Sync同步线程渲染第一个视频帧. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
				m_bFirstFrame = false;
				if (MediaStop == m_MediaState)                      //只有暂停状态才停止在第一帧
				{
					Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
					ResetEvent(m_hPauseSyncThreadEvent);
				}
			}
		}
	}
	LeaveCriticalSection(&m_secDataLock);
}

void VideoSource::SyncAudioFrame()
{
	//计时器没有初始化，直接返回
	if (m_nSyncTimeStart == 0 || !m_pMPMediaInfo.has_audio)
		return;
	//判断当前帧是否需要渲染
	if (m_pCurrentAudioSample)
	{
// 		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,一帧音频: m_nSyncTimeStart = %d, m_pCurrentAudioSample->timestamp = %d, 和 = %d, GetSysTickCount64 = %d."), __LINE__, __FUNCTION__, m_nSyncTimeStart, m_pCurrentAudioSample->timestamp, m_nSyncTimeStart + m_pCurrentAudioSample->timestamp, GetSysTickCount64());
		if (m_nSyncTimeStart + m_pCurrentAudioSample->timestamp <= GetSysTickCount64()) //符合渲染条件
		{
// 			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,渲染一帧音频: m_nSyncTimeStart = %d, m_pCurrentAudioSample->timestamp = %d, 和 = %d, GetSysTickCount64 = %d."), __LINE__, __FUNCTION__, m_nSyncTimeStart, m_pCurrentAudioSample->timestamp, m_nSyncTimeStart + m_pCurrentAudioSample->timestamp, GetSysTickCount64());
			if (m_pDemandMediaAudio)
			{
				m_pDemandMediaAudio->PushAudio(m_pCurrentAudioSample->lpData, m_pCurrentAudioSample->dataLength, m_pCurrentAudioSample->timestamp, this, m_nEnteredSceneCount != 0);
			}
			if (m_pCurrentAudioSample->timestamp - m_nFirstTsTimeStamp > m_nPts)
			{
				m_nfileLoopPlayUseTsp = m_nPts = m_pCurrentAudioSample->timestamp - m_nFirstTsTimeStamp;
			}
			SafeRelease(m_pCurrentAudioSample);
			m_pCurrentAudioSample = NULL;
			if (m_pMPMediaInfo.has_video == 0)
			{
				if (m_bFirstFrame)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Sync同步线程渲染第一个音频帧. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
					m_bFirstFrame = false;
					if (MediaStop == m_MediaState)                      //只有暂停状态才停止在第一帧
					{
						Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
						ResetEvent(m_hPauseSyncThreadEvent);
					}
				}
			}
		}
	}
}

void VideoSource::GetVideoFrame()
{
	if (m_pCurrentVideoSample || !m_pMPMediaInfo.has_video)
	{
// 		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,m_pCurrentVideoSample非空. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__,  GetCurrentThreadId(), this);
		return;
	}

	MPFrameInfo * pMPFrameInfo = GetFrame(m_hMediaProcess, 1);
	if (!pMPFrameInfo)
	{
// 		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,GetFrame为空. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		return;
	}

	if (m_width != pMPFrameInfo->width || m_height != pMPFrameInfo->height)
	{
		return;
	}

	CSampleData* videoSample = new CSampleData;
	if (!videoSample)
		return;
	videoSample->bAudio = false;
	videoSample->cx = m_width;
	videoSample->cy = m_height;

	if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
	{
		videoSample->colorType = ColorType_I420;
		videoSample->dataLength = pMPFrameInfo->framesize;
		videoSample->lpData = (LPBYTE)pMPFrameInfo->framedata;
	}
	else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)  //RGBA测试
	{
		videoSample->dataLength = pMPFrameInfo->framesize;
		videoSample->lpData = (LPBYTE)pMPFrameInfo->framedata;
		videoSample->colorType = ColorType_RGBA32REVERSE;
	}
	else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
	{
		videoSample->dataLength = pMPFrameInfo->framesize;
		videoSample->lpData = (LPBYTE)pMPFrameInfo->framedata;
		videoSample->colorType = ColorType_RGB24;
	}

	videoSample->timestamp = pMPFrameInfo->pts;
	if (videoSample->lpData)
	{
		videoSample->CheckNum = *(uint64_t*)(videoSample->lpData - 24);
	}
	m_pCurrentVideoSample = videoSample;
//  	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取一帧视频: timestamp = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentVideoSample->timestamp, GetCurrentThreadId(), this);
	EnterCriticalSection(&m_secDataLock);
	m_pCurrentMPFrameInfo = pMPFrameInfo;
	LeaveCriticalSection(&m_secDataLock);
}

void VideoSource::GetAudioFrame()
{
	if (m_pCurrentAudioSample || !m_pMPMediaInfo.has_audio)
	{
		return;
	}

	//获取一帧音频
	MPFrameInfo * frameInfo = GetFrame(m_hMediaProcess, 2);
	if (!frameInfo)
	{
		return;
	}
// 	if (!fp1)
// 		fp1 = fopen("1.pcm", "wb");
// 	fwrite((char*)frameInfo->framedata, frameInfo->framesize, 1, fp1);

	m_pCurrentAudioSample = new CSampleData;
	m_pCurrentAudioSample->bAudio = true;
	m_pCurrentAudioSample->dataLength = frameInfo->framesize;
	m_pCurrentAudioSample->lpData = (LPBYTE)Allocate_Bak(m_pCurrentAudioSample->dataLength);
	memcpy(m_pCurrentAudioSample->lpData, LPBYTE(frameInfo->framedata), frameInfo->framesize);
	m_pCurrentAudioSample->timestamp = frameInfo->pts;
 	//Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,获取一帧音频: timestamp = %d. Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, m_pCurrentAudioSample->timestamp, GetCurrentThreadId(), this);
	free(frameInfo->framedata);
	ReleaseFrame(m_hMediaProcess, frameInfo, 2);
}

void VideoSource::Synchronization()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,同步线程启动! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);

	while (!m_bExit){

		WaitForSingleObject(m_hPauseSyncThreadEvent,INFINITE);  //等待暂停事件

		if (m_bExit)                                            //等待到事件之后先判断是否是程序退出发出的事件
			break;
		
		m_bReadyDraw = true;
		                                                         
		if (m_nSyncTimeStart == 0)                               //初始化计时器
		{
			InitQPCTimer();
		}

		//渲染一帧音频
		SyncAudioFrame();

		//渲染一帧视频
		SyncVideoFrame();

		//获取一帧音频
		GetAudioFrame();

		//获取一帧视频
		GetVideoFrame();

		Sleep(10);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s, 同步线程退出! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
}
//公共操作
bool VideoSource::OpenStartFile()
{
	if (MediaPalying == m_MediaState)                              //当前状态为播放先暂停线程
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,重置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		ResetEvent(m_hPauseSyncThreadEvent);
		Sleep(20);                                                  //关闭文件前一定要停止线程，睡眠20ms等待线程挂起
	}                                   
	ColseFile();
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(m_hMediaProcess, m_pConfig->isHardwareDecode);
	EnterCriticalSection(&m_secDataLock);
	m_nPts = 0;
	m_nFirstTsTimeStamp = -1; 
	SafeRelease(m_pCurrentVideoSample);
	m_pCurrentVideoSample = NULL;
	SafeRelease(m_pCurrentAudioSample);
	m_pCurrentAudioSample = NULL;
	int ret = mp_open(m_hMediaProcess, m_playPath, m_pPlayParam, NULL, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_nMediaDuration = 0;
		if (MediaPalying == m_MediaState)
			m_MediaState = MediaStop;
		OpenFileFailed();
		m_bisAutoNextFile = false;
		LeaveCriticalSection(&m_secDataLock);
		m_bFileIsError = true;

		if (m_pConfig->isFileLoop && m_pConfig->isPlaylistLooping)
			return false;

		if (m_pConfig->CurrentIndex < m_pConfig->playlist.Num() - 1)
		{
			//这里是递归调用必须有结束条件
			return ChangeNext();
		}
		else if (m_pConfig->CurrentIndex == m_pConfig->playlist.Num() - 1 && m_pConfig->playlist.Num() >= 2)
		{
			//说明打开最后一个文件失败
			ChangeNext(false);
			auto Func = [this]()->bool
			{
				int ret = mp_open(m_hMediaProcess, m_playPath, m_pPlayParam, NULL, this, &m_pMPMediaInfo);
				if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
				{
					return false;
				}
				return true;
			};

			bool bSucc = false;
			while (m_pConfig->CurrentIndex < m_pConfig->playlist.Num() - 1)
			{
				bSucc = Func();
				if (bSucc)
					break;

				ChangeNext(false);
			}

			if (!bSucc)
				return false;
		}
		else
		{
			return false;
		}
		
	}

	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

	m_bFirstFrame = true;
	ret = mp_start(m_hMediaProcess);    //开始获取数据
	if (ret == -1)
	{
		if (MediaPalying == m_MediaState)
			m_MediaState = MediaStop;
		LeaveCriticalSection(&m_secDataLock);
		m_bFileIsError = true;
		return false;
	}
	if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
	{
		m_ColorType = DeviceOutputType_I420;
	}
	else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
	{
		m_ColorType = DeviceOutputType_RGB;
	}
	else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
	{
		m_ColorType = DeviceOutputType_RGB24;
	}
	m_nMediaDuration = m_pMPMediaInfo.stream_duration;
	m_bSeekFlag = false;
	AudioParam Param;
	Param.iBitPerSample = m_wavAudioFormat.wBitsPerSample = 16;
	Param.iChannel = m_wavAudioFormat.nChannels = m_pMPMediaInfo.a_channels;
	Param.iSamplesPerSec = m_wavAudioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	EnterLiveVideoSection();
	if (Param.iBitPerSample > 0)
		m_pDemandMediaAudio->ResetAudioParam(Param);
	LeaveLiveVideoSection();

	m_vectVideoSize.x = m_width = m_pMPMediaInfo.v_width;
	m_vectVideoSize.y = m_height = m_pMPMediaInfo.v_height;
	
	m_nFrameLines = 0;
	m_nFramePitchs = 0;
	m_nPts = 0;
	VideoFormatCallback(&m_width, &m_height);
	InitCSampleData();
	LeaveCriticalSection(&m_secDataLock);

	EnterCriticalSection(&m_secTextureDataLock);
	ChangeShader();
	LeaveCriticalSection(&m_secTextureDataLock);
	m_nSyncTimeStart = 0;
	m_bFileIsError = false;                                                              //文件打开成功可以正常播放
	SetEvent(m_hPauseSyncThreadEvent);
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,设置事件! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	return true;
}

void VideoSource::MessageLoop()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,消息线程启动! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	while (!m_bExit)
	{
		VideoMessageType MessageType;
		m_pMessageQueue.pop(MessageType);
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,从消息队列中取出一个消息，消息类型:%d.! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, MessageType, GetCurrentThreadId(), this);
		if (VideoReset == MessageType)
		{
			Sleep(20);
			if (m_bExit)
			{
				break;
			}
			ChangeReset();
		}
		else if (VideoPlay == MessageType)
		{
			Sleep(20);
			if (m_bExit)
			{
				break;
			}
			PlayMedia();
		}
		else if (VideoExit == MessageType)
		{
			break;
		}
	}
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,消息线程退出! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
}

void VideoSource::SetOpacity(DWORD Opacity)
{
	if (Opacity > 255)
		opacity255 = 255;
	else
	{
		opacity255 = Opacity;
	}
}
