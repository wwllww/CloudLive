/**
* John Bradley (jrb@turrettech.com)
*/

#include "VideoSource.h"
#include "VideoSourceConfig.h"
#include "MediaAudio.h"
#include <process.h>
void   InitConvtTbl();
void   YUV2RGB420(unsigned   char   *src, unsigned   char   *dst_ori,
	int   width, int   height);
#define TIMESTAMP 0
#define  HASSTOP    if (m_stop){break;}
/****************************************************/
/*Function : VolumeCaculate
Action : Calculate volume
Parameters ::
buf : Input&Output, Audio data buffer;
size: Input, Length of buf;
vol : Input, Level of Volume;
*/
/****************************************************/

static void VolumeCaculate(char *Dest, char* buf, UINT32 size, double vol)//buf为需要调节音量的音频数据块首地址指针，size为长度，uRepeat为重复次数，通常设为1，vol为增益倍数,可以小于1
{
	if (!size)
	{
		return;
	}
	if (vol == 1.0)
	{
		memcpy(Dest, buf, size);
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
		Dest[i] = LOBYTE(wData);
		Dest[i + 1] = HIBYTE(wData);
		i += 2;
	}
}

/****************************************************/
/* Sum   the   input */
/* Input:   input,   len */
/* Output:   input */
/* Algorithm:   add */
/****************************************************/
#define SHADER_PATH TEXT("shaders/")

IMPLEMENT_DYNIC(VideoLiveSource, "直播源", "1.0.0.1");

VideoLiveSource::VideoLiveSource(Value& data)
{
    Log(TEXT("LINE : %d, FUNC : %s ,Using FFmpeg Live Video Source"),__LINE__,String(__FUNCTION__).Array());
	colorType = DeviceOutputType_I420;
	HMediaProcess = mp_create(3); //create sync! 
	m_StatusStream = STOP_STREAM;
	m_height = 0;
	m_width = 0;
	m_pos = 0;
	CallBackWidth = 0;
	CallBackHight = 0;
	m_uCurrentTime = GetTickCount();
	m_uLastTime = GetTickCount();
	m_playPath = NULL;
	texture = NULL;
	m_hUpdataEvent = CreateEvent(NULL, true, false, NULL);
	m_hH264AACBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hYUVBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hPCMBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hRenderSleepEvent = CreateEvent(NULL, true, false, NULL);
	m_hAllowCloseEvent = CreateEvent(NULL, true, false, NULL);
	m_hPauseThreadEvent = CreateEvent(NULL, true, false, NULL);
	Captureing = false;
	m_bNeedSetImage     = false;
    isInScene = true;
	enteredSceneCount = 0;	
	audioSample = NULL;
	latestVideoSample = NULL;
    config = new VideoSourceConfig(data);
	InitializeCriticalSection(&DataLock);
	InitializeCriticalSection(&CallBackLock);
	InitializeCriticalSection(&TextureDataLock);
	m_mediaDuration = 0;
	m_nBufferTime = 0;
	m_nWarnTime = 0;
	colorConvertShader = NULL;
	RGBFieldShader = NULL;
	m_bEnterScene = true;
	m_pDemandMediaAudio = new CDemandMediaAudio;
	StartMonitor();
	D3DRender = GetD3DRender();
}

VideoLiveSource::VideoLiveSource()
{
	Log(TEXT("LINE : %d, FUNC : %s ,Using FFmpeg Live Video Source"), __LINE__, String(__FUNCTION__).Array());
	colorType = DeviceOutputType_I420;
	HMediaProcess = mp_create(3); //create sync! 
	m_StatusStream = STOP_STREAM;
	m_height = 0;
	m_width = 0;
	m_pos = 0;
	m_nBufferTime = 0;
	m_nWarnTime = 0;
	CallBackWidth = 0;
	CallBackHight = 0;
	m_uCurrentTime = GetTickCount();
	m_uLastTime = GetTickCount();
	texture = NULL;
	m_hUpdataEvent = CreateEvent(NULL, true, false, NULL);
	m_hH264AACBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hYUVBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hPCMBufferEvent = CreateEvent(NULL, true, false, NULL);
	m_hRenderSleepEvent = CreateEvent(NULL, true, false, NULL);
	m_hAllowCloseEvent = CreateEvent(NULL, true, false, NULL);
	m_hPauseThreadEvent = CreateEvent(NULL, true, false, NULL);
	Captureing = false;
	m_bNeedSetImage = false;
	isInScene = true;
	m_playPath = NULL;
	enteredSceneCount = 0;
	audioSample = NULL;
	colorConvertShader = NULL;
	colorFieldConvertShader = NULL;
	D3DRender = GetD3DRender();
	RGBFieldShader = D3DRender->CreatePixelShaderFromFile(L"shaders/Field_RGB.pShader");

	latestVideoSample = NULL;
	InitializeCriticalSection(&DataLock);
	InitializeCriticalSection(&CallBackLock);
	InitializeCriticalSection(&TextureDataLock);
	m_mediaDuration = 0;
	m_pDemandMediaAudio = new CDemandMediaAudio;
	m_bEnterScene = true;
	StartMonitor();
}

VideoLiveSource::~VideoLiveSource()
{ 
	m_stop = true;
	//SetEvent(m_hUpdataEvent);
	if (H264_AAC_STREAM == m_StatusStream) {         //当前正处在缓冲码流
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hH264AACBufferEvent);
		Log(TEXT("LINE : %d, FUNC : %s  VideoSource Live Destructor,status = H264_AAC_STREAM, This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
	}
	else if (YUV_STREAM == m_StatusStream) {          //当前正处在缓冲YUV
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hYUVBufferEvent);
		Log(TEXT("LINE : %d, FUNC : %s  VideoSource Live Destructor,status = YUV_STREAM, This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
	}
	else if (PCM_STREAM == m_StatusStream) {         //当前正处在缓冲PCM
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hPCMBufferEvent);
		Log(TEXT("LINE : %d, FUNC : %s  VideoSource Live Destructor,status = PCM_STREAM, This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
	}
	else if (PLAY_STREAM == m_StatusStream || STOP_STREAM == m_StatusStream) {
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hUpdataEvent);
		Log(TEXT("LINE : %d, FUNC : %s  VideoSource Live Destructor,status = STOP_STREAM or PLAY_STREAM, This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
	}

	SetEvent(m_hRenderSleepEvent);

	if (m_SyncThread)
	{
		WaitForSingleObject(m_SyncThread, INFINITE);
		{
			Log(TEXT("LINE : %d, FUNC : %s VideoSource Release SyncThread Success!This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
		}
	}
	
	WaitForSingleObject(m_MonitorThread, INFINITE);
	{
		Log(TEXT("LINE : %d, FUNC : %s VideoSource Release MonitorThread Success!This =0x%p"), __LINE__, String(__FUNCTION__).Array(), this);
	}
  
	if (m_hUpdataEvent)
	{
		CloseHandle(m_hUpdataEvent);
	}

	if (m_MonitorThread)
	{
		OSCloseThread(m_MonitorThread);
	}
	if (m_hYUVBufferEvent)
	{
		CloseHandle(m_hYUVBufferEvent);
	}
	if (m_hPCMBufferEvent)
	{
		CloseHandle(m_hPCMBufferEvent);
	}
	if (m_hPauseThreadEvent)
	{
		CloseHandle(m_hPauseThreadEvent);
	}
	if (m_hAllowCloseEvent)
	{
		CloseHandle(m_hAllowCloseEvent);
	}
	if (m_hRenderSleepEvent)
	{
		CloseHandle(m_hRenderSleepEvent);
	}
	if (m_SyncThread)
	{
		CloseHandle(m_SyncThread);
	}
	if (m_hH264AACBufferEvent)
	{
		CloseHandle(m_hH264AACBufferEvent);
	}
	if (m_pDemandMediaAudio)
		delete m_pDemandMediaAudio;
    if (texture) {
        delete texture;
        texture = nullptr;
    }

	if (config)
	{
		delete config;
		config = nullptr;
	}
	colorConvertShader ? delete[] colorConvertShader : true;
	m_playPath ? delete[] m_playPath : true;

	if (colorFieldConvertShader)
		delete colorFieldConvertShader;
	if (RGBFieldShader)
		delete RGBFieldShader;

	DeleteCriticalSection(&DataLock);
	DeleteCriticalSection(&CallBackLock);
	DeleteCriticalSection(&TextureDataLock);

	DestoryCSampleData();
}

bool VideoLiveSource::Init(Value &JsonParam)
{
	config = new VideoSourceConfig(JsonParam);
	UpdateSettings(JsonParam);
	return true;
}

void VideoLiveSource::StartMonitor()
{
	Log(TEXT("LINE : %d, FUNC : %s ,MonitorThread Start!"), __LINE__, String(__FUNCTION__).Array());
	m_MonitorThread = OSCreateThread((XTHREAD)VideoLiveSource::MonitorThread, this);
}
unsigned VideoLiveSource::VideoFormatCallback(
    char *chroma,
    unsigned *width, 
    unsigned *height,
    unsigned *pitches, 
    unsigned *lines)
{
	
    memcpy(chroma, CHROMA, sizeof(CHROMA) - 1);
    *pitches = *width * 4;
    *lines = *height;

	EnterCriticalSection(&TextureDataLock);
	CallBackHight = *height;
	CallBackWidth = *width;
	if (Captureing)
	{
		DWORD Width = 0, Height = 0;
		D3DRender->GetTextureWH(texture, Width, Height);
		if (!texture || Width != *width || Height != *height) {
			if (texture) {
				delete texture;
				texture = nullptr;
			}
		}
	}   
	LeaveCriticalSection(&TextureDataLock);

    mediaWidthOffset = 0;
    mediaHeightOffset = 0;
    
    mediaWidth = *width;
    mediaHeight = *height;
    
    if (false/*!config->isStretching*/) {
        float srcAspect = (float)*width / (float)*height;
        float dstAspect = (float)config->width / (float)config->height;

        if (srcAspect > dstAspect) {
            if(config->width != (*width) ) { //don't scale if size equal
                mediaWidth  = config->width;
                mediaHeight = static_cast<unsigned>(mediaWidth / srcAspect + 0.5);
            }
            mediaHeightOffset = (config->height - mediaHeight) / 2;
        } else {
            if( config->height != (*height) ) { //don't scale if size equal
                mediaHeight = config->height;
                mediaWidth  = static_cast<unsigned>(mediaHeight * srcAspect + 0.5);
            }
            mediaWidthOffset = (config->width - mediaWidth) / 2;
        }
    } else {
        mediaWidth = config->width;
        mediaHeight = config->height;
    }

    previousRenderSize.x = previousRenderSize.y = 0;
    return 1;
}


void VideoLiveSource::VideoFormatCleanup()
{
}

void VideoLiveSource::Tick(float fSeconds)
{}

void VideoLiveSource::GlobalSourceEnterScene()
{
    isInScene = true;
	if (enteredSceneCount++)
		return;
	AudioParam Param;
	Param.iBitPerSample = audioFormat.wBitsPerSample;
	Param.iChannel = audioFormat.nChannels;
	Param.iSamplesPerSec = audioFormat.nSamplesPerSec;
	EnterLiveVideoSection();
	if (Param.iBitPerSample > 0)
		m_pDemandMediaAudio->ResetAudioParam(Param);
	LeaveLiveVideoSection();
}

void VideoLiveSource::GlobalSourceLeaveScene()
{
    isInScene = false;
	if (!enteredSceneCount)
		return;
	if (--enteredSceneCount)
		return;
	EnterCriticalSection(&TextureDataLock);
	//删除视频音频渲染
	LeaveCriticalSection(&TextureDataLock);
}

void VideoLiveSource::Preprocess()
{
	if (!Captureing || enteredSceneCount == 0)
	{
		return;
	}

	if (!Captureing)
	{
		return;
	}

	CSampleData *lastSample = NULL;
	EnterCriticalSection(&DataLock);
	lastSample = latestVideoSample;
	latestVideoSample = NULL;
	LeaveCriticalSection(&DataLock);

	LPBYTE lpData;
	UINT pitch;

	EnterCriticalSection(&TextureDataLock);
	if (Captureing&&lastSample&&texture)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				if (lastSample->cx == CallBackWidth && lastSample->cy == CallBackHight && lastSample->dataLength == CallBackWidth * CallBackHight * 3 / 2)
					PackPlanar(lpData, lastSample->lpData, CallBackWidth, CallBackHight, pitch, 0, CallBackHight, CallBackWidth, 0);
				D3DRender->Unmap(texture);
			}
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt) {
			if (lastSample->cx == CallBackWidth && lastSample->cy == CallBackHight && lastSample->dataLength == CallBackWidth * CallBackHight * 4)
				D3DRender->SetImage(GetTexture(), lastSample->lpData, GS_IMAGEFORMAT_BGRA, CallBackWidth * 4);
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			if (lastSample->cx == CallBackWidth && lastSample->cy == CallBackHight && lastSample->dataLength == CallBackWidth * CallBackHight * 3)
				D3DRender->SetImage(GetTexture(), lastSample->lpData, GS_IMAGEFORMAT_BGR, CallBackWidth * 3);
		}

		lastSample->Release();
	}
	LeaveCriticalSection(&TextureDataLock);
	
}

void VideoLiveSource::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	if (!Captureing || enteredSceneCount == 0)
	{
		return;
	}
	if (previousRenderSize != size || m_bScaleFull != bScaleFull) {

		m_bScaleFull = bScaleFull;
		mediaHeightOffset = 0;
		mediaWidthOffset = 0;

		if (bScaleFull)
		{
			mediaWidth = size.x;
			mediaHeight = size.y;
		}
		else
		{
			float srcAspect = (float)CallBackWidth / (float)CallBackHight;
			float dstAspect = (float)size.x / (float)size.y;
			if (srcAspect > dstAspect) {

				mediaWidth = size.x;
				mediaHeight = static_cast<unsigned>(mediaWidth / srcAspect + 0.5);
				mediaHeightOffset = (size.y - mediaHeight) / 2;
			}
			else {
				mediaHeight = size.y;
				mediaWidth = static_cast<unsigned>(mediaHeight * srcAspect + 0.5);
				mediaWidthOffset = (size.x - mediaWidth) / 2;
			}
		}
		mediaOffset.x = (float)mediaWidthOffset;
		mediaOffset.y = (float)mediaHeightOffset;

		mediaSize.x = (float)mediaWidth;
		mediaSize.y = (float)mediaHeight;

		mediaSize += mediaOffset;

		previousRenderSize.x = size.x;
		previousRenderSize.y = size.y;

	}
	EnterCriticalSection(&TextureDataLock);
	if (texture&&m_bReadyDraw) {

		if (FilterTexture)
		{
			Shader *oldShader = NULL;
			if (bIsFieldSignal && mediaSize != videoSize)
			{
				oldShader = D3DRender->GetCurrentPixelShader();
				if (RGBFieldShader)
				{
					D3DRender->LoadPixelShader(RGBFieldShader);
				}
			}

			D3DRender->DrawSprite(FilterTexture, 0xFFFFFFFF, pos.x + mediaOffset.x, pos.y + mediaOffset.y, pos.x + mediaSize.x, pos.y + mediaSize.y);

			if (bIsFieldSignal && mediaSize != videoSize)
			{
				D3DRender->LoadPixelShader(oldShader);
			}
		}
		else
		{
			Shader *oldShader = D3DRender->GetCurrentPixelShader();
			if (mediaSize == videoSize || !bIsFieldSignal)
			{
				if (colorConvertShader)
				{
					D3DRender->LoadPixelShader(colorConvertShader);
					colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
				}
			}
			else if (bIsFieldSignal && colorFieldConvertShader)
			{
				D3DRender->LoadPixelShader(colorFieldConvertShader);
				colorFieldConvertShader->SetFloat(colorFieldConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
			}
			float fOpacity = float(100)*0.01f;
			DWORD opacity255 = DWORD(fOpacity*255.0f);
			D3DRender->DrawSprite(texture, (opacity255 << 24) | 0xFFFFFF, /*40, 0, 280, 240*/ pos.x + mediaOffset.x, pos.y + mediaOffset.y, pos.x + mediaSize.x, pos.y + mediaSize.y);
			D3DRender->LoadPixelShader(oldShader);
		}
		
    }
	LeaveCriticalSection(&TextureDataLock);
}

void VideoLiveSource::FrameCallBackFunc(void* frame, int frame_type, const void* ctx, int status)
{
	//直接返回
	
	if (status != 0)
	{
		Log(TEXT("LINE: %d, FUNC:%s, Decoder Retrun error ! ThreadID = %dThis =0x%p"), __LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), ctx);
		mp_release_frame(&frame);
		return;
	}
	
	VideoLiveSource * This_ = (VideoLiveSource *)(ctx);
	
	if (frame_type == 2) //MPFrameInfo类型
	{
		MPFrameInfo * pMPFrameInfo = reinterpret_cast<MPFrameInfo *>(frame);
		if (pMPFrameInfo->media_type == 1)//视频
		{
			
			if (This_->m_width != pMPFrameInfo->width || This_->m_height != pMPFrameInfo->height)
			{
				/*Log(TEXT("This_->m_width = %d, pMPFrameInfo->width = %d,This_->m_height = %d,pMPFrameInfo->height = %d"),
					This_->m_width, pMPFrameInfo->width, This_->m_height, pMPFrameInfo->height);*/
				mp_release_frame(&frame);
				return;
			}
			CSampleData* videoSample = new CSampleData;
			videoSample->bAudio = false;
			videoSample->dataLength = This_->CallBackWidth*This_->CallBackHight * 3 / 2;
			videoSample->lpData = (LPBYTE)Allocate_Bak(videoSample->dataLength);//pointer; //
			videoSample->cx = This_->CallBackWidth;
			videoSample->cy = This_->CallBackHight;
			videoSample->colorType = ColorType_I420;

			if (pMPFrameInfo->frame_size[0] == This_->m_width)
			{
				memcpy(videoSample->lpData, pMPFrameInfo->frame_data[0], This_->m_width*This_->m_height);
				memcpy(videoSample->lpData + This_->m_width*This_->m_height, pMPFrameInfo->frame_data[1], This_->m_width*This_->m_height / 4);
				memcpy(videoSample->lpData + This_->m_width*This_->m_height * 5 / 4, pMPFrameInfo->frame_data[2], This_->m_width*This_->m_height / 4);
			}
			else
			{
				if (pMPFrameInfo->frame_data && pMPFrameInfo->frame_size)
				{
					int i = 0;
					while (i<8){
						if (pMPFrameInfo->frame_data[i] && pMPFrameInfo->frame_size[i] > 0)
						{
								int i = 0, a = 0;
								for (i = 0; i < This_->m_height; i++)
								{
									memcpy(videoSample->lpData + a, pMPFrameInfo->frame_data[0] + i * pMPFrameInfo->frame_size[0], This_->m_width);
									a += This_->m_width;
								}
								for (i = 0; i < This_->m_height / 2; i++)
								{
									memcpy(videoSample->lpData + a, pMPFrameInfo->frame_data[1] + i * pMPFrameInfo->frame_size[1], This_->m_width / 2);
									a += This_->m_width / 2;
								}
								for (i = 0; i < This_->m_height / 2; i++)
								{
									memcpy(videoSample->lpData + a, pMPFrameInfo->frame_data[2] + i * pMPFrameInfo->frame_size[2], This_->m_width / 2);
									a += This_->m_width / 2;
								}
							
						}
						else{
							break;
						}
						++i;
					}
				}
			}
			
			videoSample->timestamp = pMPFrameInfo->pts;
			if (This_->m_StatusStream == YUV_STREAM) {
			//	Log(TEXT("LINE : %d, FUNC : %s ,iLastVideoPts = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), pMPFrameInfo->pts,This_);
				This_->m_iYUVCount++;
				if (This_->m_iYUVCount >= This_->m_pMPMediaInfo.v_frame_rate - 3)
				{
					This_->m_iYUVCount = 0;
					Log(TEXT("LINE : %d, FUNC : %s ,m_hYUVBufferEvent 缓冲成功This =0x%p"), __LINE__, String(__FUNCTION__).Array(),This_);
					SetEvent(This_->m_hYUVBufferEvent);//YUV缓冲区满
				}
			}
			This_->m_YUVBuffer.push(videoSample);
			This_->m_pts = pMPFrameInfo->pts;
			This_->m_iLastVideoFrameTimestamp = pMPFrameInfo->pts;
		}
		else if (pMPFrameInfo->media_type == 2)//音频
		{
			
			//Log(TEXT("LINE : %d, FUNC : %s ,audio_pts = %d"), __LINE__, String(__FUNCTION__).Array(), pMPFrameInfo->pts);

			CSampleData* videoSample = new CSampleData;
			videoSample->bAudio = true;
			videoSample->dataLength = pMPFrameInfo->frame_size[0];
			videoSample->lpData = (LPBYTE)Allocate_Bak(videoSample->dataLength);//pointer; //
			//VolumeCaculate((char *)videoSample->lpData, pMPFrameInfo->frame_data[0], pMPFrameInfo->frame_size[0], This_->m_iVolume / 100.0);
			videoSample->timestamp = pMPFrameInfo->pts;
			if (This_->m_StatusStream == PCM_STREAM) {
				This_->m_iPCMCount++;
				if (This_->m_iPCMCount >= /*暂时先这样估算*/This_->m_pMPMediaInfo.a_channels*This_->m_pMPMediaInfo.a_sample_rate * 2 / 4096)
				{
					This_->m_iPCMCount = 0;
					Log(TEXT("LINE : %d, FUNC : %s ,m_hPCMBufferEvent 缓冲成功This =0x%p"), __LINE__, String(__FUNCTION__).Array(),This_);
					SetEvent(This_->m_hPCMBufferEvent);//PCM缓冲区满
				}
			}
			This_->m_PCMuffer.push(videoSample);
		}
	}
	mp_release_frame(&frame);
}

void VideoLiveSource::SetParamCallBackFunc(int nMediatype, void* ctx)
{
	VideoLiveSource * This_ = (VideoLiveSource *)(ctx);
	if (This_->m_StatusStream != STOP_STREAM) {
		SetEvent(This_->m_hH264AACBufferEvent);
		Log(TEXT("LINE : %d, FUNC : %s ,m_hH264AACBufferEvent 缓冲成功,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), This_);
	}
}

void VideoLiveSource::UpdateSettings(Value &JsonParam)
{
	m_bisForceOpenURL = true;
	if (m_playPath)
	{
		config->Reload();
		m_iVolume = config->volume;

		if (bIsFieldSignal != config->bIsScanInterlace)
		{
			bIsFieldSignal = config->bIsScanInterlace;
			EnterCriticalSection(&TextureDataLock);

			ChangeShader();

			LeaveCriticalSection(&TextureDataLock);
		}

		if (!strcmp(m_playPath, config->playlist[0].CreateUTF8String()) && (m_iLastBufferTime == JsonParam["bufferTime"].asInt()))
		{
			Log(TEXT("LINE : %d, FUNC : %s ,URL is Same retrun!This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
			return;
		}
		
	}

	if (H264_AAC_STREAM == m_StatusStream) {         //当前正处在缓冲码流
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hH264AACBufferEvent);
	}
	else if (YUV_STREAM == m_StatusStream) {          //当前正处在缓冲YUV
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hYUVBufferEvent);
	}
	else if (PCM_STREAM == m_StatusStream) {         //当前正处在缓冲PCM
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hPCMBufferEvent);
	}
	else if (PLAY_STREAM == m_StatusStream || STOP_STREAM == m_StatusStream) {
		m_StatusStream = STOP_STREAM;
		SetEvent(m_hUpdataEvent);
	}
}

Vect2 VideoLiveSource::GetSize() const
{
    return videoSize;
}

void VideoLiveSource::BeginScene()
{
	EnterCriticalSection(&TextureDataLock);
	Captureing = true;

	ChangeShader();

	if (texture)
	{
		delete texture;
		texture = nullptr;
	}
	DWORD Width = 0, Heigth = 0;
	D3DRender->GetTextureWH(texture, Width, Heigth);
	if (!texture || Width != CallBackWidth || Heigth != CallBackHight) {
		if (texture) {
			delete texture;
			texture = nullptr;
		}
	}

	if (!texture) {
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			texture = D3DRender->CreateTexture(CallBackWidth, CallBackHight, GS_RGBA, nullptr, FALSE, FALSE);
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			texture = D3DRender->CreateTexture(CallBackWidth, CallBackHight, GS_BGRA, nullptr, FALSE, FALSE);
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			texture = D3DRender->CreateTexture(CallBackWidth, CallBackHight, GS_BGR, nullptr, FALSE, FALSE);
		}
	}
	LeaveCriticalSection(&TextureDataLock);
}

void VideoLiveSource::EndScene()
{
	EnterCriticalSection(&TextureDataLock);
	Captureing = false;
	if (texture)
	{
		delete texture;
		texture = nullptr;
	}
	LeaveCriticalSection(&TextureDataLock);
}

void VideoLiveSource::InitCSampleData()
{
	InitAudioCSampleData();
}

void VideoLiveSource::DestoryCSampleData()
{
	DestoryAuidoCSampleData();
}

void VideoLiveSource::InitAudioCSampleData()
{
	if (audioSample == NULL)
	{
		audioSample = new CSampleData;
		audioSample->bAudio = true;
		audioSample->dataLength = 0;
		audioSample->lpData = NULL;
		audioSample->pAudioFormat = &audioFormat;

	}
}


void VideoLiveSource::DestoryAuidoCSampleData()
{
	if (audioSample)
	{
		audioSample->Release();
		audioSample = NULL;
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
			Log(TEXT("Tried to sleep for %u seconds, that can't be right! Triggering breakpoint."), milliseconds);
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

void VideoLiveSource::Synchronization()
{
	Log(TEXT("LINE : %d, FUNC : %s ,Synchronization Start,sleepTime = %d,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), 1000 / m_pMPMediaInfo.v_frame_rate,GetCurrentThreadId(), this);
	int frameTimeNS = 1; //超时时长
	int iLastVframerate = m_pMPMediaInfo.v_frame_rate;
	UINT no_sleep_counter = 0;
	m_interval = 1000 / m_pMPMediaInfo.v_frame_rate;
	int iCurrentVideoPts = 0;
	int iLastVideoPts = 0;
	int audio_pts = 0;
	int audioStartTryTimestamp = 0;  //音频开始探测时戳戳
	int audioEndTryTimestamp = 0;    //音频结束探测时间戳
	int iDiffPts = 0;
	int iLastpts = 0;
	int iCurrentpts = 0;
	int iDiffTime = 0;
	QWORD TimeLast = 0;
	QWORD TimeCurrent = 0;
	bool bisReBeginFirstVideoFrame = true;
	bool bisTimeStampReBegin = false;
	bool bisAudioNeedTry = false;
	int iLogIndex = 0;
	
	QWORD LT = 0;
	QWORD CT = 0;
	m_iYUVBufferTotalTime = 1000;//将视频缓冲区时长设置为1s

	while (true){

		if (m_bAllowClose)
		{
			SetEvent(m_hAllowCloseEvent);
			Log(TEXT("LINE : %d, FUNC : %s ,进入 m_hPauseThreadEvent 锁等待Thread = %d,This =0x%p"),
				__LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
			WaitForSingleObject(m_hPauseThreadEvent, INFINITE);
			Log(TEXT("LINE : %d, FUNC : %s ,走出 m_hPauseThreadEvent 锁等待Thread = %d,This =0x%p"),
				__LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
			ResetEvent(m_hPauseThreadEvent);
			m_bAllowClose = false;
		}
	    if (m_FirstVideoFrameTime > 0 && m_CurrentVideoFrameTime > 0)
		{
			m_DiffVideoFrameTime =  m_CurrentVideoFrameTime - m_FirstVideoFrameTime;
			if (iLogIndex > 550 && iLogIndex < 560)
				Log(TEXT("LINE : %d, FUNC : %s 计算视频帧时间增量.m_CurrentVideoFrameTime = %lld,m_FirstVideoFrameTime = %lld,m_DiffVideoFrameTime = %lld.Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), m_CurrentVideoFrameTime, m_FirstVideoFrameTime, m_DiffVideoFrameTime,GetCurrentThreadId(), this);
		}
		m_CurrentQpcTime = GetQPCNS();
		if (m_FirstQpcTime > 0)
		{
			m_DiffQpcTime =  m_CurrentQpcTime - m_FirstQpcTime;
			if (iLogIndex > 550 && iLogIndex < 560)
				Log(TEXT("LINE : %d, FUNC : %s 计算视渲染时间增量.m_CurrentQpcTime = %lld,m_FirstQpcTime = %lld.m_DiffQpcTime = %lld.Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), m_CurrentQpcTime / 1000000, m_FirstQpcTime / 1000000, m_DiffQpcTime / 1000000, GetCurrentThreadId(), this);
		}

		if (m_DiffQpcTime / 1000000 - m_DiffVideoFrameTime > 10 && !m_bResetDiffMode)  //不是时戳跳变情况下的处理
		{
			if (m_DiffQpcTime / 1000000 - m_DiffVideoFrameTime >= frameTimeNS)
			{
				frameTimeNS = 1;
			}
			else
			{
				frameTimeNS = frameTimeNS - (m_DiffQpcTime / 1000000 - m_DiffVideoFrameTime);
			}
			if (iLogIndex > 550 && iLogIndex < 560)
				Log(TEXT("LINE : %d, FUNC : %s m_DiffQpcTime / 1000000 = %lld.m_DiffVideoFrameTime = %lld,调整渲染睡眠时间,frameTimeNS = %d.Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), m_DiffQpcTime / 1000000, m_DiffVideoFrameTime, frameTimeNS, GetCurrentThreadId(), this);
		}
		else if (m_DiffVideoFrameTime - m_DiffQpcTime / 1000000 > 10 && !m_bResetDiffMode)      //不是时戳跳变情况下的处理
		{
			
			frameTimeNS = frameTimeNS + 10;
			if (iLogIndex > 550 && iLogIndex < 560)
				Log(TEXT("LINE : %d, FUNC : %s m_DiffQpcTime / 1000000 = %lld.m_DiffVideoFrameTime = %lld,调整渲染睡眠时间,frameTimeNS = %d.Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), m_DiffQpcTime / 1000000, m_DiffVideoFrameTime, frameTimeNS, GetCurrentThreadId(), this);
		}	
		int ret = WaitForSingleObject(m_hRenderSleepEvent, frameTimeNS);
		if (WAIT_OBJECT_0 == ret) {
			//...退出
			if (m_stop)
			{
				break;
			}
			ResetEvent(m_hRenderSleepEvent);
		}
		if (m_bResetDiffMode) //上一帧视频发生了跳变,重新初始化所有计时器
		{
			Log(TEXT("LINE : %d, FUNC : %s.当前帧发生了跳变，将所有计时器重新初始化!Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(),GetCurrentThreadId(), this);
			m_bResetDiffMode = false;
			m_FirstVideoFrameTime = m_CurrentVideoFrameTime;
			m_DiffVideoFrameTime = 0;
			m_FirstQpcTime = m_CurrentQpcTime;
			m_DiffQpcTime = 0;
		}
	
		if (m_StatusStream != PLAY_STREAM)
		{
			frameTimeNS = 40;
			continue;
		}
		iLogIndex++;
		if (iLogIndex > 552)
		{
			iLogIndex = 0;
		}
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
		//视频渲染
		m_StatusStream = PLAY_STREAM;
		bisAudioNeedTry = false;

//重推处理，获取前两帧
		if (m_bisReBuffer) //刚刚重新缓冲过
		{
			if (Last_inf_Video)
			{
				Last_inf_Video->Release();
				Last_inf_Video = NULL;
			}
			if (!m_YUVBuffer.try_pop(Last_inf_Video))  //获取第一帧
			{
				Log(TEXT("LINE : %d, FUNC : %s ,m_YUVBuffer获取第一帧失败,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
				m_StatusStream = STOP_STREAM;
				frameTimeNS = 40;
				SetEvent(m_hUpdataEvent);
				continue;
			}
			if (!Last_inf_Video)
			{
				Log(TEXT("LINE : %d, FUNC : %s ,Last_inf_Video 指针为空了!  m_YUVBuffer.size = %dThis =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_YUVBuffer.unsafe_size(), this);
				continue;
			}
			m_iYUVBufferTotalTime = m_iLastVideoFrameTimestamp - Last_inf_Video->timestamp;
			m_FirstVideoFrameTime = Last_inf_Video->timestamp;  //重置第一帧时间戳时间.
			m_FirstQpcTime = GetQPCNS();                    //重置第一帧渲染时间.
			if (m_FirstVideoFrameTime == 0)                     //为了方便计算区分，第一帧时间戳为0时，主动设置为1.
			{
				m_FirstVideoFrameTime = 1;
			}
			Log(TEXT("LINE : %d, FUNC : %s ,第一帧开始时间戳: %lld,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_FirstVideoFrameTime, this);
			Log(TEXT("LINE : %d, FUNC : %s ,第一帧开始渲染时间: %lld,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_FirstQpcTime, this);
			
			if (m_iYUVBufferTotalTime <1000 || m_iYUVBufferTotalTime > 2000)
			{
				Log(TEXT("LINE : %d, FUNC : %s ,yuv缓冲时长m_iYUVBufferTotalTime = %d，不符合要求进行调整为1秒,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_iYUVBufferTotalTime, GetCurrentThreadId(), this);
				m_iYUVBufferTotalTime = 1000; //只缓冲1秒数据
				
			}
			Log(TEXT("LINE : %d, FUNC : %s ,yuv缓冲时长m_iYUVBufferTotalTime = %d，Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_iYUVBufferTotalTime, GetCurrentThreadId(), this);
			mp_get_frame(HMediaProcess, 1, m_fVideoWarnning);
			m_bisReBuffer = false;
		}

//上一帧时戳
		if (!Last_inf_Video)
		{
			Log(TEXT("LINE : %d, FUNC : %s ,Last_inf_Video 指针为空了!  m_YUVBuffer.size = %dThis =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_YUVBuffer.unsafe_size(), this);
			continue;
		}
		iLastVideoPts = Last_inf_Video->timestamp;
		m_CurrentVideoFrameTime = iLastVideoPts;          //计算时间戳总时差
		if (m_CurrentVideoFrameTime == 0)
		{
			m_CurrentVideoFrameTime = 1;
		}
		m_bReadyDraw = true;
		CSampleData* inf_Video = NULL;
		if (!m_YUVBuffer.try_pop(inf_Video))
		{
			Log(TEXT("LINE : %d, FUNC : %s ,m_YUVBuffer缓冲区空了重新打开,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
			//删除最后一帧的音频,缓冲区里面的音频在monitor里面清空
			if (Last_inf_Audio)
			{
				Last_inf_Audio->Release();
				Last_inf_Audio = NULL;
			}
			m_StatusStream = STOP_STREAM;
			frameTimeNS = 40;
			SetEvent(m_hUpdataEvent);
			continue;
		}

		if (!inf_Video || !Last_inf_Video)
		{
			Log(TEXT("LINE : %d, FUNC : %s ,inf_Video 指针为空了!  m_YUVBuffer.size = %dThis =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_YUVBuffer.unsafe_size(), this);
			frameTimeNS = 40;
			continue;
		}
		iCurrentVideoPts = inf_Video->timestamp;
		if (iLogIndex > 550 && iLogIndex < 560)
		{
			Log(TEXT("LINE : %d, FUNC : %s ,渲染视频iLastVideoPts = %d, mp_get_frame  m_YUVBuffer.size = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), iLastVideoPts, m_YUVBuffer.unsafe_size(),this);
		}
		Last_inf_Video->bFieldSignal = bIsFieldSignal;
		EnterCriticalSection(&CallBackLock);
		for (int i = 0; i < m_ListCallBack.Num(); ++i)
		{
			__DataCallBack &OneCallBack = m_ListCallBack[i];
			if (OneCallBack.CallBack)
				OneCallBack.CallBack(OneCallBack.Context, Last_inf_Video);
		}
		LeaveCriticalSection(&CallBackLock);

		EnterCriticalSection(&DataLock);
		SafeRelease(latestVideoSample);

		latestVideoSample = Last_inf_Video;
		LeaveCriticalSection(&DataLock);

		Last_inf_Video = inf_Video;

		if (iCurrentVideoPts - iLastVideoPts >= 0)
		{
			frameTimeNS = iCurrentVideoPts - iLastVideoPts;
		}
		else        //重推渲染和上一帧视频时间戳同步的音频，删除
		{
			//删除当前音频并且重新获取包括残留音频帧
			Log(TEXT("LINE : %d, FUNC : %s ,时间戳发生变化查看是否重推,iLastVideoPts = %d,iCurrentVideoPts = %d,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), iLastVideoPts, iCurrentVideoPts, GetCurrentThreadId(), this);

			//删除缓冲区的音频
			if (Last_inf_Audio)
			{
				Last_inf_Audio->Release();
				Last_inf_Audio = NULL;
			}

			while (true)
			{
				CSampleData* inf_Audio = NULL;
				if (!m_PCMuffer.try_pop(inf_Audio)) {
					break;
				}
				inf_Audio->Release();
				inf_Audio = NULL;
			}

			frameTimeNS = 40;
			continue;
		}

		CT = GetTickCount();
		if (frameTimeNS > 100)
		{
			Log(TEXT("LINE : %d, FUNC : %s ,frameTimeNS = %d,iCurrentVideoPts = %d,iLastVideoPts = %d,diff = %lld,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), frameTimeNS, iCurrentVideoPts, iLastVideoPts, CT - LT, GetCurrentThreadId(), this);
		}
		if (CT - LT > 200)
		{
			Log(TEXT("LINE : %d, FUNC : %s ,frameTimeNS = %d,diff = %lld,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), frameTimeNS, CT - LT, GetCurrentThreadId(), this);
		}
		LT = CT;
		
		m_iYUVBufferCurTime = m_iLastVideoFrameTimestamp - iCurrentVideoPts;
		if (m_iYUVBufferCurTime > 0 )
		{
			if (m_iYUVBufferCurTime > m_iYUVBufferTotalTime)
			{
				;//不需要再get
			}
			else
			{
				//需要再次get
				int iDiffTimeStamp = m_iYUVBufferTotalTime - m_iYUVBufferCurTime;
				for (int i = 0; i < iDiffTimeStamp*m_pMPMediaInfo.v_frame_rate /1000 +1; i++)
				{
					mp_get_frame(HMediaProcess, 1, m_fVideoWarnning);
				}
				
				if (m_fVideoWarnning < m_nWarnTime * 1000)
				{
					m_pos = 2;
				}
				else
				{
					m_pos = 1;
				}
			}
		}
		else
		{
			//可能重推
			mp_get_frame(HMediaProcess, 1, m_fVideoWarnning);
			if (m_fVideoWarnning < m_nWarnTime * 1000)
			{
				m_pos = 2;
			}
			else
			{
				m_pos = 1;
			}
		}


		if (frameTimeNS > 20*1000)  //时戳跳变大于20s，进行重连
		{
			Log(TEXT("LINE : %d, FUNC : %s ,frameTimeNS = %d,diff = %lld,时戳跳变太大，重新进行缓冲.Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), frameTimeNS, CT - LT, GetCurrentThreadId(), this);
			if (Last_inf_Audio)
			{
				Last_inf_Audio->Release();
				Last_inf_Audio = NULL;
			}
			m_StatusStream = STOP_STREAM;
			frameTimeNS = 40;
			SetEvent(m_hUpdataEvent);
			continue;
		}

		if (frameTimeNS > (1000/m_pMPMediaInfo.v_frame_rate)*5 )  //时戳跳变大于5倍帧率，需要进行探测
		{
			bisAudioNeedTry = true; //音频需要探测
			audioStartTryTimestamp = 0;
			audioEndTryTimestamp = 0;
			m_bResetDiffMode = true;
			Log(TEXT("LINE : %d, FUNC : %s ,需要探测音频。frameTimeNS = %d,iCurrentVideoPts = %d,iLastVideoPts = %d,diff = %lld,m_fVideoWarnning = %d,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), frameTimeNS, iCurrentVideoPts, iLastVideoPts, CT - LT, m_fVideoWarnning, GetCurrentThreadId(), this);
		}
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
		if (Last_inf_Audio&&Last_inf_Audio->lpData && (Last_inf_Audio->timestamp >= iLastVideoPts) &&( Last_inf_Audio->timestamp <= iCurrentVideoPts)) {
			
			Last_inf_Audio->pAudioFormat = (void*)&audioFormat;

			if (m_pDemandMediaAudio)
			{
				m_pDemandMediaAudio->PushAudio(Last_inf_Audio->lpData, Last_inf_Audio->dataLength, Last_inf_Audio->timestamp, this, enteredSceneCount != 0);
				mp_get_frame(HMediaProcess, 2, m_fAudioWarnning);
				if (iLogIndex > 550 && iLogIndex < 560)
					Log(TEXT("LINE : %d, FUNC : %s ,渲染音频audio_pts = %d, mp_get_frame  m_PCMuffer.size = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), audio_pts, m_PCMuffer.unsafe_size(), this);
			}
			Last_inf_Audio->Release();
			Last_inf_Audio = NULL;
		}
		else if (Last_inf_Audio && (Last_inf_Audio->timestamp > iCurrentVideoPts))
		{
			
			if(iLogIndex > 550 && iLogIndex < 560)
			{
				if (Last_inf_Audio && (Last_inf_Audio->timestamp > iCurrentVideoPts + 2000))
				{
					Log(TEXT("LINE : %d, FUNC : %s ,流严重不同步Last_inf_Audio->timestamp = %d,iLastVideoPts = %d,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), Last_inf_Audio->timestamp, iLastVideoPts, GetCurrentThreadId(), this);
				}
			}
			
			continue;
		}
		

		if (!m_pMPMediaInfo.has_video) {
			//只有音频的情况，待定
			break;
		}
		else {
			while (true) {

				CSampleData* inf_Audio = NULL;

				if (!m_PCMuffer.try_pop(inf_Audio)) {         //重新get
					if (Last_inf_Audio)
					{
						Last_inf_Audio->Release();
						Last_inf_Audio = NULL;
					}
					if(iLogIndex > 550 && iLogIndex < 560)
						Log(TEXT("LINE : %d, FUNC : %s ,音频空了，重新mp_get_frame  m_PCMuffer.unsafe_size() = %dThis =0x%p"), __LINE__, String(__FUNCTION__).Array(), audio_pts, m_PCMuffer.unsafe_size(), this);
					//重新get 音频
					for (int iIndex = 0; iIndex </*暂时先这样估算*/m_iPcmBufferNumber/*m_pMPMediaInfo.a_channels*m_pMPMediaInfo.a_sample_rate * 2 / 4096*/; iIndex++) {
						mp_get_frame(HMediaProcess, 2, m_fAudioWarnning);//音频
					}
					if (bisAudioNeedTry)
					{
						//这里需要判断条件
						frameTimeNS = 40;//视频有跳变，没有音频时，直接渲染下一帧
					}
					break;
				}

				audio_pts = inf_Audio->timestamp;
				if (audio_pts < iLastVideoPts) {
					mp_get_frame(HMediaProcess, 2, m_fAudioWarnning);
					Log(TEXT("LINE : %d, FUNC : %s ,时戳过小删除audio_pts = %d,iLastVideoPts = %d, mp_get_frame  m_PCMuffer.unsafe_size() = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), audio_pts, iLastVideoPts, m_PCMuffer.unsafe_size(), this);
					inf_Audio->Release();//直接删除
				}
				else if ((audio_pts >= iLastVideoPts) && (audio_pts <= iCurrentVideoPts)) {
					if (bisAudioNeedTry)
					{
						if (m_pMPMediaInfo.a_channels*m_pMPMediaInfo.a_sample_rate * 2 > 0)
						{
							audioStartTryTimestamp = audioStartTryTimestamp + inf_Audio->dataLength*1000 / (m_pMPMediaInfo.a_channels*m_pMPMediaInfo.a_sample_rate * 2);
						}
					}
					inf_Audio->pAudioFormat = (void*)&audioFormat;

					if (m_pDemandMediaAudio)
					{
						m_pDemandMediaAudio->PushAudio(inf_Audio->lpData, inf_Audio->dataLength, inf_Audio->timestamp, this, enteredSceneCount !=0);
						mp_get_frame(HMediaProcess, 2, m_fAudioWarnning);
						if (iLogIndex > 550 && iLogIndex < 560)
							Log(TEXT("LINE : %d, FUNC : %s ,渲染音频audio_pts = %d, mp_get_frame  m_PCMuffer.size = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), audio_pts, m_PCMuffer.unsafe_size(), this);
					}
					inf_Audio->Release();
					inf_Audio = NULL;
				}
				else {
					Last_inf_Audio = inf_Audio;

					if (bisAudioNeedTry)  //需要探测
					{
						if (audioStartTryTimestamp > 0)  //正常情况
						{
							if (frameTimeNS > audioStartTryTimestamp)
							{
								frameTimeNS = audioStartTryTimestamp;
								Log(TEXT("LINE : %d, FUNC : %s ,探测音频播放时长。audio Time = %d,frameTimeNS = %d,iCurrentVideoPts = %d,iLastVideoPts = %d,diff = %lld,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), audioStartTryTimestamp,frameTimeNS, iCurrentVideoPts, iLastVideoPts, CT - LT, GetCurrentThreadId(), this);
							}
						}
					}
					break;
				}
			}
		}
	}
	Log(TEXT("LINE : %d, FUNC : %s ,SyncThread Exit! ThreadID = %d,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(),GetCurrentThreadId(), this);
}

void VideoLiveSource::Monitor()
{
	m_MonitorThreadID = GetCurrentThreadId();
	Log(TEXT("LINE : %d, FUNC : %s ,Monitor Start,Thread = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), m_MonitorThreadID,this);
	while (TRUE){
		int ret = WaitForSingleObject(m_hUpdataEvent, INFINITE);
		if (WAIT_OBJECT_0 == ret) {
			m_pos = 0;
			Log(TEXT("LINE : %d, FUNC : %s ,UpdateSet Execute！ Open New URL! Thread = %d ,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(), m_MonitorThreadID, this);
			ResetEvent(m_hUpdataEvent);
			HASSTOP
			//先停止同步线程
			if (m_SyncThread && !m_bAllowClose)
			{
				m_bAllowClose = true;
				Log(TEXT("LINE : %d, FUNC : %s ,进入 m_hAllowCloseEvent 锁等待Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
				WaitForSingleObject(m_hAllowCloseEvent, INFINITE);
				Log(TEXT("LINE : %d, FUNC : %s ,进入 m_hAllowCloseEvent 锁等待Thread = %d,This =0x%p"),
					__LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId(), this);
				ResetEvent(m_hAllowCloseEvent);
			}
			if (HMediaProcess)
			{
				if (URLState)
				{
					DWORD beforeCloseTime = GetTickCount();
					URLState = 0;
					int ret_close = mp_close(HMediaProcess);  //处于STOP_STREAM状态
					DWORD afterCloseTime = GetTickCount();
					Log(TEXT("LINE : %d, FUNC : %s ,mp_close ret_close = %d, Cost time = %d,m_MonitorThreadID =%d,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(), ret_close, afterCloseTime - beforeCloseTime, m_MonitorThreadID, this);
				}
			}
			//在close之后清除tbb内存，否则回调函数仍然在继续执行
			tbb::concurrent_queue<CSampleData*>::iterator iterYUV = m_YUVBuffer.unsafe_begin();
			for (; iterYUV != m_YUVBuffer.unsafe_end(); iterYUV++) {
				(*iterYUV)->Release();
				*iterYUV = NULL;
			}
			m_YUVBuffer.clear();
			
			tbb::concurrent_queue<CSampleData*>::iterator iterPCM = m_PCMuffer.unsafe_begin();
			for (; iterPCM != m_PCMuffer.unsafe_end(); iterPCM++) {
				(*iterPCM)->Release();
				*iterPCM = NULL;
			}
			m_PCMuffer.clear();
			if (Last_inf_Audio)
			{
				Last_inf_Audio->Release();
				Last_inf_Audio = NULL;
			}

			if (m_pDemandMediaAudio)
				m_pDemandMediaAudio->ResetAudioDB();

			config->Reload();
			m_iVolume = config->volume;
			m_nWarnTime = config->WarnTime;
			if (config->playlist.Num() == 0)
			{
				Log(TEXT("LINE : %d, FUNC : %s ,Not Add URL!"), __LINE__, String(__FUNCTION__).Array());
				continue;
			}
			m_iLastBufferTime = m_nBufferTime;
			m_nBufferTime = config->bufferTime;
			//m_pDemandMediaAudio->SetVolumef(float(config->volume) / 100.0);
			for (unsigned int i = 0; i < config->playlist.Num(); i++) {
				String &mediaEntry = config->playlist[i];
				String token = mediaEntry.GetToken(1, L':');
				bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
				if (!isStream && !OSFileExists(config->playlist[i])) // 文件不存在
				{
					String info(config->playlist[i]);
					info += L" file do not exist";
				}
				m_playPath ? delete[] m_playPath : true;
				m_playPath = new char[strlen(config->playlist[i].CreateUTF8String()) + 1];
				memset(m_playPath, 0, strlen(config->playlist[i].CreateUTF8String()) + 1);
				memcpy(m_playPath, config->playlist[i].CreateUTF8String(), strlen(config->playlist[i].CreateUTF8String()));
				break;
			}
			char url[256] = { 0 };
			sprintf_s(url, " -f YUV");

			DWORD beforeOpenTime = GetTickCount();
			Log(TEXT("LINE : %d, FUNC : %s ,Start to Open URL %s,m_MonitorThreadID = %d,This =0x%p"), __LINE__, String(__FUNCTION__).Array(), String(m_playPath).Array(), m_MonitorThreadID, this);
			memset(&m_pMPMediaInfo, 0, sizeof m_pMPMediaInfo);
			int ret_open = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo); //处于STOP_STREAM状态
			DWORD afterOpenTime = GetTickCount();
			Log(TEXT("LINE : %d, FUNC : %s ,Open url %s Cost Time = %d, ret = %d, HMediaProcess = 0x%p, ThreadID = %u, This =0x%p"), __LINE__, String(__FUNCTION__).Array(), String(m_playPath).Array(), afterOpenTime - beforeOpenTime, ret_open, HMediaProcess, m_MonitorThreadID, this);
			HASSTOP
			if (ret_open != 0 || m_pMPMediaInfo.has_audio && (!m_pMPMediaInfo.a_channels || !m_pMPMediaInfo.a_sample_rate))
			{
				ret_open ? URLState = 0 : URLState = 1;
				Log(TEXT("LINE : %d, FUNC : %s ,Open url %s Fail! ThreadID = %u,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(), String(m_playPath).Array(), m_MonitorThreadID, this);
				WaitForSingleObject(m_hUpdataEvent, 20000);
				HASSTOP
				ResetEvent(m_hUpdataEvent);
				SetEvent(m_hUpdataEvent);//重连
				continue;
			}
			
			URLState = 1;
			HASSTOP
			
			mp_full_cachebuffer(HMediaProcess, m_nBufferTime * 1000, SetParamCallBackFunc, this);
			if (m_nBufferTime > 10) {
				m_iWaitTimeOut = m_nBufferTime * 2 * 1000;
			}
			else {
				m_iWaitTimeOut = 10 * 2 * 1000;
			}
			EnterCriticalSection(&DataLock);
			audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
			audioFormat.nChannels = m_pMPMediaInfo.a_channels;
			audioFormat.wBitsPerSample = 16;

			if (!m_pMPMediaInfo.v_frame_rate)   //SDK maybe get v_frame_rate 0
			{
				Log(TEXT("LINE : %d, FUNC : %s ,SDK FPS return 0,Manual Set 25, Thread = %d ,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(), m_MonitorThreadID, this);
				m_pMPMediaInfo.v_frame_rate = 25;
			}
			if (m_iFPS != m_pMPMediaInfo.v_frame_rate) {
				m_iFPS = m_pMPMediaInfo.v_frame_rate;
				//m_bVframeisChange = true;
			}
			m_height = m_pMPMediaInfo.v_height;
			m_width = m_pMPMediaInfo.v_width;
			videoSize.x = m_pMPMediaInfo.v_width; 
			videoSize.y = m_pMPMediaInfo.v_height;
			CallBackHight = m_height;
			CallBackWidth = m_width;
			InitCSampleData();
			Log(TEXT("m_height = %d,m_width = %d"),m_height,m_width);
			m_FrameWidth = m_width;
			m_FrameHeight = m_height;
			m_FrameLines = 0;
			m_FramePitchs = 0;
			VideoFormatCallback(m_choma, &m_FrameWidth, &m_FrameHeight, &m_FramePitchs, &m_FrameLines);


			LeaveCriticalSection(&DataLock);

			AudioParam Param;
			Param.iBitPerSample = audioFormat.wBitsPerSample;
			Param.iChannel = audioFormat.nChannels;
			Param.iSamplesPerSec = audioFormat.nSamplesPerSec;
			EnterLiveVideoSection();
			if (Param.iBitPerSample > 0)
				m_pDemandMediaAudio->ResetAudioParam(Param);
			LeaveLiveVideoSection();

			//----------------------------------------------------------------------------------------
			//--------------------------------------------------------------------------------------
			//h264缓冲
			m_bisReBuffer = true;
			m_StatusStream = H264_AAC_STREAM;  //改变状态
				
			int ret = WaitForSingleObject(m_hH264AACBufferEvent, m_iWaitTimeOut);
			if (WAIT_OBJECT_0 == ret) {
				//...成功
				Log(TEXT("LINE : %d, FUNC : %s ,m_hH264AACBufferEvent 返回This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
				HASSTOP
				ResetEvent(m_hH264AACBufferEvent);
				if (STOP_STREAM == m_StatusStream) {     //外部置重连
					SetEvent(m_hUpdataEvent);//重连
					Log(TEXT("LINE : %d, FUNC : %s ,m_hH264AACBufferEvent 外部设置,This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
					continue;
				}
			}
			else if (WAIT_TIMEOUT == ret){
				HASSTOP
				Log(TEXT("LINE : %d, FUNC : %s ,m_hH264AACBufferEvent 超时,This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
				ResetEvent(m_hH264AACBufferEvent);
				m_StatusStream = STOP_STREAM;
				SetEvent(m_hUpdataEvent);//重连
				continue;
			}
			
			//----------------------------------------------------------------------------------------
			//--------------------------------------------------------------------------------------
			//YUV缓冲
			if (m_pMPMediaInfo.has_video) {
				
				for (int iIndex = 0; iIndex < m_pMPMediaInfo.v_frame_rate; iIndex++) {
					mp_get_frame(HMediaProcess, 1, m_fVideoWarnning);
					//Log(TEXT("LINE : %d, FUNC : %s ,m_hYUVBuffer mp_get_frame,This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
					HASSTOP
				}
				m_StatusStream = YUV_STREAM;
				int ret = WaitForSingleObject(m_hYUVBufferEvent, m_iWaitTimeOut);
				if (WAIT_OBJECT_0 == ret) {
					Log(TEXT("LINE : %d, FUNC : %s ,m_hYUVBufferEvent 缓冲成功This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
					HASSTOP
					ResetEvent(m_hYUVBufferEvent);
					if (STOP_STREAM == m_StatusStream) {     //外部置重连
						Log(TEXT("LINE : %d, FUNC : %s ,m_hYUVBufferEvent 外部重置This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
						SetEvent(m_hUpdataEvent);//重连
						continue;
					}
					
				} else if (WAIT_TIMEOUT == ret){
						HASSTOP
						Log(TEXT("LINE : %d, FUNC : %s ,m_hYUVBufferEvent 超时This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
						ResetEvent(m_hYUVBufferEvent);
						m_StatusStream = STOP_STREAM;
						SetEvent(m_hUpdataEvent);//重连
						continue;
						
					}	
			}
			//----------------------------------------------------------------------------------------
			//--------------------------------------------------------------------------------------
			//PCM缓冲

			if (m_pMPMediaInfo.has_audio) {

				m_iPcmBufferNumber = m_pMPMediaInfo.a_channels*m_pMPMediaInfo.a_sample_rate * 2 / 4096;
				if (m_iPcmBufferNumber < 20)
				{
					m_iPcmBufferNumber = 40;
				}
				for (int iIndex = 0; iIndex </*暂时先这样估算*/m_iPcmBufferNumber/*m_pMPMediaInfo.a_channels*m_pMPMediaInfo.a_sample_rate * 2 / 4096*/; iIndex++) {
					mp_get_frame(HMediaProcess, 2, m_fAudioWarnning);//音频
					//Log(TEXT("LINE : %d, FUNC : %s ,m_hPCMBuffer mp_get_frameThis =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
				}
				m_StatusStream = PCM_STREAM;
				int ret = WaitForSingleObject(m_hPCMBufferEvent, m_iWaitTimeOut);
				if (WAIT_OBJECT_0 == ret) {
					Log(TEXT("LINE : %d, FUNC : %s ,m_hPCMBufferEvent 缓冲成功This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
					HASSTOP
					ResetEvent(m_hPCMBufferEvent);
					if (STOP_STREAM == m_StatusStream) {     //外部置重连
						Log(TEXT("LINE : %d, FUNC : %s ,m_hPCMBufferEvent 外部重连This =0x%p"), __LINE__, String(__FUNCTION__).Array(),this);
						SetEvent(m_hUpdataEvent);//重连
						continue;
					}
				} else if (WAIT_TIMEOUT == ret){
						Log(TEXT("LINE : %d, FUNC : %s ,m_hPCMBufferEvent 超时"), __LINE__, String(__FUNCTION__).Array());
						HASSTOP
						ResetEvent(m_hPCMBufferEvent);
						m_StatusStream = STOP_STREAM;
						SetEvent(m_hUpdataEvent);//重连
						continue;
				}
				
			}

			SetEvent(m_hRenderSleepEvent);

			EnterCriticalSection(&TextureDataLock);
			
			if (Captureing)
			{
				ChangeShader();

				if (texture)
				{
					delete texture;
					texture = nullptr;
				}
				if (!texture) {
					texture = D3DRender->CreateTexture(CallBackWidth, CallBackHight, GS_RGBA, nullptr, FALSE, FALSE);
				}
				m_bReadyDraw = false;

			}
			LeaveCriticalSection(&TextureDataLock);
			m_pos = 0;
			m_StatusStream = PLAY_STREAM;

			if (!m_SyncThread)
			{
				m_bAllowClose = false; //第一次不需要
				m_SyncThread = OSCreateThread((XTHREAD)VideoLiveSource::SynchronizationThread, this);
			}
			if (m_bAllowClose)
			{
				SetEvent(m_hPauseThreadEvent);
			}
			
			continue; //所有条件都已经成功,等待触发重连事件
		}
	}
	if (m_bAllowClose)
	{
		SetEvent(m_hPauseThreadEvent);
	}
	if (HMediaProcess)
	{
		Log(TEXT("LINE : %d, FUNC : %s   Close FFmpeg SDK! Thread = %d,This = 0x%p"), __LINE__, String(__FUNCTION__).Array(), m_MonitorThreadID, this);
		if (URLState){
			mp_close(HMediaProcess);
		}
		mp_destroy(HMediaProcess);
		HMediaProcess = NULL;
	}
	tbb::concurrent_queue<CSampleData*>::iterator iterYUV = m_YUVBuffer.unsafe_begin();
	for (; iterYUV != m_YUVBuffer.unsafe_end(); iterYUV++) {
		(*iterYUV)->Release();
	}
	m_YUVBuffer.clear();
	tbb::concurrent_queue<CSampleData*>::iterator iterPCM = m_PCMuffer.unsafe_begin();
	for (; iterPCM != m_PCMuffer.unsafe_end(); iterPCM++) {
		(*iterPCM)->Release();
	}
	m_PCMuffer.clear();
	Log(TEXT("MonitorThread Exit! Thread = %d,This = 0x%p"), m_MonitorThreadID, this);
		
	
}

String VideoLiveSource::ChooseShader(bool bNeedField)
{
	if (colorType == DeviceOutputType_RGB)
		return String();

	String strShader;
	strShader << SHADER_PATH;

	if (bNeedField)
		strShader << L"Field_";

	if (colorType == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
	else if (colorType == DeviceOutputType_YV12)
		strShader << TEXT("YVUToRGB.pShader");
	else if (colorType == DeviceOutputType_YVYU)
		strShader << TEXT("YVXUToRGB.pShader");
	else if (colorType == DeviceOutputType_YUY2)
		strShader << TEXT("YUXVToRGB.pShader");
	else if (colorType == DeviceOutputType_UYVY)
		strShader << TEXT("UYVToRGB.pShader");
	else if (colorType == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
		strShader.Clear();

	return strShader;
}

void VideoLiveSource::AudioPlayBack(const void *samples, unsigned int count, int64_t pts)
{
}

void VideoLiveSource::SetHasPreProcess(bool bPre)
{
	bHasPreProcess = bPre;
}

bool VideoLiveSource::GetHasPreProcess() const
{
	return bHasPreProcess;
}


void VideoLiveSource::RegisterDataCallBack(void *Context, DataCallBack pCb)
{
	__DataCallBack DataBack;
	DataBack.Context = Context;
	DataBack.CallBack = pCb;
	EnterCriticalSection(&CallBackLock);
	m_ListCallBack.Add(DataBack);
	LeaveCriticalSection(&CallBackLock);
}

void VideoLiveSource::UnRegisterDataCallBack(void *Context)
{
	EnterCriticalSection(&CallBackLock);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		if (OneCallBack.Context == Context)
		{
			m_ListCallBack.Remove(i);
			break;
		}
	}
	LeaveCriticalSection(&CallBackLock);
}

void VideoLiveSource::SetCanEnterScene(bool bCanEnter)
{
	m_bEnterScene = bCanEnter;
}

bool VideoLiveSource::CanEnterScene() const
{
	return m_bEnterScene;
}

const char* VideoLiveSource::GetAduioClassName() const
{
	return "VideoLiveSource";
}

IBaseAudio * VideoLiveSource::GetAudioRender()
{
	return m_pDemandMediaAudio;
}

bool VideoLiveSource::IsFieldSignal() const
{
	return bIsFieldSignal;
}

bool VideoLiveSource::GetStreamPos(UINT& Pos)
{
	Pos = m_pos;

	if (Pos == 0 && m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();
		
	return true;
}

void VideoLiveSource::ChangeShader()
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
}

void VideoLiveSource::PlayCallBackAudio(LPBYTE lpData, UINT len)
{
	CSampleData Audio;
	Audio.bAudio = true;
	Audio.lpData = lpData;
	Audio.dataLength = len;
	WAVEFORMATEX FormatAudio = audioFormat;

	if (FormatAudio.nChannels > 2)
	{
		FormatAudio.nChannels = 2;
		FormatAudio.wBitsPerSample = audioFormat.wBitsPerSample * 2;
	}

	EnterCriticalSection(&CallBackLock);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		Audio.pAudioFormat = (void*)&FormatAudio;
		if (OneCallBack.CallBack)
			OneCallBack.CallBack(OneCallBack.Context, &Audio);
	}
	LeaveCriticalSection(&CallBackLock);

	Audio.lpData = NULL;//不是动态申请的置NULL
}
