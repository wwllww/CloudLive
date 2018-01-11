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

bool PipeVideo::Init(XElement *data)
{
	b_flag = false;
	/*m_iListIndex =*/ CPipeServer::GetInstance()->AddVideoRender(this);
	String strNumber = data->GetString(TEXT("Num"), L"");
	this->data = data;
	//m_Server = new Server(this);
	SetParaToIneraction();
	//m_cmdSend.Start(strNumber);
	//m_Server->Start(strNumber);
	m_size = Vect2(-1, -1);

	m_pAudioWaveOut = NULL;
	bitmapImage = NULL;
	FileInfo = 0;

	hAudioMutex = OSCreateMutex();

	texture = NULL;
	audioOut = NULL;
	drawShader = NULL;

    hSampleMutex = OSCreateMutex();
    if(!hSampleMutex)
    {
        AppWarning(TEXT("PipeVideoPlugin: could not create sample mutex"));
        return false;
    }

    int numThreads = MAX(OSGetTotalCores()-2, 1);
	hConvertThreads = (HANDLE*)Allocate_Bak(sizeof(HANDLE)*numThreads);
	convertData = (ConvertData*)Allocate_Bak(sizeof(ConvertData)*numThreads);

    zero(hConvertThreads, sizeof(HANDLE)*numThreads);
    zero(convertData, sizeof(ConvertData)*numThreads);

   
    UpdateSettings();

    Log(TEXT("Using directshow input"));

	if (!bLoadSucceed)
		return false;

    return true;
}

PipeVideo::~PipeVideo()
{
	//if (m_Server)
	{
		//m_Server->Stop();
		//delete m_Server;
	//	m_Server = NULL;
	}

	CPipeServer::GetInstance()->RemoveVideoRender(this);
	m_cmdSend.Stop();	
    Stop();
    UnloadFilters();

    FlushSamples();

    if(hConvertThreads)
        Free(hConvertThreads);

    if(convertData)
        Free(convertData);

    if(hSampleMutex)
        OSCloseMutex(hSampleMutex);

	if (bitmapImage)
	{
		delete bitmapImage;
		bitmapImage = NULL;
	}

	if (FileInfo)
	{
		API->RemoveStreamInfo(FileInfo);
		FileInfo = 0;
	}

	if (NULL != m_pAudioWaveOut)
	{
		delete m_pAudioWaveOut;
		m_pAudioWaveOut = NULL;
	}

	if (hAudioMutex)
	{
		OSCloseMutex(hAudioMutex);
		hAudioMutex = NULL;
	}
	delete[]m_pDefaultImgbuffer;
	m_pDefaultImgbuffer = NULL;
}

#define SHADER_PATH TEXT("plugins/PipeVideoPlugin/shaders/")

String PipeVideo::ChooseShader()
{
    if(colorType == DeviceOutputType_RGB)
        return String();

    String strShader;
    strShader << SHADER_PATH;

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
    else if(colorType == DeviceOutputType_HDYC)
        strShader << TEXT("HDYCToRGB.pShader");
    else
        strShader << TEXT("RGB.pShader");

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
#include "streams.h"

bool PipeVideo::LoadFilters()
{
    if(bCapturing )
        return false;

    bool bSucceeded = false;

	if (!audioOut)
	{
		audioOut = new PipeAudioSource;
		audioOut->Initialize(this);
		API->AddAudioSource(audioOut);
	}

    HRESULT err;
    String strShader;
    bUseThreadedConversion = API->UseMultithreadedOptimizations() && (OSGetTotalCores() > 1);

    renderCX = newCX =640;
    renderCY = newCY = 480;

    bFirstFrame = true;
	colorType = DeviceOutputType_I420;
 /*   strShader = ChooseShader();
    if(strShader.IsValid())
        colorConvertShader = CreatePixelShaderFromFile(strShader);*/

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

	int audio = data->GetInt(TEXT("AudioLocalOut"), 0);

	if (audio)
	{

		String strReanderName = data->GetString(TEXT("RenderAudioName"));

		OSEnterMutex(hAudioMutex);
		if (NULL == m_pAudioWaveOut)
		{
			if (!strReanderName.Compare(TEXT("Disable")))
			{
				m_pAudioWaveOut = new AudioWaveOut;
			}
		}
		if (NULL != m_pAudioWaveOut)
		{
			m_pAudioWaveOut->Initialize(strReanderName.Array(), inputChannels, inputSamplesPerSec, inputBitsPerSample);
		}
		OSLeaveMutex(hAudioMutex);
	}
	//-----------------------

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

    KillThreads();

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

	OSEnterMutex(hAudioMutex);
	if (m_pAudioWaveOut)
	{
		delete m_pAudioWaveOut;
		m_pAudioWaveOut = NULL;
	}
	
	if (audioOut)
	{
		API->RemoveAudioSource(audioOut);
		delete audioOut;
		audioOut = NULL;
	}
	OSLeaveMutex(hAudioMutex);
}

void PipeVideo::Start()
{
	OSEnterMutex(hAudioMutex);
	if (!audioOut)
	{
		audioOut = new PipeAudioSource;
		audioOut->Initialize(this);
		API->AddAudioSource(audioOut);
	}
	OSLeaveMutex(hAudioMutex);

	if (bitmapImage)
	{
		delete bitmapImage;
		bitmapImage = NULL;
	}
	String path = L".\\plugins\\PipeVideoPlugin\\img\\PIPE_1920x1080.jpg"; //默认是16:9
// 	if (size.x / size.y < 1.55) //更接近4：3
// 	{
// 		path = L".\\plugins\\PipeVideoPlugin\\img\\PIPE_1024x768.jpg";
// 	}

	if (!OSFileExists(path)) // 文件不存在
	{
		if (FileInfo)
		{
			API->RemoveStreamInfo(FileInfo);
			FileInfo = 0;
		}

		String strInfo;
		strInfo << TEXT("背景图") << path << TEXT("不存在");
		FileInfo = API->AddStreamInfo(strInfo, StreamInfoPriority_Critical);

		return;
	}

	if (FileInfo)
	{
		API->RemoveStreamInfo(FileInfo);
		FileInfo = 0;
	}

	bitmapImage = new BitmapImage();
	bitmapImage->SetPath(path);
	bitmapImage->EnableFileMonitor(false);
	bitmapImage->Init();

	
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
	OSEnterMutex(hAudioMutex);
	if (audioOut)
	{
		API->RemoveAudioSource(audioOut);
		delete audioOut;
		audioOut = NULL;
	}
	OSLeaveMutex(hAudioMutex);
	Start();

	//if (bCapturing)
	//	return;

	String strShader = ChooseShader();

	if (colorConvertShader)
	{
		delete colorConvertShader;
		colorConvertShader = NULL;
	}

	if (strShader.IsValid())
		colorConvertShader = CreatePixelShaderFromFile(strShader);
	ChangeSize(bLoadSucceed, true);

	drawShader = CreatePixelShaderFromFile(TEXT("shaders\\DrawTexture_ColorAdjust.pShader"));
	bCapturing = true;
}

void PipeVideo::EndScene()
{
	OSEnterMutex(hAudioMutex);
	if (audioOut)
	{
		API->RemoveAudioSource(audioOut);
		delete audioOut;
		audioOut = NULL;
	}
	OSLeaveMutex(hAudioMutex);
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

    KillThreads();

    int numThreads = MAX(OSGetTotalCores()-2, 1);
    for(int i = 0; i < numThreads; i++)
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

        if(i == (numThreads - 1))
            convertData[i].endY = renderCY;
        else
            convertData[i].endY = ((renderCY/numThreads)*(i + 1)) & 0xFFFFFFFE;
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

    //-----------------------------------------------------
    // create the texture regardless, will just show up as red to indicate failure
	BYTE *textureData = (BYTE*)Allocate_Bak(renderCX*renderCY * 4);

    if(colorType == DeviceOutputType_RGB) //you may be confused, but when directshow outputs RGB, it's actually outputting BGR
    {
        msetd(textureData, 0xFFFF0000, renderCX*renderCY*4);
        texture = CreateTexture(renderCX, renderCY, GS_BGR, textureData, FALSE, FALSE);
    }
    else //if we're working with planar YUV, we can just use regular RGB textures instead
    {
        msetd(textureData, 0xFF0000FF, renderCX*renderCY*4);
        texture = CreateTexture(renderCX, renderCY, GS_RGB, textureData, FALSE, FALSE);
    }

    if(bSucceeded && bUseThreadedConversion)
    {
        if(colorType == DeviceOutputType_I420 || colorType == DeviceOutputType_YV12)
        {
            LPBYTE lpData;
            if(texture->Map(lpData, texturePitch))
                texture->Unmap();
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
// 	FILE* f = fopen(".\\plugins\\PipeVideoPlugin\\img\\互动连接源_1024x768.bmp", "rb");
// 	fseek(f, 0, SEEK_END);
// 	long size = ftell(f);
// 	fseek(f, 0, SEEK_SET);
// 	char* buffer = new char[size];
// 	fread(buffer, size, 1, f);
	if (NULL != m_dataCallBack)
	{
		int offset = *((int*)(m_pDefaultImgbuffer + 10));

		CSampleData   *VideoSample = new CSampleData;

		VideoSample->bAudio = false;
		VideoSample->dataLength = m_nDefaultImgSize - offset;
		VideoSample->lpData = (LPBYTE)Allocate_Bak(VideoSample->dataLength);//pointer; //
		VideoSample->cx = 1024;
		VideoSample->cy = 768;
		VideoSample->colorType = DeviceOutputType_RGB24;

		memcpy(VideoSample->lpData, m_pDefaultImgbuffer + offset, VideoSample->dataLength);

		m_dataCallBack(m_content, VideoSample, true);
		VideoSample->Release();
	}	
// 	delete[] buffer;
// 	buffer = NULL;
}

//数据处理的起点
void PipeVideo::ReceiveMediaSample(ISampleData *sample, bool bAudio)
{
	if (!sample /*|| !bCapturing*/)
		return;

	OSEnterMutex(hSampleMutex);

	if (bCapturing && bAudio && NULL != m_pAudioWaveOut && (m_dataCallBack == NULL || (m_dataCallBack&&enteredSceneCount)))
	{
		m_pAudioWaveOut->push_pcm_data((char*)sample->lpData, sample->AInfo->Datalen);
	}

	if (m_dataCallBack)
	{
		if (bAudio)//音频
		{
			CSampleData      *audioSample = new CSampleData;
			audioSample->bAudio = bAudio;
			audioSample->dataLength = sample->AInfo->Datalen;
			audioSample->lpData = (LPBYTE)Allocate_Bak(audioSample->dataLength);//pointer; //
			memcpy(audioSample->lpData, sample->lpData, audioSample->dataLength);
			audioSample->pAudioFormat = (void*)&audioFormat;

			m_dataCallBack(m_content, audioSample, true);
			audioSample->Release();
		} 
		else  //视频
		{
			CSampleData   *VideoSample = new CSampleData;

			VideoSample->bAudio = false;
			VideoSample->dataLength = sample->VInfo->Datalen;
			VideoSample->lpData = (LPBYTE)Allocate_Bak(VideoSample->dataLength);//pointer; //
			VideoSample->cx = sample->VInfo->Width;
			VideoSample->cy = sample->VInfo->Height;
			VideoSample->colorType = colorType;

			memcpy(VideoSample->lpData, sample->lpData, VideoSample->dataLength);

			m_dataCallBack(m_content, VideoSample, true);
			VideoSample->Release();
		}
	}

	//这个if必须放在最后
	if (bCapturing && (m_dataCallBack == NULL || (m_dataCallBack&&enteredSceneCount)))
	{
		if (bAudio)
		{
			OSEnterMutex(hAudioMutex);
			bool AudioOut = data->GetInt(TEXT("AudioOut"), 1);
			//音频数据的处理
			if (AudioOut&&audioOut)
			{
				audioOut->ReceiveAudio(sample->lpData, sample->AInfo->Datalen);
			}
			OSLeaveMutex(hAudioMutex);

			sample->Release();
		}
		else
		{
			//视频数据的处理
			SafeRelease(latestVideoSample);
			latestVideoSample = sample;
		}
	}
	else
	{
		sample->Release();
	}
	OSLeaveMutex(hSampleMutex);
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
    if(!bCapturing)
        return;

    ISampleData *lastSample = NULL;

    OSEnterMutex(hSampleMutex);
    lastSample = latestVideoSample;
    latestVideoSample = NULL;
    OSLeaveMutex(hSampleMutex);

    int numThreads = MAX(OSGetTotalCores()-2, 1);

    if(lastSample)
    { 
		newCX = lastSample->VInfo->Width;
		newCY = lastSample->VInfo->Height;

        if(colorType == DeviceOutputType_RGB)
        {
            if(texture)
            {
                ChangeSize();

                texture->SetImage(lastSample->lpData, GS_IMAGEFORMAT_BGRX, linePitch);
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
                    texture->SetImage(lpImageBuffer, GS_IMAGEFORMAT_RGBX, texturePitch);

                    bReadyToDraw = true;
                }
				else
				{
					bFirstFrame = false;						
				} 		
				if (!(renderCX == newCX && renderCY == newCY))
				{
					bFirstFrame = true;
					bReadyToDraw = false;
				}
				
                ChangeSize();

                for(int i=0; i<numThreads; i++)
                    lastSample->AddRef();

                for(int i = 0; i < numThreads; i++)
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

                if(texture->Map(lpData, pitch))
                {
                    PackPlanar(lpData, lastSample->lpData, renderCX, renderCY, pitch, 0, renderCY, linePitch, lineShift);
                    texture->Unmap();
                }

                bReadyToDraw = true;
            }
        }
        else if(colorType == DeviceOutputType_YVYU || colorType == DeviceOutputType_YUY2)
        {
            LPBYTE lpData;
            UINT pitch;

            ChangeSize();

            if(texture->Map(lpData, pitch))
            {
                Convert422To444(lpData, lastSample->lpData, pitch, true);
                texture->Unmap();
            }

            bReadyToDraw = true;
        }
        else if(colorType == DeviceOutputType_UYVY || colorType == DeviceOutputType_HDYC)
        {
            LPBYTE lpData;
            UINT pitch;

            ChangeSize();

            if(texture->Map(lpData, pitch))
            {
                Convert422To444(lpData, lastSample->lpData, pitch, false);
                texture->Unmap();
            }

            bReadyToDraw = true;
        }

        lastSample->Release();
    }
}

void PipeVideo::Render(const Vect2 &pos, const Vect2 &size)
{
	//if (m_Server->IsConnected() == false) //当连接不成功时调用该函数
	if(false)
	{
		Texture *local_texture = bitmapImage->GetTexture();
		if (local_texture)
		{
			DrawSprite(local_texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
		}

		return;
	}

	m_size = Vect2(-1, -1);

    if(texture && bReadyToDraw)
    {
        Shader *oldShader = GetCurrentPixelShader();
        SamplerState *sampler = NULL;

        float fGamma = float(100) * 0.01f;

        if(colorConvertShader)
        {
            LoadPixelShader(colorConvertShader);
            colorConvertShader->SetFloat  (colorConvertShader->GetParameterByName(TEXT("gamma")),  fGamma);
        }
        else {
           if(fGamma != 1.0f) {
                LoadPixelShader(drawShader);
                HANDLE hGamma = drawShader->GetParameterByName(TEXT("gamma"));
                if(hGamma)
                    drawShader->SetFloat(hGamma, fGamma);
            }
        }

        float x, x2;
        float y, y2;
		y = pos.y;
		y2 = y + size.y;

		int width = texture->Width();
		int height = texture->Height();
		 
		if (width > height)
		{
			x = pos.x;
			x2 = x + size.x;
		}
		else
		{
			int w = size.y * width / height;

			int RenderSide = data->GetInt(TEXT("RenderSide"), 0);

			if (RenderSide == 0) //居左
			{
				x = pos.x;
				x2 = x + w;
			}
			else if (RenderSide == 1) //居中
			{
				x = pos.x + (size.x - w)/2;
				x2 = x + w;
			}
			else  //居右
			{
				x = pos.x + size.x - w;
				x2 = x + w;
			}	
		}
 
		DWORD opacity255 = DWORD(255.0f);
		DrawSprite(texture, (opacity255 << 24) | 0xFFFFFF, x, y, x2, y2);

        if(colorConvertShader || fGamma != 1.0f)
            LoadPixelShader(oldShader);
    }
}

void PipeVideo::UpdateSettings()
{
     API->EnterSceneMutex();

     bool bWasCapturing = bCapturing;
     if(bWasCapturing) Stop();
	 //m_cmdSend.Stop();
	 SetParaToIneraction();
	 OSEnterMutex(hSampleMutex);	
     UnloadFilters();
     LoadFilters();
	 OSLeaveMutex(hSampleMutex);	

     if(bWasCapturing) Start();

	
	if (bWasCapturing)
	{
		BeginScene();
	}
	
     API->LeaveSceneMutex();
}

PipeVideo::PipeVideo()
{		
	FILE* f = fopen(".\\plugins\\PipeVideoPlugin\\img\\PIPE_1024x768.bmp", "rb");
	fseek(f, 0, SEEK_END);
	m_nDefaultImgSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	m_pDefaultImgbuffer = new char[m_nDefaultImgSize];
	fread(m_pDefaultImgbuffer, m_nDefaultImgSize, 1, f);
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

bool PipeVideo::SetParaToIneraction()
{
	if (NULL == data)
	{
		AppWarning(TEXT("PipeVideo::SetParaToIneraction empty para"));
		return FALSE;
	}
	// 设置启动互动机参数
	//if (NULL != m_Server)
	//{
	//	String strIneractionPara = L"";
	//	strIneractionPara = strIneractionPara + L"AppKey=" + data->GetString(TEXT("AppKey"), L"");
	//	strIneractionPara += L" ";
	//	strIneractionPara = strIneractionPara + L"NubeNum=" + data->GetString(TEXT("Num"), L"");
	//	strIneractionPara += L" ";
	//	strIneractionPara = strIneractionPara + L"UID=" + data->GetString(TEXT("UID"), L"");
	//	strIneractionPara += L" ";
	//	strIneractionPara = strIneractionPara + L"NickName=" + data->GetString(TEXT("NickName"), L"");
	//	m_Server->SetInterractionPara(strIneractionPara);
	//}

	// 设置互动机其他运行参数
	Command  cmdIneractionPara;
	String strNumber = data->GetString(TEXT("Num"), L"");

	cmdIneractionPara.m_AppKey = ws2s(data->GetString(TEXT("AppKey"), L""));
	cmdIneractionPara.m_visibleNo = ws2s(strNumber.Array());
	cmdIneractionPara.m_visiblePwd = ws2s(data->GetString(TEXT("UID"), L""));
	cmdIneractionPara.m_LocalNumber = "";
	cmdIneractionPara.m_monitorNo = ws2s(data->GetString(TEXT("MonitorISO"), L""));
	cmdIneractionPara.m_guideAudioCollect = ws2s(data->GetString(TEXT("DirectorAudioCapDevice"), L""));
	cmdIneractionPara.m_guideAudioPlay = ws2s(data->GetString(TEXT("DirectorAudioRenderDevice"), L""));
	cmdIneractionPara.m_guideCameraKey = ws2s(data->GetString(TEXT("DirectorVideoCapDevice"), L""));
	cmdIneractionPara.m_guideCameraValue = ws2s(data->GetString(TEXT("DirectorVideoCapDevice"), L""));

	cmdIneractionPara.m_hostAudioCollect = ws2s(data->GetString(TEXT("HostAudioCapDevice"), L""));
	cmdIneractionPara.m_hostAudioPlay = ws2s(data->GetString(TEXT("HostAudioRenderDevice"), L""));
	cmdIneractionPara.m_hostCameraKey = ws2s(data->GetString(TEXT("HostVideoCapDevice"), L""));
	cmdIneractionPara.m_hostCameraValue = ws2s(data->GetString(TEXT("HostVideoCapDevice"), L""));	
	m_cmdSend.SetIneractionPara(cmdIneractionPara);
	return true;
}

CPipeServer* CPipeServer::m_instance = NULL;
CPipeServer::CPipeServer()
{
	blive = true;
	bStop = false;

	m_iRequestNum = -1;
	bliveId = -1;

	m_pSetAudioDirectorMode = new StSetAudioMode;
	m_pSetAudioHostMode = new StSetAudioMode;

	m_pVideoHostTransMode = new StModerDataTransMode;
	m_pVideoHostTransMode->mutex_lock = OSCreateMutex();
	m_pVideoDirectorTransMode = new StModerDataTransMode;
	m_pVideoDirectorTransMode->mutex_lock = OSCreateMutex();
	m_pAudioDirectorTransMode = new StModerDataTransMode;
	m_pAudioDirectorTransMode->mutex_lock = OSCreateMutex();
	m_pAudioHostTransMode = new StModerDataTransMode;
	m_pAudioHostTransMode->mutex_lock = OSCreateMutex();

	m_pVideoHostModeTransMode = new StModerDataTransMode;
	m_pAudioHostModeTransMode = new StModerDataTransMode;

	pHostQueneList = new StQueneList;
	pDerectorQueneList = new StQueneList;
	pHostModeQueneList = new StQueneList;

	hPipeVideoMutex = OSCreateMutex();
	hPresenterVideoMutex = OSCreateMutex();
	hDirectorVideoMutex = OSCreateMutex();
	hAIOIDMutex = OSCreateMutex();

	Start();
}

CPipeServer::~CPipeServer()
{
	if (hPipeVideoMutex)
		OSCloseMutex(hPipeVideoMutex);
	if (hPresenterVideoMutex)
	{
		OSCloseMutex(hPresenterVideoMutex);
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
	if (m_pVideoHostModeTransMode)
	{
		delete m_pVideoHostModeTransMode;
		m_pVideoHostModeTransMode = NULL;
	}
	if (m_pAudioHostModeTransMode)
	{
		delete m_pAudioHostModeTransMode;
		m_pAudioHostModeTransMode = NULL;
	}

	if (pHostQueneList)
	{
		delete pHostQueneList;
		pHostQueneList = NULL;
	}

	if (pDerectorQueneList)
	{
		delete pDerectorQueneList;
		pDerectorQueneList = NULL;
	}

	if (pHostModeQueneList)
	{
		delete pHostModeQueneList;
		pHostModeQueneList = NULL;
	}
	Stop(); //停掉心跳线程
}

CPipeServer* CPipeServer::GetInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new CPipeServer;
	}
	return m_instance;
}

void CPipeServer::DestroyInstance()
{
	if (m_instance)
		delete m_instance;
	m_instance = NULL;
}

void  CPipeServer::Start() {

	//fp = fopen("E://SeeYouAgain_16k_1_16.pcm", "wb");
	//fpA = fopen("D://16k_1_16.pcm", "wb");
	//fpV = fopen("D://320_176.yuv", "wb");
	m_pPipeControl = create_pipe_control(this);
	
	ConfigFile  *AppConfig = BLiveGetProfileConfig();
	String strIni;
	ConfigFile * Gloable = BLiveGetGlobalConfig();
	if (NULL == Gloable)
	{
		AppConfig = NULL;
		Log(TEXT("管道打开互动机，无法打开全局配置"));
		return;
	}
	String InteractiveNumber = AppConfig->GetString(TEXT("Login"), TEXT("InteractiveNumber_TEST"), L"NULL");
	String strIneractionPara = L"";
	strIneractionPara = strIneractionPara + L"AppKey="+AppConfig->GetString(TEXT("Login"), TEXT("AppKey"), L"NULL");
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"NubeNum="+InteractiveNumber;
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"UID="+InteractiveNumber;
	strIneractionPara += L" ";
	strIneractionPara = strIneractionPara + L"NickName="+AppConfig->GetString(TEXT("Login"), TEXT("NickName"), L"Test");

	String strProfile = Gloable->GetString(TEXT("General"), TEXT("Profile"));
	strIni << API->GetAppDataPath() << TEXT("\\profiles\\") << strProfile << TEXT(".ini");
	String exePath = AppConfig->GetString(TEXT("Login"), TEXT("InteractionPath"));
	if (exePath.Length() == 0)
	{
		AppConfig = NULL;
		Log(TEXT("InteractionPath run fail"));
		return;
	}
	Log(TEXT("管道打开互动机%s para %s"), exePath.Array(), strIneractionPara);
	ShellExecute(NULL, L"open", exePath.Array(), strIneractionPara, 0, SW_SHOWNORMAL);
	char *CStr = "\\\\.\\Pipe\\Butel_PipeName";

	m_pPipeControl->listen_pipe(CStr, 1000, 0, 0);

	//m_thread = OSCreateThread((XTHREAD)CPipeServer::Thread_RUN, this);
	m_Hostthread = OSCreateThread((XTHREAD)CPipeServer::HostThread_RUN, this);
	m_Directorthread = OSCreateThread((XTHREAD)CPipeServer::DiretorThread_RUN, this);
	//m_HostModethread = OSCreateThread((XTHREAD)CPipeServer::HostModeThread_RUN, this);

	SendSize = 0;
	
}
  
void CPipeServer::on_pipe_accept(AsynIoErr st, const char *pipename, AIOID id, ULL64 ctx1, ULL64 ctx2){

	if (AIO_SUC == st)
	{
		StModerDataTransMode *pStModerDataTransMode = new StModerDataTransMode;
		pStModerDataTransMode->mutex_lock = OSCreateMutex();
		pStModerDataTransMode->id = id;
		m_pPipeControl->asyn_read_pipe(id, pStModerDataTransMode->receive_buffer,
			pStModerDataTransMode->recive_len, (ULL64)pStModerDataTransMode, 0);
	}
}

void CPipeServer::on_pipe_write(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2) 
{
	StModerDataTransMode *pModerDataTransMode = (StModerDataTransMode*)ctx1;
	if (st == AIO_SUC)
	{
		if (pModerDataTransMode->type == ParamSet || pModerDataTransMode->type == HostMode)
		{
			m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer,
				pModerDataTransMode->recive_len, (ULL64)pModerDataTransMode, 0);
		}
		else if (pModerDataTransMode->type == HostAVRequest || pModerDataTransMode->type == DirectorAVRequest)
		{
			return;
		}
	} 
	else
	{
		if (pModerDataTransMode->type == HostAVRequest && !pModerDataTransMode->bFirst)
			RemoveVideoCustomer(this, s2ws(oldDeviceParam->PresenterVideoCapture).c_str(), 
				s2ws(oldDeviceParam->PresenterVideoCaptureID).c_str(), HostAVRequest);
		else if (pModerDataTransMode->type == DirectorAVRequest&& !pModerDataTransMode->bFirst)
			RemoveVideoCustomer(this, s2ws(oldDeviceParam->DirectorVideoCapture).c_str(),
			s2ws(oldDeviceParam->DirectorVideoCaptureID).c_str(), DirectorAVRequest);
		else
			delete pModerDataTransMode;
	}
}

void CPipeServer::on_pipe_read(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2){

	StModerDataTransMode * pModerDataTransMode = (StModerDataTransMode *)ctx1;
	if (AIO_SUC == st)
	{
		if (pModerDataTransMode->bFirst)
		{
			AnalyzeCommand(id, pModerDataTransMode->receive_buffer, retlen, pModerDataTransMode->send_buffer, 
				pModerDataTransMode->send_len, pModerDataTransMode);
			m_pPipeControl->asyn_write_pipe(id, pModerDataTransMode->send_buffer, pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
		}
		else
		{
			AnalyzeHostModeCommand(id, buf, retlen, pModerDataTransMode);
		}
	}
	else if (AIO_FAILED == st || AIO_TIMEOUT == st)
	{
		if (pModerDataTransMode->type == HostMode)
		{
			OSEnterMutex(hPipeVideoMutex);
			std::list<PipeVideo*>::iterator iter;
			for (iter = PipeRenders.begin(); iter != PipeRenders.end(); iter++)
			{
				if ((*iter)->m_aioId == id)
				{
					(*iter)->m_aioId = -1;
					(*iter)->b_flag = false;
					break;
				}
			}
			OSLeaveMutex(hPipeVideoMutex);
		}
		m_pPipeControl->close_pipe(id);
	}
}

void CPipeServer::Stop() {

	bStop = true;
}

void CPipeServer::AudioProcess(std::wstring  DirectorAudioCapture, std::wstring  PresenterAudioCapture)
{
	if (DirectorAudioRecord)
	{
		 DirectorAudioRecord->RecordStop();
	}

	if (HostAudioRecord)
	{
		HostAudioRecord->RecordStop();
	}

	OSEnterMutex(hPresenterVideoMutex);
	m_pSetAudioDirectorMode->type = DirectorAVRequest;
	m_pSetAudioHostMode->type = HostAVRequest;
	DirectorAudioRecord = new PCMRecord(DirectorAudioCapture, m_pSetAudioDirectorMode);
	DirectorAudioRecord->SetCallback(ReceiveMediaSample);
	DirectorAudioRecord->RecordStart();
	HostAudioRecord = new PCMRecord(PresenterAudioCapture, m_pSetAudioHostMode);
	HostAudioRecord->SetCallback(ReceiveMediaSample);
	HostAudioRecord->RecordStart();
	OSLeaveMutex(hPresenterVideoMutex);
}

void CPipeServer::ReceiveMediaSample(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data)
{
	return;
	StSetAudioMode *pSetAudioMode = (StSetAudioMode*)data;
	StModerDataTransMode *pModerDataTransMode = NULL;

	if (pSetAudioMode->type == HostAVRequest)
		pModerDataTransMode = m_instance->m_pAudioHostTransMode;

	else if (pSetAudioMode->type == DirectorAVRequest)
		pModerDataTransMode = m_instance->m_pAudioDirectorTransMode;
	else 
		return;

	OSEnterMutex(pModerDataTransMode->mutex_lock);

	if (!pModerDataTransMode->bflag) {
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		return;
	}
	OSLeaveMutex(pModerDataTransMode->mutex_lock);
	QWORD curTime;
	curTime = GetQPCTimeMS();
	
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
	if (pSetAudioMode->type == HostAVRequest)
		m_instance->pHostQueneList->getData(pModerDataTransMode);
	if (pSetAudioMode->type == DirectorAVRequest)
		m_instance->pDerectorQueneList->getData(pModerDataTransMode);
	//m_instance->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer, pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
}

int CPipeServer::SetVideoData(CSampleData * RGBdata,
	unsigned long long nCtx) {
	
	StModerDataTransMode *pModerDataTransMode;
	if (nCtx == HostAVRequest)
	{
		pModerDataTransMode = m_pVideoHostTransMode;
	}
	else if (nCtx == DirectorAVRequest)
	{
		pModerDataTransMode = m_pVideoDirectorTransMode;
	}
	else
		return 0;

	OSEnterMutex(pModerDataTransMode->mutex_lock);
	if (!pModerDataTransMode->bflag) {
		OSLeaveMutex(pModerDataTransMode->mutex_lock);
		return 0;
	}
	OSLeaveMutex(pModerDataTransMode->mutex_lock);
	RGBdata->AddRef();
	
	
	TColorType color_type = (TColorType)RGBdata->colorType;
	//Log(TEXT("color_type = %d"), color_type);
	CSampleData * data;
	data = new CSampleData;
	data->cx = m_nWith;
	data->cy = m_nHeight;;
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
		//data = new CSampleData;
		//data->cx = m_nWith;
		//data->cy = m_nHeight;
		//data->dataLength = data->cx*data->cy * 3 / 2;
		//data->bAudio = false;
		//data->timestamp = RGBdata->timestamp;
		//data->lpData = (LPBYTE)Allocate_Bak(data->dataLength);
		ImgResizeYUV420(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);

	}
	else if (color_type == ColorType_YUY2 || color_type == ColorType_YVYU || color_type == ColorType_UYVY || color_type == ColorType_HDYC)
	{
		ImgResizeYUV422(RGBdata->lpData, data->lpData, RGBdata->cx, RGBdata->cy, data->cx, data->cy);
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

	//m_instance->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer, pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
	
	if (nCtx == HostAVRequest)
		m_instance->pHostQueneList->PushData(pModerDataTransMode);
	if (nCtx == DirectorAVRequest)
		m_instance->pDerectorQueneList->PushData(pModerDataTransMode);
	return 0;

}

void CPipeServer::ChangeSourceRegester(VideoCustomer* pCustomer, const String& OldDeviceName, 
	const String& OldDeviceNameID, const String& NewDeviceName,const String& NewDeviceNameID, 
	unsigned long long ctx1, unsigned long long ctx2) {

	RemoveVideoCustomer(pCustomer, OldDeviceName, OldDeviceNameID,ctx1);
	AddVideoCaptureVideoCustomer(pCustomer, NewDeviceName, NewDeviceNameID, ctx2);
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
		pModerDataTransMode->type = AudioData;
		int dataLen = msglen - sizeof(MSGHeader);
		AudioRawData pAudioRawData;
		memcpy(&pAudioRawData, pcommand + sizeof(MSGHeader), sizeof(AudioRawData));
		ISampleData *pISampleData = new ISampleData;
		pISampleData->bAudio = true;
		pISampleData->AInfo = new AudioDataInfo;
		pISampleData->AInfo->channels = pAudioRawData.nChannels;
		pISampleData->AInfo->samplerate = pAudioRawData.nSamplesPerSec;
		pISampleData->AInfo->Timestamp = pAudioRawData.timeStamp;
		pISampleData->AInfo->ampleperbits = pAudioRawData.wBitsPerSample;
		pISampleData->AInfo->Datalen = pAudioRawData.len;
		pISampleData->lpData = new unsigned char[pISampleData->AInfo->Datalen];
		memcpy(pISampleData->lpData, pcommand + sizeof(MSGHeader)+sizeof(AudioRawData), pISampleData->AInfo->Datalen);

		OSEnterMutex(hPipeVideoMutex);
		if (RenderMap.find(id) != RenderMap.end())
		{
			RenderMap[id]->ReceiveMediaSample(pISampleData, true);
			m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer, pModerDataTransMode->recive_len,
				(ULL64)pModerDataTransMode, 0);
		}
		else
		{
			pISampleData->Release();
			m_pPipeControl->close_pipe(id);
		}
		OSLeaveMutex(hPipeVideoMutex);
	}
	else if (oMSGHeader.type == VideoData)
	{
		int dataLen = msglen - sizeof(MSGHeader);
		VideoRawData pVideoRawData;
		memcpy(&pVideoRawData, pcommand + sizeof(MSGHeader), sizeof(VideoRawData));
		ISampleData *pISampleData = new ISampleData;
		pISampleData->bAudio = false;
		pISampleData->VInfo = new VideoDataInfo;
		pISampleData->VInfo->Width = pVideoRawData.width;
		pISampleData->VInfo->Height = pVideoRawData.height;
		pISampleData->VInfo->Timestamp = pVideoRawData.timeStamp;
		pISampleData->VInfo->Datalen = pVideoRawData.len;
		pISampleData->VInfo->FrameRate = pVideoRawData.nFramePerSec;
		pISampleData->lpData = new unsigned char[pISampleData->VInfo->Datalen];      // (LPBYTE)Allocate_Bak(pISampleData->VInfo->Datalen);

		memcpy(pISampleData->lpData, pcommand + sizeof(MSGHeader)+sizeof(VideoRawData), pISampleData->VInfo->Datalen);

		OSEnterMutex(hPipeVideoMutex);
		if (RenderMap.find(id) != RenderMap.end())
		{
			RenderMap[id]->ReceiveMediaSample(pISampleData, false);
			m_pPipeControl->asyn_read_pipe(id, pModerDataTransMode->receive_buffer, pModerDataTransMode->recive_len,
				(ULL64)pModerDataTransMode, 0);
		}
		else
		{
			pISampleData->Release();
			m_pPipeControl->close_pipe(id);
		}
		OSLeaveMutex(hPipeVideoMutex);
	}
	else if (oMSGHeader.type == ParamSet)
	{
		DeviceParam oDeviceParam;
		strResponse oResponse;

		if (msglen - sizeof(MSGHeader) < sizeof DeviceParam)
			return false;
		memset(&oDeviceParam, 0x00, sizeof DeviceParam);
		memcpy(&oDeviceParam, pcommand + sizeof(MSGHeader), sizeof DeviceParam);

		// m_nWith = oDeviceParam.nWidth;
		// m_nHeight = oDeviceParam.nHeight;
		m_nWith = 320;
		m_nHeight = 240;
		m_iRequestNum = oDeviceParam.nChannelNum;
		int retcode = RegisterDevice(oDeviceParam);
		//=======================
		AudioProcess(s2ws(oDeviceParam.DirectorAudioCapture).c_str(), s2ws(oDeviceParam.PresenterAudioCapture).c_str());
		//=======================
		if (oldDeviceParam == NULL)
		{
			oldDeviceParam = new DeviceParam;
		}
		memcpy(oldDeviceParam, &oDeviceParam, sizeof(oDeviceParam));
		sendMSGHeader.type = ParamSet;
		sendMSGHeader.len = sizeof(strResponse);
		oResponse.code = 0;
		memset(oResponse.description, 0, 256);
		memcpy(oResponse.description, "set success", strlen("set success"));
		memset(pModerDataTransMode->receive_buffer, 0, sizeof(pModerDataTransMode->receive_buffer));
		memcpy(pModerDataTransMode->receive_buffer, &sendMSGHeader, sizeof(sendMSGHeader));
		memcpy(pModerDataTransMode->receive_buffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
		SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
		m_pPipeControl->asyn_write_pipe(id, pModerDataTransMode->receive_buffer, SendSize,
			(ULL64)pModerDataTransMode, 0);
	}
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
					 pModerDataTransMode->type = ParamSet;
					 pModerDataTransMode->bFirst = false;
					 DeviceParam oDeviceParam;
					 strResponse oResponse;

					 if (msgLen - sizeof(MSGHeader) < sizeof DeviceParam)
						 return false;
					 memset(&oDeviceParam, 0x00, sizeof DeviceParam);
					 memcpy(&oDeviceParam, pcommand + sizeof(MSGHeader), sizeof DeviceParam);

					// m_nWith = oDeviceParam.nWidth;
					// m_nHeight = oDeviceParam.nHeight;
					 m_nWith = 320;
					 m_nHeight = 240;
					 m_iRequestNum = oDeviceParam.nChannelNum;

					 int retcode = RegisterDevice(oDeviceParam);

					 //=======================
					 AudioProcess(s2ws(oDeviceParam.DirectorAudioCapture).c_str(), s2ws(oDeviceParam.PresenterAudioCapture).c_str());
					 //=======================

					 if (oldDeviceParam == NULL)
					 {
						 oldDeviceParam = new DeviceParam;
					 }

					 memcpy(oldDeviceParam, &oDeviceParam, sizeof(oDeviceParam));

					 switch (retcode)
					 {
					 case 0:
					 {
							   sendMSGHeader.type = ParamSet;
							   sendMSGHeader.len = sizeof(strResponse);

							   oResponse.code = 0;
							   memset(oResponse.description, 0, 256);
							   memcpy(oResponse.description, "set success", strlen("set success"));

							   memset(messageBuffer, 0, sizeof(messageBuffer));
							   memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
							   memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
							   SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);

							  
							   break;
					 }
					 default:
						 Log(L"");
						 break;
					 }
					 break;
	}

	case HostAVRequest:
	{
						  
						  pModerDataTransMode->type = HostAVRequest;

						  m_pAudioHostTransMode->id = id;
						  m_pAudioHostTransMode->type = HostAVRequest;
						  m_pAudioHostTransMode->bflag = true;
						  m_pAudioHostTransMode->bFirst = false;

						  
						  m_pVideoHostTransMode->id = id;
						  m_pVideoHostTransMode->bflag = true;
						  m_pVideoHostTransMode->type = HostAVRequest;
						  m_pVideoHostTransMode->bFirst = false;

						  strResponse oResponse;
						  int retcode;
						  if (oldDeviceParam)
						  {
							  String PresenterDevice(s2ws(oldDeviceParam->PresenterVideoCapture).c_str());
							  String PresenterDeviceID(s2ws(oldDeviceParam->PresenterVideoCaptureID).c_str());
							  retcode = CheckDeviceByName(PresenterDevice, PresenterDeviceID,0);
						  }
						  switch (retcode){
						  case 0:
						  {
									sendMSGHeader.type = HostAVRequest;
									sendMSGHeader.len = sizeof(oResponse);

									oResponse.code = 0;
									memset(oResponse.description, 0, 256);
									memcpy(oResponse.description, "PresenterAVRequest OK", strlen("PresenterAVRequest OK"));

									memset(messageBuffer, 0, sizeof(messageBuffer));
									memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
									memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
									SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
									break;
						  }
						  default:
							  break;
						  }

						  break;
	}
	case DirectorAVRequest:
	{
							 
							  pModerDataTransMode->type = DirectorAVRequest;

							  m_pAudioDirectorTransMode->id = id;
							  m_pAudioDirectorTransMode->bflag = true;
							  m_pAudioDirectorTransMode->type = DirectorAVRequest;
							  m_pAudioDirectorTransMode->bFirst = false;
							  m_pVideoDirectorTransMode->id = id;
							  m_pVideoDirectorTransMode->bflag = true;
							  m_pVideoDirectorTransMode->type = DirectorAVRequest;
							  m_pVideoDirectorTransMode->bFirst = false;
							  
							  strResponse oResponse;
							  int retcode;

							  if (oldDeviceParam)
							  {
								  String DirectorDevice(s2ws(oldDeviceParam->DirectorVideoCapture).c_str());
								  String DirectorDeviceID(s2ws(oldDeviceParam->DirectorVideoCaptureID).c_str());
								  retcode = CheckDeviceByName(DirectorDevice, DirectorDeviceID, 1);
							  }
						switch (retcode){
						case 0:
						{
								  sendMSGHeader.type = DirectorAVRequest;
								  sendMSGHeader.len = sizeof(oResponse);

								  oResponse.code = 0;
								  memset(oResponse.description, 0, 256);
								  memcpy(oResponse.description, "DirectorAVRequest OK", strlen("DirectorAVRequest OK"));

								  memset(messageBuffer, 0, sizeof(messageBuffer));
								  memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
								  memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
								  SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
								  break;
						}
						
						default:
							break;
						}

					 break;
	}
	
	case HostMode:
	{
					 pModerDataTransMode->type = HostMode;
					 pModerDataTransMode->bFirst = false;

					 m_pVideoHostModeTransMode->id = pModerDataTransMode->id;
					 m_pVideoHostModeTransMode->type = VideoData;
					 m_pVideoHostModeTransMode->bFirst = false;

					 m_pAudioHostModeTransMode->id = pModerDataTransMode->id;
					 m_pAudioHostModeTransMode->type = AudioData;
					 m_pAudioHostModeTransMode->bFirst = false;

					 DeviceParam oDeviceParam;
					 strResponse oResponse;
					 strHostMode oHostMode;

					 if (msgLen - sizeof(MSGHeader) < sizeof strHostMode)
					 {
						 Log(L"");
						 return false;
					 }
					 
					memset(&oHostMode, 0x00, sizeof strHostMode);
					memcpy(&oHostMode, pcommand + sizeof(MSGHeader), sizeof strHostMode);
					OSEnterMutex(hPipeVideoMutex);
					std::list<PipeVideo*>::iterator iter;
					for (iter = PipeRenders.begin(); iter != PipeRenders.end(); iter++)
				    {
						if (!(*iter)->b_flag){
							(*iter)->b_flag = true;
							memset((*iter)->name, 0x00, 512);
							memcpy((*iter)->name, oHostMode.ChannelNumber, 512);
							RenderMap[id] = (*iter);
							break;
						}
					}
					OSLeaveMutex(hPipeVideoMutex);
					sendMSGHeader.type = HostMode;
					sendMSGHeader.len = sizeof(oResponse);

					oResponse.code = 0;
					memset(oResponse.description, 0, 256);

					memset(messageBuffer, 0, sizeof(messageBuffer));
					memcpy(messageBuffer, &sendMSGHeader, sizeof(sendMSGHeader));
					memcpy(messageBuffer + sizeof(sendMSGHeader), &oResponse, sizeof(oResponse));
					SendSize = sizeof(sendMSGHeader)+sizeof(oResponse);
					
					break;
	}
	case HeartBeat:
	{
					 
					 blive = true;
					 break;
	}
	default:
		break;
	}
}

int CPipeServer::RegisterDevice(DeviceParam & oDeviceParam){

	int iDirectorCode;
	int iPresenterCode;

	String DirectorDevive(s2ws(oDeviceParam.DirectorVideoCapture).c_str());
	String DirectorDeviveID(s2ws(oDeviceParam.DirectorVideoCaptureID).c_str());
	String PresenterDevive(s2ws(oDeviceParam.PresenterVideoCapture).c_str());
	String PresenterDeviveID(s2ws(oDeviceParam.PresenterVideoCaptureID).c_str());
	
	if (oldDeviceParam)
	{
		RemoveVideoCustomer(this, s2ws(oldDeviceParam->DirectorVideoCapture).c_str(), s2ws(oldDeviceParam->DirectorVideoCaptureID).c_str(), DirectorAVRequest);
	}
	if (oldDeviceParam)
	{
		RemoveVideoCustomer(this, s2ws(oldDeviceParam->PresenterVideoCapture).c_str(), s2ws(oldDeviceParam->PresenterVideoCaptureID).c_str(), HostAVRequest);
	}

	iDirectorCode = AddVideoCaptureVideoCustomer(this, DirectorDevive, DirectorDeviveID, DirectorAVRequest);
	iPresenterCode = AddVideoCaptureVideoCustomer(this, PresenterDevive, PresenterDeviveID, HostAVRequest);
	
	Log(TEXT("iPresenterCode = %d,iDirectorCode = %d"), iPresenterCode, iDirectorCode);

	return iDirectorCode + iPresenterCode;
}

int CPipeServer::CheckDeviceByName(const String & DeviceName, const String & DeviceNameID, 
	int flag) {
	int code;
	code = GetVideoCaptureLinkNum(DeviceName, DeviceNameID);
	if (code > 0)      //有关联设备
	{
		code = 0;
	}
	else
	{
		if (flag)
		{
			code = 2;
		}
		else
		{
code = 4;
		}
	}
	return code;
}

DWORD STDCALL CPipeServer::HostThread_RUN(LPVOID lpUnused)
{
	CPipeServer *pThis = (CPipeServer *)lpUnused;
	StModerDataTransMode *pModerDataTransMode = new StModerDataTransMode;
	while (true)
	{
		bool bret = pThis->pHostQueneList->getData(pModerDataTransMode);
		if (bret && (pModerDataTransMode->id > 0))
		{
			pThis->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer,
				pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
		}
		Sleep(5);
	}
}
DWORD STDCALL CPipeServer::DiretorThread_RUN(LPVOID lpUnused)
{
	while (true)
	{
		CPipeServer *pThis = (CPipeServer *)lpUnused;
		StModerDataTransMode *pModerDataTransMode = new StModerDataTransMode;
		while (true)
		{
			bool bret = pThis->pDerectorQueneList->getData(pModerDataTransMode);
			if (bret && (pModerDataTransMode->id > 0))
			{
				pThis->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer,
					pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
			}
			Sleep(5);
		}
	}
}
DWORD STDCALL CPipeServer::HostModeThread_RUN(LPVOID lpUnused)
{
	/*while (true)
	{
	while (true)
	{
	CPipeServer *pThis = (CPipeServer *)lpUnused;
	StModerDataTransMode *pModerDataTransMode = new StModerDataTransMode;
	while (true)
	{
	bool bret = pThis->pDerectorQueneList->getData(pModerDataTransMode);
	if (bret && (pModerDataTransMode->id > 0))
	{
	pThis->m_pPipeControl->asyn_write_pipe(pModerDataTransMode->id, pModerDataTransMode->send_buffer,
	pModerDataTransMode->send_len, (ULL64)pModerDataTransMode, 0);
	}
	Sleep(20);
	}
	}
	}*/
}

DWORD STDCALL CPipeServer::Thread_RUN(LPVOID lpUnused)
{
	CPipeServer *pThis = (CPipeServer *)lpUnused;
	if (NULL != pThis)
	{
		pThis->Exec();
		return 0;
	}
	return 0;
}

void CPipeServer::Exec(){

	while (true)
	{
		//发送心跳，和接收无关
		MSGHeader oMSGHeader;
		oMSGHeader.type = HeartBeat;
		oMSGHeader.len = sizeof MSGHeader;
		m_pPipeControl->asyn_write_pipe(bliveId, (char *)&oMSGHeader, sizeof(MSGHeader), 0, 0);

		Sleep(5000);
		
		if (blive)
			blive = false;
		else
			break;

		
	}
	Stop();
}

bool CPipeServer::MonitorHeartBeat(){

	
	return true;

}

int CPipeServer::AddVideoRender(PipeVideo* pPipeVideo) {

	OSEnterMutex(hPipeVideoMutex);
	PipeRenders.push_back(pPipeVideo);
	OSLeaveMutex(hPipeVideoMutex);

	return PipeRenders.size();
}

void CPipeServer::RemoveVideoRender(PipeVideo* pipeVideo) {

	OSEnterMutex(hPipeVideoMutex);
	std::list<PipeVideo*>::iterator iter;
	for (iter = PipeRenders.begin(); iter != PipeRenders.end();iter++)
	{
		if (*iter == pipeVideo)
		{
			PipeRenders.erase(iter);
			break;
		}
	}
	
	OSLeaveMutex(hPipeVideoMutex);

	if (PipeRenders.size() == 0)
	{
		Stop();
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


	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];

			//pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
			//pDstImg[i*dstW + j+1] = pSrcImg[tSrcH*srcW + tSrcW+1];
			//pDstImg[i*dstW + j+2] = pSrcImg[tSrcH*srcW + tSrcW+2];

		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW + tSrcW];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW + tSrcW];
		}
	}
}

void CPipeServer::ImgResizeRGB32(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	for (int i = dstH-1; i >=0; i--)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			B = pSrcImg[tSrcH*srcW*4 + tSrcW*4];
			G = pSrcImg[tSrcH*srcW*4 + tSrcW*4 + 1];
			R = pSrcImg[tSrcH*srcW*4 + tSrcW*4 + 2];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);

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
	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	for (int i = dstH - 1; i >= 0; i--)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			B = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4+1];
			G = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 2];
			R = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 3];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);

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
	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	for (int i = dstH - 1; i >= 0; i--)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			B = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 0];
			G = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 1];
			R = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 2];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);

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


	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW*2 + tSrcW];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
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
			Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);

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