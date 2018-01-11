#include "SLiveManager.h"
#include "Instance.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

bool CSLiveManager::ManagerInit(uint64_t hwnd)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	RECT Rect;
	GetClientRect((HWND)hwnd, &Rect);
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s initD3D Invoke begin! width = %d,heigth = %d", __FUNCTION__, Rect.right, Rect.bottom);

	m_D3DRender = new D3DAPI(Rect.right, Rect.bottom, (HWND)hwnd, BSParam.DeviceSetting.AdpterID);

	if (!m_D3DRender)
	{
		BUTEL_THORWERROR("m_D3DRender ¥¥Ω® ß∞‹");
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s initD3D Invoke end!", __FUNCTION__);

	mainVertexShader = m_D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader"));

	if (!mainVertexShader)
	{
		BUTEL_THORWERROR("º”‘ÿmainVertexShader ß∞‹");
	}


	mainPixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawTexture.pShader"));

	if (!mainPixelShader)
	{
		BUTEL_THORWERROR("º”‘ÿmainPixelShader ß∞‹");
	}

	yuvScalePixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DownscaleBilinear1YUV.pShader"));
	if (!yuvScalePixelShader)
	{
		BUTEL_THORWERROR("º”‘ÿyuvScalePixelShader ß∞‹");
	}

	transitionPixel = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/SceneTransition.pShader"));

	if (!transitionPixel)
	{
		BUTEL_THORWERROR("º”‘ÿtransitionPixel ß∞‹");
	}


	circleTransitionPixel = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/CircleDiffuse.pShader"));

	if (!circleTransitionPixel)
	{
		BUTEL_THORWERROR("º”‘ÿcircleTransitionPixel ß∞‹");
	}

	solidVertexShader = m_D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawSolid.vShader"));
	if (!solidVertexShader)
	{
		BUTEL_THORWERROR("º”‘ÿsolidVertexShader ß∞‹");
	}

	solidPixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawSolid.pShader"));

	if (!solidPixelShader)
	{
		BUTEL_THORWERROR("º”‘ÿsolidPixelShader ß∞‹");
	}

	BulidD3D();

	HVideoCapture = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainVideoCapture, this, 0, NULL);
	SetThreadPriority(HVideoCapture, THREAD_PRIORITY_HIGHEST);

	HVideoEncoder = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)VideoEncoderThread, this, 0, NULL);
	SetThreadPriority(HVideoEncoder, THREAD_PRIORITY_HIGHEST);

	HAudioCapture = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainAudioCapture, this, 0, NULL);
	SetThreadPriority(HAudioCapture, THREAD_PRIORITY_HIGHEST);
	
	HVideoEncoder_back = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)VideoEncoderThread_back, this, 0, NULL);
	SetThreadPriority(HVideoEncoder_back, THREAD_PRIORITY_HIGHEST);

	HLittlePreview = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RenderLittlePreviewThreadProc, this, 0, NULL);

	HStatus = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckStatusThreadProc, this, 0, NULL);

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return true;
}

void CSLiveManager::BulidD3D()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	bUseBack = BSParam.LiveSetting.bUseLiveSec;
	if (bUseBack)
	{
		if (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec)
		{
			baseCX = BSParam.LiveSetting.WidthSec;
			baseCY = BSParam.LiveSetting.HeightSec;

			baseCX_back = BSParam.LiveSetting.Width;
			baseCY_back = BSParam.LiveSetting.Height;
		}
		else
		{
			baseCX = BSParam.LiveSetting.Width;
			baseCY = BSParam.LiveSetting.Height;

			baseCX_back = BSParam.LiveSetting.WidthSec;
			baseCY_back = BSParam.LiveSetting.HeightSec;
		}

// 		baseCX_back = BSParam.LiveSetting.WidthSec;
// 		baseCY_back = BSParam.LiveSetting.HeightSec;
// 		baseCX = BSParam.LiveSetting.Width;
// 		baseCY = BSParam.LiveSetting.Height;
	}
	else
	{

		if (baseCX == BSParam.LiveSetting.Width && baseCY == BSParam.LiveSetting.Height)
			return;

		baseCX = BSParam.LiveSetting.Width;
		baseCY = BSParam.LiveSetting.Height;
	}

	baseCX = MIN(MAX(baseCX, 128), 4096);
	baseCY = MIN(MAX(baseCY, 128), 4096);

	baseCX_back = MIN(MAX(baseCX_back, 128), 4096);
	baseCY_back = MIN(MAX(baseCY_back, 128), 4096);

	scaleCX = double(baseCX);
	scaleCY = double(baseCY);

	//align width to 128bit for fast SSE YUV4:2:0 conversion
	outputCX = scaleCX & 0xFFFFFFFC;
	outputCY = scaleCY & 0xFFFFFFFE;

	outputCX_back = baseCX_back & 0xFFFFFFFC;
	outputCY_back = baseCY_back & 0xFFFFFFFE;

	for (int i = 0; i < 2; ++i)
	{
		if (mainRenderTextures[i])
		{
			delete mainRenderTextures[i];
			mainRenderTextures[i] = NULL;
		}
		mainRenderTextures[i] = m_D3DRender->CreateRenderTarget(baseCX, baseCY, GS_BGRA, FALSE);
	}


	if (transitionTexture)
	{
		delete transitionTexture;
		transitionTexture = NULL;
	}

	transitionTexture = m_D3DRender->CreateRenderTarget(baseCX, baseCY, GS_BGRA, FALSE);

	if (transNewTexture)
	{
		delete transNewTexture;
		transNewTexture = NULL;
	}

	transNewTexture = m_D3DRender->CreateRenderTarget(baseCX, baseCY, GS_BGRA, FALSE);

	if (PreRenderTexture)
	{
		delete PreRenderTexture;
		PreRenderTexture = NULL;
	}

	PreRenderTexture = m_D3DRender->CreateRenderTarget(baseCX, baseCY, GS_BGRA, FALSE);

	if (yuvRenderTextures)
	{
		delete yuvRenderTextures;
		yuvRenderTextures = NULL;
	}
	yuvRenderTextures = m_D3DRender->CreateRenderTarget(outputCX, outputCY, GS_BGRA, FALSE);

	if (copyTextures)
	{
		delete copyTextures;
		copyTextures = NULL;
	}

	copyTextures = m_D3DRender->CreateTextureRead(outputCX, outputCY);


	if (copyRGBTexture)
	{
		delete copyRGBTexture;
		copyRGBTexture = NULL;
	}

	copyRGBTexture = m_D3DRender->CreateTextureRead(outputCX, outputCY);

	if (bUseBack)
	{
		if (yuvRenderTextures_back)
		{
			delete yuvRenderTextures_back;
			yuvRenderTextures_back = NULL;
		}
		yuvRenderTextures_back = m_D3DRender->CreateRenderTarget(outputCX_back, outputCY_back, GS_BGRA, FALSE);

		if (copyTextures_back)
		{
			delete copyTextures_back;
			copyTextures_back = NULL;
		}

		copyTextures_back = m_D3DRender->CreateTextureRead(outputCX_back, outputCY_back);

	}

	if (SDITexture)
	{
		delete SDITexture;
		SDITexture = NULL;
	}

	UINT Width, Heigth;

	SDITexture = m_D3DRender->CreateTextureFromFile(L"./img/SDIOUT.png", FALSE, Width, Heigth);

	if (ss)
	{
		delete ss;
		ss = NULL;
	}

	SamplerInfo si;
	zero(&si, sizeof(si));
	si.filter = GS_FILTER_LINEAR;

	si.addressU = GS_ADDRESS_BORDER;
	si.addressV = GS_ADDRESS_BORDER;
	ss = m_D3DRender->CreateSamplerState(si);

	colorDesc.fullRange = false;
	colorDesc.primaries = ColorPrimaries_BT709;
	colorDesc.transfer = ColorTransfer_IEC6196621;
	colorDesc.matrix = outputCX >= 1280 || outputCY > 576 ? ColorMatrix_BT709 : ColorMatrix_SMPTE170M;

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

DWORD CSLiveManager::MainVideoCapture(LPVOID lparam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,ThreadId %d", __FUNCTION__, GetCurrentThreadId());
	CSLiveManager *__this = (CSLiveManager *)lparam;
	__this->MainVideoLoop();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

DWORD CSLiveManager::MainAudioCapture(LPVOID lparam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,ThreadId %d", __FUNCTION__, GetCurrentThreadId());
	CSLiveManager *__this = (CSLiveManager *)lparam;
	CoInitialize(0);
	__this->MainAudioLoop();
	CoUninitialize();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}
DWORD CSLiveManager::VideoEncoderThread(LPVOID lparam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,ThreadId %d", __FUNCTION__, GetCurrentThreadId());
	CSLiveManager *__this = (CSLiveManager *)lparam;
	__this->VideoEncoderLoop();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

DWORD CSLiveManager::VideoEncoderThread_back(LPVOID lparam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,ThreadId %d", __FUNCTION__, GetCurrentThreadId());
	CSLiveManager *__this = (CSLiveManager *)lparam;
	__this->VideoEncoderLoop_back();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

DWORD CSLiveManager::RenderLittlePreviewThreadProc(LPVOID lParam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,ThreadId %d", __FUNCTION__, GetCurrentThreadId());
	CSLiveManager *__this = (CSLiveManager *)lParam;
	__this->RenderLittlePreview();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}
