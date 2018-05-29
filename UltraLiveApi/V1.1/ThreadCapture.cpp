#include "BaseAfx.h"
#include "RTMPPublisherVector.h"
#include "OperatNew.h"
#include<sstream>


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


const float yuvFullMat[][16] = {
	{ 0.000000f, 1.000000f, 0.000000f, 0.000000f,
	0.000000f, 0.000000f, 1.000000f, 0.000000f,
	1.000000f, 0.000000f, 0.000000f, 0.000000f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.250000f, 0.500000f, 0.250000f, 0.000000f,
	-0.249020f, 0.498039f, -0.249020f, 0.501961f,
	0.498039f, 0.000000f, -0.498039f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.262700f, 0.678000f, 0.059300f, 0.000000f,
	-0.139082f, -0.358957f, 0.498039f, 0.501961f,
	0.498039f, -0.457983f, -0.040057f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.212600f, 0.715200f, 0.072200f, 0.000000f,
	-0.114123f, -0.383916f, 0.498039f, 0.501961f,
	0.498039f, -0.452372f, -0.045667f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.212200f, 0.701300f, 0.086500f, 0.000000f,
	-0.115691f, -0.382348f, 0.498039f, 0.501961f,
	0.498039f, -0.443355f, -0.054684f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.299000f, 0.587000f, 0.114000f, 0.000000f,
	-0.168074f, -0.329965f, 0.498039f, 0.501961f,
	0.498039f, -0.417046f, -0.080994f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },
};

const float yuvMat[][16] = {
	{ 0.000000f, 0.858824f, 0.000000f, 0.062745f,
	0.000000f, 0.000000f, 0.858824f, 0.062745f,
	0.858824f, 0.000000f, 0.000000f, 0.062745f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.214706f, 0.429412f, 0.214706f, 0.062745f,
	-0.219608f, 0.439216f, -0.219608f, 0.501961f,
	0.439216f, 0.000000f, -0.439216f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.225613f, 0.582282f, 0.050928f, 0.062745f,
	-0.122655f, -0.316560f, 0.439216f, 0.501961f,
	0.439216f, -0.403890f, -0.035325f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.182586f, 0.614231f, 0.062007f, 0.062745f,
	-0.100644f, -0.338572f, 0.439216f, 0.501961f,
	0.439216f, -0.398942f, -0.040274f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.182242f, 0.602293f, 0.074288f, 0.062745f,
	-0.102027f, -0.337189f, 0.439216f, 0.501961f,
	0.439216f, -0.390990f, -0.048226f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },

	{ 0.256788f, 0.504129f, 0.097906f, 0.062745f,
	-0.148223f, -0.290993f, 0.439216f, 0.501961f,
	0.439216f, -0.367788f, -0.071427f, 0.501961f,
	0.000000f, 0.000000f, 0.000000f, 1.000000f },
};


void Convert444toNV12(LPBYTE input, int width, int inPitch, int outPitch, int height, int startY, int endY, LPBYTE *output)
{
	LPBYTE lumPlane = output[0];
	LPBYTE uvPlane = output[1];

	__m128i lumMask = _mm_set1_epi32(0x0000FF00);
	__m128i uvMask = _mm_set1_epi16(0x00FF);

	for (int y = startY; y < endY; y += 2)
	{
		int yPos = y*inPitch;
		int uvYPos = (y >> 1)*outPitch;
		int lumYPos = y*outPitch;

		for (int x = 0; x < width; x += 4)
		{
			LPBYTE lpImagePos = input + yPos + (x * 4);
			int uvPos = uvYPos + x;
			int lumPos0 = lumYPos + x;
			int lumPos1 = lumPos0 + outPitch;

			__m128i line1 = _mm_load_si128((__m128i*)lpImagePos);
			__m128i line2 = _mm_load_si128((__m128i*)(lpImagePos + inPitch));

			//pack lum vals
			{
				__m128i packVal = _mm_packs_epi32(_mm_srli_si128(_mm_and_si128(line1, lumMask), 1), _mm_srli_si128(_mm_and_si128(line2, lumMask), 1));
				packVal = _mm_packus_epi16(packVal, packVal);

				*(LPUINT)(lumPlane + lumPos0) = packVal.m128i_u32[0];
				*(LPUINT)(lumPlane + lumPos1) = packVal.m128i_u32[1];
			}

			//do average, pack UV vals
			{
				__m128i addVal = _mm_add_epi64(_mm_and_si128(line1, uvMask), _mm_and_si128(line2, uvMask));
				__m128i avgVal = _mm_srai_epi16(_mm_add_epi64(addVal, _mm_shuffle_epi32(addVal, _MM_SHUFFLE(2, 3, 0, 1))), 2);
				avgVal = _mm_shuffle_epi32(avgVal, _MM_SHUFFLE(3, 1, 2, 0));

				*(LPUINT)(uvPlane + uvPos) = _mm_packus_epi16(avgVal, avgVal).m128i_u32[0];
			}
		}
	}
}

void CSLiveManager::MainVideoLoop()
{
	QWORD frameTimeNS = 1000000000 / BSParam.LiveSetting.FPS;

	HANDLE hMatrix = yuvScalePixelShader->GetParameterByName(TEXT("yuvMat"));
	HANDLE hScaleVal = yuvScalePixelShader->GetParameterByName(TEXT("baseDimensionI"));
	HANDLE hTransitionTime = transitionPixel->GetParameterByName(TEXT("transitionTime"));
	HANDLE HRadius = circleTransitionPixel->GetParameterByName(TEXT("radius"));
	HANDLE HClock = circleTransitionPixel->GetParameterByName(TEXT("clock"));
	int CopyI = 0;
	Vect2 baseSize = Vect2(float(baseCX), float(baseCY));
	Vect2 baseSize_back = Vect2(float(baseCX_back), float(baseCY_back));

	x264_picture_t* outPics[2] = { NULL };
	x264_picture_t* outPics_back[2] = { NULL };

	for (int i = 0; i < 2; ++i)
	{
		outPics[i] = new x264_picture_t;
		x264_picture_init(outPics[i]);
		x264_picture_alloc(outPics[i], X264_CSP_NV12, outputCX, outputCY);

		if (bUseBack)
		{
			outPics_back[i] = new x264_picture_t;
			x264_picture_init(outPics_back[i]);
			x264_picture_alloc(outPics_back[i], X264_CSP_NV12, outputCX_back, outputCY_back);
		}
	}


	int FrameCount = 0;

	QWORD LastVideoTimeNS = 0;
	QWORD lastStreamTime = 0;
	QWORD LastSendByte[4] = { 0 };
	double bpsTime = 0.0;
	int swapIndex = 0;
	PushStauts Status[4] = { 0 };
	int  LogShow = 0;
	QWORD StartStreamTime = GetQPCMS();
	int FPSCount = 0;

	//增加一个默认的图片源防止启动时不显示

	while (!LocalInstance)
		Sleep(1);

	IBaseVideo *ImageBlack = dynamic_cast<IBaseVideo*>(CreatStreamObject("BitmapImageSource"));
	VideoStruct InVideoStruct;
	if (ImageBlack)
	{
		Value data;
		data["path"] = "./img/Black.png";
		if (ImageBlack->Init(data))
		{
			InVideoStruct.VideoStream = shared_ptr<IBaseVideo>(ImageBlack);
			EnterCriticalSection(&LocalInstance->VideoSection);
			LocalInstance->m_VideoList.SetSize(LocalInstance->m_VideoList.Num() + 1);
			VideoStruct &VS = LocalInstance->m_VideoList[LocalInstance->m_VideoList.Num() - 1];
			VS = InVideoStruct;
			LeaveCriticalSection(&LocalInstance->VideoSection);
		}
	}

	bool bFirst = true;
	while (bRunning)
	{
		while (WaitForSingleObject(hVideoEvent, INFINITE) == WAIT_OBJECT_0)
		{
			frameTimeNS = 1000000000 / BSParam.LiveSetting.FPS;
			if (!bRunning)
				break;


			//profileIn("video thread frame");

			LastVideoTimeNS = sleepTargetTime;

			QWORD curStreamTime = LastVideoTimeNS;
			if (!lastStreamTime)
				lastStreamTime = curStreamTime - frameTimeNS;

			QWORD frameDelta = curStreamTime - lastStreamTime;

			double fSeconds = double(frameDelta)*0.000000001;

			if (bPleaseEnableProjector)
				ActuallyEnableProjector();
			else if (bPleaseDisableProjector)
				DisableProjector();


			bool bReBulid = false;
			bool bLiveIsHaveFieldSignal = false;
			bool bLocalIsHaveFieldSignal = false;
			EnterCriticalSection(&MapInstanceSec);
			for (int i = 0; i < m_InstanceList.GetSize(); ++i)
			{
				CInstanceProcess *Process = m_InstanceList.GetAt(i);
				if (!Process || (Process && Process->bNoPreView))
					continue;
				if (Process->bReBulid)
				{
					EnterCriticalSection(&Process->NetWorkSection);
					Process->BulidD3D();
					Process->bReBulid = false;
					LeaveCriticalSection(&Process->NetWorkSection);
					bReBulid = true;
				}


				if (Process->bResizeRenderView)
				{
					m_D3DRender->ResizeView(Process->SwapRender);
					Process->bResizeRenderView = false;
				}

				EnterCriticalSection(&Process->VideoSection);
				Process->SetHasPreProcess(false);
				LeaveCriticalSection(&Process->VideoSection);

				if (!bLiveIsHaveFieldSignal && Process->IsLiveInstance)
				{
					EnterCriticalSection(&Process->VideoSection);
					
					for (int j = 0; j < Process->m_VideoList.Num(); ++j)
					{
						VideoStruct &OneStruct = Process->m_VideoList[j];

						if (OneStruct.VideoStream && OneStruct.bRender)
						{
							bLiveIsHaveFieldSignal = OneStruct.VideoStream->IsFieldSignal();

							if (bLiveIsHaveFieldSignal)
								break;
						}
					}

					LeaveCriticalSection(&Process->VideoSection);
				}

				if (!bLocalIsHaveFieldSignal && !Process->IsLiveInstance && !Process->bLittlePre)
				{
					EnterCriticalSection(&Process->VideoSection);

					for (int j = 0; j < Process->m_VideoList.Num(); ++j)
					{
						VideoStruct &OneStruct = Process->m_VideoList[j];

						if (OneStruct.VideoStream && OneStruct.bRender)
						{
							bLocalIsHaveFieldSignal = OneStruct.VideoStream->IsFieldSignal();

							if (bLocalIsHaveFieldSignal)
								break;
						}
					}

					LeaveCriticalSection(&Process->VideoSection);
				}
				
			}
			LeaveCriticalSection(&MapInstanceSec);

			if (bReBulid)
			{
				BulidD3D();
				frameTimeNS = 1000000000 / BSParam.LiveSetting.FPS;
				baseSize = Vect2(float(baseCX), float(baseCY));
				baseSize_back = Vect2(float(baseCX_back), float(baseCY_back));
				for (int i = 0; i < 2; ++i)
				{
					if (outPics[i])
					{
						delete outPics[i];
						outPics[i] = NULL;
					}

					outPics[i] = new x264_picture_t;
					x264_picture_init(outPics[i]);
					x264_picture_alloc(outPics[i], X264_CSP_NV12, outputCX, outputCY);

					if (bUseBack)
					{
						if (outPics_back[i])
						{
							delete outPics_back[i];
							outPics_back[i] = NULL;
						}
						outPics_back[i] = new x264_picture_t;
						x264_picture_init(outPics_back[i]);
						x264_picture_alloc(outPics_back[i], X264_CSP_NV12, outputCX_back, outputCY_back);
					}
					else
					{
						if (outPics_back[i])
						{
							delete outPics_back[i];
							outPics_back[i] = NULL;
						}
					}
				}

			}

			//EnterCriticalSection(&MapInstanceSec);

			for (int i = 0; i < 2; ++i)
			{
				if (!LiveInstance || !LocalInstance)
					break;

				CInstanceProcess *Process = i == 0 ? LocalInstance : LiveInstance;

				bool bHasEffect = false;
			
				EnterCriticalSection(&Process->VideoSection);
				//------------------------------------
				// render the mini render texture

				Texture *RenderTexture = NULL;

				//profileIn("MainVideoLoop PreProcess")
				Process->DrawPreProcess(fSeconds);
				//profileOut

				m_D3DRender->LoadVertexShader(mainVertexShader);
				m_D3DRender->LoadPixelShader(mainPixelShader);

				m_D3DRender->EnableBlending(TRUE);
				m_D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);

				m_D3DRender->Ortho(0.0f, baseCX, baseCY, 0.0f, -100.0f, 100.0f);
				m_D3DRender->SetViewport(0, 0, baseCX, baseCY);// 一定要在PreProcess之后

				if (Process->IsLiveInstance)
				{
					//if (!bTransUpDown && !bTransDiffuse)
					{
						m_D3DRender->SetRenderTarget(mainRenderTextures[swapIndex]);
						Process->DrawRender(mainRenderTextures[swapIndex], mainVertexShader, mainPixelShader);

						RenderTexture = mainRenderTextures[swapIndex];
					}

					if (bTransDisSolving || bTransUpDown || bTransDiffuse || bRadius || bClock
						|| bTransDownUp || bTransLeftRight || bTransRightLeft)
					{

						bHasEffect = true;

						Process->DrawTransFormProcess(fSeconds);
						m_D3DRender->SetRenderTarget(transNewTexture);

						Process->DrawTransFormRender(transNewTexture, mainVertexShader, mainPixelShader);

						m_D3DRender->SetRenderTarget(transitionTexture);
						m_D3DRender->ClearRenderTarget(0xFF000000);

					
						if (bTransDisSolving)
						{
							Shader *oldPixelShader = m_D3DRender->GetCurrentPixelShader();
							m_D3DRender->LoadPixelShader(transitionPixel);

							transitionPixel->SetFloat(hTransitionTime, TransEscapeTime);
							m_D3DRender->LoadTexture(transNewTexture, 1U);

							m_D3DRender->DrawSpriteEx(mainRenderTextures[swapIndex], 0xFFFFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);

							m_D3DRender->LoadTexture(nullptr, 1U);
							m_D3DRender->LoadPixelShader(oldPixelShader);
						}
						else if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft)
						{
							m_D3DRender->LoadSamplerState(ss, 0);

						
							if (bTransUpDown)
							{
								m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
									0, 0, baseSize.x, baseSize.y, 0.0f, 1.0f - TransEscapeTime, 1.0f, 2.0f - TransEscapeTime);
							}
							else if (bTransDownUp)
							{
								m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
									0, 0, baseSize.x, baseSize.y, 0.0f, TransEscapeTime - 1.0f, 1.0f, TransEscapeTime);
							}
							else if (bTransLeftRight)
							{
								m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
									0, 0, baseSize.x, baseSize.y, 1.0f - TransEscapeTime, 0.0f, 2.0f - TransEscapeTime, 1.0f);
							}
							else if (bTransRightLeft)
							{
								m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
									0, 0, baseSize.x, baseSize.y, TransEscapeTime - 1.0f, 0.0f, TransEscapeTime, 1.0f);
							}
							//保持切换之前的画面
							m_D3DRender->SetRenderTarget(transitionAddress.get());
							m_D3DRender->ClearRenderTarget(0xFF000000);

							m_D3DRender->DrawSpriteEx(mainRenderTextures[swapIndex], 0xFFFFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);

							m_D3DRender->EnableBlending(TRUE);
							m_D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
							m_D3DRender->DrawSpriteEx(transitionTexture, 0xFFFFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);
							m_D3DRender->EnableBlending(FALSE);
						}
						else if (bTransDiffuse)
						{
							m_D3DRender->DrawSprite(mainRenderTextures[swapIndex], 0xFFFFFFFF,
								baseSize.x * TransEscapeTime,
								baseSize.y * TransEscapeTime, 
								baseSize.x * (1 - TransEscapeTime), 
								baseSize.y * (1 - TransEscapeTime));

							float halfWidth = baseSize.x / 2;
							float halfHeight = baseSize.y / 2;
							
							m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
								halfWidth * (1.0f - TransEscapeTime),
								halfHeight * (1.0f - TransEscapeTime),
								halfWidth * (1.0f + TransEscapeTime),
								halfHeight * (1.0f + TransEscapeTime),
								0.0f, 0.0f, 1.0f, 1.0f);
						}
						else if (bRadius || bClock)
						{
							Shader *oldPixelShader = m_D3DRender->GetCurrentPixelShader();
							m_D3DRender->LoadPixelShader(circleTransitionPixel);
							circleTransitionPixel->SetFloat(HRadius, TransEscapeTime);
							if (bRadius)
								circleTransitionPixel->SetFloat(HClock, 0.0f);
							else if (bClock)
							{
								circleTransitionPixel->SetFloat(HClock, 1.0f);
							}
							m_D3DRender->EnableBlending(TRUE);
							m_D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
							m_D3DRender->DrawSpriteEx(transNewTexture, 0xFFFFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);
	
							m_D3DRender->LoadPixelShader(oldPixelShader);


							m_D3DRender->SetRenderTarget(transitionAddress.get());
							m_D3DRender->ClearRenderTarget(0xFF000000);

							
							m_D3DRender->DrawSpriteEx(mainRenderTextures[swapIndex], 0x66FFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);

							
							m_D3DRender->DrawSpriteEx(transitionTexture, 0xFFFFFFFF,
								0, 0, baseSize.x, baseSize.y, 0.0f, 0.0f, 1.0f, 1.0f);
							m_D3DRender->EnableBlending(FALSE);
						}

						if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft || bRadius || bClock)
						{
							RenderTexture = transitionAddress.get();
						}
						else
						{
							RenderTexture = transitionTexture;
						}
						
						//Draw Top 源
						Process->DrawTransFormRender(RenderTexture, mainVertexShader, mainPixelShader,true);

						TransEscapeTime += frameTimeNS / (1000000 * TransFormTime);

						if (TransEscapeTime >= 1.0f)
						{
							Process->ClearVideo(false,false,false);
							Process->CopyNewToVideoList();
							Process->ClearVideoTransForm();
						}
					}

				}
				else
				{
					//profileIn("MainVideoLoop Render")
					m_D3DRender->SetRenderTarget(PreRenderTexture);
					Process->DrawRender(PreRenderTexture, mainVertexShader, mainPixelShader);
					RenderTexture = PreRenderTexture;
					//profileOut
				}

				if (!bHasEffect)
				{
					//Draw Top 源
					Process->DrawRender(RenderTexture, mainVertexShader, mainPixelShader, true);
				}


				m_D3DRender->EnableBlending(FALSE);

				//做场信号处理
				if (Process->IsLiveInstance)
				{
					if (bLiveIsHaveFieldSignal && Deinterlacer)
					{
						Deinterlacer->SetImage(RenderTexture, &DeinterConfig, outputCX, outputCY, DeviceOutputType_RGB);
						RenderTexture = Deinterlacer->GetRenderTexture();
					}
				}
				else if (bLocalIsHaveFieldSignal && DeinterlacerLocal)
				{
					DeinterlacerLocal->SetImage(RenderTexture, &DeinterConfig, outputCX, outputCY, DeviceOutputType_RGB);
					RenderTexture = DeinterlacerLocal->GetRenderTexture();
				}


				//profileIn("MainVideoLoop DrawPreview")
				if (RenderTexture)
					Process->DrawPreview(RenderTexture, Process->renderFrameSize, Process->renderFrameOffset, Process->renderFrameCtrlSize, solidVertexShader, solidPixelShader, SDITexture, HaveSDIOut(),bStartView);
				//profileOut

				if (m_bProject && Process->IsLiveInstance)
				{
					Vect2 renderFrameSize, renderFrameOffset;
					Vect2 projectorSize = Vect2(float(projectorWidth), float(projectorHeight));

					float projectorAspect = (projectorSize.x / projectorSize.y);
					float baseAspect = (baseSize.x / baseSize.y);

					if (projectorAspect < baseAspect) {
						float fProjectorWidth = float(projectorWidth);

						renderFrameSize = Vect2(fProjectorWidth, fProjectorWidth / baseAspect);
						renderFrameOffset = Vect2(0.0f, (projectorSize.y - renderFrameSize.y) * 0.5f);
					}
					else {
						float fProjectorHeight = float(projectorHeight);

						renderFrameSize = Vect2(fProjectorHeight * baseAspect, fProjectorHeight);
						renderFrameOffset = Vect2((projectorSize.x - renderFrameSize.x) * 0.5f, 0.0f);
					}

					DrawPreviewProject(RenderTexture, renderFrameSize, renderFrameOffset, projectorSize);
				}

				LeaveCriticalSection(&Process->VideoSection);
			}

			//LeaveCriticalSection(&MapInstanceSec);

			bpsTime += fSeconds;

			if (bpsTime >= 1.0f)
			{
				FPS = FrameCount;
				FrameCount = 0;

				//LastVideoTimeNS = 0;
				if (bpsTime > 2.0f)
				{
					bpsTime = 0.0f;
				}
				else
				{
					bpsTime -= 1.0f;
				}

				if (LiveInstance)
				{
					double strain[4] = { 0.0 };
					QWORD CurrentSentByte[4] = { 0 };
					memset(Status, 0, sizeof PushStauts * 4);
					for (int i = 0; i < 4; ++i)
						Status[i].Color = RGB(100, 100, 100);

					CRITICAL_SECTION *NetSection = &LiveInstance->NetWorkSection, *NetSection_back = &LiveInstance->NetWorkSection_back;
// 					if (bUseBack && (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec))
// 					{
// 						NetSection = &LiveInstance->NetWorkSection_back;
// 						NetSection_back = &LiveInstance->NetWorkSection;
// 					}
					EnterCriticalSection(NetSection);

					RTMPPublisherVectorBase *NetWork = LiveInstance->network, *NetWork_back = LiveInstance->network_back;
// 					if (bUseBack && (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec))
// 					{
// 						NetWork = LiveInstance->network_back;
// 						NetWork_back = LiveInstance->network;
// 					}

					if (NetWork || NetWork_back)
					{
						for (int i = 0; i < 2; ++i)
						{
							DWORD green = 0xFF;

							if (NetWork)
							{
								CurrentSentByte[i] = NetWork->GetCurrentSentBytes(i);
								strain[i] = NetWork->GetPacketStrain(i);
								Status[i].LostTotalNum = NetWork->NumDroppedFrames(i);

								if (NetWork->NumTotalVideoFrames(i) != 0)
								{
									UINT numTotalFrames = NetWork->NumTotalVideoFrames(i);

									if (numTotalFrames)
										Status[i].LostPercent = (float(NetWork->NumDroppedFrames(i)) / float(numTotalFrames))*100.0;

									if (strain[i] > 50.0)
										green = DWORD(((50.0 - (strain[i] - 50.0)) / 50.0)*255.0);

									double redStrain = strain[i] / 50.0;
									if (redStrain > 1.0)
										redStrain = 1.0;

									//--------------------------------
									Status[i].Color = (green << 8) | DWORD(redStrain*255.0);
								}


								if (CurrentSentByte[i] >= LastSendByte[i])
								{
									Status[i].BitRate = (CurrentSentByte[i] - LastSendByte[i]) * 8 / 1024;

									LastSendByte[i] = CurrentSentByte[i];
								}
								else
								{
									Status[i].BitRate = 0;
									LastSendByte[i] = 0;
								}

							}

							EnterCriticalSection(NetSection_back);
 							NetWork_back = LiveInstance->network_back;//这个不能注释掉
// 							if (bUseBack && (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec))
// 							{
// 								NetWork_back = LiveInstance->network;
// 							}

							if (bUseBack && NetWork_back)
							{
								green = 0xFF;
								CurrentSentByte[i + 2] = NetWork_back->GetCurrentSentBytes(i);
								strain[i + 2] = NetWork_back->GetPacketStrain(i);
								Status[i + 2].LostTotalNum = NetWork_back->NumDroppedFrames(i);

								if (0 != NetWork_back->NumTotalVideoFrames(i))
								{
									UINT numTotalFrames = NetWork_back->NumTotalVideoFrames(i);

									if (numTotalFrames)
										Status[i + 2].LostPercent = (float(NetWork_back->NumDroppedFrames(i)) / float(numTotalFrames))*100.0;

									if (strain[i + 2] > 50.0)
										green = DWORD(((50.0 - (strain[i + 2] - 50.0)) / 50.0)*255.0);

									double redStrain = strain[i + 2] / 50.0;
									if (redStrain > 1.0)
										redStrain = 1.0;

									//--------------------------------
									Status[i + 2].Color = (green << 8) | DWORD(redStrain*255.0);
								}

								if (CurrentSentByte[i + 2] >= LastSendByte[i + 2])
								{
									Status[i + 2].BitRate = (CurrentSentByte[i + 2] - LastSendByte[i + 2]) * 8 / 1024;

									LastSendByte[i + 2] = CurrentSentByte[i + 2];
								}
								else
								{
									Status[i + 2].BitRate = 0;
									LastSendByte[i + 2] = 0;
								}
							}
							LeaveCriticalSection(NetSection_back);
						}
					}
					LeaveCriticalSection(NetSection);

					if (BSParam.PushStatus2)
						BSParam.PushStatus2(uint64_t(LiveInstance),FPS,Status);

					++LogShow;

					if (LogShow >= 60)
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "\nFPS   : %d\n"
							                                "URL1   : %d kb/s Lost %d (%.2f)\n"
															"URL2   : %d kb/s Lost %d (%.2f)\n"
															"URL3   : %d kb/s Lost %d (%.2f)\n"
															"URL4   : %d kb/s Lost %d (%.2f)", 
															FPS, Status[0].BitRate, Status[0].LostTotalNum, Status[0].LostPercent,
															Status[1].BitRate, Status[1].LostTotalNum, Status[1].LostPercent,
															Status[2].BitRate, Status[2].LostTotalNum, Status[2].LostPercent,
															Status[3].BitRate, Status[3].LostTotalNum, Status[3].LostPercent);
						LogShow = 0;
					}
					
					if (LiveInstance->bRequestKeyframe)
					{
						if (LiveInstance->videoEncoder)
							LiveInstance->videoEncoder->RequestKeyframe();

						if (bUseBack && LiveInstance->videoEncoder_back)
						{
							LiveInstance->videoEncoder_back->RequestKeyframe();
						}
					}

				}

				if ((GetQPCMS() - StartStreamTime >= m_CheckTime) && m_EncodeAudioCount)
				{
					m_FPSCount = FPSCount;
					m_RealTime = GetQPCMS() - StartStreamTime;
					StartStreamTime = GetQPCMS();
					FPSCount = 0;
					SetEvent(m_CheckEvent);
				}
			}

			++FrameCount;
			++FPSCount;
			if (!bStartLive)
			{
				if (TransEscapeTime >= 1.0f)
				{
					if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft)
						m_D3DRender->LoadSamplerState(NULL, 0);
					bTransDisSolving = false;
					bTransUpDown = false;
					bTransDiffuse = false;
					bRadius = false;
					bClock = false;
					bTransDownUp = bTransLeftRight = bTransRightLeft = false;
				}
			}

			RenderSDI(swapIndex);//输出SDI

			HRESULT result;
			try{

				if (bStartLive)
				{
					//profileIn("MainVideoLoop bStartLive")
					m_D3DRender->LoadVertexShader(mainVertexShader);
					m_D3DRender->LoadPixelShader(yuvScalePixelShader);

					m_D3DRender->SetRenderTarget(yuvRenderTextures);

					switch (colorDesc.matrix)
					{
					case ColorMatrix_GBR:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[0] : (float*)yuvMat[0]);
						break;
					case ColorMatrix_YCgCo:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[1] : (float*)yuvMat[1]);
						break;
					case ColorMatrix_BT2020NCL:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[2] : (float*)yuvMat[2]);
						break;
					case ColorMatrix_BT709:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[5] : (float*)yuvMat[5]);
						break;
					case ColorMatrix_SMPTE240M:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[4] : (float*)yuvMat[4]);
						break;
					default:
						yuvScalePixelShader->SetMatrix(hMatrix, colorDesc.fullRange ? (float*)yuvFullMat[5] : (float*)yuvMat[5]);
					}


					yuvScalePixelShader->SetVector2(hScaleVal, 1.0f / baseSize);


					m_D3DRender->Ortho(0.0f, outputCX, outputCY, 0.0f, -100.0f, 100.0f);
					m_D3DRender->SetViewport(0.0f, 0.0f, outputCX, outputCY);

					if (!bLiveIsHaveFieldSignal)
					{
						if (bTransDisSolving || bTransUpDown || bTransDiffuse || bRadius || bClock || bTransDownUp || bTransLeftRight || bTransRightLeft)
						{
							if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft || bRadius || bClock)
							{
								m_D3DRender->DrawSpriteEx(transitionAddress.get(), 0xFFFFFFFF, 0.0f, 0.0f, outputCX, outputCY, 0.0f, 0.0f, 1.0f, 1.0f);
							}
							else
							{
								m_D3DRender->DrawSpriteEx(transitionTexture, 0xFFFFFFFF, 0.0f, 0.0f, outputCX, outputCY, 0.0f, 0.0f, 1.0f, 1.0f);
							}
							
						}
						else
							m_D3DRender->DrawSpriteEx(mainRenderTextures[swapIndex], 0xFFFFFFFF, 0.0f, 0.0f, outputCX, outputCY, 0.0f, 0.0f, 1.0f, 1.0f);
					}
					else if (Deinterlacer)
					{
						m_D3DRender->DrawSpriteEx(Deinterlacer->GetRenderTexture(), 0xFFFFFFFF, 0.0f, 0.0f, outputCX, outputCY, 0.0f, 0.0f, 1.0f, 1.0f);
					}

					bool bSameResolut = bUseBack && (outputCX == outputCX_back) && (outputCY == outputCY_back);

					if (!bSameResolut && bUseBack)
					{
						m_D3DRender->SetRenderTarget(yuvRenderTextures_back);

						m_D3DRender->Ortho(0.0f, outputCX_back, outputCY_back, 0.0f, -100.0f, 100.0f);
						m_D3DRender->SetViewport(0.0f, 0.0f, outputCX_back, outputCY_back);

						yuvScalePixelShader->SetVector2(hScaleVal, 1.0f / baseSize_back);

						if (!bLiveIsHaveFieldSignal)
						{
							if (bTransDisSolving || bTransUpDown || bTransDiffuse || bRadius || bClock || bTransDownUp || bTransLeftRight || bTransRightLeft)
							{
								if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft || bRadius || bClock)
								{
									m_D3DRender->DrawSpriteEx(transitionAddress.get(), 0xFFFFFFFF, 0.0f, 0.0f, outputCX_back, outputCY_back, 0.0f, 0.0f, 1.0f, 1.0f);
								}
								else
								{
									m_D3DRender->DrawSpriteEx(transitionTexture, 0xFFFFFFFF, 0.0f, 0.0f, outputCX_back, outputCY_back, 0.0f, 0.0f, 1.0f, 1.0f);
								}
							}
							else
								m_D3DRender->DrawSpriteEx(mainRenderTextures[swapIndex], 0xFFFFFFFF, 0.0f, 0.0f, outputCX_back, outputCY_back, 0.0f, 0.0f, 1.0f, 1.0f);
						}
						else if (Deinterlacer)
						{
							m_D3DRender->DrawSpriteEx(Deinterlacer->GetRenderTexture(), 0xFFFFFFFF, 0.0f, 0.0f, outputCX_back, outputCY_back, 0.0f, 0.0f, 1.0f, 1.0f);
						}
					}

					if (TransEscapeTime >= 1.0f)
					{
						if (bTransUpDown || bTransDownUp || bTransLeftRight || bTransRightLeft)
							m_D3DRender->LoadSamplerState(NULL, 0);
						bTransDisSolving = false;
						bTransUpDown = false;
						bTransDiffuse = false;
						bRadius = false;
						bClock = false;
						bTransDownUp = bTransLeftRight = bTransRightLeft = false;
					}

					BYTE *lpData;
					UINT Pitch;
					//profileIn("video thread copyTextures");
					m_D3DRender->CopyTexture(copyTextures, yuvRenderTextures);
					//profileOut

					if (FAILED(result = m_D3DRender->Map(copyTextures, lpData, Pitch)))
					{
						BUTEL_THORWERROR("Map Failed! ");
					}

					//profileIn("MainVideoLoop Convert444toNV12")
					Convert444toNV12((LPBYTE)lpData, outputCX, Pitch, outputCX, outputCY, 0, outputCY, outPics[swapIndex]->img.plane);
					//profileOut
					m_D3DRender->Unmap(copyTextures);

					InterlockedExchangePointer((volatile PVOID*)&Outpic, outPics[swapIndex]);


					if (!bSameResolut && bUseBack)
					{
						BYTE *lpData;
						UINT Pitch;
						//profileIn("video thread copyTexturesback");
						m_D3DRender->CopyTexture(copyTextures_back, yuvRenderTextures_back);
						//profileOut

						if (FAILED(result = m_D3DRender->Map(copyTextures_back, lpData, Pitch)))
						{
							BUTEL_THORWERROR("Map Failed! ");
						}
						//profileIn("MainVideoLoop Convert444toNV12_back")
						Convert444toNV12((LPBYTE)lpData, outputCX_back, Pitch, outputCX_back, outputCY_back, 0, outputCY_back, outPics_back[swapIndex]->img.plane);
						//profileOut
						m_D3DRender->Unmap(copyTextures_back);

						InterlockedExchangePointer((volatile PVOID*)&Outpic_back, outPics_back[swapIndex]);
					}
					else if (bSameResolut)
					{
						InterlockedExchangePointer((volatile PVOID*)&Outpic_back, outPics[swapIndex]);
					}
				}
			//profileOut


			}
			catch (CErrorBase& e)
			{
				if (result == DXGI_ERROR_DEVICE_REMOVED)
				{
					String message;

					HRESULT reason = m_D3DRender->GetDeviceRemovedReason();

					switch (reason)
					{
					case DXGI_ERROR_DEVICE_RESET:
					case DXGI_ERROR_DEVICE_HUNG:
						message = TEXT("Your video card or driver froze and was reset. Please check for possible hardware / driver issues.");
						break;
					case DXGI_ERROR_DEVICE_REMOVED:
						message = TEXT("Your video card disappeared from the system. Please check for possible hardware / driver issues.");
						break;
					case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
						message = TEXT("Your video driver reported an internal error. Please check for possible hardware / driver issues.");
						break;
					case DXGI_ERROR_INVALID_CALL:
						message = TEXT("Your video driver reported an invalid call. Please check for possible driver issues.");
						break;
					default:
						message = TEXT("DXGI_ERROR_DEVICE_REMOVED");
						break;
					}

					message << TEXT(" This error can also occur if you have enabled opencl in x264 custom settings.");

					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:Texture->Map failed: 0x%08x 0x%08x\r\n\r\n%s -- %s", result, reason, WcharToAnsi(message.Array()).c_str(), e.m_Error.c_str());

					delete m_D3DRender;
					m_D3DRender = NULL;
					RECT Rect;

					if (LiveInstance)
					{
						GetClientRect(LiveInstance->RenderHwnd, &Rect);
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s initD3D Invoke begin! width = %d,heigth = %d", __FUNCTION__, Rect.right, Rect.bottom);
						m_D3DRender = new D3DAPI(Rect.right, Rect.bottom, LiveInstance->RenderHwnd, BSParam.DeviceSetting.AdpterID);
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s initD3D Invoke end!", __FUNCTION__);
					}


					try
					{
						if (mainVertexShader)
							delete mainVertexShader;

						mainVertexShader = m_D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader"));

						if (!mainVertexShader)
						{
							BUTEL_THORWERROR("加载mainVertexShader失败");
						}

						if (mainPixelShader)
							delete mainPixelShader;

						mainPixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawTexture.pShader"));

						if (!mainPixelShader)
						{
							BUTEL_THORWERROR("加载mainPixelShader失败");
						}

						if (yuvScalePixelShader)
							delete yuvScalePixelShader;

						yuvScalePixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DownscaleBilinear1YUV.pShader"));

						if (!yuvScalePixelShader)
						{
							BUTEL_THORWERROR("加载yuvScalePixelShader失败");
						}


						if (transitionPixel)
							delete transitionPixel;

						transitionPixel = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/SceneTransition.pShader"));

						if (!transitionPixel)
						{
							BUTEL_THORWERROR("加载transitionPixel失败");
						}


						if (circleTransitionPixel)
							delete circleTransitionPixel;

						circleTransitionPixel = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/CircleDiffuse.pShader"));

						if (!circleTransitionPixel)
						{
							BUTEL_THORWERROR("加载circleTransitionPixel失败");
						}

						if (solidVertexShader)
							delete solidVertexShader;

						solidVertexShader = m_D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawSolid.vShader"));
						if (!solidVertexShader)
						{
							BUTEL_THORWERROR("加载solidVertexShader失败");
						}

						if (solidPixelShader)
							delete solidPixelShader;

						solidPixelShader = m_D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawSolid.pShader"));

						if (!solidPixelShader)
						{
							BUTEL_THORWERROR("加载solidPixelShader失败");
						}


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

						baseCX = BSParam.LiveSetting.Width;
						baseCY = BSParam.LiveSetting.Height;

						baseCX = MIN(MAX(baseCX, 128), 4096);
						baseCY = MIN(MAX(baseCY, 128), 4096);

						scaleCX = double(baseCX);
						scaleCY = double(baseCY);

						outputCX = scaleCX & 0xFFFFFFFC;
						outputCY = scaleCY & 0xFFFFFFFE;

						BulidD3D();

						EnterCriticalSection(&MapInstanceSec);
						for (int i = 0; i < m_InstanceList.GetSize(); ++i)
						{
							CInstanceProcess *Process = m_InstanceList.GetAt(i);
							if (Process)
							{
								Process->BulidD3D();
							}
						}
						LeaveCriticalSection(&MapInstanceSec);
					}
					catch (CErrorBase& e1)
					{
						Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s", e1.m_Error.c_str());
					}


				}

			}
			m_D3DRender->Flush();

			lastStreamTime = curStreamTime;

			//LastVideoTimeNS += frameTimeNS;

			++swapIndex %= 2;

// 			EnterCriticalSection(&MapInstanceSec);
// 			//profileIn("MainVideoLoop DrawLittlePreview");
// 			for (int i = 0; i < m_InstanceList.GetSize(); ++i)
// 			{
// 				CInstanceProcess *Process = m_InstanceList.GetAt(i);
// 				if (Process && Process->bLittlePre && !Process->bNoPreView)
// 				{
// 					//Process->DrawLittlePreview(mainVertexShader, mainPixelShader,SDITexture);
// 					Process->DrawLittlePreview(mainLittleVertexShader, mainLittlePixelShader, SDILittleTexture);
// 				}
// 			}
// 			//profileOut
// 			LeaveCriticalSection(&MapInstanceSec);

			if (bFirst)
			{
				bFirst = false;
				EnterCriticalSection(&LocalInstance->VideoSection);

				for (UINT i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
				{
					VideoStruct &VS = LocalInstance->m_VideoList[i];

					if (VS.VideoStream.get() == ImageBlack)
					{
						VS.VideoStream.reset();

						LocalInstance->m_VideoList.Remove(i);
						break;
					}
				}

				LeaveCriticalSection(&LocalInstance->VideoSection);
			}

		}
		//profileOut
	}

	EnterCriticalSection(&LiveInstance->NetWorkSection);
	bOutPicDel = true;
	for (int i = 0; i < 2; ++i)
	{
		if (outPics[i])
		{
			delete outPics[i];
			outPics[i] = NULL;
		}
	}
	LeaveCriticalSection(&LiveInstance->NetWorkSection);

	if (bUseBack)
	{
		EnterCriticalSection(&LiveInstance->NetWorkSection_back);
		bOutPicDel_Back = true;
		for (int i = 0; i < 2; ++i)
		{
			if (outPics_back[i])
			{
				delete outPics_back[i];
				outPics_back[i] = NULL;
			}
		}
		LeaveCriticalSection(&LiveInstance->NetWorkSection_back);
	}
	
}

static void VolumeCaculate(float *buffer, int totalFloats, float mulVal) {

	float sum = 0.0f;
	int totalFloatsStore = totalFloats;

	if ((UPARAM(buffer) & 0xF) == 0)
	{
		UINT alignedFloats = totalFloats & 0xFFFFFFFC;
		__m128 sseMulVal = _mm_set_ps1(mulVal);

		__m128 maxVal = _mm_set_ps1(1.0f);
		__m128 minVal = _mm_set_ps1(-1.0f);

		for (UINT i = 0; i < alignedFloats; i += 4)
		{
			__m128 sseScaledVals = _mm_mul_ps(_mm_load_ps(buffer + i), sseMulVal);

			sseScaledVals = _mm_min_ps(sseScaledVals, maxVal);
			sseScaledVals = _mm_max_ps(sseScaledVals, minVal);

			_mm_store_ps(buffer + i, sseScaledVals);
		}

		buffer += alignedFloats;
		totalFloats -= alignedFloats;
	}

	for (int i = 0; i < totalFloats; ++i)
	{
		buffer[i] *= mulVal;

		if (buffer[i] > 1.0f)
		{
			buffer[i] = 1.0f;
		}
		else if (buffer[i] < -1.0f)
		{
			buffer[i] = -1.0f;
		}
	}
}

inline void CalculateVolumeLevels(float *buffer, int totalFloats, float mulVal, float &RMS, float &MAX)
{
	float sum = 0.0f;
	int totalFloatsStore = totalFloats;

	float Max = 0.0f;

	if ((UPARAM(buffer) & 0xF) == 0)
	{
		UINT alignedFloats = totalFloats & 0xFFFFFFFC;
		//__m128 sseMulVal = _mm_set_ps1(mulVal);
		__m128 sseScaledVals;
		__m128 sseSquares;
		for (UINT i = 0; i<alignedFloats; i += 4)
		{
			//sseScaledVals = _mm_mul_ps(_mm_load_ps(buffer + i), sseMulVal);
			sseScaledVals = _mm_load_ps(buffer + i);

			/*compute squares and add them to the sum*/
			sseSquares = _mm_mul_ps(sseScaledVals, sseScaledVals);
			sum += sseSquares.m128_f32[0] + sseSquares.m128_f32[1] + sseSquares.m128_f32[2] + sseSquares.m128_f32[3];

			/*
			sse maximum of squared floats
			concept from: http://stackoverflow.com/questions/9795529/how-to-find-the-horizontal-maximum-in-a-256-bit-avx-vector
			*/
// 			__m128 sseSquaresP = _mm_shuffle_ps(sseSquares, sseSquares, _MM_SHUFFLE(1, 0, 3, 2));
// 			__m128 halfmax = _mm_max_ps(sseSquares, sseSquaresP);
// 			__m128 halfmaxP = _mm_shuffle_ps(halfmax, halfmax, _MM_SHUFFLE(0, 1, 2, 3));
// 			__m128 maxs = _mm_max_ps(halfmax, halfmaxP);
// 
// 			Max = max(Max, maxs.m128_f32[0]);
		}

		buffer += alignedFloats;
		totalFloats -= alignedFloats;
	}

	for (int i = 0; i<totalFloats; i++)
	{
		float val = buffer[i] * mulVal;
		float pow2Val = val * val;
		sum += pow2Val;
		//Max = max(Max, pow2Val);
	}

	RMS = sqrt(sum / totalFloatsStore);
	//MAX = sqrt(Max);
}

inline float toDB(float RMS)
{
	float db = 20.0f * log10(RMS);
	if (!_finite(db))
		return VOL_MIN;
	return db;
}

void CSLiveManager::MainAudioLoop()
{
	unsigned int audioSamplesPerSec = BSParam.LiveSetting.AudioSampleRate;
	unsigned int audioSampleSize = audioSamplesPerSec / 100;


	List<float> mixBuffer;
	mixBuffer.SetSize(audioSampleSize * 2);

	List<float> leftaudioData;
	List<float> rightaudioData;
	leftaudioData.SetSize(audioSampleSize);
	rightaudioData.SetSize(audioSampleSize);


	UINT audioFramesSinceMeterUpdate = 0;
	QWORD StartAudio = GetQPCMS();
	int AudioCount = 0;
	while (bRunning)
	{
		//profileIn("MainAudioLoop")
		if (LocalInstance)
		{
			vector<IBaseAudio *> AudioList;
			EnterCriticalSection(&LocalInstance->AudioSection);
			for (UINT i = 0; i < LocalInstance->m_AudioList.Num(); ++i) {

				AudioList.push_back(LocalInstance->m_AudioList[i].AudioStream.get());
				
			}
			LeaveCriticalSection(&LocalInstance->AudioSection);

			if (AudioList.size())
			{
				for (IBaseAudio *Audio : AudioList)
				{
					if (FindVideoInLiveIntance(Audio))
					{
						EnterCriticalSection(&LocalInstance->AudioSection);
						for (UINT i = 0; i < LocalInstance->m_AudioList.Num(); ++i) {

							if (LocalInstance->m_AudioList[i].AudioStream.get() == Audio)
							{
								LocalInstance->m_AudioList[i].AudioStream->SetAudioParam(leftdesktopVol, rightdesktopVol, desktopVol, m_bPlayPcmLocal, m_bPlayPcmLive, m_quotietyVolume, m_bProject);
								break;
							}

						}
						LeaveCriticalSection(&LocalInstance->AudioSection);
						
					}
					else
					{
						EnterCriticalSection(&LocalInstance->AudioSection);
						for (UINT i = 0; i < LocalInstance->m_AudioList.Num(); ++i) {

							if (LocalInstance->m_AudioList[i].AudioStream.get() == Audio)
							{
								LocalInstance->m_AudioList[i].AudioStream->SetAudioParam(leftdesktopVol, rightdesktopVol, desktopVol, m_bPlayPcmLocal, false, m_quotietyVolume, m_bProject);
								break;
							}

						}
						LeaveCriticalSection(&LocalInstance->AudioSection);
						
					}
				}
			}

		}

		if (LiveInstance)
		{
			if (LiveInstance->bReBulidAudio && !bStartLive)//在直播中不允许更改
			{
				audioSamplesPerSec = BSParam.LiveSetting.AudioSampleRate;
				audioSampleSize = audioSamplesPerSec / 100;

				mixBuffer.SetSize(audioSampleSize * 2);

				leftaudioData.SetSize(audioSampleSize);
				rightaudioData.SetSize(audioSampleSize);
				LiveInstance->bReBulidAudio = false;


				if (LocalInstance)
				{
					EnterCriticalSection(&LocalInstance->AudioSection);
					for (UINT i = 0; i < LocalInstance->m_AudioList.Num(); ++i) {

						LocalInstance->m_AudioList[i].AudioStream->SetSampleRateHz(audioSamplesPerSec);

					}
					LeaveCriticalSection(&LocalInstance->AudioSection);
				}

				EnterCriticalSection(&LiveInstance->AudioSection);
				for (UINT i = 0; i < LiveInstance->m_AudioList.Num(); ++i) {

					LiveInstance->m_AudioList[i].AudioStream->SetSampleRateHz(audioSamplesPerSec);

				}
				LeaveCriticalSection(&LiveInstance->AudioSection);

			}

			if (LiveInstance->QueryNewAudio())
			{
				QWORD timestamp = LiveInstance->bufferedAudioTimes[0];
				LiveInstance->bufferedAudioTimes.Remove(0);


				zero(mixBuffer.Array(), audioSampleSize * 2 * sizeof(float));
				zero(rightaudioData.Array(), audioSampleSize * sizeof (float));
				zero(leftaudioData.Array(), audioSampleSize * sizeof (float));

				bool bHasMix = false;

				//profileIn("MainAudioLoop MixAudio")

				vector<IBaseAudio *> AudioListLive;
				EnterCriticalSection(&LiveInstance->AudioSection);

				for (UINT i = 0; i < LiveInstance->m_AudioList.Num(); ++i) {
					float *auxBuffer;
					AudioListLive.push_back(LiveInstance->m_AudioList[i].AudioStream.get());
					if (LiveInstance->m_AudioList[i].AudioStream->GetBuffer(&auxBuffer, timestamp))
					{
						LiveInstance->MixAudio(mixBuffer.Array(), auxBuffer, audioSampleSize * 2, false);
						bHasMix = true;
					}
				}

				LeaveCriticalSection(&LiveInstance->AudioSection);

				//防止死锁
				if (AudioListLive.size())
				{
					for (IBaseAudio *Audio : AudioListLive)
					{
						if (FindVideoInLocalIntance(Audio))
						{
							EnterCriticalSection(&LiveInstance->AudioSection);
							for (UINT i = 0; i < LiveInstance->m_AudioList.Num(); ++i) {

								if (LiveInstance->m_AudioList[i].AudioStream.get() == Audio)
								{
									LiveInstance->m_AudioList[i].AudioStream->SetAudioParam(leftdesktopVol, rightdesktopVol, desktopVol, m_bPlayPcmLocal, m_bPlayPcmLive, m_quotietyVolume, m_bProject);
									break;
								}

							}
							LeaveCriticalSection(&LiveInstance->AudioSection);

						}
						else
						{
							EnterCriticalSection(&LiveInstance->AudioSection);
							for (UINT i = 0; i < LiveInstance->m_AudioList.Num(); ++i) {

								if (LiveInstance->m_AudioList[i].AudioStream.get() == Audio)
								{
									LiveInstance->m_AudioList[i].AudioStream->SetAudioParam(leftdesktopVol, rightdesktopVol, desktopVol, false, m_bPlayPcmLive, m_quotietyVolume, m_bProject);
									break;
								}

							}
							LeaveCriticalSection(&LiveInstance->AudioSection);

						}
					}
				}
				//profileOut;

				float rightdesktopRMS = 0, rightdesktopMx = 0;
				float leftdesktopRMS = 0, leftdesktopMx = 0;
				float desktopVolGain = 0, leftdesktopVolGain = 0, rightdesktopVolGain = 0;
				//profileIn("MainAudioLoop CalculateVolumeLevels")
				if (bHasMix) {
					if (desktopVol > 1.0)
					{
						desktopVolGain = desktopVol - 1.0;
					}

					if (leftdesktopVol > 1.0)
					{
						leftdesktopVolGain = leftdesktopVol - 1.0;
					}

					if (rightdesktopVol > 1.0)
					{
						rightdesktopVolGain = rightdesktopVol - 1.0;
					}

					if (desktopVol + desktopVolGain*m_quotietyVolume != 1.0f)
						VolumeCaculate(mixBuffer.Array(), audioSampleSize * 2, desktopVol + desktopVolGain*m_quotietyVolume);
				}
				//----------------------------------------------------------------------------
				// convert RMS and Max of samples to dB 
				audioFramesSinceMeterUpdate += audioSampleSize;
				if (audioFramesSinceMeterUpdate >= (audioSampleSize * 10)) {

					for (int iIndex = 0, iCount = 0; iIndex < audioSampleSize * 2; iIndex += 2, ++iCount)
					{
						leftaudioData[iCount] = mixBuffer[iIndex];
						rightaudioData[iCount] = mixBuffer[iIndex + 1];
					}

// 					if (1.f != leftdesktopVol + leftdesktopVolGain*m_quotietyVolume)
// 						VolumeCaculate(leftaudioData.Array(), audioSampleSize, leftdesktopVol + leftdesktopVolGain*m_quotietyVolume);
// 
// 					if (1.f != rightdesktopVol + rightdesktopVolGain*m_quotietyVolume)
// 						VolumeCaculate(rightaudioData.Array(), audioSampleSize, rightdesktopVol + rightdesktopVolGain*m_quotietyVolume);
// 
// 					for (int iIndex = 0, iCount = 0; iIndex < audioSampleSize * 2; iIndex += 2, ++iCount){
// 
// 						mixBuffer[iIndex] = leftaudioData[iCount];
// 						mixBuffer[iIndex + 1] = rightaudioData[iCount];
// 					}

					CalculateVolumeLevels(rightaudioData.Array(), audioSampleSize, 1.0f, rightdesktopRMS, rightdesktopMx);
					CalculateVolumeLevels(leftaudioData.Array(), audioSampleSize, 1.0f, leftdesktopRMS, leftdesktopMx);

					if (desktopVol >= 1.0f && desktopVol < 1.8f)
					{
						leftdesktopRMS = leftdesktopRMS / m_quotietyVolume;
						rightdesktopRMS = rightdesktopRMS / m_quotietyVolume;
					}
					else if (desktopVol < 1.0)
					{
						leftdesktopRMS = leftdesktopRMS / 10.0f;
						rightdesktopRMS = rightdesktopRMS / 10.0f;
					}

					leftdesktopRMS = toDB(leftdesktopRMS);
					rightdesktopRMS = toDB(rightdesktopRMS);

					
					if (BSParam.LiveAudioCb)
						BSParam.LiveAudioCb(leftdesktopRMS, rightdesktopRMS);

					audioFramesSinceMeterUpdate = 0;

					
				}
				//profileOut
				WAVEFORMATEX  audioFormat;
				audioFormat.nSamplesPerSec = audioSamplesPerSec;
				audioFormat.nChannels = 2;
				audioFormat.wBitsPerSample = 32;

				BlackMagic* blackMagic = BlackMagic::Instance();
				OSEnterMutex(SDIMutex);
				if (SIDIDs.size() && bHasMix)
				{
					for (auto& id : SIDIDs)
					{
						if (id.enable)
							blackMagic->SDI_RenderDevice(id, mixBuffer.Array(), 0, 0, ColorFormat_RGBA32REVERSE, true, &audioFormat, mixBuffer.Num() * 4, true);
					}
				}
				OSLeaveMutex(SDIMutex);

				if (LiveInstance->bStartLive)
				{
					//profileIn("MainAudioLoop EncodeAudioSegment")
					LiveInstance->EncodeAudioSegment(mixBuffer.Array(), audioSampleSize, timestamp);
					//profileOut

					if (LiveInstance->bFristAudioEncode)
					{
						LiveInstance->bFristAudioEncode = false;
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:第一个音频时间戳 %llu", timestamp);
					}
				}
				else
				{
					EnterCriticalSection(&LiveInstance->SoundDataMutex);
					for (UINT i = 0; i < LiveInstance->pendingAudioFrames.Num(); ++i)
						LiveInstance->pendingAudioFrames[i].audioData.Clear();
					LiveInstance->pendingAudioFrames.Clear();
					LeaveCriticalSection(&LiveInstance->SoundDataMutex);

					if (bUseBack)
					{
						EnterCriticalSection(&LiveInstance->SoundDataMutex_back);
						for (UINT i = 0; i < LiveInstance->pendingAudioFrames_back.Num(); ++i)
							LiveInstance->pendingAudioFrames_back[i].audioData.Clear();
						LiveInstance->pendingAudioFrames_back.Clear();
						LeaveCriticalSection(&LiveInstance->SoundDataMutex_back);
					}
				}

				++AudioCount;
				if (GetQPCMS() - StartAudio >= m_CheckTime)
				{
					m_EncodeAudioCount = AudioCount;

					m_RealAudioFrameTime = (float)(GetQPCMS() - StartAudio) / m_EncodeAudioCount;
					AudioCount = 0;
					StartAudio = GetQPCMS();
				}
			}
			else
			{
				Sleep(5);
			}
		}

		//profileOut
	}

	if (LiveInstance)
	{
		EnterCriticalSection(&LiveInstance->SoundDataMutex);
		for (UINT i = 0; i < LiveInstance->pendingAudioFrames.Num(); ++i)
			LiveInstance->pendingAudioFrames[i].audioData.Clear();
		LiveInstance->pendingAudioFrames.Clear();
		LeaveCriticalSection(&LiveInstance->SoundDataMutex);

		if (bUseBack)
		{
			EnterCriticalSection(&LiveInstance->SoundDataMutex_back);
			for (UINT i = 0; i < LiveInstance->pendingAudioFrames_back.Num(); ++i)
				LiveInstance->pendingAudioFrames_back[i].audioData.Clear();
			LiveInstance->pendingAudioFrames_back.Clear();
			LeaveCriticalSection(&LiveInstance->SoundDataMutex_back);
		}
	}
}


void CSLiveManager::VideoEncoderLoop()
{
	sleepTargetTime = GetQPCNS();
	bfirstTimeStamp = true;
	bool bufferedFrames = false;
	//bool bFirst = true;
	
	FrameProcessInfo ProcessInfo;
	FrameProcessInfo ProcessInfo_back;

	QWORD StartEncodeTime = GetQPCMS();
	bool bHasEncoder = false;
	bool bMainLive = true;
	int EncodeFrameCount = 0;
	bool bCanCheck = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:VideoEncoderLoop 开始时间 %llu", sleepTargetTime / 1000000);
	while (bRunning)
	{
		//QWORD StartTime = GetQPCNS();
		QWORD frameTimeNS = 1000000000 / BSParam.LiveSetting.FPS;
		SetEvent(hVideoEvent);
		Sleep2NS(sleepTargetTime += frameTimeNS / 2);

		//profileIn("VideoEncoderLoop")
		if ((bStartLive && Outpic)/* || bufferedFrames*/)
		{
			//bFirst = false;
			if (LiveInstance->bfirstTimeStamp)
			{
				CurrentVideoTime = 0;
				StartVideoTime = GetQPCNS();
				LiveInstance->bfirstTimeStamp = false;
				bNewStart = true;
				bCanCheck = true;
				bCanSecondCheck = true;
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:第一个视频时间戳 %llu", StartVideoTime / 1000000);
			}
			else
			{
				CurrentVideoTime = (GetQPCNS() - StartVideoTime) / 1000000;
			}

			ProcessInfo.firstFrameTime = StartVideoTime / 1000000;
			ProcessInfo.frameTimestamp = CurrentVideoTime;
			if (bStartLive)
			{
				ProcessInfo.pic = Outpic;
				ProcessInfo.pic->i_pts = CurrentVideoTime;
			}
			else
			{
				ProcessInfo.pic = NULL;
			}

			bool bSetEvent = false;
			if (bUseBack && Outpic_back)
			{
				SetEvent(hVideoEvent_back);
				bSetEvent = true;
			}
			//profileIn("VideoEncoderLoop ProcessFrame")
			
			
			if (bUseBack && (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec))
			{
				bMainLive = false;
				EnterCriticalSection(&LiveInstance->NetWorkSection_back);
				if (!bOutPicDel_Back && LiveInstance)
				{
					bHasEncoder = LiveInstance->ProcessFrame_back(ProcessInfo);
				}
				LeaveCriticalSection(&LiveInstance->NetWorkSection_back);
			}
			else
			{
				bMainLive = true;
				EnterCriticalSection(&LiveInstance->NetWorkSection);
				if (!bOutPicDel && LiveInstance)
				{
					bHasEncoder = LiveInstance->ProcessFrame(ProcessInfo);

				}
				LeaveCriticalSection(&LiveInstance->NetWorkSection);
			}

			if (bHasEncoder)
				bCanCheck = false;
			
			if (!bHasEncoder && bNewStart && bCanCheck)
			{
				if ((GetQPCNS() - StartVideoTime) / 1000000 >= (bMainLive ? 3000 : 3500))
				{
					bNewStart = false;
					
					if (LiveInstance->LiveParam.TipsCb)
					{
						LiveInstance->LiveParam.TipsCb(-101, bMainLive ? "主直播硬编码不出数据,请换用软编码！": "次直播硬编码不出数据,请换用软编码！");
					}
				}
			}
			
			//profileOut
			if (bSetEvent)
				WaitForSingleObject(hVideoComplete, INFINITE);

		}
		else
		{
			bNewStart = false;
		}

		//这里不会进入
// 		if (!bStartLive && LiveInstance)
// 		{
// 			//快速开始直播崩溃
// 			bufferedFrames = false;//LiveInstance->videoEncoder ? LiveInstance->videoEncoder->HasBufferedFrames() : false;
// 			if (bUseBack){
// 				bufferedFrames = false;//bufferedFrames || LiveInstance->videoEncoder_back ? LiveInstance->videoEncoder_back->HasBufferedFrames() : false;
// 			}
// 
// 			if (!bufferedFrames && !bFirst)
// 			{
// 				if (LiveInstance->videoEncoder)
// 					delete LiveInstance->videoEncoder;
// 				LiveInstance->videoEncoder = NULL;
// 
// 				if (LiveInstance->videoEncoder_back)
// 					delete LiveInstance->videoEncoder_back;
// 				LiveInstance->videoEncoder_back = NULL;
// 				bFirst = true;
// 
// 				LiveInstance->StopLive(false);
// 			}
// 		}

		Sleep2NS(sleepTargetTime += frameTimeNS / 2);

		//profileOut
		++EncodeFrameCount;

		if (GetQPCMS() - StartEncodeTime >= m_CheckTime)
		{
			m_EncodeVideoCount = EncodeFrameCount;
			m_RealFrameTime = (float)(GetQPCMS() - StartEncodeTime) / m_EncodeVideoCount;
			StartEncodeTime = GetQPCMS();
			EncodeFrameCount = 0;
		}
	}

}

void CSLiveManager::VideoEncoderLoop_back()
{
	FrameProcessInfo ProcessInfo;
	bool bMainLive = true;
	bool bHasEncoder = false;
	while (bRunning)
	{
		while (WaitForSingleObject(hVideoEvent_back, INFINITE) == WAIT_OBJECT_0)
		{
			if (!bRunning)
				break;

			//profileIn("VideoEncoderLoop_back")
			if (Outpic_back)
			{
				ProcessInfo.firstFrameTime = StartVideoTime / 1000000;
				ProcessInfo.frameTimestamp = CurrentVideoTime;
				if (bStartLive)
				{
					ProcessInfo.pic = Outpic_back;
					ProcessInfo.pic->i_pts = CurrentVideoTime;
				}
				else
				{
					ProcessInfo.pic = NULL;
				}
				
				//profileIn("VideoEncoderLoop_back ProcessFrame_back")

				if (BSParam.LiveSetting.Width < BSParam.LiveSetting.WidthSec || BSParam.LiveSetting.Height < BSParam.LiveSetting.HeightSec)
				{
					bMainLive = true;
					EnterCriticalSection(&LiveInstance->NetWorkSection);
					if (!bOutPicDel && LiveInstance)
					{
						bHasEncoder = LiveInstance->ProcessFrame(ProcessInfo);
					}
					LeaveCriticalSection(&LiveInstance->NetWorkSection);
				}
				else
				{
					bMainLive = false;
					EnterCriticalSection(&LiveInstance->NetWorkSection_back);
					if (!bOutPicDel_Back && LiveInstance)
					{
						bHasEncoder = LiveInstance->ProcessFrame_back(ProcessInfo);
					}
					LeaveCriticalSection(&LiveInstance->NetWorkSection_back);
				}

				//profileOut
				SetEvent(hVideoComplete);

				if (bHasEncoder)
					bCanSecondCheck = false;

				if (!bHasEncoder && bNewStart && bCanSecondCheck)
				{
					if ((GetQPCNS() - StartVideoTime) / 1000000 >= (bMainLive ? 3000 : 3500))
					{
						bNewStart = false;

						if (LiveInstance->LiveParam.TipsCb)
						{
							LiveInstance->LiveParam.TipsCb(-101, bMainLive ? "主直播硬编码不出数据,请换用软编码！" : "次直播硬编码不出数据,请换用软编码！");
						}
					}
				}

			}
			//profileOut
		}
	}
}

void CSLiveManager::RenderLittlePreview()
{
	QWORD SleepTime = GetQPCNS();
	while (bRunning)
	{
		QWORD frameTimeNS = 1000000000 / BSParam.LiveSetting.FPS;
		
		EnterCriticalSection(&MapInstanceSec);
		//profileIn("MainVideoLoop DrawLittlePreview");
		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *Process = m_InstanceList.GetAt(i);
			if (Process && Process->bLittlePre && !Process->bNoPreView)
			{
				Process->DrawLittlePreview();
			}
		}
		//profileOut
        LeaveCriticalSection(&MapInstanceSec);

		Sleep2NS(SleepTime += frameTimeNS);
	}
	
}

