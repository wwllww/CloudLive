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

#include "PipeVideoPlugin.h"
#include "BitmapImage.h"
#include <shellapi.h>
#include <TlHelp32.h>
#include <sstream>
#include <thread>
//extern "C"{
//#include "libswscale/swscale.h"  
//#include "libavutil/opt.h"  
//#include "libavutil/imgutils.h"
//
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//}
struct ResSize
{
    UINT cx;
    UINT cy;
};

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DWORD STDCALL PackPlanarThread(ConvertData *data);

#define NEAR_SILENT  3000
#define NEAR_SILENTf 3000.0

bool PipeVideo::Init(Value &data)
{
	b_flag = false;
	bThreadRuning = true;

	this->data = data;

	Log::writeMessage(LOG_RTSPSERV,1,"line: %d   func:%s,  --PipeVideo is start,Create a PipeServer!", __LINE__, __FUNCTION__);
	CPipeServer::GetInstance()->Start(data);
	CPipeServer::GetInstance()->AddVideoRender(this);
	

	m_size = Vect2(-1, -1);

	//m_pAudioWaveOut = NULL;
	bitmapImage = NULL;
	FileInfo = 0;

	hAudioMutex = OSCreateMutex();

	texture = NULL;
	audioOut = NULL;
	drawShader = NULL;

    hSampleMutex = OSCreateMutex();
    if(!hSampleMutex)
    {
		Log::writeError(LOG_RTSPSERV, 1, "PipeVideoPlugin: could not create sample mutex");
        return false;
    }

//     int numThreads = MAX(OSGetTotalCores()-2, 1);
	hConvertThreads = NULL; //(HANDLE*)Allocate_Bak(sizeof(HANDLE)*numThreads);
	convertData = NULL;// (ConvertData*)Allocate_Bak(sizeof(ConvertData)*numThreads);
// 
//     zero(hConvertThreads, sizeof(HANDLE)*numThreads);
//     zero(convertData, sizeof(ConvertData)*numThreads);

	hAudioThreads = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioThread, this, 0, NULL);
   
    UpdateSettings(data);

	Log::writeMessage(LOG_RTSPSERV, 1, "Using PipeVideo input");

// 	if (!bLoadSucceed)
// 		return false;

    return true;
}

PipeVideo::~PipeVideo()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeVideo is end,remove a PipeVideo from PipeServer!", __LINE__, __FUNCTION__);
	CPipeServer::GetInstance()->RemoveVideoRender(this);

    Stop();
    UnloadFilters();

	bThreadRuning = false;

	if (hAudioThreads)
	{
		if (WAIT_TIMEOUT == WaitForSingleObject(hAudioThreads, 5000))
		{
			TerminateThread(hAudioThreads,0x11);
		}
		CloseHandle(hAudioThreads);
	}

    FlushSamples();

    if(hConvertThreads)
        Free(hConvertThreads);

    if(convertData)
        Free(convertData);

    if(hSampleMutex)
        OSCloseMutex(hSampleMutex);

	if (hListMutex)
		OSCloseMutex(hListMutex);

	if (bitmapImage)
	{
		delete bitmapImage;
		bitmapImage = NULL;
	}

	OSEnterMutex(hAudioMutex);

	if (audioOut)
	{
		delete audioOut;
		audioOut = NULL;
	}
	OSLeaveMutex(hAudioMutex);

	if (hAudioMutex)
	{
		OSCloseMutex(hAudioMutex);
		hAudioMutex = NULL;
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

// 	if (m_pDefaultImgbuffer)
// 		delete[]m_pDefaultImgbuffer;
// 	m_pDefaultImgbuffer = NULL;

	if (m_pDefaultImgYUVbuffer)
		delete[] m_pDefaultImgYUVbuffer;
	m_pDefaultImgYUVbuffer = NULL;

}

String PipeVideo::ChooseShader(bool bNeedField)
{
    if(colorType == DeviceOutputType_RGB)
        return String();

    String strShader;
    strShader << SHADER_PATH;

	if (bNeedField)
		strShader << L"Field_";

    if(colorType == DeviceOutputType_I420)
        strShader << TEXT("YUVToRGB.pShader");
    else if(colorType == DeviceOutputType_YV12)
        strShader << TEXT("YVUToRGB.pShader");
    else if(colorType == DeviceOutputType_YVYU)
        strShader << TEXT("YVXUToRGB.pShader");
    else if(colorType == DeviceOutputType_YUY2)
        strShader << TEXT("YUXVToRGB.pShader");
    else if(colorType == DeviceOutputType_UYVY)
        strShader << TEXT("UYVToRGB.pShader");
	else if (colorType == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
		strShader.Clear();

    return strShader;
}


const float yuv709Mat[16] = {0.182586f,  0.614231f,  0.062007f, 0.062745f,
                            -0.100644f, -0.338572f,  0.439216f, 0.501961f,
                             0.439216f, -0.398942f, -0.040274f, 0.501961f,
                             0.000000f,  0.000000f,  0.000000f, 1.000000f};

const float yuvMat[16] = {0.256788f,  0.504129f,  0.097906f, 0.062745f,
                         -0.148223f, -0.290993f,  0.439216f, 0.501961f,
                          0.439216f, -0.367788f, -0.071427f, 0.501961f,
                          0.000000f,  0.000000f,  0.000000f, 1.000000f};

//#include <winnt.h>
typedef TCHAR* PTCHAR;
//#include "streams.h"

bool PipeVideo::LoadFilters()
{
    if(bCapturing )
        return false;

    bool bSucceeded = false;

	OSEnterMutex(hAudioMutex);
	if (!audioOut)
	{
		if (!AudioParamInit)
		{
			Param.bitsPerSample = 16;
			Param.channels = 1;
			Param.samplesPerSec = 8000;
			AudioParamInit = true;
		}

		audioOut = new PipeAudioSource;
		audioOut->Initialize(this, Param);
	}
	OSLeaveMutex(hAudioMutex);

    String strShader;
	bUseThreadedConversion = false;//API->UseMultithreadedOptimizations() && (OSGetTotalCores() > 1);

	
	if (bitmapImage)
	{
		Vect2 &Size = bitmapImage->GetSize();

		renderCX = newCX = Size.x;
		renderCY = newCY = Size.y;
	}
	else
	{
		renderCX = newCX = 1280;
		renderCY = newCY = 720;
	}

    bFirstFrame = true;

	ChangeShader();

    //if(colorType != DeviceOutputType_RGB && !colorConvertShader)
    //{
    //    AppWarning(TEXT("PipeVideoPlugin: Could not create color space conversion pixel shader"));
    //    goto cleanFinish;
    //}

	//----------------------
	UINT  inputChannels;
	UINT  inputSamplesPerSec;
	UINT  inputBitsPerSample;
	UINT  inputBlockSize;
	DWORD inputChannelMask;

	inputBitsPerSample = 16;
	inputBlockSize = 2;
	inputChannelMask = 0;
	inputChannels = 1;
	inputSamplesPerSec = 8000;

	//int audio = data->GetInt(TEXT("AudioLocalOut"), 0);
	//if (audio)
	//{
	//	String strReanderName = data->GetString(TEXT("RenderAudioName"));
	//	OSEnterMutex(hAudioMutex);
	//	if (NULL == m_pAudioWaveOut)
	//	{
	//		if (!strReanderName.Compare(TEXT("Disable")))
	//		{
	//			m_pAudioWaveOut = new AudioWaveOut;
	//		}
	//	}
	//	if (NULL != m_pAudioWaveOut)
	//	{
	//		m_pAudioWaveOut->Initialize(strReanderName.Array(), inputChannels, inputSamplesPerSec, inputBitsPerSample);
	//	}
	//	OSLeaveMutex(hAudioMutex);
	//}
	////-----------------------

	audioFormat.nChannels = inputChannels;
	audioFormat.nSamplesPerSec = inputSamplesPerSec;
	audioFormat.wBitsPerSample = inputBitsPerSample;

    bSucceeded = true;

cleanFinish:

    if(!bSucceeded)
    {
        bCapturing = false;

        if(colorConvertShader)
        {
            delete colorConvertShader;
            colorConvertShader = NULL;
        }

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
    if (renderCX <= 0 || renderCX >= 8192) { newCX = renderCX = 640;}
    if (renderCY <= 0 || renderCY >= 8192) { newCY = renderCY = 480;}

	//这里是4.18号注释掉的
    //ChangeSize(bSucceeded, true);
	bLoadSucceed = bSucceeded;
    return bSucceeded;
}

void PipeVideo::UnloadFilters()
{
    if(texture)
    {
        delete texture;
        texture = NULL;
    }

//    KillThreads();

//     if(colorConvertShader)
//     {
//         delete colorConvertShader;
//         colorConvertShader = NULL;
//     }

    if(lpImageBuffer)
    {
        Free(lpImageBuffer);
        lpImageBuffer = NULL;
    }

	FlushSamples();
}

void PipeVideo::ResetAudioParam(const AudioParam& param)
{
	OSEnterMutex(hAudioMutex);

	AudioParamInit = true;
	Param = param;
	EnterLiveVideoSection();
	if (audioOut)
	{
		audioOut->Initialize(this, Param);
	}
	else
	{
		audioOut = new PipeAudioSource;
		audioOut->Initialize(this, Param);
	}
	LeaveLiveVideoSection();
	audioFormat.nChannels = param.channels;
	audioFormat.nSamplesPerSec = param.samplesPerSec;
	audioFormat.wBitsPerSample = param.bitsPerSample;
	Log::writeMessage(LOG_RTSPSERV, 1,"--nChannels = %d.samplesPerSec = %d.bitsPerSample = %d", param.channels, param.samplesPerSec, param.bitsPerSample);
	OSLeaveMutex(hAudioMutex);
}

void PipeVideo::Start()
{
	OSEnterMutex(hAudioMutex);
	if (!audioOut)
	{
		if (!AudioParamInit)
		{
			Param.bitsPerSample = 16;
			Param.channels = 1;
			Param.samplesPerSec = 8000;
			AudioParamInit = true;
		}

		audioOut = new PipeAudioSource;
		audioOut->Initialize(this, Param);
	}
	OSLeaveMutex(hAudioMutex);

	String path = L".\\img\\PIPE_1920x1080.png"; //默认是16:9

	if (!OSFileExists(path)) // 文件不存在
	{
		return;
	}

	if (!bitmapImage)
	{
		bitmapImage = new BitmapImage();
		bitmapImage->SetPath(path);
		bitmapImage->EnableFileMonitor(false);
		bitmapImage->Init();
	}

	if (!b_flag)
	{
		Vect2 &Size = bitmapImage->GetSize();

		renderCX = newCX = Size.x;
		renderCY = newCY = Size.y;
	}
}

void PipeVideo::Stop()
{
	
	if (drawShader)
	{
		delete drawShader;
		drawShader = NULL;
	}

    if(!bCapturing)
        return;

    bCapturing = false;
    FlushSamples();

	if (bitmapImage)
	{
		delete bitmapImage;
		bitmapImage = NULL;
	}
}

void PipeVideo::BeginScene()
{
// 	OSEnterMutex(hAudioMutex);
// 	if (audioOut)
// 	{
// 		delete audioOut;
// 		audioOut = NULL;
// 	}
// 	OSLeaveMutex(hAudioMutex);
	Start();

	ChangeShader();

	if (!texture)
		ChangeSize(bLoadSucceed, true);

	if (!drawShader)
		drawShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders\\DrawTexture_ColorAdjust.pShader"));

	bCapturing = true;
}

void PipeVideo::EndScene()
{
// 	OSEnterMutex(hAudioMutex);
// 	if (audioOut)
// 	{
// 		delete audioOut;
// 		audioOut = NULL;
// 	}
// 	OSLeaveMutex(hAudioMutex);
    Stop();

	if (drawShader)
	{
		delete drawShader;
		drawShader = NULL;
	}
}

void PipeVideo::GlobalSourceLeaveScene()
{
    if (!enteredSceneCount)
        return;
    if (--enteredSceneCount)
        return;
}

void PipeVideo::GlobalSourceEnterScene()
{
    if (enteredSceneCount++)
        return;

	OSEnterMutex(hAudioMutex);
	if (!audioOut)
	{
		if (!AudioParamInit)
		{
			Param.bitsPerSample = 16;
			Param.channels = 1;
			Param.samplesPerSec = 8000;
			AudioParamInit = true;
		}

		audioOut = new PipeAudioSource;
		audioOut->Initialize(this, Param);
	}
	else
	{
		audioOut->Initialize(this, Param);
	}
	OSLeaveMutex(hAudioMutex);
}

void PipeVideo::KillThreads()
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

void PipeVideo::ChangeSize(bool bSucceeded, bool bForce)
{
    if (!bForce && renderCX == newCX && renderCY == newCY)
        return;

    renderCX = newCX;
    renderCY = newCY;

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

//     KillThreads();
// 
//     int numThreads = MAX(OSGetTotalCores()-2, 1);
//     for(int i = 0; i < numThreads; i++)
//     {
//         convertData[i].width  = lineSize;
//         convertData[i].height = renderCY;
//         convertData[i].sample = NULL;
//         convertData[i].hSignalConvert  = CreateEvent(NULL, FALSE, FALSE, NULL);
//         convertData[i].hSignalComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
//         convertData[i].linePitch = linePitch;
//         convertData[i].lineShift = lineShift;
// 
//         if(i == 0)
//             convertData[i].startY = 0;
//         else
//             convertData[i].startY = convertData[i-1].endY;
// 
//         if(i == (numThreads - 1))
//             convertData[i].endY = renderCY;
//         else
//             convertData[i].endY = ((renderCY/numThreads)*(i + 1)) & 0xFFFFFFFE;
//     }
// 
//     if(colorType == DeviceOutputType_YV12 || colorType == DeviceOutputType_I420)
//     {
//         for(int i=0; i<numThreads; i++)
//             hConvertThreads[i] = OSCreateThread((XTHREAD)PackPlanarThread, convertData+i);
//     }

    if(texture)
    {
        delete texture;
        texture = NULL;
    }

    //-----------------------------------------------------
    // create the texture regardless, will just show up as red to indicate failure
	BYTE *textureData = (BYTE*)Allocate_Bak(renderCX*renderCY * 4);

    if(colorType == DeviceOutputType_RGB) //you may be confused, but when directshow outputs RGB, it's actually outputting BGR
    {
        msetd(textureData, 0xFFFF0000, renderCX*renderCY*4);
		texture = D3DRender->CreateTexture(renderCX, renderCY, GS_BGR, textureData, FALSE, FALSE);
    }
    else //if we're working with planar YUV, we can just use regular RGB textures instead
    {
        msetd(textureData, 0xFF0000FF, renderCX*renderCY*4);
		texture = D3DRender->CreateTexture(renderCX, renderCY, GS_RGB, textureData, FALSE, FALSE);
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
	//bFirstFrame = false;
}

void PipeVideo::ReceiveState(bool connected)
{
	int w = 1024, h = 768; //w,h应该设置为图片原始大小  

	if (m_ListCallBack.Num())
	{
		CSampleData VideoSample;

		VideoSample.bAudio = false;
		VideoSample.dataLength = 1024 * 768 * 3 / 2;
		VideoSample.lpData = (LPBYTE)m_pDefaultImgYUVbuffer;//pointer; //
		VideoSample.cx = 1024;
		VideoSample.cy = 768;
		VideoSample.colorType = ColorType_I420;


		OSEnterMutex(hListMutex);
		for (int i = 0; i < m_ListCallBack.Num(); ++i)
		{
			__DataCallBack &OneBack = m_ListCallBack[i];
			OneBack.CallBack(OneBack.Context, &VideoSample);
		}
		OSLeaveMutex(hListMutex);

		VideoSample.lpData = NULL;
	}	
// 	delete[] buffer;
// 	buffer = NULL;
}

//数据处理的起点
void PipeVideo::ReceiveMediaSample(ISampleData *sample, bool bAudio)
{
	if (!sample /*|| !bCapturing*/)
		return;

	//互动在这里做了增益
	if (bAudio && audioOut)
	{
		if (audioOut->fVolume != 1.0f)
		{
			short *Tem = (short*)sample->lpData;
			for (int i = 0; i < sample->AInfo.Datalen; i += 2)
			{
				long sVolume = Tem[i / 2];

				sVolume *= audioOut->fVolume;

				if (sVolume > 0x7fff)
				{
					sVolume = 0x7fff;
				}
				else if (sVolume < -0x8000)
				{
					sVolume = -0x8000;
				}

				Tem[i / 2] = (short)sVolume;
			}
		}
	}

	OSEnterMutex(hListMutex);
	if (m_ListCallBack.Num())
	{
		if (!bAudio)
		{
			CSampleData  VideoSample;;

			VideoSample.bAudio = false;
			VideoSample.dataLength = sample->VInfo.Datalen;
			VideoSample.lpData = sample->lpData;// (LPBYTE)Allocate_Bak(VideoSample->dataLength);
			VideoSample.cx = sample->VInfo.Width;
			VideoSample.cy = sample->VInfo.Height;
			VideoSample.colorType = colorType;
			VideoSample.bFieldSignal = bInterlaceSignal;

			for (int i = 0; i < m_ListCallBack.Num(); ++i)
			{
				__DataCallBack &OneBack = m_ListCallBack[i];
				OneBack.CallBack(OneBack.Context, &VideoSample);
				m_bFirstReceiveData = true;
			}
			VideoSample.lpData = NULL;
		}
		else //音频
		{
			CSampleData  audioSample;
			audioSample.bAudio = true;
			audioSample.dataLength = sample->AInfo.Datalen;
			audioSample.lpData = sample->lpData;// (LPBYTE)Allocate_Bak(audioSample->dataLength);
			audioSample.timestamp = sample->AInfo.Timestamp;
			audioSample.pAudioFormat = (void*)&audioFormat;

			for (int i = 0; i < m_ListCallBack.Num(); ++i)
			{
				__DataCallBack &OneBack = m_ListCallBack[i];
				OneBack.CallBack(OneBack.Context, &audioSample);
			}
			audioSample.lpData = NULL;

			m_qwrdAudioTime = GetQPCMS();
		}
	}

	OSLeaveMutex(hListMutex);

	//这个if必须放在最后
	if (bCapturing && enteredSceneCount)
	{
		if (bAudio)
		{
			OSEnterMutex(hAudioMutex);
			//bool AudioOut = data->GetInt(TEXT("AudioOut"), 1);
			//音频数据的处理

			ListParam Param;
			Param.pData = sample;
			Param.len = sample->AInfo.Datalen;
			Param.TimeStamp = sample->AInfo.Timestamp;
			ListAudioSample.push_back(Param);
			OSLeaveMutex(hAudioMutex);

			//sample->Release();
		}
		else
		{

			OSEnterMutex(hSampleMutex);
			//SafeRelease(latestVideoSample);
			//latestVideoSample = sample;
			ListParam Param;
			Param.pData = sample;
			Param.len = sample->VInfo.Datalen;
			Param.TimeStamp = sample->VInfo.Timestamp;
			ListSample.push_back(Param);

// 			static int iCount = 0;
// 			static DWORD StartTime = GetQPCMS();
// 			++iCount;
// 
// 			DWORD EndTime = GetQPCMS();
// 			Log::writeMessage(LOG_RTSPSERV, 1, "互动---接收到 %d 个数据用时 %d ms,平均 %0.4f ms,当前 Size %d", iCount, EndTime - StartTime, (float)(EndTime - StartTime) / iCount, ListSample.size());

			if (ListSample.size() > 50)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "当前ListSample size %d,ListAudioSample %d,开始删除", ListSample.size(), ListAudioSample.size());
				while (ListSample.size() > 35)
				{
					ListParam &Param = ListSample.front();
					Param.pData->Release();
					ListSample.pop_front();
				}
				//删除音频
				ListParam &Param = ListSample.front();

				OSEnterMutex(hAudioMutex);
				
				while (ListAudioSample.size())
				{
					ListParam& __AudioParam = ListAudioSample.front();

					if (__AudioParam.TimeStamp < Param.TimeStamp - 40)
					{
						__AudioParam.pData->Release();
						ListAudioSample.pop_front();
					}
					else
					{
						break;
					}

				}
				OSLeaveMutex(hAudioMutex);

				Log::writeMessage(LOG_RTSPSERV, 1, "删除结束后ListSample size %d,ListAudioSample %d", ListSample.size(), ListAudioSample.size());
			}

			OSLeaveMutex(hSampleMutex);
		}
	}
	else
	{
		FlushSamples();

		if (bAudio && audioOut)
		{
			audioOut->ReceiveAudio(sample->lpData, sample->AInfo.Datalen, sample->AInfo.Timestamp, false);
		}
		sample->Release();
		bReadyToDraw = false;
		m_LastTimeStamp = 0;
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

//设置默认图片刷新
void PipeVideo::Tick(float fSeconds)
{
	//if (bitmapImage)
	//{
	//	bitmapImage->Tick(fSeconds);
	//}
}

void PipeVideo::Preprocess()
{
	if (!bCapturing || enteredSceneCount == 0)
		return;

    ISampleData *lastSample = NULL;

	OSEnterMutex(hSampleMutex);
	if (ListSample.size())
	{
		ListParam &Param = ListSample.front();
		lastSample = Param.pData;
		m_LastTimeStamp = Param.TimeStamp;
		//上面为引用，所以要在赋值要在pop_front之前
		ListSample.pop_front();
	}
	OSLeaveMutex(hSampleMutex);

    if(lastSample)
    { 
		newCX = lastSample->VInfo.Width;
		newCY = lastSample->VInfo.Height;

        if(colorType == DeviceOutputType_RGB)
        {
			if (!texture)
			{
				ChangeSize(true, true);
			}
			else
			{
				ChangeSize();
			}

			D3DRender->SetImage(texture, lastSample->lpData, GS_IMAGEFORMAT_BGRX, linePitch);
			bReadyToDraw = true;

        }
        else if(colorType == DeviceOutputType_I420 || colorType == DeviceOutputType_YV12)
        {
			LPBYTE lpData;
			UINT pitch;

			if (!texture)
			{
				ChangeSize(true, true);
			}
			else
			{
				ChangeSize();
			}


			if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				PackPlanar(lpData, lastSample->lpData, renderCX, renderCY, pitch, 0, renderCY, linePitch, lineShift);
				D3DRender->Unmap(texture);
			}

			bReadyToDraw = true;

        }
        else if(colorType == DeviceOutputType_YVYU || colorType == DeviceOutputType_YUY2)
        {
            LPBYTE lpData;
            UINT pitch;

			if (!texture)
			{
				ChangeSize(true, true);
			}
			else
			{
				ChangeSize();
			}

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

			if (!texture)
			{
				ChangeSize(true, true);
			}
			else
			{
				ChangeSize();
			}

			if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
            {
                Convert422To444(lpData, lastSample->lpData, pitch, false);
				D3DRender->Unmap(texture);
            }

            bReadyToDraw = true;
        }

        lastSample->Release();
    }
}

void PipeVideo::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	//if (m_Server->IsConnected() == false) //当连接不成功时调用该函数
	if (!b_flag || !m_bFirstReceiveData)
	{
		if (bitmapImage)
		{
			Texture *local_texture = bitmapImage->GetTexture();
			if (local_texture)
			{
				D3DRender->DrawSprite(local_texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
			}
		}
		m_LastTimeStamp = 0;
		return;
	}

	m_size = Vect2(-1, -1);

    if(texture && bReadyToDraw)
    {
		float x, x2;
		float y, y2;

		if (bScaleFull)
		{
			x = pos.x;
			y = pos.y;
			x2 = x + size.x;
			y2 = y + size.y;
		}
		else
		{
			y = pos.y;
			y2 = y + size.y;

			DWORD width, height;
			D3DRender->GetTextureWH(texture, width, height);

			if (width > height)
			{
				//等比例缩放
				float aspect = (float)size.x / size.y;

				if (aspect > 1.0f)
				{
					y = pos.y;
					y2 = y + size.y;

					float NewWidth = width * size.y / height;

					x = pos.x + (size.x - NewWidth) / 2;
					x2 = x + NewWidth;

					if (NewWidth > size.x)
					{
						x = pos.x;
						x2 = pos.x + size.x;

						float NewHeight = height * size.x / width;

						y = pos.y + (size.y - NewHeight) / 2;
						y2 = y + NewHeight;
					}
				}
				else
				{
					x = pos.x;
					x2 = pos.x + size.x;

					float NewHeight = height * size.x / width;

					y = pos.y + (size.y - NewHeight) / 2;
					y2 = y + NewHeight;

					if (NewHeight > size.y)
					{
						y = pos.y;
						y2 = y + size.y;

						float NewWidth = width * size.y / height;

						x = pos.x + (size.x - NewWidth) / 2;
						x2 = x + NewWidth;
					}
				}
			}
			else
			{
				int w = size.y * width / height;

				int RenderSide = data["RenderSide"].asInt();

				if (w < size.x)
				{
					if (RenderSide == 0) //居左
					{
						x = pos.x;
						x2 = x + w;
					}
					else if (RenderSide == 1) //居中
					{
						x = pos.x + (size.x - w) / 2;
						x2 = x + w;
					}
					else  //居右
					{
						x = pos.x + size.x - w;
						x2 = x + w;
					}
				}
				else
				{
					x = pos.x;
					x2 = pos.x + size.x;

					float NewHeight = height * size.x / width;

					y = pos.y + (size.y - NewHeight) / 2;
					y2 = y + NewHeight;

					if (NewHeight > size.y)
					{
						y = pos.y;
						y2 = y + size.y;

						float NewWidth = width * size.y / height;

						x = pos.x + (size.x - NewWidth) / 2;
						x2 = x + NewWidth;
					}
				}

			}
		}
		


		if (FilterTexture)
		{
			Shader *oldShader = NULL;
			if (bInterlaceSignal && Vect2(x2, y2) != GetSize())
			{
				oldShader = D3DRender->GetCurrentPixelShader();
				if (RGBFieldShader)
				{
					D3DRender->LoadPixelShader(RGBFieldShader);
				}
			}

			D3DRender->DrawSprite(FilterTexture, 0xFFFFFFFF, x, y, x2, y2);

			if (bInterlaceSignal && Vect2(x2, y2) != GetSize())
			{
				D3DRender->LoadPixelShader(oldShader);
			}
		}
		else
		{
			Shader *oldShader = D3DRender->GetCurrentPixelShader();

			OSEnterMutex(hSampleMutex);//保护colorConvertShader

			if (Vect2(x2, y2) == GetSize() || !bInterlaceSignal)
			{
				if (colorConvertShader)
				{
					D3DRender->LoadPixelShader(colorConvertShader);
					colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
				}
			}
			else if (bInterlaceSignal && colorFieldConvertShader)
			{
				D3DRender->LoadPixelShader(colorFieldConvertShader);
				colorFieldConvertShader->SetFloat(colorFieldConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
			}
			
			OSLeaveMutex(hSampleMutex);

			D3DRender->DrawSprite(texture, 0xFFFFFFFF, x, y, x2, y2);

			D3DRender->LoadPixelShader(oldShader);

		}
		
    }
}

void PipeVideo::UpdateSettings(Value &data)
{
	this->data = data;

	if (bInterlaceSignal != data["ScanInterlace"].asUInt())
	{
		bInterlaceSignal = data["ScanInterlace"].asUInt();

		OSEnterMutex(hSampleMutex);

		ChangeShader();

		OSLeaveMutex(hSampleMutex);

	}
	if (!bCapturing)
	{
		OSEnterMutex(hSampleMutex);
		UnloadFilters();
		LoadFilters();
		OSLeaveMutex(hSampleMutex);
	}

	bCapturing = true;
}

IMPLEMENT_DYNIC(PipeVideo, "互动连接源","1.0.0.1")

void RGB24ToRGB32(char* des, char*pSrcImg, int srcW, int srcH)
{
	for (int i = 0; i < srcH; i++)
	{
		int tSrcH = i;
		for (int j = 0; j < srcW; j++)
		{
			int tSrcW = j;
			*des++ = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3];
			*des++ = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 1];
			*des++ = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 2];
			*des++ = 0xff;
		}
	}
}

void RGB24ToYUV420(int Width, int Height, unsigned char* RgbBuffer, unsigned char*YuvBuffer)
{
	unsigned char * yuvBuf = YuvBuffer;
	int nWidth = Width;
	int nHeight = Height;
	/////////////////////
	int i, j;
	unsigned char*bufY = yuvBuf;
	unsigned char*bufU = yuvBuf + nWidth * nHeight;
	unsigned char*bufV = bufU + (nWidth* nHeight * 1 / 4);
	unsigned char*Y = bufY;
	unsigned char*U = bufU;
	unsigned char*V = bufV;
	unsigned char*bufRGB;

	unsigned char y, u, v, r, g, b, testu, testv;

	if (NULL == RgbBuffer)
	{
		return;
	}
	for (j = 0; j < nHeight; j++)
	{
		bufRGB = RgbBuffer + nWidth * (nHeight - 1 - j) * 3;
		for (i = 0; i < nWidth; i++)
		{
			int pos = nWidth * i + j;
			r = *(bufRGB++);
			g = *(bufRGB++);
			b = *(bufRGB++);
			y = (unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;//16
			v = (unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128; //128          
			u = (unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
			*(bufY++) = max(0, min(y, 255));

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (u > 255)
				{
					u = 255;
				}
				if (u < 0)
				{
					u = 0;
				}
				*(bufU++) = u;
			}
			else
			{
				if (i % 2 == 0)
				{
					if (v > 255)
					{
						v = 255;
					}
					if (v < 0)
					{
						v = 0;
					}
					*(bufV++) = v;
				}
			}
		}
	}
}

PipeVideo::PipeVideo()
{		
	D3DRender = GetD3DRender();
	FILE* f = fopen(".\\img\\PIPE_1024x768.bmp", "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		m_nDefaultImgSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* srcBuff = new char[m_nDefaultImgSize];
		fread(srcBuff, m_nDefaultImgSize, 1, f);

		int offset = *((int*)(srcBuff + 10));

		//m_pDefaultImgbuffer = new char[1024 * 768 * 4];
		//RGB24ToRGB32(m_pDefaultImgbuffer, srcBuff + offset, 1024, 768);

		m_pDefaultImgYUVbuffer = new char[1024 * 768 * 3 / 2];
		RGB24ToYUV420(1024, 768, (unsigned char *)(srcBuff + offset), (unsigned char *)m_pDefaultImgYUVbuffer);

		delete[] srcBuff;
		srcBuff = NULL;

		fclose(f);
	}
	latestVideoSample = NULL;
	bHasPre = false;
	colorConvertShader = NULL;
	colorFieldConvertShader = NULL;
	RGBFieldShader = D3DRender->CreatePixelShaderFromFile(L"shaders/Field_RGB.pShader");
	lpImageBuffer = NULL;
	colorType = DeviceOutputType_I420;
	bCapturing = false;
	bInterlaceSignal = false;
	enteredSceneCount = 0;
	hListMutex = OSCreateMutex();
}

void PipeVideo::SetHasPreProcess(bool bHasPre)
{
	this->bHasPre = bHasPre;
}

bool PipeVideo::GetHasPreProcess() const
{
	return bHasPre;
}

const char* PipeVideo::GetAduioClassName() const
{
	return "PipeAudioSource";
}

IBaseAudio * PipeVideo::GetAudioRender()
{
	return audioOut;
}

void PipeVideo::RegisterDataCallBack(void *Context, DataCallBack pCb)
{
	__DataCallBack DataBack;
	DataBack.Context = Context;
	DataBack.CallBack = pCb;
	OSEnterMutex(hListMutex);
	m_ListCallBack.Add(DataBack);
	OSLeaveMutex(hListMutex);
}

void PipeVideo::UnRegisterDataCallBack(void *Context)
{
	OSEnterMutex(hListMutex);
	for (int i = 0; i < m_ListCallBack.Num(); ++i)
	{
		__DataCallBack &OneCallBack = m_ListCallBack[i];
		if (OneCallBack.Context == Context)
		{
			m_ListCallBack.Remove(i);
			break;
		}
	}
	OSLeaveMutex(hListMutex);
}

#include <locale.h>

std::string ws2s(String ws)
{
	if (ws.IsEmpty())
	{
		return "";
	}
	std::string curLocale = setlocale(LC_ALL, NULL); // curLocale = "C";

	setlocale(LC_ALL, "chs");

	const wchar_t* _Source = ws.Array();
	size_t _Dsize = 2 * ws.Length() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);
	std::string result = _Dest;
	delete[]_Dest;

	setlocale(LC_ALL, curLocale.c_str());

	return result;
}

std::wstring s2ws(const std::string& s)
{
	if (s.size() <= 0)
	{
		return L"";
	}
	setlocale(LC_ALL, "chs");

	const char* _Source = s.c_str();
	size_t _Dsize = s.size() + 1;
	wchar_t *_Dest = new wchar_t[_Dsize];
	wmemset(_Dest, 0, _Dsize);
	mbstowcs(_Dest, _Source, _Dsize);
	std::wstring result = _Dest;
	delete[]_Dest;

	setlocale(LC_ALL, "C");

	return result;
}

CPipeServer* CPipeServer::m_instance = NULL;
CPipeServer::CPipeServer()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer is start", __LINE__, __FUNCTION__);
	m_processID = -1;
	m_iRequestNum = -1;
	SendSize = 0;
	m_bDirectorStop = false;
	m_bHostStop = false;
	m_bStop = false;
	m_DirectorID = -1;
	m_HostID = -1;
	m_bFirstLogDirectVideo = true;
	m_bFirstLogDirectAudio = true;
	m_bFirstLogHostVideo = true;
	m_bFirstLogHostAudio = true;
	m_bFirstStart = true;
	m_hEvent = CreateEvent(NULL, true, false,NULL);
	m_hEventTick = CreateEvent(NULL, true, false, NULL);
	m_hRegisterEvent = NULL;
	DirectorAudioRecord = NULL;
	HostAudioRecord = NULL;
	oldDeviceParam = NULL;
	ThirdAudioRecord = NULL;
}

CPipeServer::~CPipeServer()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer is end", __LINE__, __FUNCTION__);
	
	if (m_hCloseRestartClientThreadEvent)
	{
		int ret = WaitForSingleObject(m_hCloseRestartClientThreadEvent, INFINITE);
		if (WAIT_OBJECT_0 == ret)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --RestartClientThread is Exit!", __LINE__, __FUNCTION__);
		}
		
	}
	if (m_hCloseTickEvent)
	{
		int ret = WaitForSingleObject(m_hCloseTickEvent, INFINITE);
		if (WAIT_OBJECT_0 == ret)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --TickThread is Exit!", __LINE__, __FUNCTION__);
		}
	}
	int iKeepReleaseTime = 2000;
	while (m_iLRefs >0)
	{
		iKeepReleaseTime = iKeepReleaseTime - 200;
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
		Sleep(200);
		if (iKeepReleaseTime < 0)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!仍然大于0，被强制退出", __LINE__,__FUNCTION__, m_iLRefs);
			break;
		}
	}

	if (hPipeVideoMutex)
	{
		OSCloseMutex(hPipeVideoMutex);
	}
	if (m_hRegisterMutex)
	{
		OSCloseMutex(m_hRegisterMutex);
	}
	if (m_hDataStListMutex)
	{
		OSCloseMutex(m_hDataStListMutex);
	}
	if (oldDeviceParam)
	{
		delete oldDeviceParam;
		oldDeviceParam = NULL;
	}
	
	if (m_pSetAudioDirectorMode)
	{
		delete m_pSetAudioDirectorMode;
		m_pSetAudioDirectorMode = NULL;
	}
	if (m_pSetAudioHostMode)
	{
		delete m_pSetAudioHostMode;
		m_pSetAudioHostMode = NULL;
	}
	if (m_pSetAudioThird)
	{
		delete m_pSetAudioThird;
		m_pSetAudioThird = NULL;
	}
	if (m_pVideoHostTransMode)
	{
		OSCloseMutex(m_pVideoHostTransMode->mutex_lock);
		delete m_pVideoHostTransMode;
		m_pVideoHostTransMode = NULL;
		
	}
	if (m_pVideoDirectorTransMode)
	{
		OSCloseMutex(m_pVideoDirectorTransMode->mutex_lock);
		delete m_pVideoDirectorTransMode;
		m_pVideoDirectorTransMode = NULL;

	}
	if (m_pAudioDirectorTransMode)
	{
		OSCloseMutex(m_pAudioDirectorTransMode->mutex_lock);
		delete m_pAudioDirectorTransMode;
		m_pAudioDirectorTransMode = NULL;
		
	}
	if (m_pAudioHostTransMode)
	{
		OSCloseMutex(m_pAudioHostTransMode->mutex_lock);
		delete m_pAudioHostTransMode;
		m_pAudioHostTransMode = NULL;
	}
		if (m_pAudioThirdTransMode)
	{
		OSCloseMutex(m_pAudioThirdTransMode->mutex_lock);
		delete m_pAudioThirdTransMode;
		m_pAudioThirdTransMode = NULL;
	}
	if (m_pVideoThirdTransMode)
	{
		OSCloseMutex(m_pVideoThirdTransMode->mutex_lock);
		delete m_pVideoThirdTransMode;
		m_pVideoThirdTransMode = NULL;
	}
	if (m_pPipeControl)
	{
		destroy_pipe_control(m_pPipeControl);
	}
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
	}
	if (m_hEventTick)
	{
		CloseHandle(m_hEventTick);
	}
	if (m_hRegisterEvent)
	{
		CloseHandle(m_hRegisterEvent);
	}
	if (timeThread)
	{
		CloseHandle(timeThread);
	}
	if (m_RestartClientThread)
	{
		CloseHandle(m_RestartClientThread);
	}
	if (m_hCloseTickEvent)
	{
		CloseHandle(m_hCloseTickEvent);
	}
	if (m_hCloseRestartClientThreadEvent)
	{
		CloseHandle(m_hCloseRestartClientThreadEvent);
	}
}

CPipeServer* CPipeServer::GetInstance()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  being called", __LINE__, __FUNCTION__);
	if (m_instance == NULL)
	{
		m_instance = new CPipeServer;
	}
	return m_instance;
}

void CPipeServer::DestroyInstance()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer destory", __LINE__, __FUNCTION__);
	if (m_instance)
		delete m_instance;
	m_instance = NULL;
}

bool STDCALL CSleepToNS(QWORD qwNSTime)
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
			Log::writeMessage(LOG_RTSPSERV, 1, "Tried to sleep for %u seconds, that can't be right! Triggering breakpoint.", milliseconds);
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

void CPipeServer::Tick()
{
	QWORD start = GetQPCNS();
	//QWORD timeNS = 1000000000/2;

	while (!bShutdownTickThread)
	{
		//CSleepToNS(start += timeNS);
		int ret = WaitForSingleObject(m_hEventTick, 500);
		if (WAIT_OBJECT_0 == ret)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer EnventTick is being Set,Tick Thread should release , thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
			SetEvent(m_hCloseTickEvent);
			return;
		}
		
		if (!bShutdownTickThread)
		{
			OSEnterMutex(hPipeVideoMutex);

			std::list<PipeVideo*>::iterator pos = PipeRenders.begin();
			std::list<PipeVideo*>::iterator end = PipeRenders.end();
			for (; pos != end; ++pos)
			{
				if (!(*pos)->b_flag)
				{
					(*pos)->ReceiveState(false);
				}
			}

			OSLeaveMutex(hPipeVideoMutex);
		}
	}
}

DWORD STDCALL CPipeServer::RestartClientThread(LPVOID lpUnused)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer RestartClientThread Start, thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
	int ret = WaitForSingleObject(m_instance->m_hEvent, 10000);
	if (WAIT_OBJECT_0 == ret)
	{
		//事件被置为有效，CPipeserver需要析构
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Envent is being Set,PipeSever should release , thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
		SetEvent(m_instance->m_hCloseRestartClientThreadEvent);
		return 0;
	}
	else if (WAIT_FAILED == ret)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer WaitForSingleObject Envent is error ,getlasterror = %d , thread_ID = %d", __LINE__, __FUNCTION__, GetLastError(), GetCurrentThreadId());
	}
	while (true)
	{
		/*Sleep(3000);*/
		int ret = WaitForSingleObject(m_instance->m_hEvent, 3000);
		if (WAIT_OBJECT_0 == ret)
		{
			//事件被置为有效，CPipeserver需要析构
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Envent is being Set,PipeSever should release , thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
			SetEvent(m_instance->m_hCloseRestartClientThreadEvent);
			return 0;
		}
		else if (WAIT_FAILED == ret)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer WaitForSingleObject Envent is error ,getlasterror = %d , thread_ID = %d", __LINE__, __FUNCTION__, GetLastError(), GetCurrentThreadId());
		}
		bool bFlag = m_instance->CheckProcess(m_instance->m_ProcessName.c_str());
		if (!bFlag)
		{
			//不存在进程
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Restart Interactive, thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
			if (m_instance->m_bRestartInteractive)
				m_instance->Restart();
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer m_bRestartInteractive is false,thread is should be killed by processEnd , thread_ID = %d", __LINE__,__FUNCTION__, GetCurrentThreadId());
			}
		}
	}
	return 0;
}

int  CPipeServer::Start(Value &data) {

	if (!m_bFirstStart)
	{
		return 0;
	}
	m_bFirstStart = false;
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer First Start", __LINE__, __FUNCTION__);
	hPipeVideoMutex = OSCreateMutex();
	m_hRegisterMutex = OSCreateMutex();
	m_hDataStListMutex = OSCreateMutex();
	m_pSetAudioDirectorMode = new StSetAudioMode;
	m_pSetAudioHostMode = new StSetAudioMode;
	m_pSetAudioThird = new StSetAudioMode;

	m_pVideoHostTransMode = new StModerDataTransMode;
	m_pVideoHostTransMode->mutex_lock = OSCreateMutex();

	m_pVideoDirectorTransMode = new StModerDataTransMode;
	m_pVideoDirectorTransMode->mutex_lock = OSCreateMutex();

	m_pVideoThirdTransMode = new StModerDataTransMode;
	m_pVideoThirdTransMode->mutex_lock = OSCreateMutex();

	m_pAudioDirectorTransMode = new StModerDataTransMode;
	m_pAudioDirectorTransMode->mutex_lock = OSCreateMutex();

	m_pAudioHostTransMode = new StModerDataTransMode;
	m_pAudioHostTransMode->mutex_lock = OSCreateMutex();

	m_pAudioThirdTransMode = new StModerDataTransMode;
	m_pAudioThirdTransMode->mutex_lock = OSCreateMutex();

	m_pPipeControl = create_pipe_control(this);
// 	ConfigFile  *AppConfig = BLiveGetProfileConfig();
// 	String strIni;
// 	ConfigFile * Gloable = BLiveGetGlobalConfig();
// 	if (NULL == Gloable)
// 	{
// 		AppConfig = NULL;
// 		Log(TEXT("管道打开互动机，无法打开全局配置"));
// 		return -1;
// 	}
	m_bLogFlag =  0;
	m_iCountFrame = 250;

	InteractiveNumber = L"91400015";

	if (!data["NubeNum"].isNull())
	{
		InteractiveNumber = Asic2WChar(data["NubeNum"].asString().c_str()).c_str();
	}
	Appkey = L"7a1ca063929c40eba3be799f953f3f54";
	if (!data["AppKey"].isNull())
	{
		Appkey = Asic2WChar(data["AppKey"].asString().c_str()).c_str();
	}

	NickName = L"Test";
	if (!data["NickName"].isNull())
	{
		NickName = Asic2WChar(data["NickName"].asString().c_str()).c_str();
	}

	strIneractionPara = L"";
	strIneractionPara = strIneractionPara + L"AppKey=" + Appkey;
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"NubeNum="+InteractiveNumber;
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"UID="+InteractiveNumber;
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"NickName=" + NickName;//L"Test";

	exePath = L".\\InteractionClient\\InteractionClient.exe";

	if (!data["InteractionPath"].isNull())
	{
		exePath = Asic2WChar(data["InteractionPath"].asString().c_str()).c_str();
	}

	if (exePath.Length() == 0)
	{
		Log::writeError(LOG_RTSPSERV,1,"InteractionPath run fail");
		return -1;
	}
	std::wstring PATH = exePath.Array();
	int pos = PATH.find_last_of('\\');
	m_ProcessName = PATH.substr(pos+1);
	Log::writeMessage(LOG_RTSPSERV, 1, "管道打开互动机%s para %s", WcharToAnsi(exePath.Array()).c_str(), WcharToAnsi(strIneractionPara.Array()).c_str());
	char *CStr = "\\\\.\\Pipe\\Butel_PipeName";                //先监听，后启动进程
	m_pPipeControl->listen_pipe(CStr, 1000, 0, 0);
	ShellExecute(NULL, L"open", exePath.Array(), strIneractionPara, 0, SW_SHOWNORMAL);
	
	m_RestartClientThread = OSCreateThread((XTHREAD)CPipeServer::RestartClientThread, NULL); //启动server就开始
	m_hCloseRestartClientThreadEvent = CreateEvent(NULL, true, false, NULL);//用于释放RestartClientThread
	timeThread = OSCreateThread((XTHREAD)CPipeServer::TickThread, this);
	m_hCloseTickEvent = CreateEvent(NULL, true, false, NULL);//用于释放TickThread

	return 0;
}

DWORD STDCALL CPipeServer::ReleaseThread(LPVOID lpUnused)
{
	m_instance->m_pAudioDirectorTransMode->bflag = false;
	m_instance->m_pAudioHostTransMode->bflag = false;
	m_instance->m_pAudioThirdTransMode->bflag = false;

	m_instance->m_pVideoDirectorTransMode->bflag = false;
	m_instance->m_pVideoHostTransMode->bflag = false;
	m_instance->m_pVideoThirdTransMode->bflag = false;

	m_instance->m_pAudioDirectorTransMode->id = -1;
	m_instance->m_pAudioHostTransMode->id = -1;
	m_instance->m_pAudioThirdTransMode->id = -1;

	m_instance->m_pVideoDirectorTransMode->id = -1;
	m_instance->m_pVideoHostTransMode->id = -1;
	m_instance->m_pVideoThirdTransMode->id = -1;

	m_instance->m_bHostStop = true;
	m_instance->m_bDirectorStop = true;

	if (m_instance->DirectorAudioRecord)
	{
		m_instance->DirectorAudioRecord->RecordStop();
		delete m_instance->DirectorAudioRecord;
		m_instance->DirectorAudioRecord = NULL;
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer DirectorAudioRecord is release thread_ID = %d", __LINE__,__FUNCTION__, GetCurrentThreadId());
	}

	if (m_instance->HostAudioRecord)
	{
		m_instance->HostAudioRecord->RecordStop();
		delete m_instance->HostAudioRecord;
		m_instance->HostAudioRecord = NULL;
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostAudioRecord is release thread_ID = %d", __LINE__, __FUNCTION__, GetCurrentThreadId());
	}

	if (m_instance->ThirdAudioRecord)
	{
		m_instance->ThirdAudioRecord->RecordStop();
		delete m_instance->ThirdAudioRecord;
		m_instance->ThirdAudioRecord = NULL;
		Log(TEXT("line: %d   func:%s,  --PipeServer CommandCenterAudioRecord is release thread_ID = %d"), __LINE__, String(__FUNCTION__).Array(), GetCurrentThreadId());
	}

	if (m_instance->oldDeviceParam)
	{
		m_instance->DeviceDirector.UnRegisterSharedDevice();
		m_instance->DeviceHost.UnRegisterSharedDevice();
		//增加指挥中心
		m_instance->DeviceCenter.UnRegisterSharedDevice();

		delete m_instance->oldDeviceParam;
		m_instance->oldDeviceParam = NULL;
	}
	
	return 0;
}

void CPipeServer::Restart()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer ReInit Interactive Param thread_ID = %d", __LINE__,__FUNCTION__, GetCurrentThreadId());
	Sleep(500);
	if (oldDeviceParam)
	{
		delete oldDeviceParam;
		oldDeviceParam = NULL;
	}
	m_DirectorID = -1;
	m_HostID = -1;
	m_ThirdID = -1;
	m_bFirstLogDirectVideo = true;
	m_bFirstLogDirectAudio = true;
	m_bFirstLogHostVideo = true;
	m_bFirstLogHostAudio = true;
	m_bFirstLogThirdAudio = true;
	m_bFirstLogThirdVideo = true;
	m_pModerDataTransModeList.clear();
	m_bHostStop = false; 
	m_bDirectorStop = false;

	m_bLogFlag = 0;

	if (exePath.Length() == 0)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "InteractionPath run fail");
		return;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "管道打开互动机%s para %s", WcharToAnsi(exePath.Array()).c_str(), WcharToAnsi(strIneractionPara.Array()).c_str());
	ShellExecute(NULL, L"open", exePath.Array(), strIneractionPara, 0, SW_SHOWNORMAL);
	char *CStr = "\\\\.\\Pipe\\Butel_PipeName";
	m_pPipeControl->listen_pipe(CStr, 1000, 0, 0);
}

void CPipeServer::Stop() {

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Stop, get m_hDataStListMutex,m_pModerDataTransModeList size = %d, thread_ID = %d",
		__LINE__, __FUNCTION__, m_pModerDataTransModeList.size(), GetCurrentThreadId());

	OSEnterMutex(m_hDataStListMutex);
	std::list<AIOID>::iterator iter = m_pModerDataTransModeList.begin();
	for (; iter != m_pModerDataTransModeList.end(); iter++)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer own stop ,close_pipe %d ,thread_ID = %d", __LINE__, __FUNCTION__, *iter, GetCurrentThreadId());
		m_pPipeControl->close_pipe(*iter);  //主动关闭
	}
	m_pModerDataTransModeList.clear();  //清空列表
	OSLeaveMutex(m_hDataStListMutex); //这里只有信令导播和主持人三种管道id

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Stop, leave m_hDataStListMutex, m_pModerDataTransModeList size = %d, thread_ID = %d",
		__LINE__, String(__FUNCTION__).Array(), m_pModerDataTransModeList.size(),GetCurrentThreadId());

	delete m_instance;
	m_instance = NULL;
}

void CPipeServer::on_pipe_accept(AsynIoErr st, const char *pipename, AIOID id, ULL64 ctx1, ULL64 ctx2){

	if (AIO_SUC == st)
	{

		AddRef();
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
		StModerDataTransMode *pStModerDataTransMode = new StModerDataTransMode;
		pStModerDataTransMode->id = id;
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer pipe_accept get m_hDataStListMutex AIOID  =%d,m_pModerDataTransModeList size =%d, thread_ID = %d",
			__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(), GetCurrentThreadId());
		OSEnterMutex(m_hDataStListMutex);
		m_pModerDataTransModeList.push_back(id);
		OSLeaveMutex(m_hDataStListMutex);
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer pipe_accept leave m_hDataStListMutex AIOID  =%d ,m_pModerDataTransModeList size =%d,thread_ID = %d",
			__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(), GetCurrentThreadId());
		pStModerDataTransMode->AddRef();
		m_pPipeControl->asyn_read_pipe(id, pStModerDataTransMode->receive_buffer,
			pStModerDataTransMode->recive_len, (ULL64)pStModerDataTransMode, 0);
	}
}

void CPipeServer::on_pipe_write(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2) 
{
	StModerDataTransMode *pModerDataTransMode = (StModerDataTransMode*)ctx1;
	pModerDataTransMode->SubRef();//原子操作不需要锁
	//如果最后关闭的时候此项系数不为0，那么将不允许关闭

	if (st == AIO_SUC)
	{
		if (pModerDataTransMode->type == ParamSet || pModerDataTransMode->type == HostMode)   //后续会读取数据
		{
			if (!pModerDataTransMode->bHasAPipeVideoRender) //没有可用的互动链接源进行渲染，自动关闭管道并删除.
			{
				m_pPipeControl->close_pipe(id);
				OSEnterMutex(m_hDataStListMutex);
				std::list<AIOID>::iterator iter_Host = m_pModerDataTransModeList.begin();
				for (; iter_Host != m_pModerDataTransModeList.end(); iter_Host++)
				{
					if (*iter_Host == id)
					{
						m_pModerDataTransModeList.erase(iter_Host);    //更新id列表
						Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --没有可用互动源了,erase a id from m_pModerDataTransModeList id = %d, size = %d",
							__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size());
						break;
					}
				}
				OSLeaveMutex(m_hDataStListMutex);

				if (pModerDataTransMode->LRefs == 0)
				{
					SubRef();
					delete pModerDataTransMode;    
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__, id, m_iLRefs);
				}
				else
				{
					SubRef();
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
						__LINE__, __FUNCTION__, id, pModerDataTransMode->LRefs, GetCurrentThreadId());
					delete pModerDataTransMode;   
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);

				}
			}
			else
			{
				pModerDataTransMode->AddRef();
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,id = %d, 投递了一个读命令", __LINE__, __FUNCTION__, id);
				m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer,
					pModerDataTransMode->recive_len, (ULL64)pModerDataTransMode, 0);
			}
			
		}
		else if (pModerDataTransMode->type == HostAVRequest || pModerDataTransMode->type == DirectorAVRequest || pModerDataTransMode->type == CommandCenterAVRequest)//发送音视频数据直接返回
		{
			
			if (pModerDataTransMode->Datatype != AudioData && pModerDataTransMode->Datatype != VideoData)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer release ModerDataTransMode AIOID  %d ,type = %d,thread_ID = %d",
					__LINE__, __FUNCTION__, id, pModerDataTransMode->type, GetCurrentThreadId());
				if (pModerDataTransMode->LRefs == 0)
				{
					SubRef();
					delete pModerDataTransMode;    //第一次管道对象删除
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__, id, m_iLRefs);
				}
				else
				{
					SubRef();
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
						__LINE__, __FUNCTION__, id, pModerDataTransMode->LRefs, GetCurrentThreadId());
					delete pModerDataTransMode;    //第一次管道对象删除
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
					
				}

			}
			else
			{
				
				OSEnterMutex(pModerDataTransMode->mutex_lock);
				pModerDataTransMode->refs--;
				OSLeaveMutex(pModerDataTransMode->mutex_lock);
				if (m_bLogFlag)
				{
				
					if (pModerDataTransMode->Datatype == VideoData)
					{
						if (pModerDataTransMode->type == HostAVRequest)
						{
							SentHostTimeCurrent++;
							if (SentHostTimeCurrent - SentHostTimeLast > m_iCountFrame)
							{
								SentHostTimeLast = SentHostTimeCurrent;
								Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer send Host Video OK!,id = %d,ref = %d",
									__LINE__, __FUNCTION__, pModerDataTransMode->id, pModerDataTransMode->refs);
							}
						}
					
					}
					else if (pModerDataTransMode->type == DirectorAVRequest)
					{
						SendDirectTimeCurrent++;
						if (SendDirectTimeCurrent - SendDirectTimeLast > m_iCountFrame)
						{
							SendDirectTimeLast = SendDirectTimeCurrent;
							Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer send Direct Video OK!,id = %d,refs = %d",
								__LINE__, __FUNCTION__, pModerDataTransMode->id, pModerDataTransMode->refs);
						}
					}
				}

			}
				
			return;
		}
		else if (pModerDataTransMode->type == AddPipeVideoCommand || pModerDataTransMode->type == DelPipeVideoCommand || pModerDataTransMode->type == RenamePipeVideoCommand)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "发送 %d 命令成功 消息 %s", pModerDataTransMode->type,pModerDataTransMode->send_buffer + sizeof MSGHeader);
			delete pModerDataTransMode;
			pModerDataTransMode = NULL;
		}
	} 
	else
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer pipe_write faild AIOID  %d ,type = %d,thread_ID = %d",
			__LINE__, __FUNCTION__, id, pModerDataTransMode->Datatype, GetCurrentThreadId());

		pModerDataTransMode->bflag = false;   //写管道失败不释放
		//并不在这里关闭管道，因为即便是非管道关闭，管道其它问题引起的发送数据失败，也只是到达100帧后停止发送，不在这里关闭管道
		if ((pModerDataTransMode->type == HostAVRequest || pModerDataTransMode->type == DirectorAVRequest) &&
			(pModerDataTransMode->Datatype == AudioData || pModerDataTransMode->Datatype == VideoData))
		{
			//导播和主持人发送数据才有计数器
			OSEnterMutex(pModerDataTransMode->mutex_lock);
			pModerDataTransMode->refs--;
			OSLeaveMutex(pModerDataTransMode->mutex_lock);
		}
		if (m_processID == id)
		{

			ResetInitParam();
			if (pModerDataTransMode->type == ParamSet)
			{
				if (pModerDataTransMode->LRefs == 0)
				{
					SubRef();
					delete pModerDataTransMode;    //第一次管道对象删除
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__, id, m_iLRefs);
					ReleaseThread(NULL);
					return;
				}
				else
				{
					SubRef();
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
						__LINE__, __FUNCTION__, id, pModerDataTransMode->LRefs, GetCurrentThreadId());
					delete pModerDataTransMode;    //第一次管道对象删除
					pModerDataTransMode = NULL;
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
					ReleaseThread(NULL);
					return;
				}
			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "发送 %d 命令失败 消息 %s",pModerDataTransMode->type,pModerDataTransMode->send_buffer + sizeof MSGHeader);
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				return;
			}
		}

		if (pModerDataTransMode->type == HostMode)
		{
			m_pPipeControl->close_pipe(id);
			OSEnterMutex(m_hDataStListMutex);
			std::list<AIOID>::iterator iter_Host = m_pModerDataTransModeList.begin();
			for (; iter_Host != m_pModerDataTransModeList.end(); iter_Host++)
			{
				if (*iter_Host == id)
				{
					m_pModerDataTransModeList.erase(iter_Host);    //更新id列表
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --没有可用互动源了,erase a id from m_pModerDataTransModeList id = %d, size = %d",
						__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size());
					break;
				}
			}
			OSLeaveMutex(m_hDataStListMutex);

			if (pModerDataTransMode->LRefs == 0)
			{
				SubRef();
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__ ,id, m_iLRefs);
			}
			else
			{
				SubRef();
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
					__LINE__,__FUNCTION__, id, pModerDataTransMode->LRefs, GetCurrentThreadId());
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func : %s, --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);

			}
		}
	}
}

void CPipeServer::on_pipe_read(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2){

	StModerDataTransMode * pModerDataTransMode = (StModerDataTransMode *)ctx1;
	pModerDataTransMode->SubRef(); 

	if (AIO_SUC == st)
	{
		if (pModerDataTransMode->bFirst)
		{
			AnalyzeCommand(id, pModerDataTransMode->receive_buffer, retlen, pModerDataTransMode->send_buffer, 
				pModerDataTransMode->send_len, pModerDataTransMode);
			pModerDataTransMode->AddRef();
			MsgType TemMsgType = pModerDataTransMode->type;
			m_pPipeControl->asyn_write_pipe(id, pModerDataTransMode->send_buffer, pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);

			//这里asyn_write_pipe 可能会失败，导致在on_write_pipe时删除pModerDataTransMode，再用pModerDataTransMode崩溃
			if (TemMsgType == ParamSet) //说明是信令管道
			{
				//遍历当前互动源列表发送互动源名字
				ProcessNameCommand(NULL, NULL, AddPipeVideoCommand, true);
			}
		}
		else
		{
			AnalyzeHostModeCommand(id, buf, retlen, pModerDataTransMode);
		}
	}
	else if (AIO_FAILED == st || AIO_TIMEOUT == st)
	{
		if (m_processID == id)  //信令管道被关闭
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer m_processPipe faild AIOID  %d,thread_ID = %d",
				__LINE__, __FUNCTION__, id,GetCurrentThreadId());
			if (pModerDataTransMode->LRefs == 0)
			{
				SubRef();
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__, id, m_iLRefs);
			}
			else
			{
				SubRef();
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
					__LINE__,__FUNCTION__, id, pModerDataTransMode->LRefs, GetCurrentThreadId());
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
				
			}


			if (m_bStop)        //主动关闭，之前已经关闭了所有id，清空了m_pModerDataTransModeList
			{
				//do nothing!
			}
			else
			{
				ReleaseThread(NULL);
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_processID = %d, m_pModerDataTransModeList size = %d, get m_hDataStListMutex ,thread_ID=%d",
					__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(), GetCurrentThreadId());
				OSEnterMutex(m_hDataStListMutex);
				std::list<AIOID>::iterator iter = m_pModerDataTransModeList.begin();
				for (; iter != m_pModerDataTransModeList.end(); iter++)
				{
					if (*iter == id)
					{
						m_pModerDataTransModeList.erase(iter);    //更新id列表
						Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_processID = %d, m_pModerDataTransModeList size = %d, thread_ID=%d",
							__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(),GetCurrentThreadId());
						m_pPipeControl->close_pipe(id);   //不是主动关闭
						break;
					}
					
				}

				iter = m_pModerDataTransModeList.begin();
				for (; iter != m_pModerDataTransModeList.end(); iter++)
				{
					if (*iter == m_HostID)
					{
						m_pModerDataTransModeList.erase(iter);    //更新id列表
						Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_HostID = %d, m_pModerDataTransModeList size = %d, thread_ID=%d",
							__LINE__, __FUNCTION__, m_HostID, m_pModerDataTransModeList.size(), GetCurrentThreadId());
						m_pPipeControl->close_pipe(m_HostID);
						break;
					}
				}

				iter = m_pModerDataTransModeList.begin();
				for (; iter != m_pModerDataTransModeList.end(); iter++)
				{
					if (*iter == m_DirectorID)
					{
						m_pModerDataTransModeList.erase(iter);    //更新id列表
						Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_DirectorID = %d, m_pModerDataTransModeList size = %d, thread_ID=%d",
							__LINE__, __FUNCTION__, m_DirectorID, m_pModerDataTransModeList.size(), GetCurrentThreadId());
						m_pPipeControl->close_pipe(m_DirectorID); //被动关闭主播和导播id但是不需要释放
						break;
					}
				}

				iter = m_pModerDataTransModeList.begin();
				for (; iter != m_pModerDataTransModeList.end(); iter++)
				{
					if (*iter == m_ThirdID)
					{
						m_pModerDataTransModeList.erase(iter);    //更新id列表
						Log(TEXT("line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_ThirdID = %d, m_pModerDataTransModeList size = %d, thread_ID=%d"),
							__LINE__, String(__FUNCTION__).Array(), m_ThirdID, m_pModerDataTransModeList.size(), GetCurrentThreadId());
						m_pPipeControl->close_pipe(m_ThirdID); //被动关闭主播和导播id但是不需要释放
						break;
					}
				}
				OSLeaveMutex(m_hDataStListMutex);

				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer  对方关闭互动机,erase  m_processID = %d, m_pModerDataTransModeList size = %d, leave m_hDataStListMutex ,thread_ID=%d",
					__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(), GetCurrentThreadId());
			}

			ResetInitParam();
		}
		else if (pModerDataTransMode->type == HostMode) //主持人模式,此模式数据读取失败不需要关闭管道,但是需要移除
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer on_pipe_read get hPipeVideoMutex ,RenderMap.size = %d,PipeRenders size =%d,thread_id = %d",
				__LINE__, __FUNCTION__, RenderMap.size(), PipeRenders.size(), GetCurrentThreadId());

			OSEnterMutex(hPipeVideoMutex);
			if (RenderMap.find(id) != RenderMap.end())
			{
				RenderMap.erase(RenderMap.find(id));
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 对方关闭一个主持人模式会话 ,erase a PipeVideo from RenderMap,id =%d,RenderMap size = %d,thread_id = %d",
					__LINE__, __FUNCTION__, id, RenderMap.size(), GetCurrentThreadId());
			}

			Log::writeMessage(LOG_RTSPSERV, 1, "主持人模式关闭 id = %llu", id);
			std::list<PipeVideo*>::iterator iter;
			for (iter = PipeRenders.begin(); iter != PipeRenders.end(); iter++)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "(*iter)->m_aioId %llu", (*iter)->m_aioId);
				if ((*iter)->m_aioId == id)
				{
					(*iter)->m_aioId = -1;
					(*iter)->b_flag = false;
					(*iter)->m_bFirstReceiveData = false;
					(*iter)->m_NickName = L"NULL";
					(*iter)->FlushSamples();
					std::stringstream SourceId;
					uint64_t InstanceID = 0;
					if (!(*iter)->data["InstanceID"].isNull())
					{
						SourceId << (*iter)->data["InstanceID"].asString().c_str();
						SourceId >> InstanceID;
					}
					NickNameCallBack NameCb = (NickNameCallBack)GetNickNameCallBack();
					if (NameCb)
					{
						NameCb(InstanceID, (uint64_t)(*iter), NULL);
					}
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer PipeRenders ,release a pos, id = %d,PipeRenders size = %d,thread_id = %d",
						__LINE__, __FUNCTION__, id, PipeRenders.size(),GetCurrentThreadId());
					break;
				}
			}
			OSLeaveMutex(hPipeVideoMutex);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer on_pipe_read leave hPipeVideoMutex ,RenderMap.size = %d,PipeRenders size =%d,thread_id = %d",
				__LINE__,__FUNCTION__, RenderMap.size(), PipeRenders.size(), GetCurrentThreadId());

			
			
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 对方关闭HostMode close get m_hDataStListMutex id = %d,  m_pModerDataTransModeList size = %d,thread_ID=%d",
				__LINE__, __FUNCTION__, id,m_pModerDataTransModeList.size(), GetCurrentThreadId());

			OSEnterMutex(m_hDataStListMutex);
			std::list<AIOID>::iterator iter_Host = m_pModerDataTransModeList.begin();
			for (; iter_Host != m_pModerDataTransModeList.end(); iter_Host++)
			{
				if (*iter_Host == id)
				{
					m_pModerDataTransModeList.erase(iter_Host);    //更新id列表
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode 对方关闭,erase a id from m_pModerDataTransModeList id = %d, size = %d",
						__LINE__, __FUNCTION__, id,m_pModerDataTransModeList.size());
					break;
				}
			}
			OSLeaveMutex(m_hDataStListMutex);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode close leave m_hDataStListMutex id = %d,  m_pModerDataTransModeList size = %d,thread_ID=%d",
				__LINE__, __FUNCTION__, id, m_pModerDataTransModeList.size(), GetCurrentThreadId());

			if (pModerDataTransMode->LRefs == 0)
			{
				SubRef();
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  正常删除id = %d,--m_iLRefs = %d!", __LINE__, __FUNCTION__, id, m_iLRefs);
			}
			else
			{
				SubRef();
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 已经关闭 id = %d,pModerDataTransMode->LRefs =  %d,应该为0，删除可能会崩溃，thread_ID = %d",
					__LINE__, __FUNCTION__, id,pModerDataTransMode->LRefs, GetCurrentThreadId());
				delete pModerDataTransMode;
				pModerDataTransMode = NULL;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --m_iLRefs = %d!", __LINE__, __FUNCTION__, m_iLRefs);
				
			}

		}
	}
}

void CPipeServer::AudioProcess(const std::wstring&  DirectorAudioCapture, const std::wstring&   PresenterAudioCapture, const std::wstring&  ThirdAudioCapture)
{
	if (DirectorAudioRecord)
	{
		//这里不用调用delete 因为RecordStop时应将自我删除了
		if (wcscmp(DirectorAudioRecord->GetAuidoName().c_str(), DirectorAudioCapture.c_str()) != 0)
		{
			DirectorAudioRecord->RecordStop();
			delete DirectorAudioRecord;
			DirectorAudioRecord = NULL;
		}
	}
	if (HostAudioRecord)
	{
		if (wcscmp(HostAudioRecord->GetAuidoName().c_str(), PresenterAudioCapture.c_str()) != 0)
		{
			HostAudioRecord->RecordStop();
			delete HostAudioRecord;
			HostAudioRecord = NULL;
		}
	}
	if (ThirdAudioRecord)
	{
		if (wcscmp(ThirdAudioRecord->GetAuidoName().c_str(), ThirdAudioCapture.c_str()) != 0)
		{
			ThirdAudioRecord->RecordStop();
			delete ThirdAudioRecord;
			ThirdAudioRecord = NULL;
		}
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Start Audio name1 %s, name2 %s",
		__LINE__, __FUNCTION__, String(DirectorAudioCapture.c_str()).Array(), String(PresenterAudioCapture.c_str()).Array());

	if (!DirectorAudioRecord)
	{
		m_pSetAudioDirectorMode->type = DirectorAVRequest;
		DirectorAudioRecord = new PCMRecord(DirectorAudioCapture, m_pSetAudioDirectorMode);
		int ret = DirectorAudioRecord->InitParam(m_nSamplesRate, m_nAudioChannel, m_nBitPerSample);
		if (ret < 0)
		{
			delete DirectorAudioRecord;
			DirectorAudioRecord = NULL;
			Log(TEXT("line: %d   func:%s,  --PipeServer Start Audio name %s, 失败!!!"),
				__LINE__, String(__FUNCTION__).Array(), String(DirectorAudioCapture.c_str()).Array());
		}
		else
		{
			DirectorAudioRecord->SetCallback(ReceiveMediaSample);
			DirectorAudioRecord->RecordStart();
		}
	}
	if (!HostAudioRecord)
	{
		m_pSetAudioHostMode->type = HostAVRequest;
		HostAudioRecord = new PCMRecord(PresenterAudioCapture, m_pSetAudioHostMode);
		int ret = HostAudioRecord->InitParam(m_nSamplesRate, m_nAudioChannel, m_nBitPerSample);
		if (ret < 0)
		{
			delete HostAudioRecord;
			HostAudioRecord = NULL;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Start Audio name %s, 失败!!!",
				__LINE__, String(__FUNCTION__).Array(), String(PresenterAudioCapture.c_str()).Array());
		}
		else
		{
			HostAudioRecord->SetCallback(ReceiveMediaSample);
			HostAudioRecord->RecordStart();
		}
	}


	if (!ThirdAudioRecord)
	{
		m_pSetAudioThird->type = CommandCenterAVRequest;
		ThirdAudioRecord = new PCMRecord(ThirdAudioCapture, m_pSetAudioThird);
		int ret = ThirdAudioRecord->InitParam(m_nSamplesRate, m_nAudioChannel, m_nBitPerSample);
		if (ret < 0)
		{
			delete ThirdAudioRecord;
			ThirdAudioRecord = NULL;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Start Audio name %s, 失败!!!",
				__LINE__, String(__FUNCTION__).Array(), String(ThirdAudioCapture.c_str()).Array());
		}
		else
		{
			ThirdAudioRecord->SetCallback(ReceiveMediaSample);
			ThirdAudioRecord->RecordStart();
		}
	}
}

void CPipeServer::ReceiveMediaSample(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data)
{
	if (m_instance->m_bStop)
	{
		return;
	}
	StSetAudioMode *pSetAudioMode = (StSetAudioMode*)data;
	StModerDataTransMode *pModerDataTransMode = NULL;

	if (pSetAudioMode->type == HostAVRequest){

		if (m_instance->m_bFirstLogHostAudio)
		{
			m_instance->m_bFirstLogHostAudio = false;
			Log::writeMessage(LOG_RTSPSERV,1,"line: %d   func:%s,  --PipeServer first send Host Audio nChannels = %d, nSamplesPerSec = %d, wBitsPerSample = %d,len = %d",
				__LINE__, __FUNCTION__, info.nChannels, info.nSamplesPerSec, info.wBitsPerSample,len);
		}
		pModerDataTransMode = m_instance->m_pAudioHostTransMode;
	}
	else if (pSetAudioMode->type == DirectorAVRequest)
	{
		if (m_instance->m_bFirstLogDirectAudio)
		{
			m_instance->m_bFirstLogDirectAudio = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Director Audio nChannels = %d, nSamplesPerSec = %d, wBitsPerSample = %d,len = %d",
				__LINE__,__FUNCTION__, info.nChannels, info.nSamplesPerSec, info.wBitsPerSample, len);
		}
		pModerDataTransMode = m_instance->m_pAudioDirectorTransMode;
	}
	else if (pSetAudioMode->type == CommandCenterAVRequest)
	{
		if (m_instance->m_bFirstLogThirdAudio)
		{
			m_instance->m_bFirstLogThirdAudio = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Third Audio nChannels = %d, nSamplesPerSec = %d, wBitsPerSample = %d,len = %d",
				__LINE__, String(__FUNCTION__).Array(), info.nChannels, info.nSamplesPerSec, info.wBitsPerSample, len);
		}
		pModerDataTransMode = m_instance->m_pAudioThirdTransMode;
	}else
		return;

	OSEnterMutex(pModerDataTransMode->mutex_lock);

	if (!pModerDataTransMode->bflag || pModerDataTransMode->refs > 100) {
		if (pModerDataTransMode->refs > 100)
		{
			if (pModerDataTransMode->bFirstRefsMoreThan100Log)
			{
				pModerDataTransMode->bFirstRefsMoreThan100Log = false;
				Log::writeMessage(LOG_RTSPSERV, 1, "LINE:%d, FUNC:%s,refs = %d,type = %d,datatype = %d,id = %d", __LINE__, __FUNCTION__, pModerDataTransMode->refs, pModerDataTransMode->type, pModerDataTransMode->Datatype, pModerDataTransMode->id);
			}
		}
		
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		return;
	}
	pModerDataTransMode->refs++;
	OSLeaveMutex(pModerDataTransMode->mutex_lock);
	QWORD curTime;
	curTime = GetQPCMS();
	
	AudioRawData pAudioRawData;
	pAudioRawData.len = len;
	pAudioRawData.timeStamp = curTime;
	pAudioRawData.nChannels = info.nChannels;
	pAudioRawData.nSamplesPerSec = info.nSamplesPerSec;
	pAudioRawData.wBitsPerSample = info.wBitsPerSample;
	
	MSGHeader hreder;
	hreder.type = AudioData;
	hreder.len = sizeof(AudioRawData)+len;

	pModerDataTransMode->send_len = sizeof(MSGHeader)+sizeof(AudioRawData)+len; 

	memcpy(pModerDataTransMode->send_buffer, &hreder, sizeof(MSGHeader));
	memcpy(pModerDataTransMode->send_buffer + sizeof(MSGHeader), &pAudioRawData, sizeof(AudioRawData));
	memcpy(pModerDataTransMode->send_buffer + sizeof(MSGHeader)+sizeof(AudioRawData), ptr, len);
	pModerDataTransMode->AddRef();//
	m_instance->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer, pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
	
}

int CPipeServer::SetVideoData(CSampleData * RGBdata,
	unsigned long long nCtx) {
	
	if (m_bStop)
	{
		return 0;
	}
	StModerDataTransMode *pModerDataTransMode;
	if (nCtx == HostAVRequest)
	{
		if (m_bFirstLogHostVideo)
		{
			m_bFirstLogHostVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Host Video Width = %d, Heigth = %d, ColorType = %d,id = %d",
				__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, m_pVideoHostTransMode->id);
		}
		pModerDataTransMode = m_pVideoHostTransMode;
		if (m_bLogFlag)
		{
			HostTimeCurrent++;
			if (HostTimeCurrent - HostTimeLast > m_iCountFrame)
			{
				HostTimeLast = HostTimeCurrent;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 10s send Host Video Width = %d, Heigth = %d, ColorType = %d,id = %d,ref = %d",
					__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, m_pVideoHostTransMode->id, m_pVideoHostTransMode->refs);
			}
		}
		
	}
	else if (nCtx == DirectorAVRequest)
	{
		if (m_bFirstLogDirectVideo)
		{
			m_bFirstLogDirectVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Direct Video Width = %d, Heigth = %d, ColorType = %d,id = %d,ref = %d",
				__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, m_pVideoDirectorTransMode->id, m_pVideoDirectorTransMode->refs);
		}
		pModerDataTransMode = m_pVideoDirectorTransMode;
		if (m_bLogFlag)
		{
			DirectTimeCurrent++;
			if (DirectTimeCurrent - DirectTimeLast > m_iCountFrame)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 10s send Direct Video Width = %d, Heigth = %d, ColorType = %d, id = %d,ref = %d",
					__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, m_pVideoDirectorTransMode->id, m_pVideoDirectorTransMode->refs);
				DirectTimeLast = DirectTimeCurrent;
			}
		}
	}
	else if (nCtx == CommandCenterAVRequest)
	{
		if (m_bFirstLogThirdVideo)
		{
			m_bFirstLogThirdVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send CommandCenterAVRequest Video Width = %d, Heigth = %d, ColorType = %d,id = %d,ref = %d",
				__LINE__, String(__FUNCTION__).Array(), RGBdata->cx, RGBdata->cy, RGBdata->colorType, m_pVideoThirdTransMode->id, m_pVideoThirdTransMode->refs);
		}
		pModerDataTransMode = m_pVideoThirdTransMode;
	}
	else
		return 0;

	OSEnterMutex(pModerDataTransMode->mutex_lock);
	if (!pModerDataTransMode->bflag || pModerDataTransMode->refs>100) {
		if (pModerDataTransMode->refs > 100)
		{
			if (pModerDataTransMode->bFirstRefsMoreThan100Log)
			{
				pModerDataTransMode->bFirstRefsMoreThan100Log = false;
				Log::writeMessage(LOG_RTSPSERV, 1, "LINE:%d, FUNC:%s,refs = %d,type = %d,datatype = %d,id = %d", __LINE__, __FUNCTION__, pModerDataTransMode->refs, pModerDataTransMode->type, pModerDataTransMode->Datatype, pModerDataTransMode->id);
			}
		}
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		return 0;
	}
	pModerDataTransMode->refs++;
	RGBdata->AddRef();
	TColorType color_type = (TColorType)RGBdata->colorType;
	CSampleData * data;
	data = new CSampleData;
	if (nCtx == HostAVRequest)
	{
		data->cx = m_nWithHost;
		data->cy = m_nHeightHost;
	}
	else if (nCtx == DirectorAVRequest)
	{
		data->cx = m_nWithDirect;
		data->cy = m_nHeightDirect;
	}
	else
	{
		data->cx = m_nWithThird;
		data->cy = m_nHeightThird;
	}

	data->dataLength = data->cx*data->cy * 3 / 2;
	data->bAudio = false;
	data->timestamp = RGBdata->timestamp;
	data->lpData = (LPBYTE)Allocate_Bak(data->dataLength);
	if (color_type == ColorType_RGB)
	{	
		ImgResizeRGB32(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_RGB24)
	{
		ImgResizeRGB24(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_RGBA32REVERSE)
	{
		ImgResizeRGB32R(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_I420 || color_type == ColorType_YV12)
	{
		ImgResizeYUV420(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);

	}
	else if (color_type == ColorType_YVYU || color_type == ColorType_HDYC)
	{
		ImgResizeYUV422(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_UYVY)
	{
		ImgResizeUYVY(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_YUY2)
	{
		ImgResizeYUY2(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else
	{
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		RGBdata->Release();
		data->Release();
		return -1;
	}
	RGBdata->Release();
	VideoRawData pVideoRawData;
	pVideoRawData.len = data->dataLength;
	pVideoRawData.width = data->cx;
	pVideoRawData.height = data->cy;
	pVideoRawData.color = COLOR_YUV420P;
	pVideoRawData.timeStamp = data->timestamp;
	pVideoRawData.nFramePerSec = 25;

	MSGHeader hreder;
	hreder.type = VideoData;
	hreder.len = sizeof(VideoRawData)+data->dataLength;
	pModerDataTransMode->send_len = (sizeof(MSGHeader)+sizeof(VideoRawData)+data->dataLength); 

	memcpy(pModerDataTransMode->send_buffer, &hreder, sizeof(MSGHeader));
	memcpy(pModerDataTransMode->send_buffer + sizeof MSGHeader, &pVideoRawData, sizeof(VideoRawData));
	memcpy(pModerDataTransMode->send_buffer + sizeof MSGHeader + sizeof(VideoRawData), data->lpData, data->dataLength);

	data->Release();
	pModerDataTransMode->AddRef();
	//发送前系数加一
	m_instance->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer, 
		pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
	OSLeaveMutex(pModerDataTransMode->mutex_lock);
	return 0;
}

bool CPipeServer::AnalyzeHostModeCommand(AIOID id, char *pcommand, int msglen,
	StModerDataTransMode * pModerDataTransMode)
{
	MSGHeader oMSGHeader, sendMSGHeader;
	if (msglen < sizeof MSGHeader)
		return false;
	memset(&oMSGHeader, 0x00, sizeof MSGHeader);
	memcpy(&oMSGHeader, pcommand, sizeof MSGHeader);
	if (oMSGHeader.type == AudioData)
	{
		int dataLen = msglen - sizeof(MSGHeader);
		AudioRawData pAudioRawData;
		memcpy(&pAudioRawData, pcommand + sizeof(MSGHeader), sizeof(AudioRawData));
		ISampleData *pISampleData = new ISampleData;
		pISampleData->bAudio = true;
		pISampleData->AInfo.channels = pAudioRawData.nChannels;
		pISampleData->AInfo.samplerate = pAudioRawData.nSamplesPerSec;
		pISampleData->AInfo.Timestamp = pAudioRawData.timeStamp;
		//Log(TEXT("LINE:%d,FUNC:%s pAudioRawData.timeStamp ： %llu."), __LINE__, String(__FUNCTION__).Array(), pAudioRawData.timeStamp);
		pISampleData->AInfo.ampleperbits = pAudioRawData.wBitsPerSample;
		pISampleData->AInfo.Datalen = pAudioRawData.len;
		pISampleData->lpData = new unsigned char[pISampleData->AInfo.Datalen];
		memcpy(pISampleData->lpData, pcommand + sizeof(MSGHeader)+sizeof(AudioRawData), pISampleData->AInfo.Datalen);

		OSEnterMutex(hPipeVideoMutex);
		if (RenderMap.find(id) != RenderMap.end())
		{
			if (pModerDataTransMode->bFirstSetAudioParam)
			{
				pModerDataTransMode->bFirstSetAudioParam = false;
				AudioParam oAudioParam;
				oAudioParam.bitsPerSample = pISampleData->AInfo.ampleperbits;
				oAudioParam.channels = pISampleData->AInfo.channels;
				oAudioParam.samplesPerSec = pISampleData->AInfo.samplerate;
				RenderMap[id]->ResetAudioParam(oAudioParam);
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func : %s, --PipeServer HostMode ResetAudioParam bitsPerSample = %d, channels = %d, samplesPerSec = %d, AIOID = %d",
					__LINE__, __FUNCTION__, pISampleData->AInfo.ampleperbits, 
					pISampleData->AInfo.channels, pISampleData->AInfo.samplerate,id);
			}
			RenderMap[id]->ReceiveMediaSample(pISampleData, true);
			pModerDataTransMode->AddRef(); 
			m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer, pModerDataTransMode->recive_len,
				(ULL64)pModerDataTransMode, 0);
		}
		/*else
		{
			pISampleData->Release();
			m_pPipeControl->close_pipe(id);
			Log(TEXT("line: %d   func:%s,  --PipeServer HostMode Close Pipe AIOID = %d"),
				__LINE__, String(__FUNCTION__).Array(), id);
			
		}*/
		
		OSLeaveMutex(hPipeVideoMutex);
	}
	else if (oMSGHeader.type == VideoData)
	{
		int dataLen = msglen - sizeof(MSGHeader);
		VideoRawData pVideoRawData;
		memcpy(&pVideoRawData, pcommand + sizeof(MSGHeader), sizeof(VideoRawData));
		ISampleData *pISampleData = new ISampleData;
		pISampleData->bAudio = false;
		pISampleData->VInfo.Width = pVideoRawData.width;
		pISampleData->VInfo.Height = pVideoRawData.height;
		pISampleData->VInfo.Timestamp = pVideoRawData.timeStamp;
		pISampleData->VInfo.Datalen = pVideoRawData.len;
		pISampleData->VInfo.FrameRate = pVideoRawData.nFramePerSec;
		pISampleData->lpData = new unsigned char[pISampleData->VInfo.Datalen];      // (LPBYTE)Allocate_Bak(pISampleData->VInfo->Datalen);

		memcpy(pISampleData->lpData, pcommand + sizeof(MSGHeader)+sizeof(VideoRawData), pISampleData->VInfo.Datalen);

		OSEnterMutex(hPipeVideoMutex);
		if (RenderMap.find(id) != RenderMap.end())
		{
			if (pModerDataTransMode->bFirstSetVideoParam)
			{
				pModerDataTransMode->bFirstSetVideoParam = false;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode VideoParam Wideth = %d, Heigth = %d, len = %d,nFramePerSec = %d, AIOID = %d",
					__LINE__, __FUNCTION__, pVideoRawData.width,
					pVideoRawData.height, pVideoRawData.len, pVideoRawData.nFramePerSec,id);
			}
			RenderMap[id]->ReceiveMediaSample(pISampleData, false); //
			pModerDataTransMode->AddRef();
			m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer, pModerDataTransMode->recive_len,
				(ULL64)pModerDataTransMode, 0);
		}
		/*else
		{
		pISampleData->Release();
		m_pPipeControl->close_pipe(id);
		Log(TEXT("line: %d   func:%s,  --PipeServer HostMode Close Pipe AIOID = %d"),
		__LINE__, String(__FUNCTION__).Array(), id);
		}*/
		OSLeaveMutex(hPipeVideoMutex);
	}
	else if (oMSGHeader.type == ParamSet)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer type ParamSet AIOID  %d",
			__LINE__, __FUNCTION__, id);
		DeviceParam oDeviceParam;
		memset(&oDeviceParam, 0x00, sizeof DeviceParam);
		oDeviceParam.nAudioChannel = 1;
		oDeviceParam.nBitPerSample = 16;
		oDeviceParam.nSamplesRate = 16000;
		strResponse oResponse;

		//if (msglen - sizeof(MSGHeader) < sizeof DeviceParam)   //旧版程序的兼容
		{
			//memcpy(&oDeviceParam, pcommand + sizeof(MSGHeader), msgLen - sizeof(MSGHeader));
			int len = msglen - sizeof(MSGHeader);
			int offset = 0;
			if (len >= sizeof (int))


			{
				memcpy(&oDeviceParam.nWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.nHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.nChannelNum, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.DirectorAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.DirectorAudioRender, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.DirectorVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 256)
			{
				memcpy(oDeviceParam.DirectorVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
				len = len - 256;
				offset = offset + 256;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.PresenterAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.PresenterAudioRender, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.PresenterVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 256)
			{
				memcpy(oDeviceParam.PresenterVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
				len = len - 256;
				offset = offset + 256;
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.ZCRWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.ZCRWidth = %d.",
					__LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.ZCRWidth);
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.ZCRHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.ZCRHeight = %d.",
					__LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.ZCRHeight);
				len = len - sizeof (int);
				offset = offset + sizeof (int);

			}

			if (len >= sizeof (int))
			{

				memcpy(&oDeviceParam.nSamplesRate, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.nSamplesRate = %d.",
					__LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.nSamplesRate);
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.nAudioChannel, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.nBitPerSample, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.CommandCenterAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 128)
			{
				memcpy(oDeviceParam.CommandCenterVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
				len = len - 128;
				offset = offset + 128;
			}

			if (len >= 256)
			{
				memcpy(oDeviceParam.CommandCenterVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
				len = len - 256;
				offset = offset + 256;
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.CCRWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			if (len >= sizeof (int))
			{
				memcpy(&oDeviceParam.CCRHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
				len = len - sizeof (int);
				offset = offset + sizeof (int);
			}

			m_nWithDirect = oDeviceParam.nWidth;
			m_nHeightDirect = oDeviceParam.nHeight;
			m_nWithHost = oDeviceParam.ZCRWidth;
			m_nHeightHost = oDeviceParam.ZCRHeight;
			m_nSamplesRate = oDeviceParam.nSamplesRate;
			m_nAudioChannel = oDeviceParam.nAudioChannel;
			m_nBitPerSample = oDeviceParam.nBitPerSample;
			m_nWithThird = oDeviceParam.CCRWidth;
			m_nHeightThird = oDeviceParam.CCRHeight;

			Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 收到数据, oDeviceParam.nWidth = %d,oDeviceParam.nHeight = %d,oDeviceParam.ZCRWidth = %d,oDeviceParam.ZCRHeight = %d.oDeviceParam.nSamplesRate = %d.oDeviceParam.nAudioChannel = %d. oDeviceParam.nBitPerSample = %d.",
				__LINE__, String(__FUNCTION__).Array(), oDeviceParam.nWidth, oDeviceParam.nHeight, oDeviceParam.ZCRWidth, oDeviceParam.ZCRHeight, oDeviceParam.nSamplesRate, oDeviceParam.nAudioChannel, oDeviceParam.nBitPerSample);
		}
		
		//m_nWith = 320;
		//m_nHeight = 240;
		m_iRequestNum = oDeviceParam.nChannelNum;
		m_bFirstLogHostVideo = true;
		m_bFirstLogHostAudio = true;
		m_bFirstLogDirectVideo = true;
		m_bFirstLogDirectAudio = true;
		m_bFirstLogThirdAudio = true;
		m_bFirstLogThirdVideo = true;
		int retcode = RegisterDevice(oDeviceParam);
		//=======================

		AudioProcess(s2ws(oDeviceParam.DirectorAudioCapture).c_str(), s2ws(oDeviceParam.PresenterAudioCapture).c_str(), s2ws(oDeviceParam.CommandCenterAudioCapture).c_str());

// 		std::thread AudioPro(&CPipeServer::AudioProcess,this, s2ws(oDeviceParam.DirectorAudioCapture).c_str(),
// 			s2ws(oDeviceParam.PresenterAudioCapture).c_str(),
// 			s2ws(oDeviceParam.CommandCenterAudioCapture).c_str());
// 		AudioPro.detach();
		//=======================
		if (oldDeviceParam == NULL)
		{
			oldDeviceParam = new DeviceParam;
		}
		memcpy(oldDeviceParam, &oDeviceParam, sizeof(oDeviceParam));
		sendMSGHeader.type = ParamSet;
		sendMSGHeader.len = sizeof(strResponse);
		oResponse.code = retcode;
		memset(oResponse.description, 0, 256);
		if (retcode == 0)
		{
			memcpy(oResponse.description, "set success", strlen("set success"));
		}
		else if (retcode == 1)
		{
			memcpy(oResponse.description, "set Director ERROR", strlen("set Director ERROR"));
		}
		else if (retcode == 2)
		{
			memcpy(oResponse.description, "set Host ERROR", strlen("set Host ERROR"));
		}
		else if (retcode == 3)
		{
			memcpy(oResponse.description, "set Director and Host ERROR", strlen("set Director and Host ERROR"));
		}
		
		memset(pModerDataTransMode->receive_buffer, 0, sizeof(pModerDataTransMode->receive_buffer));
		memcpy(pModerDataTransMode->receive_buffer, &sendMSGHeader, sizeof(sendMSGHeader));
		memcpy(pModerDataTransMode->receive_buffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
		SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
		pModerDataTransMode->AddRef();
		m_pPipeControl->asyn_write_pipe(id, pModerDataTransMode->receive_buffer, SendSize,
			(ULL64)pModerDataTransMode, 0);
	}
	return true;
}

bool CPipeServer::AnalyzeCommand(AIOID id, char *pcommand, int msgLen, char * messageBuffer, 
	int& SendSize, StModerDataTransMode * pModerDataTransMode) {

	MSGHeader oMSGHeader, sendMSGHeader;
	if (msgLen < sizeof MSGHeader)
		return false;
	memset(&oMSGHeader, 0x00, sizeof MSGHeader);
	memcpy(&oMSGHeader, pcommand, sizeof MSGHeader);

	switch (oMSGHeader.type) {
	case ParamSet:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer type ParamSet AIOID  %d, thread_ID = %d",
						 __LINE__, __FUNCTION__, id,GetCurrentThreadId());
					 pModerDataTransMode->type = ParamSet;
					 m_processID = id;
					 m_instance->m_bStop = false;
					 pModerDataTransMode->bFirst = false;
					 DeviceParam oDeviceParam;
					 memset(&oDeviceParam, 0x00, sizeof DeviceParam);
					 oDeviceParam.nAudioChannel = 1;
					 oDeviceParam.nBitPerSample = 16;
					 oDeviceParam.nSamplesRate = 16000;
					 strResponse oResponse;

					// if (msgLen - sizeof(MSGHeader) < sizeof DeviceParam)   //旧版程序的兼容
					 {
						 //memcpy(&oDeviceParam, pcommand + sizeof(MSGHeader), msgLen - sizeof(MSGHeader));
						 int len = msgLen - sizeof(MSGHeader);
						 int offset = 0;
						 Log(TEXT("line: %d   func:%s, 收到数据长度 : len = %d."),
							 __LINE__, String(__FUNCTION__).Array(), len);
						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.nWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }
						
						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.nHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.nChannelNum, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.DirectorAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.DirectorAudioRender, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.DirectorVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 256)
						 {
							 memcpy(oDeviceParam.DirectorVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
							 len = len - 256;
							 offset = offset + 256;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.PresenterAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.PresenterAudioRender, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.PresenterVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 256)
						 {
							 memcpy(oDeviceParam.PresenterVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
							 len = len - 256;
							 offset = offset + 256;
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.ZCRWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.ZCRWidth = %d.",
								 __LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.ZCRWidth);
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.ZCRHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.ZCRHeight = %d.",
								 __LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.ZCRHeight);
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);

						 }

						 if (len >= sizeof (int))
						 {
							 
							 memcpy(&oDeviceParam.nSamplesRate, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 依然收到数据收到数据长度 : len = %d.oDeviceParam.nSamplesRate = %d.",
								 __LINE__, String(__FUNCTION__).Array(), len, oDeviceParam.nSamplesRate);
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.nAudioChannel, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.nBitPerSample, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.CommandCenterAudioCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 128)
						 {
							 memcpy(oDeviceParam.CommandCenterVideoCapture, pcommand + sizeof(MSGHeader)+offset, 128);
							 len = len - 128;
							 offset = offset + 128;
						 }

						 if (len >= 256)
						 {
							 memcpy(oDeviceParam.CommandCenterVideoCaptureID, pcommand + sizeof(MSGHeader)+offset, 256);
							 len = len - 256;
							 offset = offset + 256;
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.CCRWidth, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }

						 if (len >= sizeof (int))
						 {
							 memcpy(&oDeviceParam.CCRHeight, pcommand + sizeof(MSGHeader)+offset, sizeof (int));
							 len = len - sizeof (int);
							 offset = offset + sizeof (int);
						 }
						
						 m_nWithDirect = oDeviceParam.nWidth;
						 m_nHeightDirect = oDeviceParam.nHeight;
						 m_nWithHost = oDeviceParam.ZCRWidth;
						 m_nHeightHost = oDeviceParam.ZCRHeight;
						 m_nSamplesRate = oDeviceParam.nSamplesRate;
						 m_nAudioChannel = oDeviceParam.nAudioChannel;
						 m_nBitPerSample = oDeviceParam.nBitPerSample;
						 m_nWithThird = oDeviceParam.CCRWidth;
						 m_nHeightThird = oDeviceParam.CCRHeight;

						 Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, 收到数据, oDeviceParam.nWidth = %d,oDeviceParam.nHeight = %d,oDeviceParam.ZCRWidth = %d,oDeviceParam.ZCRHeight = %d.oDeviceParam.nSamplesRate = %d.oDeviceParam.nAudioChannel = %d. oDeviceParam.nBitPerSample = %d.",
							 __LINE__, String(__FUNCTION__).Array(), oDeviceParam.nWidth, oDeviceParam.nHeight, oDeviceParam.ZCRWidth, oDeviceParam.ZCRHeight, oDeviceParam.nSamplesRate, oDeviceParam.nAudioChannel, oDeviceParam.nBitPerSample);
					 }

					 m_iRequestNum = oDeviceParam.nChannelNum;
					 m_bFirstLogHostVideo = true;
					 m_bFirstLogHostAudio = true;
					 m_bFirstLogDirectVideo = true;
					 m_bFirstLogDirectAudio = true;
					 m_bFirstLogThirdAudio = true;
					 m_bFirstLogThirdVideo = true;

					 if (!m_hRegisterEvent)
					 {
						 m_hRegisterEvent = CreateEvent(NULL, true, false, NULL);
					 }
					 else
					 {
						 ResetEvent(m_hRegisterEvent);
					 }

					 int retcode = RegisterDevice(oDeviceParam);
					 Log::writeMessage(LOG_RTSPSERV, 1,"line: %d   func:%s, retcode = %d,请查看返回值是否为0！",
						 __LINE__, __FUNCTION__, retcode);
					 //=======================
					 AudioProcess(s2ws(oDeviceParam.DirectorAudioCapture).c_str(), s2ws(oDeviceParam.PresenterAudioCapture).c_str(), s2ws(oDeviceParam.CommandCenterAudioCapture).c_str());
					 //=======================
					 if (oldDeviceParam == NULL)
					 {
						 oldDeviceParam = new DeviceParam;
					 }
					 memcpy(oldDeviceParam, &oDeviceParam, sizeof(oDeviceParam));
					 SetEvent(m_hRegisterEvent);
					 Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s, retcode = %d,将要填写返回值",
						 __LINE__, __FUNCTION__, retcode);

					   sendMSGHeader.type = ParamSet;
					   sendMSGHeader.len = sizeof(strResponse);
					   oResponse.code = retcode;
					   memset(oResponse.description, 0, 256);
					   memcpy(oResponse.description, "0 is ok, other is wrong,camera can not use!", strlen("0 is ok, other is wrong,camera can not use!"));
					   memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
					   memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
					   SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
					   Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s, ParamSet id = %d,投递了一个写命令，请查看是否返回!!!!",
						   __LINE__, __FUNCTION__, id);

					 break;
	}
	case HostAVRequest:
	{
						  Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer type HostAVRequest AIOID  %d, thread_ID = %d",
							  __LINE__, __FUNCTION__, id,GetCurrentThreadId());
						  pModerDataTransMode->type = HostAVRequest;

						  m_HostID = id;
						  m_pAudioHostTransMode->id = id;
						  m_pAudioHostTransMode->type = HostAVRequest;
						  m_pAudioHostTransMode->bflag = true;
						  m_pAudioHostTransMode->bFirst = false;
						  m_pAudioHostTransMode->Datatype = AudioData;

						  m_pVideoHostTransMode->id = id;
						  m_pVideoHostTransMode->bflag = true;
						  m_pVideoHostTransMode->type = HostAVRequest;
						  m_pVideoHostTransMode->bFirst = false;
						  m_pVideoHostTransMode->Datatype = VideoData;

						  strResponse oResponse;
						  int retcode;
						  if (oldDeviceParam)
						  {
							  String PresenterDevice(s2ws(oldDeviceParam->PresenterVideoCapture).c_str());
							  String PresenterDeviceID(s2ws(oldDeviceParam->PresenterVideoCaptureID).c_str());
							  retcode = CheckDeviceByName(PresenterDevice, PresenterDeviceID,0);
						  }
						
						  sendMSGHeader.type = HostAVRequest;
						  sendMSGHeader.len = sizeof(oResponse);
						  oResponse.code = retcode;
						  memset(oResponse.description, 0, 256);
						  memcpy(oResponse.description, "0 is OK!other is wrong", sizeof("0 is OK!other is wrong"));
						  memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
						  memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
						  SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
						  break;
	}
	case DirectorAVRequest:
	{ 
							  Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func : %s, --PipeServer type DirectorAVRequest AIOID  %d, thread_ID = %d",
								  __LINE__, __FUNCTION__, id,GetCurrentThreadId());
							  pModerDataTransMode->type = DirectorAVRequest;

							  m_DirectorID = id;
							  m_pAudioDirectorTransMode->id = id;                 //不要释放
							  m_pAudioDirectorTransMode->bflag = true;
							  m_pAudioDirectorTransMode->type = DirectorAVRequest;
							  m_pAudioDirectorTransMode->bFirst = false;
							  m_pAudioDirectorTransMode->Datatype = AudioData;

							  m_pVideoDirectorTransMode->id = id;
							  m_pVideoDirectorTransMode->bflag = true;
							  m_pVideoDirectorTransMode->type = DirectorAVRequest;
							  m_pVideoDirectorTransMode->bFirst = false;
							  m_pVideoDirectorTransMode->Datatype = VideoData;
							  
							  strResponse oResponse;
							  int retcode;
							  if (oldDeviceParam)
							  {
								  String DirectorDevice(s2ws(oldDeviceParam->DirectorVideoCapture).c_str());
								  String DirectorDeviceID(s2ws(oldDeviceParam->DirectorVideoCaptureID).c_str());
								  retcode = CheckDeviceByName(DirectorDevice, DirectorDeviceID, 1);
							  }
						
							  sendMSGHeader.type = DirectorAVRequest;
							  sendMSGHeader.len = sizeof(oResponse);

							  oResponse.code = retcode;
							  memset(oResponse.description, 0, 256);
							  memcpy(oResponse.description, "0 is OK!other is wrong", sizeof("0 is OK!other is wrong"));
							  memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
							  memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
							  SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
							  break;
						
	}
	
	case CommandCenterAVRequest:
	{
						 Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer type Third AIOID  %d, thread_ID = %d",
								  __LINE__, String(__FUNCTION__).Array(), id, GetCurrentThreadId());
							  pModerDataTransMode->type = CommandCenterAVRequest;

							  m_ThirdID = id;
							  m_pAudioThirdTransMode->id = id;                 //不要释放
							  m_pAudioThirdTransMode->bflag = true;
							  m_pAudioThirdTransMode->type = CommandCenterAVRequest;
							  m_pAudioThirdTransMode->bFirst = false;
							  m_pAudioThirdTransMode->Datatype = AudioData;

							  m_pVideoThirdTransMode->id = id;
							  m_pVideoThirdTransMode->bflag = true;
							  m_pVideoThirdTransMode->type = CommandCenterAVRequest;
							  m_pVideoThirdTransMode->bFirst = false;
							  m_pVideoThirdTransMode->Datatype = VideoData;

							  strResponse oResponse;
							  int retcode;
							  if (oldDeviceParam)
							  {
								  String ThirdDevice(s2ws(oldDeviceParam->CommandCenterVideoCapture).c_str());
								  String ThirdDeviceID(s2ws(oldDeviceParam->CommandCenterVideoCaptureID).c_str());
								  retcode = CheckDeviceByName(ThirdDevice, ThirdDeviceID, 1);
							  }
							  
							  sendMSGHeader.type = CommandCenterAVRequest;
							  sendMSGHeader.len = sizeof(oResponse);

							  oResponse.code = retcode;
							  memset(oResponse.description, 0, 256);
							  memcpy(oResponse.description, "0 is OK!other is wrong", sizeof("0 is OK!other is wrong"));
							  memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
							  memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
							  SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
							  break;
	}
	
	case HostMode:
	{
					 Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer type HostMode AIOID  %d,, thread_ID = %d",
						 __LINE__,__FUNCTION__, id,GetCurrentThreadId());
					 pModerDataTransMode->type = HostMode;
					 pModerDataTransMode->bFirst = false;
					 DeviceParam oDeviceParam;
					 strResponse oResponse;
					 strHostMode oHostMode;
					 oResponse.code = -1;
					 pModerDataTransMode->bHasAPipeVideoRender = false;
					 if (msgLen - sizeof(MSGHeader) < sizeof strHostMode)
					 {
						 Log::writeMessage(LOG_RTSPSERV, 1, "");
						 return false;
					 }
					 
					memset(&oHostMode, 0x00, sizeof strHostMode);
					memcpy(&oHostMode, pcommand + sizeof(MSGHeader), sizeof strHostMode);
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode get hPipeVideoMutex id = %d, thread_id = %d",
						__LINE__, __FUNCTION__,id, GetCurrentThreadId());

					//memset(m_cNickName, 0, 64);
					//memcpy(m_cNickName, oHostMode.Nickname,64);
					OSEnterMutex(hPipeVideoMutex);
					for (auto iter = PipeRenders.begin(); iter != PipeRenders.end(); iter++)
				    {
						if (strcmp(((*iter)->data["Name"]).asString().c_str(), oHostMode.InteractionName) == 0)
						{
							(*iter)->b_flag = true;
							(*iter)->m_aioId = id;
							oResponse.code = 0;
							pModerDataTransMode->bHasAPipeVideoRender = true;
							memset((*iter)->name, 0x00, sizeof((*iter)->name));
							memcpy((*iter)->name, oHostMode.ChannelNumber, sizeof((*iter)->name));
							(*iter)->m_NickName = Asic2WChar(oHostMode.Nickname).c_str();
							RenderMap[id] = (*iter);
							std::stringstream SourceId;
							uint64_t InstanceID = 0;
							if (!(*iter)->data["InstanceID"].isNull())
							{
								SourceId << (*iter)->data["InstanceID"].asString().c_str();
								SourceId >> InstanceID;
							}
							NickNameCallBack NameCb = (NickNameCallBack)GetNickNameCallBack();
							if (NameCb)
							{
								NameCb(InstanceID, (uint64_t)(*iter), oHostMode.Nickname);
							}
							Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer AIOID  %d is success get a pos for PipeVideo, thread_ID = %d",
								__LINE__, __FUNCTION__, id, GetCurrentThreadId());
							break;
						}
					}
					OSLeaveMutex(hPipeVideoMutex);

					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode leave hPipeVideoMutex id = %d, thread_id = %d",
						__LINE__, __FUNCTION__, id,GetCurrentThreadId());
					sendMSGHeader.type = HostMode;
					sendMSGHeader.len = sizeof(oResponse);
					memset(oResponse.description, 0, sizeof (oResponse.description));

					if (oResponse.code == -1)
					{
						memcpy(oResponse.description, oHostMode.InteractionName, sizeof(oHostMode.InteractionName));
					}

					memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
					memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
					SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
					
					break;
	}
	case HeartBeat:
	{		 
					 break;
	}
	default:
		break;
	}
	return true;
}

int CPipeServer::RegisterDevice(DeviceParam & oDeviceParam){

	int iDirectorCode = 0;
	int iPresenterCode = 0;
	int iThridCode = 0;
	int iMaxWidth = 0;
	int iMaxHeight = 0;
	bool b_registerDirector = false;
	bool b_registerPresenter = false;
	bool b_registerThird = false;

	String DirectorDevive(s2ws(oDeviceParam.DirectorVideoCapture).c_str());
	String DirectorDeviveID(s2ws(oDeviceParam.DirectorVideoCaptureID).c_str());
	String PresenterDevive(s2ws(oDeviceParam.PresenterVideoCapture).c_str());
	String PresenterDeviveID(s2ws(oDeviceParam.PresenterVideoCaptureID).c_str());
	String ThirdDevive(s2ws(oDeviceParam.CommandCenterVideoCapture).c_str());
	String ThirdDeviveID(s2ws(oDeviceParam.CommandCenterVideoCaptureID).c_str());
	if (!DirectorDevive.DataLength() || !PresenterDevive.DataLength() || !ThirdDevive.DataLength())
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --Director and Presenter Device is NULL , thread_id = %d",
			__LINE__, __FUNCTION__, GetCurrentThreadId());
		return 0;
	}

	m_bDirectorHostSameName = DirectorDeviveID.Compare(PresenterDeviveID);
	m_bDirectorThirdSameName = DirectorDeviveID.Compare(ThirdDeviveID);
	m_bHostThirdSameName = PresenterDeviveID.Compare(ThirdDeviveID);
	if (m_bDirectorHostSameName && m_bDirectorThirdSameName) //三个都一样
	{
		if (oDeviceParam.nWidth > oDeviceParam.ZCRWidth)
		{
			if (oDeviceParam.nWidth > oDeviceParam.CCRWidth)
			{
				iMaxWidth = oDeviceParam.nWidth;
				iMaxHeight = oDeviceParam.nHeight;
			}
			else
			{
				iMaxWidth = oDeviceParam.CCRWidth;
				iMaxHeight = oDeviceParam.CCRHeight;
			}
		}
		else
		{
			if (oDeviceParam.ZCRWidth > oDeviceParam.CCRWidth)
			{
				iMaxWidth = oDeviceParam.ZCRWidth;
				iMaxHeight = oDeviceParam.ZCRHeight;
			}
			else
			{
				iMaxWidth = oDeviceParam.CCRWidth;
				iMaxHeight = oDeviceParam.CCRHeight;
			}
		}
		if (!b_registerDirector)
		{
			DeviceDirector.UnRegisterSharedDevice();

			DeviceDirector.DeviceName = DirectorDevive;
			DeviceDirector.DeviceID = DirectorDeviveID;
			DeviceDirector.iWidth = iMaxWidth;
			DeviceDirector.iHeigth = iMaxHeight;
			DeviceDirector.iType = DirectorAVRequest;
			DeviceDirector.Server = this;

			iDirectorCode = DeviceDirector.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());

			b_registerDirector = true;
		}
		if (!b_registerPresenter)
		{
			DeviceHost.UnRegisterSharedDevice();

			DeviceHost.DeviceName = PresenterDevive;
			DeviceHost.DeviceID = PresenterDeviveID;
			DeviceHost.iWidth = iMaxWidth;
			DeviceHost.iHeigth = iMaxHeight;
			DeviceHost.iType = HostAVRequest;
			DeviceHost.Server = this;

			iPresenterCode = DeviceHost.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());
			b_registerPresenter = true;
		}

		if (!b_registerThird)
		{
			DeviceCenter.UnRegisterSharedDevice();

			DeviceCenter.DeviceName = ThirdDevive;
			DeviceCenter.DeviceID = ThirdDeviveID;
			DeviceCenter.iWidth = iMaxWidth;
			DeviceCenter.iHeigth = iMaxHeight;
			DeviceCenter.iType = CommandCenterAVRequest;
			DeviceCenter.Server = this;

			iThridCode = DeviceCenter.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());
			b_registerThird = true;
		}
	}
	else //名字不相同，分开设置
	{
		if (!b_registerDirector)
		{

			DeviceDirector.UnRegisterSharedDevice();

			DeviceDirector.DeviceName = DirectorDevive;
			DeviceDirector.DeviceID = DirectorDeviveID;
			DeviceDirector.iWidth = m_nWithDirect;
			DeviceDirector.iHeigth = m_nHeightDirect;
			DeviceDirector.iType = DirectorAVRequest;
			DeviceDirector.Server = this;

			iDirectorCode = DeviceDirector.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());
			b_registerDirector = true;
		}
		if (!b_registerPresenter)
		{
			DeviceHost.UnRegisterSharedDevice();
			DeviceHost.DeviceName = PresenterDevive;
			DeviceHost.DeviceID = PresenterDeviveID;
			DeviceHost.iWidth = m_nWithHost;
			DeviceHost.iHeigth = m_nHeightHost;
			DeviceHost.iType = HostAVRequest;
			DeviceHost.Server = this;

			iPresenterCode = DeviceHost.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());
			b_registerPresenter = true;
		}

		if (!b_registerThird)
		{
			DeviceCenter.UnRegisterSharedDevice();

			DeviceCenter.DeviceName = ThirdDevive;
			DeviceCenter.DeviceID = ThirdDeviveID;
			DeviceCenter.iWidth = m_nWithThird;
			DeviceCenter.iHeigth = m_nHeightThird;
			DeviceCenter.iType = CommandCenterAVRequest;
			DeviceCenter.Server = this;

			iThridCode = DeviceCenter.RegisterSharedDevice(ReceiveVideoData);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer Register leave m_hRegisterMutex, thread_id = %d",
				__LINE__, __FUNCTION__, GetCurrentThreadId());
			b_registerThird = true;
		}

	}

	if (iDirectorCode != 0 && iPresenterCode != 0)
	{
		return 3;
	}
	else if (iDirectorCode != 0 && iPresenterCode == 0)
	{
		return 1;
	}
	else if (iDirectorCode == 0 && iPresenterCode != 0)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

int CPipeServer::CheckDeviceByName(const String & DeviceName, const String & DeviceNameID, 
	int flag) {
	int code;
	code = 0;// GetVideoCaptureLinkNum(DeviceName, DeviceNameID);
	if (code > 0)      //有关联设备
	{
		code = 0;
	}
	return code;
}

int CPipeServer::AddVideoRender(PipeVideo* pPipeVideo) {

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer AddVideoRender get hPipeVideoMutex, PipeRenders size =%d,thread_id = %d",
		__LINE__, __FUNCTION__, PipeRenders.size(), GetCurrentThreadId());

	OSEnterMutex(hPipeVideoMutex);
	m_bRestartInteractive = true;
	PipeRenders.push_back(pPipeVideo);
	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func : %s, --PipeServer add a PipeVideo to PipeRenders, PipeRenders.size = %d",
		__LINE__, __FUNCTION__, PipeRenders.size());

	ProcessNameCommand(pPipeVideo->data["Name"].asString().c_str(), NULL, AddPipeVideoCommand);

	OSLeaveMutex(hPipeVideoMutex);

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer AddVideoRender leave hPipeVideoMutex, PipeRenders size =%d,thread_id = %d",
		__LINE__, __FUNCTION__, PipeRenders.size(), GetCurrentThreadId());
	return PipeRenders.size();
}

void CPipeServer::RemoveVideoRender(PipeVideo* pipeVideo) {

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer get hPipeVideoMutex,RenderMap.size() = %d, thread_ID = %d",
		__LINE__, __FUNCTION__, RenderMap.size(),GetCurrentThreadId());

	OSEnterMutex(hPipeVideoMutex);
	std::map<AIOID, PipeVideo*>::iterator iter_map = RenderMap.begin();
	while (iter_map != RenderMap.end())
	{
		if (iter_map->second == pipeVideo)
		{
			//先关闭管道id
			m_pPipeControl->close_pipe(iter_map->first);
			//在m_pModerDataTransModeList中移除此id
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 移除一个互动源HostMode close get m_hDataStListMutex id = %d,  m_pModerDataTransModeList size = %d,thread_ID=%d",
				__LINE__, __FUNCTION__, iter_map->first, m_pModerDataTransModeList.size(), GetCurrentThreadId());

			OSEnterMutex(m_hDataStListMutex);
			std::list<AIOID>::iterator iter = m_pModerDataTransModeList.begin();
			for (; iter != m_pModerDataTransModeList.end(); iter++)
			{
				if (*iter == iter_map->first)
				{
					m_pModerDataTransModeList.erase(iter);    //更新id列表
					Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 移除一个互动源,erase a id from m_pModerDataTransModeList id = %d, size = %d",
						__LINE__, __FUNCTION__, iter_map->first, m_pModerDataTransModeList.size());
					break;
				}
			}
			OSLeaveMutex(m_hDataStListMutex);

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer HostMode close leave m_hDataStListMutex id = %d,  m_pModerDataTransModeList size = %d,thread_ID=%d",
				__LINE__, __FUNCTION__, iter_map->first, m_pModerDataTransModeList.size(), GetCurrentThreadId());

			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer erase a PipeVideo from RenderMap,close pipe ID  = %d,thread_ID = %d",
				__LINE__, __FUNCTION__, iter_map->first, GetCurrentThreadId());
			iter_map = RenderMap.erase(iter_map);
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer erase a PipeVideo from RenderMap,RenderMap size = %d,thread_ID = %d",
				__LINE__, __FUNCTION__, RenderMap.size(), GetCurrentThreadId());
			break;
		}
		else
		{
			iter_map++;
		}
		
	}
	std::list<PipeVideo*>::iterator iter;
	for (iter = PipeRenders.begin(); iter != PipeRenders.end();iter++)
	{
		if (*iter == pipeVideo)
		{
			PipeRenders.erase(iter);
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer erase a PipeVideo from PipeRenders,PipeRenders size = %d,thread_ID = %d",
				__LINE__, __FUNCTION__, PipeRenders.size(), GetCurrentThreadId());
			break;
		}
	}

	ProcessNameCommand(pipeVideo->data["Name"].asString().c_str(), NULL, DelPipeVideoCommand);

	OSLeaveMutex(hPipeVideoMutex);

	Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer leave hPipeVideoMutex,RenderMap.size() = %d, thread_ID = %d",
		__LINE__, __FUNCTION__, RenderMap.size(), GetCurrentThreadId());

	if (PipeRenders.size() == 0)
	{
		if (m_hRegisterEvent)
		{
			int ret = WaitForSingleObject(m_hRegisterEvent, INFINITE);
			if (WAIT_OBJECT_0 == ret)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --反注册可以退出了!", __LINE__, __FUNCTION__);
			}
		}

		SetEvent(m_hEvent);
		SetEvent(m_hEventTick);
		m_bStop = true;
		m_bRestartInteractive = false;
		bool bFlag = m_instance->CheckProcess(m_instance->m_ProcessName.c_str());
		if (bFlag)
		{
			int ExitCode = 0;
			bool ret = TerminateProcess(m_instance->GetProcessHandle(m_instance->m_th32ProcessID), ExitCode);
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer kill Interactive Process ID: %d,ExitCode : %d, ret = %d,thread_ID = %d",
				__LINE__, __FUNCTION__, m_instance->m_th32ProcessID, ExitCode, ret,GetCurrentThreadId());
		}
		Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer PipeRenders size = 0,Stop itself,thread_ID = %d",
			__LINE__, __FUNCTION__,GetCurrentThreadId());

		ReleaseThread(NULL);
		m_instance->Stop();
		//结束互动机进程
	}
}

void CPipeServer::ImgResizeYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW / 2;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *VSrcImg = pSrcImg + srcW*srcH + srcW*srcH / 4;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW / 2;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int tSrcH, tSrcW;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW + tSrcW];
		}
	}

	for (int i = 0; i < VdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW + tSrcW];
		}
	}
}


void CPipeServer::ImgResizeYV12(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *VSrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW / 2;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *USrcImg = pSrcImg + srcW*srcH + srcW*srcH / 4;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW / 2;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;


	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int tSrcH, tSrcW;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW + tSrcW];
		}
	}

	for (int i = 0; i < VdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW + tSrcW];
		}
	}
}

void CPipeServer::ImgResizeRGB32(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH-1; i >=0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW*4 + tSrcW*4];
			G = pSrcImg[tSrcH*srcW*4 + tSrcW*4 + 1];
			R = pSrcImg[tSrcH*srcW*4 + tSrcW*4 + 2];

			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;

			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void CPipeServer::ImgResizeRGB32R(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH - 1; i >= 0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4+1];
			G = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 2];
			R = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 3];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			/*Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);*/
			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;


			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void CPipeServer::ImgResizeRGB24(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH - 1; i >= 0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 0];
			G = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 1];
			R = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 2];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			/*Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);*/
			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;


			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void CPipeServer::ImgResizeYUV422(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *VSrcImg = pSrcImg + srcW*srcH + srcW*srcH / 2;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	int tSrcH, tSrcW;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW*2 + tSrcW];
		}
	}

	for (int i = 0; i < VdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW*2 + tSrcW];
		}
	}
}

void CPipeServer::RGB24ToYUV420(int Width, int Height, unsigned char* RgbBuffer, unsigned char*YuvBuffer)
{
	unsigned char * yuvBuf = YuvBuffer;
	int nWidth = Width;
	int nHeight = Height;
	/////////////////////
	int i, j;
	unsigned char*bufY = yuvBuf;
	unsigned char*bufU = yuvBuf + nWidth * nHeight;
	unsigned char*bufV = bufU + (nWidth* nHeight * 1 / 4);
	unsigned char*Y = bufY;
	unsigned char*U = bufU;
	unsigned char*V = bufV;
	unsigned char*bufRGB;

	unsigned char y, u, v, r, g, b;

	if (NULL == RgbBuffer)
	{
		return;
	}
	for (j = 0; j < nHeight; j++)
	{
		bufRGB = RgbBuffer + nWidth * (nHeight - 1 - j) * 3;
		for (i = 0; i < nWidth; i++)
		{
			int pos = nWidth * i + j;
			r = *(bufRGB++);
			g = *(bufRGB++);
			b = *(bufRGB++);
			y = (unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;//16
			v = (unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128; //128          
			u = (unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
			*(bufY++) = max(0, min(y, 255));

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (u > 255)
				{
					u = 255;
				}
				if (u < 0)
				{
					u = 0;
				}
				*(bufU++) = u;
			}
			else
			{
				if (i % 2 == 0)
				{
					if (v > 255)
					{
						v = 255;
					}
					if (v < 0)
					{
						v = 0;
					}
					*(bufV++) = v;
				}
			}
		}
	}
}

void CPipeServer::RGB32toYUV420P(unsigned char *pSrcRGB32, unsigned char *pDstYUV420P, int width, int height)
{

	int32_t Y, U, V, R, G, B;
	/*R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;*/
	int linesize = width * 4;
	unsigned char* pLine = pSrcRGB32;
	
	unsigned char*bufY = pDstYUV420P;
	unsigned char*bufU = pDstYUV420P + width * height;
	unsigned char*bufV = pDstYUV420P + (width* height * 1 / 4);

	//unsigned char* pDstYLine = bufY;
	for (size_t i = 0; i < height; i++)
	{
		for (size_t j = 0; j < width; j++)
		{
			R = *(pLine + j * 4 + 0);
			G = *(pLine + j * 4 + 1);
			B = *(pLine + j * 4 + 2);
			/*Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);*/
			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;


			Y = min(255, max(0, Y));
			//U = min(255, max(0, U));
			//V = min(255, max(0, V));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
				U = 255;
				}
				if (U < 0)
				{
				U = 0;
				}
				*(bufU++) = U;
			} 
			else
			{
				if (i % 2 == 0)
				{
					if (V > 255)
					{
						V = 255;
					}
					if (V < 0)
					{
						V = 0;
					}
					*(bufV++) = V;
				}
			}
			//*bufU = U;
			//*bufV = V;
		}
		pLine += linesize;
	}
	//////	Y = (int32_t)( 0.257 * R + 0.504 * G + 0.098 * B);
	//////	U = (int32_t)( 0.439 * R - 0.368 * G - 0.071 * B + 128);
	//////	V = (int32_t)(-0.148 * R - 0.291 * G + 0.439 * B + 128);
	//RGBQUAD yuv = { (uint8_t)V, (uint8_t)U, (uint8_t)Y, 0 };
	return ;
}

void CPipeServer::ImgResizeUYVY(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *YSrcImg = pSrcImg + 1;
	unsigned char *USrcImg = pSrcImg;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;


	unsigned char *VSrcImg = pSrcImg + 2;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;


	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;


	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

// 	if (3 * srcW > 4 * srcH)
// 	{
// 		CorrectSrcW = srcH * 4 / 3;
// 		CorrectSrcH = srcH;
// 		CropW = (srcW - CorrectSrcW) / 2;
// 	}
// 	else
// 	{
// 		CorrectSrcW = srcW;
// 		CorrectSrcH = srcW * 3 / 4;
// 		CropH = (srcH - CorrectSrcH) / 2;
// 	}
// 
// 	double rateH = (double)CorrectSrcH / (double)dstH;
// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;


	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[(tSrcH*VsrcW + tSrcW) * 4];
		}
	}

	return;
}

void CPipeServer::ImgResizeYUY2(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *YSrcImg = pSrcImg;
	unsigned char *USrcImg = pSrcImg + 1;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *VSrcImg = pSrcImg + 3;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH/2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW/2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[(tSrcH*VsrcW + tSrcW) * 4];
		}
	}

	return;
}

bool CPipeServer::CheckProcess(const WCHAR* CheckName)
{
	HANDLE procSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (procSnap == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	PROCESSENTRY32 procEntry = { 0 };
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	BOOL bRet = Process32First(procSnap, &procEntry);
	int iCount = 0;
	while (bRet)
	{
		if (!wcsicmp(procEntry.szExeFile, CheckName))
		{
			++iCount;
			m_th32ProcessID = procEntry.th32ProcessID;
			break;
		}

		bRet = Process32Next(procSnap, &procEntry);
	}
	CloseHandle(procSnap);
	if (iCount > 0)
		return true;
	else
		return false;
}

HANDLE CPipeServer::GetProcessHandle(int nID)//通过进程ID获取进程句柄
{
	return OpenProcess(PROCESS_ALL_ACCESS, FALSE, nID);
}

void CPipeServer::ReceiveVideoData(void *context, CSampleData *RGBdata)
{
	SharedDevice *Device = reinterpret_cast<SharedDevice*>(context);
	CPipeServer *pThis = reinterpret_cast<CPipeServer*>(Device->Server);

	if (!Device || !pThis)
		return;

	if (pThis->m_bStop)
	{
		return;
	}
	StModerDataTransMode *pModerDataTransMode;
	if (Device->iType == HostAVRequest)
	{
		if (pThis->m_bFirstLogHostVideo)
		{
			pThis->m_bFirstLogHostVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Host Video Width = %d, Heigth = %d, ColorType = %d,id = %d",
				__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, pThis->m_pVideoHostTransMode->id);
		}
		pModerDataTransMode = pThis->m_pVideoHostTransMode;
		if (pThis->m_bLogFlag)
		{
			pThis->HostTimeCurrent++;
			if (pThis->HostTimeCurrent - pThis->HostTimeLast > pThis->m_iCountFrame)
			{
				pThis->HostTimeLast = pThis->HostTimeCurrent;
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 10s send Host Video Width = %d, Heigth = %d, ColorType = %d,id = %d,ref = %d",
					__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, pThis->m_pVideoHostTransMode->id, pThis->m_pVideoHostTransMode->refs);
			}
		}

	}
	else if (Device->iType == DirectorAVRequest)
	{
		if (pThis->m_bFirstLogDirectVideo)
		{
			pThis->m_bFirstLogDirectVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer first send Direct Video Width = %d, Heigth = %d, ColorType = %d,id = %d,ref = %d",
				__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, pThis->m_pVideoDirectorTransMode->id, pThis->m_pVideoDirectorTransMode->refs);
		}
		pModerDataTransMode = pThis->m_pVideoDirectorTransMode;
		if (pThis->m_bLogFlag)
		{
			pThis->DirectTimeCurrent++;
			if (pThis->DirectTimeCurrent - pThis->DirectTimeLast > pThis->m_iCountFrame)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 10s send Direct Video Width = %d, Heigth = %d, ColorType = %d, id = %d,ref = %d",
					__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, pThis->m_pVideoDirectorTransMode->id, pThis->m_pVideoDirectorTransMode->refs);
				pThis->DirectTimeLast = pThis->DirectTimeCurrent;
			}
		}
	}
	else if (Device->iType == CommandCenterAVRequest)
	{
		if (m_instance->m_bFirstLogThirdVideo)
		{
			m_instance->m_bFirstLogThirdVideo = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "line: %d   func:%s,  --PipeServer 10s send Direct Video Width = %d, Heigth = %d, ColorType = %d, id = %d,ref = %d",
				__LINE__, __FUNCTION__, RGBdata->cx, RGBdata->cy, RGBdata->colorType, pThis->m_pVideoDirectorTransMode->id, pThis->m_pVideoDirectorTransMode->refs);
		}
		pModerDataTransMode = m_instance->m_pVideoThirdTransMode;
	}
	else
		return;

	OSEnterMutex(pModerDataTransMode->mutex_lock);
	if (!pModerDataTransMode->bflag || pModerDataTransMode->refs > 100) {
		if (pModerDataTransMode->refs > 100)
		{
			if (pModerDataTransMode->bFirstRefsMoreThan100Log)
			{
				pModerDataTransMode->bFirstRefsMoreThan100Log = false;
				Log::writeMessage(LOG_RTSPSERV, 1, "LINE:%d, FUNC:%s,refs = %d,type = %d,datatype = %d,id = %d", __LINE__, __FUNCTION__, pModerDataTransMode->refs, pModerDataTransMode->type, pModerDataTransMode->Datatype, pModerDataTransMode->id);
			}
		}
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		return;
	}
	pModerDataTransMode->refs++;
	RGBdata->AddRef();
	TColorType color_type = (TColorType)RGBdata->colorType;
	CSampleData * data;
	data = new CSampleData;
	if (Device->iType == HostAVRequest)
	{
		data->cx = pThis->m_nWithHost;
		data->cy = pThis->m_nHeightHost;
	}
	else if (Device->iType == DirectorAVRequest)
	{
		data->cx = pThis->m_nWithDirect;
		data->cy = pThis->m_nHeightDirect;
	}
	else
	{
		data->cx = pThis->m_nWithThird;
		data->cy = pThis->m_nHeightThird;
	}

	data->dataLength = data->cx*data->cy * 3 / 2;
	data->bAudio = false;
	data->timestamp = RGBdata->timestamp;
	data->lpData = (LPBYTE)Allocate_Bak(data->dataLength);
	if (color_type == ColorType_RGB)
	{
		pThis->ImgResizeRGB32(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_RGB24)
	{
		pThis->ImgResizeRGB24(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_RGBA32REVERSE)
	{
		pThis->ImgResizeRGB32R(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_I420 || color_type == ColorType_YV12)
	{
		if (color_type == ColorType_I420)
			pThis->ImgResizeYUV420(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
		else
		{
			pThis->ImgResizeYV12(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
		}

	}
	else if (color_type == ColorType_YVYU || color_type == ColorType_HDYC)
	{
		pThis->ImgResizeYUV422(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_UYVY)
	{
		pThis->ImgResizeUYVY(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else if (color_type == ColorType_YUY2)
	{
		pThis->ImgResizeYUY2(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
	}
	else
	{
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		RGBdata->Release();
		data->Release();
		return;
	}
	RGBdata->Release();
	VideoRawData pVideoRawData;
	pVideoRawData.len = data->dataLength;
	pVideoRawData.width = data->cx;
	pVideoRawData.height = data->cy;
	pVideoRawData.color = COLOR_YUV420P;
	pVideoRawData.timeStamp = data->timestamp;
	pVideoRawData.nFramePerSec = 25;

	MSGHeader hreder;
	hreder.type = VideoData;
	hreder.len = sizeof(VideoRawData)+data->dataLength;
	pModerDataTransMode->send_len = (sizeof(MSGHeader)+sizeof(VideoRawData)+data->dataLength);

	memcpy(pModerDataTransMode->send_buffer, &hreder, sizeof(MSGHeader));
	memcpy(pModerDataTransMode->send_buffer + sizeof MSGHeader, &pVideoRawData, sizeof(VideoRawData));
	memcpy(pModerDataTransMode->send_buffer + sizeof MSGHeader + sizeof(VideoRawData), data->lpData, data->dataLength);

	data->Release();
	pModerDataTransMode->AddRef();
	//发送前系数加一
	m_instance->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer,
		pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
	OSLeaveMutex(pModerDataTransMode->mutex_lock);
}

void CPipeServer::ResetInitParam()
{
	m_bHasSendFristPipeName = false;
}

void CPipeServer::ProcessNameCommand(const char *NewName, const char *OldName, MsgType type, bool bFristAdd)
{
	if (type != AddPipeVideoCommand && type != DelPipeVideoCommand && type != RenamePipeVideoCommand)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "错误的MsgType %d",type);
		return;
	}
	if (!bFristAdd && (!m_bHasSendFristPipeName || m_processID == -1))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "信令管道还未建立连接");
		return;
	}

	//遍历当前互动源列表发送互动源名字
	StModerDataTransMode *pModerVideoName = new StModerDataTransMode;
	pModerVideoName->type = type;
	pModerVideoName->id = m_processID;

	MSGHeader *NickHeader = (MSGHeader*)pModerVideoName->send_buffer;
	NickHeader->type = type;

	Json::Value PipeVideoNameList;
	Json::Value &OneList = PipeVideoNameList["NameList"];

	if (bFristAdd && type == AddPipeVideoCommand)
	{
		int Index = 0;
		OSEnterMutex(hPipeVideoMutex);
		for (auto iter = PipeRenders.begin(); iter != PipeRenders.end(); iter++)
		{
			if (!(*iter)->data["Name"].isNull())
			{
				OneList[Index++] = (*iter)->data["Name"].asString().c_str();
			}
		}
		m_bHasSendFristPipeName = true;
		OSLeaveMutex(hPipeVideoMutex);
	}
	else
	{
		if (type == AddPipeVideoCommand || type == DelPipeVideoCommand)
			OneList[Json::UInt(0)] = NewName;
		else
		{
			OneList[Json::UInt(0)]["OldName"] = OldName;
			OneList[Json::UInt(0)]["NewName"] = NewName;
		}
	}
	

	std::string &strList = PipeVideoNameList.toStyledString();
	NickHeader->len = strList.length();

	int Sendlen = strList.length() + sizeof MSGHeader;
	memcpy(pModerVideoName->send_buffer + sizeof MSGHeader, strList.c_str(), strList.length());
	pModerVideoName->send_buffer[Sendlen] = 0;


	m_pPipeControl->asyn_write_pipe(m_processID, pModerVideoName->send_buffer, Sendlen + 1, (ULL64)pModerVideoName, 0);

}


void PipeVideo::SetCanEnterScene(bool bCanEnter)
{
	m_bEnterScene = bCanEnter;
}

bool PipeVideo::CanEnterScene() const
{
	return m_bEnterScene;
}

bool PipeVideo::IsFieldSignal() const
{
	return bInterlaceSignal;
}

DWORD PipeVideo::AudioThread(LPVOID Param)
{
	PipeVideo *Video = (PipeVideo*)Param;
	Video->m_qwrdAudioTime = GetQPCMS();
	while (Video->bThreadRuning)
	{
		if (Video->m_LastTimeStamp > 0)
		{
			ListParam __AudioParam;
			OSEnterMutex(Video->hAudioMutex);

			while(Video->ListAudioSample.size() > 0)
			{
				__AudioParam = Video->ListAudioSample.front();

				if (__AudioParam.TimeStamp <= Video->m_LastTimeStamp)
				{
					if (Video->audioOut)
					{
						Video->audioOut->ReceiveAudio(__AudioParam.pData->lpData, __AudioParam.len, __AudioParam.TimeStamp,true);
					}
					__AudioParam.pData->Release();

					Video->ListAudioSample.pop_front();
				}
				else
					break;
			}

			OSLeaveMutex(Video->hAudioMutex);
		}

		//检测是否要重置音频

		if (GetQPCMS() - Video->m_qwrdAudioTime > 1000)
		{
			OSEnterMutex(Video->hAudioMutex);

			if (Video->audioOut)
			{
				Video->audioOut->ResetAudioDB();
			}

			OSLeaveMutex(Video->hAudioMutex);

			Video->m_qwrdAudioTime = GetQPCMS();
		}

		Sleep(10);
	}
	return 0;
}

void PipeVideo::ChangeShader()
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

void PipeVideo::RenameSource(const char *NewName)
{
	if (NewName)
	{
		CPipeServer::GetInstance()->ProcessNameCommand(NewName,data["Name"].asString().c_str(),RenamePipeVideoCommand);
		data["Name"] = NewName;
	}
}

