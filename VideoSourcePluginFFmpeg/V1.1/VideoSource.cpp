/**
* John Bradley (jrb@turrettech.com)
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

VideoSource::VideoSource(Value& data)
{
    Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,Using Demand Video Source, This = %x"),__LINE__,__FUNCTION__,this);
	
	HMediaProcess = mp_create(0); //create synchronization Interface!

	m_height = 0;
	m_width = 0;
	CallBackWidth = 0;
	CallBackHight = 0;
	m_mediaDuration = 0;
	enteredSceneCount = 0;
	memset(m_playPath, 0, 256);
	texture = NULL;
	audioSample = NULL;
	latestVideoSample = NULL;
	Captureing = false;
	colorConvertShader = NULL;
	D3DRender = GetD3DRender();
	m_MediaState = MediaStop;
    config = new VideoSourceConfig(data);
	InitializeCriticalSection(&DataLock);
	InitializeCriticalSection(&AudioDataLock);
	InitializeCriticalSection(&TextureDataLock);
	InitializeCriticalSection(&CallBackLock);
	m_pDemandMediaAudio = new CDemandMediaAudio;
	RGBFieldShader = NULL;
	colorFieldConvertShader = NULL;
}

VideoSource::VideoSource()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d, FUNC:%s ,Using Demand Video Source, This = %x"), __LINE__, __FUNCTION__, this);

	HMediaProcess = mp_create(0); //create synchronization Interface!

	m_height = 0;
	m_width = 0;
	CallBackWidth = 0;
	CallBackHight = 0;
	m_mediaDuration = 0;
	enteredSceneCount = 0;
	videoSize = Vect2(640, 360);
	texture = NULL;
	audioSample = NULL;
	latestVideoSample = NULL;
	colorConvertShader = NULL;
	colorFieldConvertShader = NULL;
	memset(m_playPath, 0, 256);
	Captureing = false;
	D3DRender = GetD3DRender();
	m_MediaState = MediaStop;

	InitializeCriticalSection(&DataLock);
	InitializeCriticalSection(&AudioDataLock);
	InitializeCriticalSection(&TextureDataLock);
	InitializeCriticalSection(&CallBackLock);
	m_pDemandMediaAudio = new CDemandMediaAudio;
	Deinterlacer = NULL;//new CDeinterlacer;
	RGBFieldShader = D3DRender->CreatePixelShaderFromFile(L"shaders/Field_RGB.pShader");
}

bool VideoSource::Init(Value &JsonParam)
{
	config = new VideoSourceConfig(JsonParam);
	UpdateSettings(JsonParam);

	DeinterConfig.processor = DEINTERLACING_PROCESSOR_GPU;
	DeinterConfig.doublesFramerate = false;
	DeinterConfig.fieldOrder = FIELD_ORDER_TFF | FIELD_ORDER_BFF;
	DeinterConfig.type = DEINTERLACING_YADIF;

	return true;
}

VideoSource::~VideoSource()
{ 
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s  VideoSource Demand Destructor,This = %x"), __LINE__, __FUNCTION__,this);
	m_stop = true;
	if (HMediaProcess)
	{
		mp_close(HMediaProcess);
		HMediaProcess = NULL;
	}

	if (m_hCloseSyncThreadEvent)
	{
		WaitForSingleObject(m_hCloseSyncThreadEvent, INFINITE);
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s VideoSource Release SyncThread Success! This = %x"), __LINE__, __FUNCTION__,this);
		}
	}

	if (m_hCloseSyncThreadEvent)
	{
		CloseHandle(m_hCloseSyncThreadEvent);
	}

	if (timeThread)
	{
		OSCloseThread(timeThread);
	}
	
	if (m_pDemandMediaAudio)
		delete m_pDemandMediaAudio;

    if (texture) {
        delete texture;
        texture = nullptr;
    }
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		inf->Release();
	}
	m_AudioAACBuffer.clear();

    delete config;
    config = nullptr;
	DeleteCriticalSection(&DataLock);
	DeleteCriticalSection(&AudioDataLock);
	DeleteCriticalSection(&TextureDataLock);
	DeleteCriticalSection(&CallBackLock);
	DestoryCSampleData();

	if (Deinterlacer)
		delete Deinterlacer;

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

unsigned VideoSource::VideoFormatCallback(
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
    
    if (!config->isStretching) {
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


void VideoSource::VideoFormatCleanup()
{
}

void VideoSource::Tick(float fSeconds)
{}

void VideoSource::GlobalSourceEnterScene()
{
	if (enteredSceneCount++)
		return;
	{
		if (config && config->isSenceChangeStartPlay)
		{
			ChangeReset();
		}
		mp_start(HMediaProcess);
		m_bPlay = true;
		m_MediaState = MediaPalying;
		m_bNeedUpdate = true;

		AudioParam Param;
		Param.iBitPerSample = audioFormat.wBitsPerSample;
		Param.iChannel = audioFormat.nChannels;
		Param.iSamplesPerSec = audioFormat.nSamplesPerSec;
		EnterLiveVideoSection();
		if (Param.iBitPerSample > 0)
			m_pDemandMediaAudio->ResetAudioParam(Param);
		LeaveLiveVideoSection();
	}
}

void VideoSource::GlobalSourceLeaveScene()
{
	if (!enteredSceneCount)
		return;
	if (--enteredSceneCount)
		return;
	if (m_bPlay)
	{
		mp_stop(HMediaProcess);//切换特效时会卡
		m_bPlay = false;
		m_bNeedUpdate = true;
		m_MediaState = MediaStop;
	}
}

void VideoSource::Preprocess()
{
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
// 		if (Deinterlacer)
// 		{
// 			if (lastSample->cx == CallBackWidth && lastSample->cy == CallBackHight)
// 				Deinterlacer->SetImage(lastSample->lpData, &DeinterConfig, CallBackWidth, CallBackHight, colorType);
// 		}
// 		else
// 		{
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

			if (Deinterlacer)
			{
				if (lastSample->cx == CallBackWidth && lastSample->cy == CallBackHight)
					Deinterlacer->SetImage(texture, &DeinterConfig, CallBackWidth, CallBackHight, colorType);
			}
//		}
		//uint64_t uCurrentCheckNum = *(uint64_t *)(lastSample->lpData - 24);
		UINT64 nHigeCheckNum = lastSample->CheckNum & 0xFF00000000000000;
		if (nHigeCheckNum != 0) //最高字节不为0
		{
			lastSample->Release();
		}
		else
		{
			//Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 释放lastSample之前，校验值不匹配: uCurrentCheckNum = 0x%x,lastSample->CheckNum =0x%x.lastSample->lpData :0x%x.lastSample :0x%x.ThreadID = %d,This = 0x%x"), 
			//	__LINE__, __FUNCTION__, uCurrentCheckNum, lastSample->CheckNum, lastSample->lpData, lastSample,GetCurrentThreadId(), this);

			//UINT64 nHigeCheckNum = lastSample->CheckNum & 0xFF00000000000000;
			//UINT64 nHigeCurrrntNum = uCurrentCheckNum & 0xFF00000000000000;

			//if (nHigeCheckNum == nHigeCurrrntNum)//最高字节匹配
			//{
			//	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 最高位校验值匹配释放: uCurrentCheckNum = 0x%p,lastSample->CheckNum =0x%p.nHigeCurrrntNum = 0x%p.nHigeCheckNum = 0x%p.lastSample->lpData :0x%p.lastSample :0x%p.ThreadID = %d,This = 0x%p"),
			//		__LINE__, __FUNCTION__, uCurrentCheckNum, lastSample->CheckNum, nHigeCurrrntNum, nHigeCheckNum, lastSample->lpData, lastSample, GetCurrentThreadId(), this);
			//	lastSample->Release();
			//}
			//else
			//{
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 最高位校验值为0，不释放: uCurrentCheckNum = 0x%p,lastSample->CheckNum =0x%p.nHigeCurrrntNum = 0x%p.nHigeCheckNum = 0x%p.lastSample->lpData :0x%p.lastSample :0x%p.ThreadID = %d,This = 0x%p"),
					__LINE__, __FUNCTION__, 0, lastSample->CheckNum, 0, nHigeCheckNum, lastSample->lpData, lastSample, GetCurrentThreadId(), this);
			//}
		}
	}
	LeaveCriticalSection(&TextureDataLock);
}

void VideoSource::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	if (!Captureing || !m_pMPMediaInfo.has_video)
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
	if (texture&&Captureing&&m_bReadyDraw) {

		if (Deinterlacer)
		{
			Deinterlacer->RenderTexture(Vect2(pos.x + mediaOffset.x, pos.y + mediaOffset.y), Vect2(pos.x + mediaSize.x, pos.y + mediaSize.y));
		}
		else
		{
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
				if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
				{
					Shader *oldShader = D3DRender->GetCurrentPixelShader();
					float fGamma = 1.0f;

					if (mediaSize == videoSize || !bIsFieldSignal)
					{
						if (colorConvertShader)
						{
							D3DRender->LoadPixelShader(colorConvertShader);
							colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), fGamma);
						}
					}
					else if (bIsFieldSignal && colorFieldConvertShader)
					{
						D3DRender->LoadPixelShader(colorFieldConvertShader);
						colorFieldConvertShader->SetFloat(colorFieldConvertShader->GetParameterByName(TEXT("gamma")), fGamma);
					}

					D3DRender->DrawSprite(texture, 0xFFFFFFFF, pos.x + mediaOffset.x, pos.y + mediaOffset.y, pos.x + mediaSize.x, pos.y + mediaSize.y);
					D3DRender->LoadPixelShader(oldShader);
				}
				else
				{
					D3DRender->DrawSprite(texture, 0xFFFFFFFF, pos.x + mediaOffset.x, pos.y + mediaOffset.y, pos.x + mediaSize.x, pos.y + mediaSize.y);
				}
			}
			
		}
		
    }
	LeaveCriticalSection(&TextureDataLock);
}

const char* VideoSource::GetAduioClassName() const
{
	return "VideoSource";
}

void VideoSource::FrameCallBackFunc(void* frame, int frame_type, const void* ctx, int status)
{
	VideoSource * This_ = (VideoSource *)(ctx);
	if (status != 0)
	{
		EnterCriticalSection(&(This_->DataLock));
		if (!This_->m_bFirstVideo)
		{
			This_->m_bFirstVideo = true;
		}
		LeaveCriticalSection(&(This_->DataLock));

		if (-1 == status)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, Decoder Retrun error ! ThreadID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), This_);
			//This_->m_bFileIsError = true;
		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, Current File Data is Over! ThreadID = %d, This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), This_);
		}
		mp_release_frame(&frame);
		return;
	}
	if (frame_type == 2) //MPFrameInfo类型
	{
		MPFrameInfo * pMPFrameInfo = reinterpret_cast<MPFrameInfo *>(frame);
		This_->fileLoopPlayUseTsp = pMPFrameInfo->pts - This_->m_bFirstTsTimeStamp;
		if (pMPFrameInfo->media_type == 1)//视频
		{
			if (This_->m_width != pMPFrameInfo->width || This_->m_height != pMPFrameInfo->height)
			{
				mp_release_frame(&frame);
				return;
			}

			if (This_->m_VideoYuvBuffer.size() > 100)
			{
				mp_release_frame(&frame);
				return;
			}
			CSampleData* videoSample = new CSampleData;
			if (!videoSample)
				return;
			videoSample->bAudio = false;
			videoSample->cx = This_->CallBackWidth;
			videoSample->cy = This_->CallBackHight;

			if (MP_PIX_FMT_YUV420P == This_->m_pMPMediaInfo.v_pix_fmt)
			{
				
				videoSample->dataLength = This_->CallBackWidth*This_->CallBackHight * 3 / 2;
				videoSample->lpData = (LPBYTE)Allocate_Bak(videoSample->dataLength);//pointer; //
				if (!videoSample->lpData)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 申请内存失败，丢弃当前帧 ThreadID = %d, This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), This_);
					mp_release_frame(&frame);
					videoSample->Release();
					return;
				}
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
									if (!pMPFrameInfo->frame_data[0])
									{
										videoSample->Release();
										mp_release_frame(&frame);
										return;
									}
									memcpy(videoSample->lpData + a, pMPFrameInfo->frame_data[0] + i * pMPFrameInfo->frame_size[0], This_->m_width);
									a += This_->m_width;
								}
								for (i = 0; i < This_->m_height / 2; i++)
								{
									if (!pMPFrameInfo->frame_data[1])
									{
										videoSample->Release();
										mp_release_frame(&frame);
										return;
									}
									memcpy(videoSample->lpData + a, pMPFrameInfo->frame_data[1] + i * pMPFrameInfo->frame_size[1], This_->m_width / 2);
									a += This_->m_width / 2;
								}
								for (i = 0; i < This_->m_height / 2; i++)
								{
									if (!pMPFrameInfo->frame_data[2])
									{
										videoSample->Release();
										mp_release_frame(&frame);
										return;
									}
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
			}
			else if (MP_PIX_FMT_BGRA == This_->m_pMPMediaInfo.v_pix_fmt)  //RGBA测试
			{
				videoSample->dataLength = This_->CallBackWidth*This_->CallBackHight * 4;
				videoSample->lpData = (LPBYTE)Allocate_Bak(videoSample->dataLength);//pointer; //
				videoSample->colorType = ColorType_RGBA32REVERSE;
				if (!pMPFrameInfo->frame_data[0])
				{
					videoSample->Release();
					mp_release_frame(&frame);
					return;
				}
				memmove(videoSample->lpData, pMPFrameInfo->frame_data[0], pMPFrameInfo->frame_size[0] * pMPFrameInfo->height);
			}
			else if (MP_PIX_FMT_RGB24 == This_->m_pMPMediaInfo.v_pix_fmt)
			{
				videoSample->dataLength = This_->CallBackWidth*This_->CallBackHight * 3;

				videoSample->dataLength = This_->CallBackWidth*This_->CallBackHight * 3;
				videoSample->lpData = (LPBYTE)Allocate_Bak(videoSample->dataLength);//pointer; //
				videoSample->colorType = ColorType_RGB24;
				if (!pMPFrameInfo->frame_data[0])
				{
					videoSample->Release();
					mp_release_frame(&frame);
					return;
				}
				memmove(videoSample->lpData, pMPFrameInfo->frame_data[0], pMPFrameInfo->frame_size[0] * pMPFrameInfo->height);
			}

			EnterCriticalSection(&(This_->DataLock));
			if (-1 == This_->m_bFirstTsTimeStamp)
			{
				This_->m_bFirstTsTimeStamp = pMPFrameInfo->pts;
			}
			This_->m_pMPMediaInfo.has_audio ? true : This_->m_pts = pMPFrameInfo->pts - This_->m_bFirstTsTimeStamp;
			videoSample->timestamp = pMPFrameInfo->pts;
			videoSample->CheckNum = *(uint64_t*)(videoSample->lpData - 24);
			This_->m_VideoYuvBuffer.push_back(videoSample);
			if (!This_->m_bFirstVideo)
			{
				This_->m_bFirstVideo = true;
			}
			if (This_->m_bChangePos)
			{
				This_->m_bChangePos = false;
				This_->m_ChangePosPts = pMPFrameInfo->pts;
			}
			This_->m_pts = pMPFrameInfo->pts - This_->m_bFirstTsTimeStamp;
			
			LeaveCriticalSection(&(This_->DataLock));
		}
		else if (pMPFrameInfo->media_type == 2)//音频
		{
			
			if (-1 == This_->m_bFirstTsTimeStamp)
			{
				This_->m_bFirstTsTimeStamp = pMPFrameInfo->pts;
			}
			
			EnterCriticalSection(&(This_->DataLock));
			if (!This_->m_pMPMediaInfo.has_video)
			{
				//VolumeCaculate(pMPFrameInfo->frame_data[0], pMPFrameInfo->frame_size[0], This_->m_iVolume / 100.0);
				This_->m_pts = pMPFrameInfo->pts - This_->m_bFirstTsTimeStamp;
				
				CSampleData* audioSample = new CSampleData;
				if (!audioSample)
				{
					LeaveCriticalSection(&(This_->DataLock));
					return;
				}
				audioSample->bAudio = true;
				audioSample->dataLength = pMPFrameInfo->frame_size[0];
				audioSample->lpData = (LPBYTE)Allocate_Bak(audioSample->dataLength);//pointer; //
				memcpy(audioSample->lpData, LPBYTE(pMPFrameInfo->frame_data[0]), pMPFrameInfo->frame_size[0]);
				audioSample->timestamp = pMPFrameInfo->pts;
				EnterCriticalSection(&This_->AudioDataLock);
				This_->m_AudioAACBuffer.push_back(audioSample);
				LeaveCriticalSection(&This_->AudioDataLock);
				if (!This_->m_bFirstVideo)
				{
					This_->m_bFirstVideo = true;
				}
				LeaveCriticalSection(&(This_->DataLock));
			}
			else
			{
				if (This_->m_bChangePos)
				{
					LeaveCriticalSection(&(This_->DataLock));
					mp_release_frame(&frame);
					return;
				}
				if (This_->m_ChangePosPts > pMPFrameInfo->pts)
				{
					LeaveCriticalSection(&(This_->DataLock));
					mp_release_frame(&frame);
					return;
				}
				//VolumeCaculate(pMPFrameInfo->frame_data[0], pMPFrameInfo->frame_size[0], This_->m_iVolume / 100.0);
				//This_->m_pts = pMPFrameInfo->pts - This_->m_bFirstTsTimeStamp;
				CSampleData* audioSample = new CSampleData;

				if (!audioSample)
				{
					LeaveCriticalSection(&(This_->DataLock));
					return;
				}

				audioSample->bAudio = true;
				audioSample->dataLength = pMPFrameInfo->frame_size[0];
				audioSample->lpData = (LPBYTE)Allocate_Bak(audioSample->dataLength);//pointer; //
				memcpy(audioSample->lpData, LPBYTE(pMPFrameInfo->frame_data[0]), pMPFrameInfo->frame_size[0]);
				audioSample->timestamp = pMPFrameInfo->pts;
				EnterCriticalSection(&This_->AudioDataLock);
				This_->m_AudioAACBuffer.push_back(audioSample);
				LeaveCriticalSection(&This_->AudioDataLock);

				LeaveCriticalSection(&(This_->DataLock));
			}
			
		}
	}
	mp_release_frame(&frame);
}

void VideoSource::UpdateSettings(Value &JsonParam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,UpdateSettings Execute！ Open New File! This = %x"), __LINE__, __FUNCTION__,this);
	fileLoopPlayUseTsp = 0;
	bool bisSame = false;
	if (!JsonParam["DeskTopSetting"].isNull())
	{
		UINT Index = JsonParam["CurrentIndex"].asUInt();
		if (Index >= config->playlist.Num())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Index %d is too big,config->playlist.Num is %d", __FUNCTION__, Index, config->playlist.Num());
			Index = 0;
		}
		
		config->element["CurrentIndex"] = Index;
		config->CurrentIndex = Index;
		config->isHaveSelect = true;
		Log::writeMessage(LOG_RTSPSERV, 1, "%s DeskTopSetting config->playlist.Num is %d", __FUNCTION__, config->playlist.Num());
	}
	else
	{
		config->Reload();
	}
	
	//m_pDemandMediaAudio->SetVolumef(JsonParam["volume"].asDouble() / 100);
	if (config->playlist.Num() == 0)
	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
		//m_MediaState = MediaStop;
		//SetMediaState(m_MediaState, true);//强制更新
		ColseFile();
		memset(m_playPath, 0, 256);
		AudioParam Param;
		Param.iBitPerSample = 16;
		Param.iChannel = 2;
		Param.iSamplesPerSec = 44100;
		EnterLiveVideoSection();
		m_pDemandMediaAudio->ResetAudioParam(Param);
		LeaveLiveVideoSection();
		m_mediaDuration = 0;
		return;
	}
	m_iVolume = config->volume;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Set Volume = %d! This =%x"), __LINE__, __FUNCTION__, m_iVolume,this);
	if (!config->isHaveSelect)
	{
		for (int iIndex = 0; iIndex < config->playlist.Num(); iIndex++)
		{
			String &mediaEntry = config->playlist[iIndex];
			String token = mediaEntry.GetToken(1, L':');
			bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
			if (!isStream && !OSFileExists(config->playlist[iIndex])) // 文件不存在
			{
// 				String info(config->playlist[iIndex]);
// 				info += L" file do not exist";
// 				AddCreateInfo(info);
			}
			if (!strcmp(WcharToAnsi(config->playlist[iIndex].Array()).c_str(), m_playPath))
			{
				config->CurrentIndex = iIndex;
				config->Save();
				config->Reload();
				break;
			}
			
		}
	}
	unsigned int i = config->CurrentIndex;
	if (strcmp(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str(), m_playPath)) //不是当前播放文件
	{
		if (i < config->playlist.Num())
		{
			String &mediaEntry = config->playlist[i];
			String token = mediaEntry.GetToken(1, L':');
			bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
			if (!isStream && !OSFileExists(config->playlist[i])) // 文件不存在
			{
				String info(config->playlist[i]);
				info += L" file do not exist";
				//AddCreateInfo(info);
			}
			memset(m_playPath, 0, 256);
			memcpy(m_playPath, WcharToAnsi(config->playlist[i].Array()).c_str(), strlen(WcharToAnsi(config->playlist[i].Array()).c_str()));
		}
	}
	else
	{
		bisSame = true;
	}

	if (i < config->playlist.Num() - 1)
	{
		m_bisLastFile = false;
	}
	if (i == config->playlist.Num() - 1)  //最后一个文件,向后禁止切换
	{
		m_bisLastFile = true;
	}

	if (bIsFieldSignal != config->bIsScanInterlace)
	{
		bIsFieldSignal = config->bIsScanInterlace;
		EnterCriticalSection(&TextureDataLock);
		ChangeShader();
		LeaveCriticalSection(&TextureDataLock);
	}
	
	if (bisSame)
	{
		return;
	}
		
	char url[256] = { 0 };
	if (m_bFirstLoadFile)  //如果第一次加载文件是可以从配置文件读取暂停位置的，如果不是第一次加载文件，并且没有在场景中那么暂停在第一帧
	{
		m_bFirstLoadFile = false;
		m_pos = 0;
	}
	else
	{
		m_pos = 0;
	}
	ColseFile();
	sprintf_s(url, "-re -f YUV -ss  %d", m_pos);
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	EnterCriticalSection(&DataLock);
	SafeRelease(latestVideoSample);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		LeaveCriticalSection(&DataLock);
		OpenFileFailed();
		return;
	}
	
	if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
	{
		colorType = DeviceOutputType_I420;
	}
	else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
	{
		colorType = DeviceOutputType_RGB;
	}
	else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
	{
		colorType = DeviceOutputType_RGB24;
	}
	
	m_mediaDuration = m_pMPMediaInfo.stream_duration;
	if (!m_pMPMediaInfo.v_frame_rate)   //SDK maybe get v_frame_rate 0
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,SDK FPS return 0,Manual Set 25, Thread = %d ,This = 0x%p"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		m_pMPMediaInfo.v_frame_rate = 25;
	}
	if (m_iFPS != m_pMPMediaInfo.v_frame_rate) {
		m_iFPS = m_pMPMediaInfo.v_frame_rate;
		m_bVframeisChange = true;
	}
	audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	audioFormat.nChannels = m_pMPMediaInfo.a_channels;
	audioFormat.wBitsPerSample = 16;
	m_bVframeisChange = true;
	m_height = m_pMPMediaInfo.v_height;
	m_width = m_pMPMediaInfo.v_width;
	CallBackHight = m_height;
	CallBackWidth = m_width;
	videoSize.x = m_pMPMediaInfo.v_width;
	videoSize.y = m_pMPMediaInfo.v_height;
	InitCSampleData();
	m_bPlay = true;
	m_ChangePosPts = 0;
	m_bFirstVideo = false;
	m_bFirstTsTimeStamp = -1;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);

	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

	m_FrameWidth = m_width;
	m_FrameHeight = m_height;
	m_FrameLines = 0;
	m_FramePitchs = 0;
	m_pts = 0;
	VideoFormatCallback(m_choma, &m_FrameWidth, &m_FrameHeight, &m_FramePitchs, &m_FrameLines);

	ret = mp_start(HMediaProcess);
	if (ret != 0)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Start File Failed！ Path: %s,This = %x"), __LINE__, __FUNCTION__, m_playPath, this);
		LeaveCriticalSection(&DataLock);
		return;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Path: %s,frame_rate =%d,nSamplesPerSec = %d,nChannels = %d,wBitsPerSample = 16,This = %x"), 
		__LINE__, __FUNCTION__, m_playPath, m_pMPMediaInfo.v_frame_rate, m_pMPMediaInfo.a_sample_rate, m_pMPMediaInfo.a_channels,this);
	
	if (!m_hCloseSyncThreadEvent)
	{
		m_hCloseSyncThreadEvent = CreateEvent(NULL, true, false, NULL);
	}

	if (!timeThread)
	{
		timeThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TickThread, this, 0, NULL);
	}
	LeaveCriticalSection(&DataLock);

	AudioParam Param;
	Param.iBitPerSample = audioFormat.wBitsPerSample;
	Param.iChannel = audioFormat.nChannels;
	Param.iSamplesPerSec = audioFormat.nSamplesPerSec;
	EnterLiveVideoSection();
	if (Param.iBitPerSample > 0)
		m_pDemandMediaAudio->ResetAudioParam(Param);
	LeaveLiveVideoSection();

	EnterCriticalSection(&TextureDataLock);

	if (Captureing)
	{
		ChangeShader();

		if (texture)
		{
			//ClearTexture();
			delete texture;
			texture = nullptr;
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
		m_bReadyDraw = false;
	}

	ChangeShader();

	LeaveCriticalSection(&TextureDataLock);
	m_bNeedUpdate = true;
	bInitSuc = true;
	if (m_MediaState == MediaStop)
	{
		while (true)
		{
			Sleep(100);
			if (m_bFirstVideo){
				mp_stop(HMediaProcess);
				break;
			}
		}
	}
}

bool VideoSource::ChangePos()
{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath, this);
		fileLoopPlayUseTsp = 0;
		ColseFile();
		char url[256] = { 0 };
		if (m_pos == m_mediaDuration && m_pos > 3600 * 1000)
		{
			m_pos = m_pos + m_bFirstTsTimeStamp;
			sprintf_s(url, "-re -f YUV -ss  %d", m_pos -= 30000);  //重新定位的文件位置
		}
		else if (m_pos == m_mediaDuration && m_pos > 10000)
		{
			m_pos = m_pos + m_bFirstTsTimeStamp;
			sprintf_s(url, "-re -f YUV -ss  %d", m_pos -= 10000);
		}
		else
		{
			m_pos = m_pos + m_bFirstTsTimeStamp;
			sprintf_s(url, "-re -f YUV -ss  %d", m_pos);
		}
		memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
		mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
		EnterCriticalSection(&DataLock);
		int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
		if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
		{
			if (!ret)
				m_mediaDuration = 0;
			OpenFileFailed();
			LeaveCriticalSection(&DataLock);
			return 0;
		}
		m_bPlay = true;
		m_bclear = true;
		m_bChangePos = true;
		if (m_pos == 0)
		{
			m_bChangePos = false;
		}
		m_ChangePosPts = 0;
		for (CSampleData* inf : m_VideoYuvBuffer)
		{
			video_pts = 0;
			audio_pts = 0;
			inf->Release();
		}
		m_VideoYuvBuffer.clear();
		EnterCriticalSection(&AudioDataLock);
		for (CSampleData* inf : m_AudioAACBuffer)
		{
			video_pts = 0;
			audio_pts = 0;
			inf->Release();
		}
		m_AudioAACBuffer.clear();
		LeaveCriticalSection(&AudioDataLock);

		if (m_pDemandMediaAudio)
			m_pDemandMediaAudio->ResetAudioDB();

		mp_start(HMediaProcess);    //开始获取数据
		LeaveCriticalSection(&DataLock);
		
		if (m_MediaState == MediaStop)
		{
			m_bFirstVideo = false;
			while (true)
			{
				Sleep(100);
				if (m_bFirstVideo){
					mp_stop(HMediaProcess);
					break;
				}
			}
		}
		return true;
}

bool VideoSource::ChangeStop()
{
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath, this);
	fileLoopPlayUseTsp = 0;
	if (HMediaProcess)            //关闭文件
	{
		if (m_bPlay)
		{
			ColseFile();
		}
		m_bPlay = false; 
		return true;
	}
	else
	{
		return false;
	}
	
}

bool VideoSource::ChangePlay()
{
	fileLoopPlayUseTsp = 0;
	if (!m_bisLastFileStop || config->isPlaylistLooping)
	{
		mp_start(HMediaProcess);    //开始获取数据
		m_bPlay = true;
		m_MediaState = MediaPalying;
		//SetMediaState(m_MediaState, true);//单纯的播放暂停文件，那么直接更新状态
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
	fileLoopPlayUseTsp = 0;
	if (config->playlist.Num() == 0)
	{
		return false;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath, this);
	ColseFile();

	char url[256] = { 0 };
	sprintf_s(url, "-re -f YUV -ss  %d", 0);  //重新定位的文件位置
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	EnterCriticalSection(&DataLock);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		OpenFileFailed();
		LeaveCriticalSection(&DataLock);
		return 0;
	}
	m_bPlay = true;
	m_bclear = true;
	m_ChangePosPts = 0;
	m_bChangePos = false;
	m_pts = 0;
	m_bFirstTsTimeStamp = -1;
	m_bFirstVideo = false;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);

	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

	mp_start(HMediaProcess);    //开始获取数据

	LeaveCriticalSection(&DataLock);
	
	while (true && !enteredSceneCount)
	{
		Sleep(100);
		if (m_bFirstVideo){
			mp_stop(HMediaProcess);
			m_bNeedUpdate = true;
			m_MediaState = MediaStop;
			break; 
		}
	}
	if (enteredSceneCount)
	{
		m_MediaState = MediaPalying;
	}
	else
	{
		m_MediaState = MediaStop;
	}

	//SetMediaState(m_MediaState, true);//强制更新
	return true;
	
}

bool VideoSource::ChangeNext()
{
	fileLoopPlayUseTsp = 0;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s, This = %x"), __LINE__, __FUNCTION__, m_playPath,this);
	ColseFile();

	//config->Reload();//去掉这个
	if (config->playlist.Num() == 0)  //全部删除文件
	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
// 		m_MediaState = MediaStop;
// 		SetMediaState(m_MediaState, true);//强制停止并更新
		memset(m_playPath, 0, 256);
		return false;
	}

	//unsigned int i = 0;
	if (config->isFileLoop && config->isPlaylistLooping)  //需要循环并且为单个文件循环
	{
		//继续播放当前文件
	}
	else if (config->isPlaylistLooping && config->isListLoop)   //需要循环并且为列表文件循环
	{
		char currentPath[256] = { 0 };
		/*for (; i < config->playlist.Num(); i++) */{
			
			String &mediaEntry = config->playlist[config->CurrentIndex];
			String token = mediaEntry.GetToken(1, L':');
			bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
			if (!isStream && !OSFileExists(config->playlist[config->CurrentIndex])) // 文件不存在
			{
// 				String info(config->playlist[config->CurrentIndex]);
// 				info += L" file do not exist";
// 				AddCreateInfo(info);
			}
			memset(currentPath, 0, 256);
			memcpy(currentPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str(), strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));

			int ret = strcmp(currentPath, m_playPath);
			if (ret == 0)
			{
				if (config->CurrentIndex == config->playlist.Num() - 1) //当前播放为最后一个文件，下次播放第一个
				{
					config->CurrentIndex = 0;
					String &mediaEntry = config->playlist[config->CurrentIndex];
					String token = mediaEntry.GetToken(1, L':');
					bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
					if (!isStream && !OSFileExists(config->playlist[config->CurrentIndex])) // 文件不存在
					{
// 						String info(config->playlist[config->CurrentIndex]);
// 						info += L" file do not exist";
// 						AddCreateInfo(info);
					}
					memset(m_playPath, 0, 256);
					memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));
				}
				else               //播放下一个文件
				{
					config->CurrentIndex = config->CurrentIndex + 1;
					String &mediaEntry = config->playlist[config->CurrentIndex];
					String token = mediaEntry.GetToken(1, L':');
					bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
					if (!isStream && !OSFileExists(config->playlist[config->CurrentIndex])) // 文件不存在
					{
// 						String info(config->playlist[config->CurrentIndex]);
// 						info += L" file do not exist";
// 						AddCreateInfo(info);
					}
					memset(m_playPath, 0, 256);
					memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));
					
				}
				
			}
		}
// 		SetPrevState(true);
// 		SetNextState(true);
// 		if (config->CurrentIndex == 0)  //通常是第一个文件开始播放,向前禁止切换
// 		{
// 			if (GetPrevState())
// 			{
// 				SetPrevState(false);
// 			}
// 		}
// 		if (config->CurrentIndex < config->playlist.Num() - 1)
// 		{
// 			m_bisLastFile = false;
// 		}
// 		if (config->CurrentIndex == config->playlist.Num() - 1)  //最后一个文件,向后禁止切换
// 		{
// 			m_bisLastFile = true;
// 			if (GetNextState())
// 			{
// 				SetNextState(false);
// 			}
// 		}
// 
// 		if (config->playlist.Num() == 1) //前后都禁止切换
// 		{
// 			if (GetPrevState())
// 			{
// 				SetPrevState(false);
// 			}
// 
// 			if (GetNextState())
// 			{
// 				SetNextState(false);
// 			}
// 		}
	}
	else if (!config->isPlaylistLooping)        //不循环播放，顺序播放所有文件，播完最后一个停止
	{
		char currentPath[256] = { 0 };
		{

			String &mediaEntry = config->playlist[config->CurrentIndex];
			String token = mediaEntry.GetToken(1, L':');
			bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
			if (!isStream && !OSFileExists(config->playlist[config->CurrentIndex])) // 文件不存在
			{
// 				String info(config->playlist[config->CurrentIndex]);
// 				info += L" file do not exist";
// 				AddCreateInfo(info);
			}
			memset(currentPath, 0, 256);
			memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));

			int ret = strcmp(currentPath, m_playPath);
			if (ret == 0)
			{
				if (config->CurrentIndex == config->playlist.Num() - 1) //当前播放为最后一个文件，不再播放直接返回
				{
					if (m_bisOpen)
					{
						m_bisOpen = false;
						goto Label;
					}
					else
					{
						m_MediaState = MediaStop;
						//SetMediaState(m_MediaState, true);//最后一个文件强制停止
						m_bisLastFileStop = true;
						return false;
					}
					
				}
				else               //播放下一个文件
				{
					config->CurrentIndex = config->CurrentIndex + 1;
					String &mediaEntry = config->playlist[config->CurrentIndex];
					String token = mediaEntry.GetToken(1, L':');
					bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
					if (!isStream && !OSFileExists(config->playlist[config->CurrentIndex])) // 文件不存在
					{
// 						String info(config->playlist[config->CurrentIndex]);
// 						info += L" file do not exist";
// 						AddCreateInfo(info);
					}
					memset(m_playPath, 0, 256);
					memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));
				
				}
			}
		}
// 		SetPrevState(true);
// 		SetNextState(true);
// 		if (config->CurrentIndex == 0)  //通常是第一个文件开始播放,向前禁止切换
// 		{
// 			if (GetPrevState())
// 			{
// 				SetPrevState(false);
// 			}
// 		}
// 		if (config->CurrentIndex < config->playlist.Num() - 1)
// 		{
// 			m_bisLastFile = false;
// 		}
// 		if (config->CurrentIndex == config->playlist.Num() - 1)  //最后一个文件,向后禁止切换
// 		{
// 			m_bisLastFile = true;
// 			if (GetNextState())
// 			{
// 				SetNextState(false);
// 			}
// 		}
// 
// 		if (config->playlist.Num() == 1) //前后都禁止切换
// 		{
// 			if (GetPrevState())
// 			{
// 				SetPrevState(false);
// 			}
// 
// 			if (GetNextState())
// 			{
// 				SetNextState(false);
// 			}
// 		}
	}

Label:
	config->Save();
	m_MediaState = MediaPalying;  //一定是播放
	//SetPlayIndex(config->CurrentIndex);
	//SetMediaState(m_MediaState, true);//强制更新


	char url[256] = { 0 };
	m_pos = 0;      //重新定位到开头
	sprintf_s(url, "-re -f YUV -ss  %d", m_pos);  
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	m_bisAutoNextFile = true;
	EnterCriticalSection(&DataLock);
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		OpenFileFailed();
		m_bisAutoNextFile = false;
		LeaveCriticalSection(&DataLock);
		return false;
	}
	m_pts = 0;
	m_bFirstTsTimeStamp = -1;
	m_bPlay = true;
	m_ChangePosPts = 0;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);
	mp_start(HMediaProcess);    //开始获取数据
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Path: %s,frame_rate =%d,nSamplesPerSec = %d,nChannels = %d,wBitsPerSample = 16,This = %x"),
		__LINE__, __FUNCTION__, m_playPath, m_pMPMediaInfo.v_frame_rate, m_pMPMediaInfo.a_sample_rate, m_pMPMediaInfo.a_channels, this);
	m_mediaDuration = m_pMPMediaInfo.stream_duration;
	if (!m_pMPMediaInfo.v_frame_rate)   //SDK maybe get v_frame_rate 0
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,SDK FPS return 0,Manual Set 25, Thread = %d ,This = 0x%p"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		m_pMPMediaInfo.v_frame_rate = 25;
	}
	if (m_iFPS != m_pMPMediaInfo.v_frame_rate) {
		m_iFPS = m_pMPMediaInfo.v_frame_rate;
		m_bVframeisChange = true;
	}
	audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	audioFormat.nChannels = m_pMPMediaInfo.a_channels;
	audioFormat.wBitsPerSample = 16;
	m_height = m_pMPMediaInfo.v_height;
	m_width = m_pMPMediaInfo.v_width;
	m_bVframeisChange = true;
	CallBackHight = m_height;
	CallBackWidth = m_width;
	videoSize.x = m_pMPMediaInfo.v_width;
	videoSize.y = m_pMPMediaInfo.v_height;
	InitCSampleData();
	

	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

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
		
	EnterCriticalSection(&TextureDataLock);
	if (Captureing)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_I420;
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB;
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB24;
		}

		ChangeShader();

		if (texture)
		{
			//ClearTexture();
			delete texture;
			texture = nullptr;
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
		m_bReadyDraw = false;
	}
	
	LeaveCriticalSection(&TextureDataLock);
	return true;
	
}

bool VideoSource::ChangeNext_API()
{
	//不可能是最后一个
	fileLoopPlayUseTsp = 0;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s"), __LINE__, __FUNCTION__, m_playPath);
	ColseFile();
	//config->Reload();//去掉这个
	
	//只要执行，那么config->CurrentIndex 一定小于config->playlist.NUM
	config->CurrentIndex++;
	memset(m_playPath, 0, 256);
	memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));
	config->Save();
// 	SetPrevState(true);
// 	SetNextState(true);
// 	if (config->CurrentIndex == 0)  //通常是第一个文件开始播放,向前禁止切换
// 	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 	}
// 	if (config->CurrentIndex < config->playlist.Num() - 1)
// 	{
// 		m_bisLastFile = false;
// 	}
// 	if (config->CurrentIndex == config->playlist.Num() - 1)  //最后一个文件,向后禁止切换
// 	{
// 		m_bisLastFile = true;
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
// 	}
// 
// 	if (config->playlist.Num() == 1) //前后都禁止切换
// 	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
// 	}
// 	SetPlayIndex(config->CurrentIndex);
// 	m_MediaState = GetCurrentState();
// 	SetMediaState(m_MediaState, true);//强制更新
	char url[256] = { 0 };
	m_pos = 0;      //重新定位到开头
	sprintf_s(url, "-re -f YUV -ss  %d", m_pos);
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	EnterCriticalSection(&DataLock);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		
		OpenFileFailed();
		LeaveCriticalSection(&DataLock);
		return false;
	}
	m_pts = 0;
	m_bFirstTsTimeStamp = -1;
	m_bPlay = true;
	m_ChangePosPts = 0;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);
	mp_start(HMediaProcess);    //开始获取数据
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Path: %s,frame_rate =%d,nSamplesPerSec = %d,nChannels = %d,wBitsPerSample = 16,This = %x"),
		__LINE__, __FUNCTION__, m_playPath, m_pMPMediaInfo.v_frame_rate, m_pMPMediaInfo.a_sample_rate, m_pMPMediaInfo.a_channels, this);
	m_mediaDuration = m_pMPMediaInfo.stream_duration;
	if (!m_pMPMediaInfo.v_frame_rate)   //SDK maybe get v_frame_rate 0
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,SDK FPS return 0,Manual Set 25, Thread = %d ,This = 0x%p"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		m_pMPMediaInfo.v_frame_rate = 25;
	}
	if (m_iFPS != m_pMPMediaInfo.v_frame_rate) {
		m_iFPS = m_pMPMediaInfo.v_frame_rate;
		m_bVframeisChange = true;
	}
	audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	audioFormat.nChannels = m_pMPMediaInfo.a_channels;
	audioFormat.wBitsPerSample = 16;
	m_height = m_pMPMediaInfo.v_height;
	m_width = m_pMPMediaInfo.v_width;
	videoSize.x = m_pMPMediaInfo.v_width;
	videoSize.y = m_pMPMediaInfo.v_height;
	m_bVframeisChange = true;
	CallBackHight = m_height;
	CallBackWidth = m_width;
	InitCSampleData();
	
	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

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

	EnterCriticalSection(&TextureDataLock);
	if (Captureing)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_I420;
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB;
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB24;
		}

		ChangeShader();

		if (texture)
		{
		//	ClearTexture();
			delete texture;
			texture = nullptr;
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
		m_bReadyDraw = false;
	}
	
	LeaveCriticalSection(&TextureDataLock);
	if (m_MediaState == MediaStop)
	{
		m_bFirstVideo = false;
		while (true)
		{
			Sleep(100);
			if (m_bFirstVideo){
				mp_stop(HMediaProcess);
				break;
			}
		}
	}
	
	return true;
}

void VideoSource::SetDirectPlay(const String DirectPlayFile)
{
	fileLoopPlayUseTsp = 0;
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s, FILE = %s"), __LINE__, __FUNCTION__, m_playPath);
	//config->Reload();去掉这个
	
	if (DirectPlayFile.Compare(config->playlist[config->CurrentIndex]))
		return;
	char currentPath[256] = { 0 };
	unsigned int i = 0;
	for (; i < config->playlist.Num(); i++) {
		if (DirectPlayFile.Compare(config->playlist[i]))
		{
			String &mediaEntry = config->playlist[i];
			String token = mediaEntry.GetToken(1, L':');
			bool isStream = token.Length() >= 2 && token[0] == L'/' && token[1] == L'/';
			if (!isStream && !OSFileExists(config->playlist[i])) // 文件不存在
			{
// 				String info(config->playlist[i]);
// 				info += L" file do not exist";
// 				AddCreateInfo(info);
			}
			memset(m_playPath, 0, 256);
			memcpy(m_playPath, WcharToAnsi(config->playlist[i].Array()).c_str() , strlen(WcharToAnsi(config->playlist[i].Array()).c_str()));
			config->CurrentIndex = i;
			break;
		}
	}
	
// 	SetPrevState(true);
// 	SetNextState(true);
// 	if (i == 0)  //通常是第一个文件开始播放,向前禁止切换
// 	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 	}
// 	if (i < config->playlist.Num() - 1)
// 	{
// 		m_bisLastFile = false;
// 	}
// 	if (i == config->playlist.Num() - 1)  //最后一个文件,向后禁止切换
// 	{
// 		m_bisLastFile = true;
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
// 	}
// 
// 	if (config->playlist.Num() == 1) //前后都禁止切换
// 	{
// 		if (GetPrevState())
// 		{
// 			SetPrevState(false);
// 		}
// 
// 		if (GetNextState())
// 		{
// 			SetNextState(false);
// 		}
// 	}
	config->Save();
	ColseFile();
	char url[256] = { 0 };
	m_pos = 0;      //重新定位到开头
	sprintf_s(url, "-re -f YUV -ss  %d", m_pos);
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	EnterCriticalSection(&DataLock);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		OpenFileFailed();
		LeaveCriticalSection(&DataLock);
		return;
	}
	m_bPlay = true;
	m_ChangePosPts = 0;
	m_pts = 0;
	m_bFirstTsTimeStamp = -1;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);
	mp_start(HMediaProcess);    //开始获取数据
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Path: %s,frame_rate =%d,nSamplesPerSec = %d,nChannels = %d,wBitsPerSample = 16,This = %x"),
		__LINE__, __FUNCTION__, m_playPath, m_pMPMediaInfo.v_frame_rate, m_pMPMediaInfo.a_sample_rate, m_pMPMediaInfo.a_channels, this);
	m_mediaDuration = m_pMPMediaInfo.stream_duration;
	if (!m_pMPMediaInfo.v_frame_rate)   //SDK maybe get v_frame_rate 0
	{
		Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,SDK FPS return 0,Manual Set 25, Thread = %d ,This = 0x%p"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
		m_pMPMediaInfo.v_frame_rate = 25;
	}
	if (m_iFPS != m_pMPMediaInfo.v_frame_rate) {
		m_iFPS = m_pMPMediaInfo.v_frame_rate;
		m_bVframeisChange = true;
	}
	audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	audioFormat.nChannels = m_pMPMediaInfo.a_channels;
	audioFormat.wBitsPerSample = 16;
	m_height = m_pMPMediaInfo.v_height;
	m_width = m_pMPMediaInfo.v_width;
	videoSize.x = m_pMPMediaInfo.v_width;
	videoSize.y = m_pMPMediaInfo.v_height;
	m_bVframeisChange = true;
	CallBackHight = m_height;
	CallBackWidth = m_width;
	InitCSampleData();
	
	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

	m_FrameWidth = m_width;
	m_FrameHeight = m_height;
	m_FrameLines = 0;
	m_FramePitchs = 0;
	
	VideoFormatCallback(m_choma, &m_FrameWidth, &m_FrameHeight, &m_FramePitchs, &m_FrameLines);
	LeaveCriticalSection(&DataLock);

	EnterCriticalSection(&TextureDataLock);
	if (Captureing)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_I420;
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB;
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB24;
		}

		ChangeShader();

		if (texture)
		{
			//	ClearTexture();
			delete texture;
			texture = nullptr;
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
		m_bReadyDraw = false;
	}
	
	LeaveCriticalSection(&TextureDataLock);
	if (m_MediaState == MediaStop)
	{
		m_bFirstVideo = false;
		while (true)
		{
			Sleep(100);
			if (m_bFirstVideo){
				mp_stop(HMediaProcess);
				break;
			}
		}
	}
}

Vect2 VideoSource::GetSize() const 
{
	if (Vect2(0,0) != videoSize)
		 return videoSize;
	else
	{
		return Vect2(1280, 720);
	}
}

void VideoSource::BeginScene()
{
	EnterCriticalSection(&TextureDataLock);
	Captureing = true;
	
	ChangeShader();

	//drawShader = CreatePixelShaderFromFile(TEXT("shaders\\DrawTexture_ColorAdjust.pShader"));
	//if (texture)
	//{
		//ClearTexture();
	//	delete texture;
	//	texture = nullptr;
	//}

	DWORD Width = 0, Heigth = 0;
	if (texture)
		D3DRender->GetTextureWH(texture, Width, Heigth);

	if (Width != CallBackWidth || Heigth != CallBackHight) {
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

void VideoSource::EndScene()
{
	EnterCriticalSection(&TextureDataLock);
	Captureing = false;
	if (texture)
	{
		//ClearTexture();
		delete texture;
		texture = nullptr;
	}
	
	LeaveCriticalSection(&TextureDataLock);
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
	if (audioSample == NULL)
	{
		audioSample = new CSampleData;
		if (!audioSample)
			return;
		audioSample->bAudio = true;
		audioSample->dataLength = 0;
		audioSample->lpData = NULL;
		audioSample->pAudioFormat = &audioFormat;
	}
}

void VideoSource::DestoryAuidoCSampleData()
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

void VideoSource::Synchronization()
{
	QWORD streamTimeStart = GetQPCNS(); // current nano second
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,YUV420_2_RGB32 Thread is start! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	int iLastVframerate = m_pMPMediaInfo.v_frame_rate;
	m_interval = 1000 / m_pMPMediaInfo.v_frame_rate;
	QWORD frameTimeNS = 1000000000 / m_pMPMediaInfo.v_frame_rate;	// one frame nano second
	QWORD sleepTargetTime = streamTimeStart + frameTimeNS;
	UINT no_sleep_counter = 0;

	while (!m_stop){
		if (!SleepToNS(sleepTargetTime += (frameTimeNS)))
			no_sleep_counter++;
		else
			no_sleep_counter = 0;
		if (!m_bPlay)
		{
			continue;
		}

		
		if (m_bVframeisChange)
		{
			m_bVframeisChange = false;
			if (iLastVframerate != m_pMPMediaInfo.v_frame_rate)
			{
				iLastVframerate = m_pMPMediaInfo.v_frame_rate;
				if (m_pMPMediaInfo.v_frame_rate == 0)
				{
					m_pMPMediaInfo.v_frame_rate = 25;
				}

				frameTimeNS = 1000000000 / m_pMPMediaInfo.v_frame_rate;
				m_interval = 1000 / m_pMPMediaInfo.v_frame_rate;
				Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s,Synchronization Thread Sleep Time %d ms Thread ID = %d,This = 0x%p"), __LINE__, __FUNCTION__, 1000 / m_pMPMediaInfo.v_frame_rate, GetCurrentThreadId(), this);
			}
		}

		if (latestVideoSample && enteredSceneCount && (GetMaxFPS() >= m_pMPMediaInfo.v_frame_rate))
		{
			int Index = 0;
			while (latestVideoSample && enteredSceneCount && (Index < 10))
			{
				Sleep(1);
				if (++Index >= 10)
					Log::writeMessage(LOG_RTSPSERV, 1, "%s 没采到这一帧跳过 this = 0x%p ThreadID = %d", __FUNCTION__,this,GetCurrentThreadId());
			}
		}

		EnterCriticalSection(&DataLock);
		if (m_VideoYuvBuffer.size())
		{
			m_bReadyDraw = true;
			{
				CSampleData* inf = *m_VideoYuvBuffer.begin();
				m_VideoYuvBuffer.pop_front();
				video_pts = inf->timestamp;
				inf->bFieldSignal = bIsFieldSignal;
				EnterCriticalSection(&CallBackLock);
				for (int i = 0; i < m_ListCallBack.Num(); ++i)
				{
					__DataCallBack &OneCallBack =  m_ListCallBack[i];
					if (OneCallBack.CallBack)
						OneCallBack.CallBack(OneCallBack.Context, inf);
				}
				LeaveCriticalSection(&CallBackLock);

		
				if (latestVideoSample)
				{
					//uint64_t uCurrentCheckNum = *(uint64_t *)(latestVideoSample->lpData - 24);
					UINT64 nHigeCheckNum = latestVideoSample->CheckNum & 0xFF00000000000000;
					if (nHigeCheckNum != 0)
					{
						SafeRelease(latestVideoSample);
					}
					else
					{
						//Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 释放latestVideoSample出错，校验值不匹配: uCurrentCheckNum =0x%p,lastSample->CheckNum =0x%p.ThreadID = %d,This = 0x%p"),
						//	__LINE__, __FUNCTION__, uCurrentCheckNum, latestVideoSample->CheckNum, GetCurrentThreadId(), this);

						//UINT64 nHigeCheckNum = latestVideoSample->CheckNum & 0xFF00000000000000;
						//UINT64 nHigeCurrrntNum = uCurrentCheckNum & 0xFF00000000000000;

						//if (nHigeCheckNum == nHigeCurrrntNum)//最高字节匹配
						//{
						//	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 最高位校验值匹配释放: uCurrentCheckNum = 0x%p,latestVideoSample->CheckNum =0x%p.nHigeCurrrntNum = 0x%p.nHigeCheckNum = 0x%p.latestVideoSample->lpData :0x%p.latestVideoSample :0x%p.ThreadID = %d,This = 0x%p"),
						//		__LINE__, __FUNCTION__, uCurrentCheckNum, latestVideoSample->CheckNum, nHigeCurrrntNum, nHigeCheckNum, latestVideoSample->lpData, latestVideoSample, GetCurrentThreadId(), this);
						//	SafeRelease(latestVideoSample);
						//}
						//else
						//{
							Log::writeMessage(LOG_RTSPSERV, 1, ("LINE: %d, FUNC:%s, 最高位校验值不匹配，不释放: uCurrentCheckNum = 0x%p,latestVideoSample->CheckNum =0x%p.nHigeCurrrntNum = 0x%p.nHigeCheckNum = 0x%p.latestVideoSample->lpData :0x%p.latestVideoSample :0x%p.ThreadID = %d,This = 0x%p"),
								__LINE__, __FUNCTION__, 0, latestVideoSample->CheckNum, 0, nHigeCheckNum, latestVideoSample->lpData, latestVideoSample, GetCurrentThreadId(), this);
						//}

					}
				}
				latestVideoSample = inf;
			}
		}
		LeaveCriticalSection(&DataLock);

		EnterCriticalSection(&AudioDataLock);
		if (m_pMPMediaInfo.has_video)
		{
			do
			{
				if (m_AudioAACBuffer.size())
				{
					CSampleData* tsAudio = *m_AudioAACBuffer.begin();
					audio_pts = tsAudio->timestamp;
					m_AudioAACBuffer.pop_front();
					if (m_bclear)
					{
						if (audio_pts < video_pts - 200)
						{

							Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s, 因为音频小于视频200ms，所以丢弃音频! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
							for (CSampleData* inf : m_AudioAACBuffer)
							{
								inf->Release();
							}
							m_AudioAACBuffer.clear();
							m_bclear = false;
							break;
						}
					}

					if (audio_pts <= video_pts + m_interval)
					{
						if (m_pDemandMediaAudio)
						{
							m_pDemandMediaAudio->PushAudio(tsAudio->lpData, tsAudio->dataLength, tsAudio->timestamp, this, enteredSceneCount != 0);
						}
						tsAudio->Release();
					}
					else
					{
						m_AudioAACBuffer.push_front(tsAudio);
						break;
					}
				}
				else
				{
					break;
				}

			} while (1);
		}
		else
		{
			do
			{
				if (m_AudioAACBuffer.size())
				{
					CSampleData* tsAudio = *m_AudioAACBuffer.begin();
					m_AudioAACBuffer.pop_front();
					audio_pts = tsAudio->timestamp;

					if (m_pDemandMediaAudio)
					{
						m_pDemandMediaAudio->PushAudio(tsAudio->lpData, tsAudio->dataLength, tsAudio->timestamp,this,enteredSceneCount != 0);
					}
					tsAudio->Release();

				}
				else
				{
					break;
				}

			} while (1);
		}

		LeaveCriticalSection(&AudioDataLock);

	}
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE:%d,FUNC:%s, SyncThread Exit! Thread ID = %d,This = %x"), __LINE__, __FUNCTION__, GetCurrentThreadId(), this);
	SetEvent(m_hCloseSyncThreadEvent);
}

String VideoSource::ChooseShader(bool bNeedField)
{
	if (colorType == DeviceOutputType_RGB || colorType == DeviceOutputType_RGB24)
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
	if (config->playlist.Num() == 0)
	{
		m_MediaState = MediaStop;
		//SetMediaState(m_MediaState, true);//当前文件数量为0，把按钮状态置为停止
		return false;
	}

	return ChangePlay();
}

bool VideoSource::StopMedia()
{
	bool result = true;
	if (m_mediaDuration && m_bPlay)
	{
		result = ChangeStop();
	}
	return result;
}

bool VideoSource::ReSetMedia()
{
	bool result = false;
	if (m_mediaDuration)
	{
		result = ChangeReset();
	}

	return result;
}

bool VideoSource::PauseMedia()
{
	bool result = true;
	mp_stop(HMediaProcess);    //开始获取数据
	m_bPlay = false;
	m_MediaState = MediaStop;
	return result;
}

bool VideoSource::SetStreamPos(UINT nPos)
{
	bool bRet = false;
	if (m_mediaDuration)
	{
		m_pos = nPos;
		bRet = ChangePos();
	}
	return bRet;
}

bool VideoSource::PlayNextFile()
{
	bool ret = ChangeNext_API();
	while (!ret && (config->CurrentIndex != config->playlist.Num() - 1))    //最后一个文件
	{
		ret = ChangeNext_API();
	}
	if (!ret && config->CurrentIndex == config->playlist.Num() - 1)//最后一个文件
	{
		bool retNext = ChangeNext();
		if (!ret && !m_bisAutoNextFile)    //播放第一个文件仍然是无法打开的
		{
			int LoopIndex = 0;//尝试次数
			while (!ret && (config->CurrentIndex != config->playlist.Num() - 1 && LoopIndex <= config->playlist.Num()-1))    //最后一个文件
			{
				ret = ChangeNext_API();
			}
		}
	}
	return ret;
}

bool VideoSource::PlayPrevFile()
{
	 bool ret = ChangePrev();
	 while (!ret && (config->CurrentIndex != 0))     //第一个文件
	 {
		 ret = ChangePrev();
	 }
	 return ret;
}

bool VideoSource::GetStreamPos(UINT &nPos)
{
	if (config->playlist.Num() == 0)
	{
		nPos = 0;
		return true;
	}
	if (!m_mediaDuration)
	{
		nPos = 0;
		return true;
	}
	int64_t nTime = m_pts;
	//判断原则，视频播放完成，自动播放下一个文件，完成标准，当前时间戳等于上一次的时间戳且不等于0并且和总时长相差不到一秒
	if ((m_ptsLast == m_pts && m_pts + 1000 > m_mediaDuration) || (m_ptsLast == m_pts && fileLoopPlayUseTsp + 1000 > m_mediaDuration))
	{
		//注意最后一帧视频可能不是和总时长在同一个秒数位，这种情况把最后时间戳设置为文件时长
		m_ptsLast = m_pts = m_mediaDuration;
	}
	if (m_ptsLast == m_pts && m_ptsLast != 0 && m_pts + 1000 > m_mediaDuration)
	{	
		if (MediaPalying == m_MediaState)
		{
			bool ret = ChangeNext();
			int iLoopIndex = 0; //寻找播放文件的次数不能大于循环列表个数，否则说明全是错误文件
			while (!ret && !m_bisAutoNextFile && iLoopIndex <= config->playlist.Num())
			{
				iLoopIndex++;
				ret = ChangeNext();
			}
		}
	}
	m_ptsLast = m_pts;
	if (m_bNeedUpdate)
	{
		m_bNeedUpdate = false;
		//SetMediaState(m_MediaState, true);
	}
	//if (m_bFileIsError && m_ptsLast != 0)   //文件中途打开失败，执行下一个文件。
	//{
	//	if (m_pts + 1000 < m_mediaDuration)
	//	{
	//		ChangeNext();
	//	}
	//	m_bFileIsError = false;
	//	
	//}
	nPos = nTime;
	return true;
}

int64_t VideoSource::GetMediaDuration()
{
		return m_mediaDuration;
}

bool VideoSource::ChangePrev()
{
	//不能是第一个文件，因为第一个文件禁止向前
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE = %d,FUNC = %s,This = %x"), __LINE__, __FUNCTION__,this );
	ColseFile();
	//config->Reload();
	
	//只要能够执行就认为文件数量大于当前索引减一
	config->CurrentIndex--;
	memset(m_playPath, 0, 256);
	memcpy(m_playPath, WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str() , strlen(WcharToAnsi(config->playlist[config->CurrentIndex].Array()).c_str()));
	config->Save();

	char url[256] = { 0 };
	m_pos = 0;      //重新定位到开头
	sprintf_s(url, "-re -f YUV -ss  %d", m_pos);
	memset(&m_pMPMediaInfo, 0, sizeof(MPMediaInfo));
	mp_set_hardwaredecode(HMediaProcess, config->isHardwareDecode);
	EnterCriticalSection(&DataLock);
	int ret = mp_open(HMediaProcess, m_playPath, url, FrameCallBackFunc, this, &m_pMPMediaInfo);
	if (ret != 0 || (-1 == m_pMPMediaInfo.v_pix_fmt) && m_pMPMediaInfo.has_video)
	{
		if (!ret)
			m_mediaDuration = 0;
		OpenFileFailed();
		LeaveCriticalSection(&DataLock);
		return false;
	}
	m_pts = 0;
	m_bFirstTsTimeStamp = -1;
	m_bPlay = true;
	m_ChangePosPts = 0;
	for (CSampleData* inf : m_VideoYuvBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_VideoYuvBuffer.clear();
	EnterCriticalSection(&AudioDataLock);
	for (CSampleData* inf : m_AudioAACBuffer)
	{
		video_pts = 0;
		audio_pts = 0;
		inf->Release();
	}
	m_AudioAACBuffer.clear();
	LeaveCriticalSection(&AudioDataLock);
	mp_start(HMediaProcess);    //开始获取数据
	Log::writeMessage(LOG_RTSPSERV, 1, ("LINE : %d, FUNC : %s ,Path: %s,frame_rate =%d,nSamplesPerSec = %d,nChannels = %d,wBitsPerSample = 16,This = %x"),
		__LINE__, __FUNCTION__, m_playPath, m_pMPMediaInfo.v_frame_rate, m_pMPMediaInfo.a_sample_rate, m_pMPMediaInfo.a_channels, this);
	m_mediaDuration = m_pMPMediaInfo.stream_duration;
	if (m_pMPMediaInfo.v_frame_rate != 0)
	{
		m_interval = 1000 / m_pMPMediaInfo.v_frame_rate;
	}
	audioFormat.nSamplesPerSec = m_pMPMediaInfo.a_sample_rate;
	audioFormat.nChannels = m_pMPMediaInfo.a_channels;
	audioFormat.wBitsPerSample = 16;
	m_height = m_pMPMediaInfo.v_height;
	m_width = m_pMPMediaInfo.v_width;
	videoSize.x = m_pMPMediaInfo.v_width;
	videoSize.y = m_pMPMediaInfo.v_height;
	m_bVframeisChange = true;
	CallBackHight = m_height;
	CallBackWidth = m_width;
	InitCSampleData();

	AudioParam Param;
	Param.iBitPerSample = audioFormat.wBitsPerSample;
	Param.iChannel = audioFormat.nChannels;
	Param.iSamplesPerSec = audioFormat.nSamplesPerSec;
	EnterLiveVideoSection();
	if (Param.iBitPerSample > 0)
		m_pDemandMediaAudio->ResetAudioParam(Param);
	LeaveLiveVideoSection();
	

	if (m_pDemandMediaAudio)
		m_pDemandMediaAudio->ResetAudioDB();

	m_FrameWidth = m_width;
	m_FrameHeight = m_height;
	m_FrameLines = 0;
	m_FramePitchs = 0;
	
	VideoFormatCallback(m_choma, &m_FrameWidth, &m_FrameHeight, &m_FramePitchs, &m_FrameLines);
	LeaveCriticalSection(&DataLock);

	EnterCriticalSection(&TextureDataLock);
	if (Captureing)
	{
		if (MP_PIX_FMT_YUV420P == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_I420;
		}
		else if (MP_PIX_FMT_BGRA == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB;
		}
		else if (MP_PIX_FMT_RGB24 == m_pMPMediaInfo.v_pix_fmt)
		{
			colorType = DeviceOutputType_RGB24;
		}

		ChangeShader();

		if (texture)
		{
			//ClearTexture();
			delete texture;
			texture = nullptr;
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
		m_bReadyDraw = false;
	}
	
	LeaveCriticalSection(&TextureDataLock);
	if (m_MediaState == MediaStop)
	{
		m_bFirstVideo = false;
		while (true)
		{
			Sleep(100);
			if (m_bFirstVideo){
				mp_stop(HMediaProcess);
				break;
			}
		}
	}
	return true;
}

void VideoSource::GetPlayList(StringList & PlayList)
{
	if (config){
		for (unsigned int i = 0; i < config->playlist.Num(); i++) {
			PlayList.Add(config->playlist[i]);
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
// 	if (m_mediaDuration== 0)
// 	{
// 		return false;
// 	}
	JsonInfo["DemandFileDuration"] = m_mediaDuration;
	Value &PlayArry = JsonInfo["playlist"];
	PlayArry.resize(config->playlist.Num());
	for (int i = 0; i < config->playlist.Num(); ++i)
	{
		PlayArry[i] = WcharToAnsi(config->playlist[i].Array()).c_str();
	}
	JsonInfo["CurrentIndex"] = config->CurrentIndex;
	JsonInfo["IsLoop"] = config->isListLoop;
	JsonInfo["IsListLoop"] = config->isPlaylistLooping;
	JsonInfo["IsFileLoop"] = config->isFileLoop;
	JsonInfo["MediaStatus"] = m_MediaState;

	if (m_MediaState != MediaPalying /*&& bCanResetAudio*/)
	{
		if (m_pDemandMediaAudio)
			m_pDemandMediaAudio->ResetAudioDB();

		bCanResetAudio = false;
	}
//	else if (m_MediaState == MediaPalying)
//	{
//		bCanResetAudio = true;
//	}

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
				  m_MediaState = MediaPause;
				  return PauseMedia();
	}

	case Play:
	{
				 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
				 m_MediaState = MediaPalying;
				 return PlayMedia();

	}

	case ReStart:
	{
					m_MediaState = MediaPalying;
					Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 重新播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					return ReSetMedia();
	}
	case Stop:
	{
				 m_MediaState = MediaStop;
				 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 重新播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
				 return StopMedia();
	}

	case PlayNext:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 重新播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					 if (config->playlist.Num() && config->CurrentIndex < config->playlist.Num() - 1 && config->CurrentIndex >= 0)
					 {
						 return PlayNextFile();
					 }
					 break;
	}
	case PlayPrev:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "FUNC : %s, LINE:%d,  文件操作 重新播放OperType =  %d.", __FUNCTION__, __LINE__, OperType);
					 if (config->playlist.Num() && config->CurrentIndex > 0 && config->CurrentIndex < config->playlist.Num())
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
	if (m_bSwitch && enteredSceneCount >= 1)
	{
		if (config && config->isSenceChangeStartPlay)
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
	EnterCriticalSection(&CallBackLock);
	m_ListCallBack.Add(DataBack);
	LeaveCriticalSection(&CallBackLock);
}

void VideoSource::UnRegisterDataCallBack(void *Context)
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

void VideoSource::SetHasPreProcess(bool bPre)
{
	bHasPreProcess = bPre;
}

bool VideoSource::GetHasPreProcess() const
{
	return bHasPreProcess;
}

void VideoSource::OpenFileFailed()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LINE = %d,FUNC = %s,打开文件失败 FILE = %s", __LINE__, __FUNCTION__, m_playPath);
	HintInfo *pHintInfo = new HintInfo;

	if (!pHintInfo)
		return;

	pHintInfo->FileName = m_playPath;
	pHintInfo->Index = FileIndex;
	FileIndex++;
	if (FileIndex > 2)
	{
		FileIndex = 0;
	}
	OSCreateThread((XTHREAD)VideoSource::FileHintThread, pHintInfo);
}

void VideoSource::ColseFile()
{
	try
	{
		if (HMediaProcess)
			mp_close(HMediaProcess);
	}
	catch (...)
	{

	}
}

bool VideoSource::IsFieldSignal() const
{
	return bIsFieldSignal;
}

void VideoSource::ChangeShader()
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
	else if ((colorType == DeviceOutputType_RGB || colorType == DeviceOutputType_RGB24) && !bIsFieldSignal)
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

void VideoSource::PlayCallBackAudio(LPBYTE lpData, UINT len)
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


