#include "Instance.h"
#include "OperatNew.h"
#include "Error.h"
#include "Encoder.h"
#include "RTMPStuff.h"
#include "RTMPPublisherVector.h"
#include "FileStream.h"
#include "HttpLive.h"

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


extern void Convert444toNV12(LPBYTE input, int width, int inPitch, int outPitch, int height, int startY, int endY, LPBYTE *output);
extern CSectionLock DelaySection;
CInstanceProcess::CInstanceProcess(const SLiveParam *Param)
{
	InitializeCriticalSection(&VideoSection);
	InitializeCriticalSection(&AudioSection);
	InitializeCriticalSection(&SoundDataMutex);
	InitializeCriticalSection(&SoundDataMutex_back);
	InitializeCriticalSection(&NetWorkSection);
	InitializeCriticalSection(&NetWorkSection_back);
	m_hMutexRawA = OSCreateMutex();
	m_hMutexRawV = OSCreateMutex();
	memcpy(&LiveParam, Param,sizeof(SLiveParam));
	bRunning = true;
	bResizeRenderView = false;
	RenderHwnd = NULL;
	bReBulid = false;
	bReBulidAudio = false;
	CurrentAudioTime = 0;
	videoEncoder = NULL;
	videoEncoder_back = NULL;
	network = NULL;
	network_back = NULL;
	bStartLive = false;
	bLiveInstanceRecordOnly = false;
	bRecord = false;
	bCanRecord = false;
	lastAudioTimestamp = 0;
	lastAudioTimestamp_back = 0;
	bOutPicDel = false;
	IsReject = false;
	audioEncoder = NULL;
	IsLiveInstance = false;
	SwapRender = NULL;
	bLittlePre = false;
	bShutDown = false;
	bNoPreView = false;
	bUseBackInstance = LiveParam.LiveSetting.bUseLiveSec;
	ConfigCB = Param->ConfigCB;
	bForceKilled = false;
	MultiRender = NULL;
	RecordWidth = 0;
	RecordHeight = 0;
	m_hEncodeThread = NULL;
	m_firstFrameTimestamp = -1;
	bHasAudio = false;
	bNewD3DSelf = false;
	HVideoCapture = HAudioCapture = NULL;
	D3DRender = CSLiveManager::GetInstance()->GetD3DRender();

	__HttpLive = NULL;

	LeftDb = -96;
	RightDb = -96;
	DbWidth = 10;

	bNeedPGM = true;
	bNeedPVW = false;

	ZeroMemory(&PreviewArea,sizeof VideoArea);
}

CInstanceProcess::~CInstanceProcess()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s InstanceProcess 0x%p 开始析构",__FUNCTION__,this);
	bStartLive = false;
	bRunning = false;
	

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num();++i)
	{
		VideoStruct &Video = m_VideoList[i];

		if (Video.VideoStream)
		{
			if (Video.bGlobalStream && Video.VideoStream.use_count() == 2)
			{
				IBaseVideo *BaseVideo = Video.VideoStream.get();

				if (BaseVideo)
				{
					BaseVideo->GlobalSourceLeaveScene();
					if (Video.VideoDevice)
					{
						Video.VideoDevice->GlobalSourceLeaveScene();
						Video.VideoDevice->SetCanEnterScene(true);
					}
					BaseVideo->SetCanEnterScene(true);
				}
				//这里检测ShardVideo里有没有DeviceSource;

				if (strcmp(Video.VideoStream->GainClassName(), "DeviceSource") == 0)
				{
					bool bFind = false;
					for (int i = 0; i < SharedDevice::VideoList.size(); ++i)
					{
						ShardVideo& OneVideo = SharedDevice::VideoList[i];

						if (OneVideo.VideoStream->GetDeviceID())
						{
							if (strcmp(OneVideo.VideoStream->GetDeviceName(), BaseVideo->GetDeviceName()) == 0 && strcmp(OneVideo.VideoStream->GetDeviceID(), BaseVideo->GetDeviceID()) == 0)
							{
								bFind = true;
								break;
							}
						}
						else
						{
							if (strcmp(OneVideo.VideoStream->GetDeviceName(), BaseVideo->GetDeviceName()) == 0)
							{
								bFind = true;
								break;
							}
						}
					}

					if (bFind)
					{
						if (bLittlePre && !bNoPreView)
						{
							BaseVideo->UnRegisterDataCallBack(this);

						}
					}
				}

			}
			else if (Video.bGlobalStream)
			{
				IBaseVideo *BaseVideo = Video.VideoStream.get();
				if (bLittlePre && !bNoPreView)
				{
					BaseVideo->UnRegisterDataCallBack(this);

				}
			}
			Video.VideoStream.reset();
		}

		if (Video.VideoDevice)//这里新添加的可能有问题
		{
			Video.VideoDevice->UnRegisterDataCallBack(this);
			Video.VideoDevice.reset();
		}

		if (Video.Config)
		{
			Video.Config.reset();
		}
	}

	for (UINT i = 0; i < m_Filter.Num(); ++i)
	{
		Filter &FTer = m_Filter[i];

		vector<__FilterSturct> &BaseFilter = FTer.BaseFilter;

		for (UINT j = 0; j < BaseFilter.size(); ++j)
		{
			__FilterSturct &OneFilter = BaseFilter[j];

			if (OneFilter._Filter)
				OneFilter._Filter.reset();
		}

		if (FTer.RenderTarget)
			FTer.RenderTarget.reset();
	}

	LeaveCriticalSection(&VideoSection);


	bRecord = false; //移动到这里来，视频捕捉设备录制中删除视频捕捉源死锁

	if (bForceKilled)
	{
		for (UINT i = 0; i < m_AudioList.Num(); ++i)
		{
			AudioStruct &Audio = m_AudioList[i];

			if (Audio.bMustDel)
				Audio.AudioStream.reset();
			if (Audio.Config)
				Audio.Config.reset();
		}
	}
	else
	{
		EnterCriticalSection(&AudioSection);
		for (UINT i = 0; i < m_AudioList.Num(); ++i)
		{
			AudioStruct &Audio = m_AudioList[i];

			if (Audio.bMustDel)
				Audio.AudioStream.reset();
			if (Audio.Config)
				Audio.Config.reset();
		}
		LeaveCriticalSection(&AudioSection);
	}
	

	if (m_hEncodeThread)
	{
		if (WAIT_TIMEOUT == WaitForSingleObject(m_hEncodeThread, 5000))
		{
			TerminateThread(m_hEncodeThread,0);
		}

		CloseHandle(m_hEncodeThread);
	}

	if (videoEncoder)
		delete videoEncoder;
	videoEncoder = NULL;

	if (videoEncoder_back)
		delete videoEncoder_back;
	videoEncoder_back = NULL;

	if (audioEncoder)
		delete audioEncoder;
	audioEncoder = NULL;

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:NetWork开始析构!");
	if (network)
		delete network;
	network = NULL;
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:NetWork析构完成!");

	if (network_back)
	{
		delete network_back;
		network_back = NULL;
	}
	
	if (SwapRender)
		delete SwapRender;

	if (MultiRender)
		delete MultiRender;

	if (bNewD3DSelf && D3DRender)
	{
		delete D3DRender;
	}

	if (__HttpLive)
		delete __HttpLive;

	OSCloseMutex(m_hMutexRawA);
	OSCloseMutex(m_hMutexRawV);
	DeleteCriticalSection(&VideoSection);
	DeleteCriticalSection(&AudioSection);
	DeleteCriticalSection(&SoundDataMutex);
	DeleteCriticalSection(&SoundDataMutex_back);
	DeleteCriticalSection(&NetWorkSection);
	DeleteCriticalSection(&NetWorkSection_back);
	
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s InstanceProcess 0x%p 析构完成", __FUNCTION__, this);
}

const char* CInstanceProcess::GainClassName()
{
	return "CInstanceProcess";
}

void CInstanceProcess::CreateStream(const Value& Jvalue, VideoArea *Area, uint64_t *StreamID1, uint64_t *StreamID2 /*= NULL*/)
{
	Value *JVData = new Value;
	if (Jvalue["data"].isNull())
	{
		CONFINGFUN fp = GetConfigFunc(Jvalue["ClassName"].asString().c_str());
		//CONFINGFUN fp = GetConfigFunc("ProcTopWindowSource");

		char TemID[50] = { 0 };
		sprintf_s(TemID, "%llu", (uint64_t)this);
		(*JVData)["InstanceID"] = TemID;

		//对直播源进行特殊处理

		if (strcmp(Jvalue["ClassName"].asString().c_str(), "VideoLiveSource") == 0)
		{
			(*JVData)["PlayList"] = Jvalue["PlayList"];
		}

		if (!fp || (fp && !fp(*JVData, true)))
		{
			delete JVData;
			BUTEL_THORWERROR("取消操作或 %s 类配置不存在", Jvalue["ClassName"].asString().c_str());
		}
		(*JVData)["Name"] = Jvalue["Name"].asString().c_str();
		
	}
	else
	{
		*JVData = Jvalue["data"];

		if (!Jvalue["Name"].isNull())
		{
			(*JVData)["Name"] = Jvalue["Name"].asString().c_str();
		}

		char TemID[50] = { 0 };
		sprintf_s(TemID, "%llu", (uint64_t)this);
		(*JVData)["InstanceID"] = TemID;

		if (strcmp(Jvalue["ClassName"].asString().c_str(), "VideoLiveSource") == 0)
		{
			(*JVData)["PlayList"] = Jvalue["PlayList"];
		}
	}

	if (strcmp(Jvalue["ClassName"].asString().c_str(), "DeviceSource") == 0)
	{
		for (int i = 0; i < SharedDevice::VideoList.size(); ++i)
		{
			ShardVideo& OneVideo = SharedDevice::VideoList[i];

			bool bFind = false;
			if (OneVideo.VideoStream->GetDeviceID())
			{
				if (strcmp(OneVideo.VideoStream->GetDeviceName(), (*JVData)["deviceName"].asString().c_str()) == 0 && strcmp(OneVideo.VideoStream->GetDeviceID(), (*JVData)["deviceID"].asString().c_str()) == 0)
				{
					bFind = true;
				}
			}
			else
			{
				if (strcmp(OneVideo.VideoStream->GetDeviceName(), (*JVData)["deviceName"].asString().c_str()) == 0)
				{
					bFind = true;
				}
			}

			if (Area && bFind)
			{
				//视频流
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s DeviceSource left = %.2f,top = %.2f,width = %.2f,height = %.2f", __FUNCTION__, Area->left, Area->top, Area->width, Area->height);

				try
				{
					if (!OneVideo.VideoStream->Init(const_cast<Value&>(*JVData)))
					{
						BUTEL_THORWERROR("%s Init失败", Jvalue["ClassName"].asString().c_str());
					}
				}
				catch (CErrorBase& e)
				{
					delete JVData;
					BUTEL_THORWERROR(e.m_Error.c_str());
				}
				IBaseVideo *BaseStream = dynamic_cast<IBaseVideo*>(CreatStreamObject("DeviceSource"));
				BaseStream->SetInt(L"bNeedCheckThread", 0);
				BaseStream->Init(const_cast<Value&>(*JVData));
				*StreamID1 = (uint64_t)BaseStream;
				char Tem[50] = { 0 };
				sprintf_s(Tem, "%llu", *StreamID1);

				(*JVData)["SourceID"] = Tem;
				sprintf_s(Tem, "%llu", OneVideo.VideoStream.get());
				(*JVData)["DeviceSourceID"] = Tem;

				VideoStruct InVideoStruct;
				InVideoStruct.VideoDevice = OneVideo.VideoStream;
				InVideoStruct.VideoStream = shared_ptr<IBaseVideo>(BaseStream);
				InVideoStruct.pos.x = Area->left;
				InVideoStruct.pos.y = Area->top;
				InVideoStruct.size.x = Area->width;
				InVideoStruct.size.y = Area->height;
				InVideoStruct.Config = shared_ptr<Value>(JVData, [](Value *v){if (v) delete v; });
				if (bLittlePre)
				{
					if (!bNoPreView)
					{
						RECT Rect;
						GetClientRect(RenderHwnd, &Rect);
						MultiRender = new CMultimediaRender;
						MultiRender->SetVideoRender(Vect2(PreviewArea.left, PreviewArea.top), Vect2(PreviewArea.width, PreviewArea.height));
					}
					InVideoStruct.bGlobalStream = true;
					OneVideo.VideoStream->BeginScene();
					OneVideo.VideoStream->RegisterDataCallBack(this, StreamCallBack);

					if (MultiRender)
						MultiRender->InitD3DReSize();

					//增加录制回调
					OneVideo.VideoStream->RegisterDataCallBack(this, RecordCallBack);
					InVideoStruct.VideoStream->RegisterDataCallBack(this, RecordCallBack);
					bCanRecord = (*JVData)["UseRecorder"].asInt() == 1;
				}

				if (OneVideo.VideoStream->GetAduioClassName())
				{
					IBaseAudio *AudioStream = NULL;
					try
					{
						BUTEL_IFNULLRETURNERROR(StreamID2);
						AudioStream = OneVideo.VideoStream->GetAudioRender();
						if (!AudioStream)
						{
							BUTEL_THORWERROR("处理类 %s 中包含音频但音频流没有创建成功", Jvalue["ClassName"].asString().c_str());
						}
					}
					catch (CErrorBase& e)
					{
						delete JVData;
						BUTEL_THORWERROR(e.m_Error.c_str());
					}

					*StreamID2 = (uint64_t)AudioStream;

					InVideoStruct.AudioStream = AudioStream;
					EnterCriticalSection(&AudioSection);
					AudioStruct InAudio;
					InAudio.AudioStream = shared_ptr<IBaseAudio>(AudioStream);
					InAudio.VideoStream = OneVideo.VideoStream.get();

					m_AudioList.SetSize(m_AudioList.Num() + 1);
					AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
					AS = InAudio;
					LeaveCriticalSection(&AudioSection);
				}
				else if (StreamID2)
				{
					*StreamID2 = 0;
				}


				EnterCriticalSection(&VideoSection);
				m_VideoList.SetSize(m_VideoList.Num() + 1);
				VideoStruct &VS = m_VideoList[m_VideoList.Num() - 1];
				VS = InVideoStruct;
				LeaveCriticalSection(&VideoSection);
				return;
			}


		}

		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 在 SharedDevice::VideoList.size = %d 中没有找到,则创建新的", __FUNCTION__, SharedDevice::VideoList.size());

	}

	if (0 == strcmp(Jvalue["ClassName"].asString().c_str(), "PipeVideo"))
	{
		(*JVData)["NubeNum"] = Jvalue["NubeNum"];
		(*JVData)["AppKey"] = Jvalue["AppKey"];
		(*JVData)["NickName"] = Jvalue["NickName"];
		(*JVData)["InteractionPath"] = Jvalue["InteractionPath"];
	}

	CPObject *BaseStream = CreatStreamObject(Jvalue["ClassName"].asString().c_str());
	//CPObject *BaseStream = CreatStreamObject("ProcTopWindowSource");
	if (!BaseStream)
	{
		delete JVData;
		BUTEL_THORWERROR("没有找到 %s 处理类", Jvalue["ClassName"].asString().c_str());
	}

	if (Area)
	{
		//视频流
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s left = %.2f,top = %.2f,width = %.2f,height = %.2f",__FUNCTION__,Area->left,Area->top,Area->width,Area->height);
		
		IBaseVideo *VideoStream = dynamic_cast<IBaseVideo*>(BaseStream);
		if (!VideoStream)
		{
			delete JVData;
			delete BaseStream;
			BUTEL_THORWERROR("%s 不是视频处理类", Jvalue["ClassName"].asString().c_str());
		}

		JVData->removeMember("DeviceSourceID");


		//在这里先注册回调再Init
		if (bLittlePre)
		{
			if (!bNoPreView)
			{
				RECT Rect;
				GetClientRect(RenderHwnd, &Rect);
				MultiRender = new CMultimediaRender;
				if (0 == strcmp(Jvalue["ClassName"].asString().c_str(), "PipeVideo"))
				{
					//互动连接源
					MultiRender->SetIsInteraction(true);

				}

				if (MultiRender)
					MultiRender->InitD3DReSize();

				MultiRender->SetVideoRender(Vect2(PreviewArea.left, PreviewArea.top), Vect2(PreviewArea.width, PreviewArea.height));
			}


			VideoStream->RegisterDataCallBack(this, StreamCallBack);

			if (strcmp(VideoStream->GainClassName(), "DeviceSource") == 0)
			{
				//增加录制回调
				VideoStream->RegisterDataCallBack(this, RecordCallBack);
			}

		}


		VideoStream->Init(const_cast<Value&>(*JVData));//try去掉了

		*StreamID1 = (uint64_t)VideoStream;

		char Tem[50] = { 0 };
		sprintf_s(Tem, "%llu", *StreamID1);

		(*JVData)["SourceID"] = Tem;

		VideoStruct InVideoStruct;
		if (strcmp(Jvalue["ClassName"].asString().c_str(), "DeviceSource") == 0)
		{
			IBaseVideo *BaseStream = dynamic_cast<IBaseVideo*>(CreatStreamObject("DeviceSource"));
			BaseStream->SetInt(L"bNeedCheckThread", 0);
			BaseStream->Init(const_cast<Value&>(*JVData));
			*StreamID1 = (uint64_t)BaseStream;
			sprintf_s(Tem, "%llu", *StreamID1);

			(*JVData)["SourceID"] = Tem;
			sprintf_s(Tem, "%llu", VideoStream);
			(*JVData)["DeviceSourceID"] = Tem;

			InVideoStruct.VideoStream = shared_ptr<IBaseVideo>(BaseStream);
			InVideoStruct.VideoDevice = shared_ptr<IBaseVideo>(VideoStream);

			if (bLittlePre)
			{
				bCanRecord = (*JVData)["UseRecorder"].asInt() == 1;
			}
		}
		else
		{
			InVideoStruct.VideoStream = shared_ptr<IBaseVideo>(VideoStream);
		}
		
		InVideoStruct.pos.x = Area->left;
		InVideoStruct.pos.y = Area->top;
		InVideoStruct.size.x = Area->width;
		InVideoStruct.size.y = Area->height;
		InVideoStruct.Config = shared_ptr<Value>(JVData, [](Value *v){if (v) delete v; });
		InVideoStruct.Crop.x = Area->CropLeft;
		InVideoStruct.Crop.y = Area->CropTop;
		InVideoStruct.Crop.w = Area->CropRight;
		InVideoStruct.Crop.z = Area->CropBottom;

		if (bLittlePre)
		{
			InVideoStruct.bGlobalStream = true;
			VideoStream->BeginScene();
		}

		if (VideoStream->GetAduioClassName())
		{
			IBaseAudio *AudioStream = NULL;
			try
			{
				BUTEL_IFNULLRETURNERROR(StreamID2);
				AudioStream = VideoStream->GetAudioRender();
				if (!AudioStream)
				{
					BUTEL_THORWERROR("处理类 %s 中包含音频但音频流没有创建成功", Jvalue["ClassName"].asString().c_str());
				}
			}
			catch (CErrorBase& e)
			{
				BUTEL_THORWERROR(e.m_Error.c_str());
			}

			*StreamID2 = (uint64_t)AudioStream;

			AudioStream->SetSampleRateHz(LiveParam.LiveSetting.AudioSampleRate);

			InVideoStruct.AudioStream = AudioStream;
			EnterCriticalSection(&AudioSection);
			AudioStruct InAudio;
			InAudio.AudioStream = shared_ptr<IBaseAudio>(AudioStream);
			InAudio.VideoStream = VideoStream;

			m_AudioList.SetSize(m_AudioList.Num() + 1);
			AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
			AS = InAudio;
			LeaveCriticalSection(&AudioSection);
		}
		else if (StreamID2)
		{
			*StreamID2 = 0;
		}

		EnterCriticalSection(&VideoSection);
		m_VideoList.SetSize(m_VideoList.Num() + 1);
		VideoStruct &VS = m_VideoList[m_VideoList.Num() - 1];
		VS = InVideoStruct;
		LeaveCriticalSection(&VideoSection);
	}
	else
	{
		//音频流

		IBaseAudio *AudioStream = dynamic_cast<IBaseAudio*>(BaseStream);
		if (!AudioStream)
		{
			delete JVData;
			delete BaseStream;
			BUTEL_THORWERROR("%s 不是音频处理类", Jvalue["ClassName"].asString().c_str());
		}

		if (!AudioStream->Init(const_cast<Value&>(Jvalue)))
		{
			delete JVData;
			delete AudioStream;
			BUTEL_THORWERROR("%s Init失败", Jvalue["ClassName"].asString().c_str());
		}

		*StreamID1 = (uint64_t)AudioStream;
		//AudioStream->SetSampleRateHz(LiveParam.Encoder.AudioSampleRate);
		AudioStruct InAudio;
		InAudio.AudioStream = shared_ptr<IBaseAudio>(AudioStream);
		InAudio.Config = shared_ptr<Value>(JVData);
		InAudio.bMustDel = true;
		if (bLittlePre)
		{
			InAudio.bGlobalStream = true;
		}
		EnterCriticalSection(&AudioSection);
		m_AudioList.SetSize(m_AudioList.Num() + 1);
		AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
		AS = InAudio;
		LeaveCriticalSection(&AudioSection);
	}
}



void CInstanceProcess::DrawPreview(Texture* Prev, const Vect2 &renderSize, const Vect2 &renderOffset, const Vect2 &renderCtrlSize, Shader *solidVertexShader, Shader *solidPixelShader, Texture* SDITexture, bool HasOutSDI, bool bStart)
{
	D3DRender->SetRenderTarget(SwapRender);

	D3DRender->Ortho(0.0f, renderCtrlSize.x, renderCtrlSize.y, 0.0f, -100.0f, 100.0f);
	D3DRender->SetViewport(0.0f, 0.0f, renderCtrlSize.x, renderCtrlSize.y);


	D3DRender->ClearRenderTarget(0xFF171718);

	D3DRender->DrawSprite(Prev, 0xFFFFFFFF,
		renderOffset.x, renderOffset.y,
		renderOffset.x + renderSize.x, renderOffset.y + renderSize.y);

	bool bUseEdit = false;
	for (int i = 0; i < m_VideoList.Num(); ++i)
	{
		VideoStruct& OneStruct = m_VideoList[i];

		if (m_VideoList[i].bRender && m_VideoList[i].bSelect)
		{
			bUseEdit = true;
		}

		if (m_VideoList[i].bRender && strcmp(OneStruct.VideoStream->GainClassName(), "AgentSource") == 0 && !OneStruct.VideoStream->GetGlobalSource())
		{
			Vect2 &pos = m_VideoList[i].pos * (renderSize / Vect2(outputCX, outputCY)) + renderOffset;
			Vect2 &size = m_VideoList[i].size * (renderSize / Vect2(outputCX, outputCY));

			Vect4 &crop = m_VideoList[i].GetCrop();


			UINT BaseWidth, BaseHeight;
			GetBaseSize(BaseWidth, BaseHeight);
			Vect2 BaseSize(BaseWidth, BaseHeight);
			Vect2 &scale = renderSize / BaseSize;

			crop.x *= scale.x; crop.y *= scale.y;
			crop.z *= scale.y; crop.w *= scale.x;

			D3DRender->SetCropping(crop.x, crop.y, crop.w, crop.z);
			D3DRender->DrawSprite(OneStruct.VideoStream->GetTexture(), 0xFFFFFFFF,
				pos.x, pos.y,
				pos.x + size.x, pos.y + size.y);
			D3DRender->SetCropping(0, 0, 0, 0);
		}
	}

	if (IsLiveInstance && HasOutSDI && bStart)
	{
		D3DRender->EnableBlending(TRUE);
		D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
		DWORD Width, Height;
		D3DRender->GetTextureWH(SDITexture, Width, Height);
		D3DRender->DrawSprite(SDITexture, 0xFFFFFFFF, renderFrameOffset.x + renderFrameSize.x - Width, renderFrameOffset.y, renderFrameOffset.x + renderFrameSize.x, renderFrameOffset.y + Height);
		D3DRender->EnableBlending(FALSE);
	}

	if (bUseEdit)
	{
		D3DRender->LoadVertexShader(solidVertexShader);
		D3DRender->LoadPixelShader(solidPixelShader);
		UINT BaseWidth, BaseHeight;
		GetBaseSize(BaseWidth, BaseHeight);

		Vect2 BaseSize(BaseWidth, BaseHeight);
		Vect2 &scale = renderSize / BaseSize;
		for (int i = 0; i < m_VideoList.Num(); ++i)
		{
			if (m_VideoList[i].bRender && m_VideoList[i].bSelect)
			{
				Vect2 &pos = m_VideoList[i].pos * (renderSize / Vect2(outputCX, outputCY)) + renderOffset;
				Vect2 &size = m_VideoList[i].size * (renderSize / Vect2(outputCX, outputCY));
				Vect2 &selectBoxSize = Vect2(10.0f, 10.0f);
				Vect4 &crop = m_VideoList[i].GetCrop();
			
				crop.x *= scale.x; crop.y *= scale.y;
				crop.z *= scale.y; crop.w *= scale.x;
				pos.x += crop.x;
				pos.y += crop.y;
				size.x -= (crop.x + crop.w);
				size.y -= (crop.y + crop.z);

				D3DRender->DrawBox(pos, selectBoxSize);
				D3DRender->DrawBox((pos + size) - selectBoxSize, selectBoxSize);
				D3DRender->DrawBox(pos + Vect2(size.x - selectBoxSize.x, 0.0f), selectBoxSize);
				D3DRender->DrawBox(pos + Vect2(0.0f, size.y - selectBoxSize.y), selectBoxSize);

				// Top
				if (CloseFloat(crop.y, 0.0f)) solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0xFF0000);
				else solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0x00FF00);
				D3DRender->DrawBox(pos, Vect2(size.x, 0.0f));

				// Left
				if (CloseFloat(crop.x, 0.0f)) solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0xFF0000);
				else solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0x00FF00);
				D3DRender->DrawBox(pos, Vect2(0.0f, size.y));

				// Right
				if (CloseFloat(crop.w, 0.0f)) solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0xFF0000);
				else solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0x00FF00);
				D3DRender->DrawBox(pos + Vect2(size.x, 0.0f), Vect2(0.0f, size.y));

				// Bottom
				if (CloseFloat(crop.z, 0.0f)) solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0xFF0000);
				else solidPixelShader->SetColor(solidPixelShader->GetParameter(0), 0x00FF00);
				D3DRender->DrawBox(pos + Vect2(0.0f, size.y), Vect2(size.x, 0.0f));
			}
		}
	}

	D3DRender->Present(SwapRender);
}

void CInstanceProcess::DrawPreProcess(float fSeconds)
{
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoList[i];

		if (OneVideo.VideoDevice && !OneVideo.VideoDevice->GetHasPreProcess() && OneVideo.bRender)
		{
			OneVideo.VideoDevice->Preprocess();
			OneVideo.VideoDevice->Tick(fSeconds);
			OneVideo.VideoDevice->SetHasPreProcess(true);
		}
		else if (!OneVideo.VideoDevice)
		{
			if (OneVideo.VideoStream && !OneVideo.VideoStream->GetHasPreProcess() && OneVideo.bRender)
			{
				OneVideo.VideoStream->Preprocess();
				OneVideo.VideoStream->Tick(fSeconds);
				OneVideo.VideoStream->SetHasPreProcess(true);
			}
		}
		
	}
}

void CInstanceProcess::SetHasPreProcess(bool bPrePro)
{
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoList[i];
		if (OneVideo.VideoStream && OneVideo.bRender)
		{
			if (OneVideo.VideoDevice)
			{
				OneVideo.VideoDevice->SetHasPreProcess(bPrePro);
			}
			else
			{
				OneVideo.VideoStream->SetHasPreProcess(bPrePro);
			}
		}
	}

	for (UINT i = 0; i < m_VideoListTransForm.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoListTransForm[i];
		if (OneVideo.VideoStream && OneVideo.bRender)
		{
			if (OneVideo.VideoDevice)
			{
				OneVideo.VideoDevice->SetHasPreProcess(bPrePro);
			}
			else
			{
				OneVideo.VideoStream->SetHasPreProcess(bPrePro);
			}
			
		}
	}
}


void CInstanceProcess::DrawTransFormProcess(float fSeconds)
{
	for (UINT i = 0; i < m_VideoListTransForm.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoListTransForm[i];
		if (OneVideo.VideoDevice && !OneVideo.VideoDevice->GetHasPreProcess() && OneVideo.bRender)
		{
			OneVideo.VideoDevice->Preprocess();
			OneVideo.VideoDevice->Tick(fSeconds);
			OneVideo.VideoDevice->SetHasPreProcess(true);
		}
		else
		{
			if (OneVideo.VideoStream && !OneVideo.VideoStream->GetHasPreProcess() && OneVideo.bRender)
			{
				OneVideo.VideoStream->Preprocess();
				OneVideo.VideoStream->Tick(fSeconds);
				OneVideo.VideoStream->SetHasPreProcess(true);
			}
		}
	
	}
}


void CInstanceProcess::DrawTransFormRender(Texture *PreTexture, Shader *VertexShader, Shader *PixShader, bool bDrawTop)
{
	D3DRender->EnableBlending(TRUE);
	D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f); //特效中切入直播混合不生效又加一次

	if (!bDrawTop)
		D3DRender->ClearRenderTarget(0xFF000000);

	for (UINT i = 0; i < m_VideoListTransForm.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoListTransForm[i];

		bool bNeedDraw = OneVideo.VideoStream && OneVideo.bRender;

		if (bNeedDraw && (OneVideo.bTop == bDrawTop))
		{
			Texture *LastTexture = NULL;

			IBaseVideo *Agent = OneVideo.VideoStream->GetGlobalSource();
			for (int j = 0; j < m_FilterTransForm.Num(); ++j)
			{
				if (m_FilterTransForm[j].IVideo == OneVideo.VideoStream.get() || Agent == m_FilterTransForm[j].IVideo || (m_FilterTransForm[j].IVideo == OneVideo.VideoDevice.get()))
				{
					Vect2 &Size = OneVideo.VideoStream->GetSize();
					D3DRender->SetRenderTarget(m_FilterTransForm[j].RenderTarget.get());
					D3DRender->ClearRenderTarget(0xFF000000);

					D3DRender->Ortho(0.0f, Size.x, Size.y, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0, 0, Size.x, Size.y);
					if (OneVideo.VideoDevice)
					{
						OneVideo.VideoDevice->Render(Vect2(0.0f, 0.0f), Size, NULL, !OneVideo.bScale, IsLiveInstance);
					}
					else
					{
						OneVideo.VideoStream->Render(Vect2(0.0f, 0.0f), Size, NULL, !OneVideo.bScale, IsLiveInstance);
					}
					

					LastTexture = m_FilterTransForm[j].RenderTarget.get();

					vector<__FilterSturct>& BaseFilter = m_FilterTransForm[j].BaseFilter;

					for (int k = 0; k < BaseFilter.size(); ++k)
					{
						__FilterSturct& OneFilter = BaseFilter[k];

						Texture *RT = OneFilter._Filter->GetRenderTarget();

						if (RT)
						{
							D3DRender->SetRenderTarget(RT);
							D3DRender->ClearRenderTarget(0xFF000000);
							//OneFilter._Filter->UpDataSetting(OneFilter.JsonParam);
							OneFilter._Filter->FilterRender(LastTexture, Size);

							LastTexture = OneFilter._Filter->GetRenderTarget();
						}

					}

					break;
				}
			}

			if (LastTexture)
			{
				D3DRender->SetRenderTarget(PreTexture);
				if (!bLittlePre)
				{
					D3DRender->Ortho(0.0f, baseCX, baseCY, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0, 0, baseCX, baseCY);
				}
				else
				{
					D3DRender->Ortho(0.0f, renderFrameCtrlSize.x, renderFrameCtrlSize.y, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0.0f, 0.0f, renderFrameCtrlSize.x, renderFrameCtrlSize.y);
				}
				D3DRender->LoadVertexShader(VertexShader);
				D3DRender->LoadPixelShader(PixShader);
			}
			Vect4 &Crop = OneVideo.GetCrop();
			D3DRender->SetCropping(Crop.x, Crop.y, Crop.w, Crop.z);
			if (OneVideo.VideoDevice)
			{
				OneVideo.VideoDevice->Render(OneVideo.pos, OneVideo.size, LastTexture, !OneVideo.bScale, IsLiveInstance);
			}
			else
			{
				OneVideo.VideoStream->Render(OneVideo.pos, OneVideo.size, LastTexture, !OneVideo.bScale, IsLiveInstance);
			}
			
			D3DRender->SetCropping(0, 0, 0, 0);
		}
	}
	D3DRender->EnableBlending(FALSE);
}

void CInstanceProcess::DrawRender(Texture *PreTexture, Shader *VertexShader, Shader *PixShader, bool bDrawTop)
{
	if (!bDrawTop)
		D3DRender->ClearRenderTarget(0xFF000000);

	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoList[i];
		bool bNeedDraw = OneVideo.VideoStream && OneVideo.bRender;

		if (bNeedDraw && (OneVideo.bTop == bDrawTop))
		{
			Texture *LastTexture = NULL;

			IBaseVideo *Agent = OneVideo.VideoStream->GetGlobalSource();
			for (int j = 0; j < m_Filter.Num(); ++j)
			{
				if (m_Filter[j].IVideo == OneVideo.VideoStream.get() || Agent == m_Filter[j].IVideo || (m_Filter[j].IVideo == OneVideo.VideoDevice.get()))
				{
					Vect2 &Size = OneVideo.VideoStream->GetSize();

					if (Size != Vect2(m_Filter[j].Width, m_Filter[j].Height))
					{
						m_Filter[j].Width = Size.x;
						m_Filter[j].Height = Size.y;

						m_Filter[j].RenderTarget.reset(D3DRender->CreateRenderTarget(Size.x, Size.y, GS_BGRA, FALSE));
					}
					D3DRender->SetRenderTarget(m_Filter[j].RenderTarget.get());
					D3DRender->ClearRenderTarget(0xFF000000);

					D3DRender->Ortho(0.0f, Size.x, Size.y, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0, 0, Size.x, Size.y);

					if (OneVideo.VideoDevice)
					{
						OneVideo.VideoDevice->Render(Vect2(0.0f, 0.0f), Size, NULL, !OneVideo.bScale, IsLiveInstance);
					}
					else
					{
						OneVideo.VideoStream->Render(Vect2(0.0f, 0.0f), Size, NULL, !OneVideo.bScale, IsLiveInstance);
					}
					
					LastTexture = m_Filter[j].RenderTarget.get();

					vector<__FilterSturct>& BaseFilter = m_Filter[j].BaseFilter;

					for (int k = 0; k < BaseFilter.size(); ++k)
					{
						__FilterSturct& OneFilter = BaseFilter[k];
						
						Texture *RT = OneFilter._Filter->GetRenderTarget();

						if (RT)
						{
							D3DRender->SetRenderTarget(RT);
							D3DRender->ClearRenderTarget(0xFF000000);
							//OneFilter._Filter->UpDataSetting(OneFilter.JsonParam); //改分辨率的时候更新这个
							OneFilter._Filter->FilterRender(LastTexture, Size);

							//这里RT有可能改变所以调用GetRenderTarget获取最新的
							LastTexture = OneFilter._Filter->GetRenderTarget();
						}

					}

					break;
				}
			}

			if (LastTexture)
			{
				D3DRender->SetRenderTarget(PreTexture);
				if (!bLittlePre)
				{
					D3DRender->Ortho(0.0f, baseCX, baseCY, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0, 0, baseCX, baseCY);
				}
				else
				{
					D3DRender->Ortho(0.0f, renderFrameCtrlSize.x, renderFrameCtrlSize.y, 0.0f, -100.0f, 100.0f);
					D3DRender->SetViewport(0.0f, 0.0f, renderFrameCtrlSize.x, renderFrameCtrlSize.y);
				}
				D3DRender->LoadVertexShader(VertexShader);
				D3DRender->LoadPixelShader(PixShader);
			}

			Vect4 &Crop = OneVideo.GetCrop();
			D3DRender->SetCropping(Crop.x, Crop.y, Crop.w, Crop.z);
			if (OneVideo.VideoDevice)
			{
				OneVideo.VideoDevice->Render(OneVideo.pos, OneVideo.size, LastTexture, !OneVideo.bScale, IsLiveInstance);
			}
			else
			{
				OneVideo.VideoStream->Render(OneVideo.pos, OneVideo.size, LastTexture, !OneVideo.bScale, IsLiveInstance);
			}
			D3DRender->SetCropping(0, 0, 0, 0);
		}
	}
}

void CInstanceProcess::DrawDb(Shader *PixShader)
{

	if (RightDb != -96 || LeftDb != -96)
	{
		D3DRender->LoadPixelShader(PixShader);


		if (RightDb > -96 && RightDb <= 0)
		{
			UINT Height = PreviewArea.height / 96 * RightDb + PreviewArea.height;

			D3DRender->DrawSpriteEx(DbTexture.get(), 0xFFFFFFFF, PreviewArea.left + PreviewArea.width - DbWidth, PreviewArea.top + PreviewArea.height - Height, PreviewArea.left + PreviewArea.width, PreviewArea.top + PreviewArea.height, 0.0f, 1.0f - (float)Height / PreviewArea.height, 1.0f, 1.0f);
		}


		if (LeftDb > -96 && LeftDb <= 0)
		{
			UINT Height = PreviewArea.height / 96 * LeftDb + PreviewArea.height;
			UINT Width = DbWidth * 2 + 1;

			D3DRender->DrawSpriteEx(DbTexture.get(), 0xFFFFFFFF, PreviewArea.left + PreviewArea.width - Width, PreviewArea.top + PreviewArea.height - Height, PreviewArea.left + PreviewArea.width - Width + DbWidth, PreviewArea.top + PreviewArea.height, 0.0f, 1.0f - (float)Height / PreviewArea.height, 1.0f, 1.0f);
		}
	}

}

void CInstanceProcess::ResizeRenderFrame(bool bRedrawRenderFrame)
{
	int curCX, curCY;
	float mainAspect;
	// 获取输出渲染窗口的宽高比，和值
	if (bRunning)
	{
		curCX = outputCX;
		curCY = outputCY;
		mainAspect = float(curCX) / float(curCY);
	}

	// Get area to render in
	RECT Rect;
	GetClientRect(RenderHwnd, &Rect);
	int x, y;
	UINT controlWidth = Rect.right;
	UINT controlHeight = Rect.bottom;

	// Scale to fit，等比例缩放
	Vect2 renderSize = Vect2(float(controlWidth), float(controlHeight));
	float renderAspect = renderSize.x / renderSize.y;

	if (renderAspect > mainAspect)
	{
		renderSize.x = renderSize.y*mainAspect;
		x = int((float(controlWidth) - renderSize.x)*0.5f);
		y = 0;
	}
	else
	{
		renderSize.y = renderSize.x / mainAspect;
		x = 0;
		y = int((float(controlHeight) - renderSize.y)*0.5f);
	}

	renderFrameOffset.x = x;
	renderFrameOffset.y = y;
	renderFrameSize.x = int(renderSize.x + 0.5f) & 0xFFFFFFFE;
	renderFrameSize.y = int(renderSize.y + 0.5f) & 0xFFFFFFFE;
	renderFrameCtrlSize.x = controlWidth;
	renderFrameCtrlSize.y = controlHeight;

	if (bRunning && bRedrawRenderFrame)
	{
		bResizeRenderView = true;
	}
}


void GradualChangeColor(void *pDest,UINT Picth,UINT size)
{
	if (!pDest)
		return;

	DWORD StartColor = 0;
	UINT Height = size / Picth;

	DWORD *destQW = (DWORD*)pDest;
	for (int j = 0; j < Height; ++j)
	{
		if (j <= Height / 2)
		{
			StartColor = (0xFF << 24) | (0xFF << 16) | ((BYTE)(255 * j * 2 / Height) << 8);
		}
		else
		{
			int Red = Height - j;
			StartColor = (0xFF << 24) | ((255 * Red * 2 / Height ) << 16) | (0xFF << 8);
		}
		

		for (int i = 0; i < Picth; i += 4)
		{
			*(destQW++) = StartColor;
		}

	}
}

void CInstanceProcess::BulidD3D()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	if (bUseBackInstance)
	{
		if (LiveParam.LiveSetting.Width < LiveParam.LiveSetting.WidthSec || LiveParam.LiveSetting.Height < LiveParam.LiveSetting.HeightSec)
		{
			baseCX = LiveParam.LiveSetting.WidthSec;
			baseCY = LiveParam.LiveSetting.HeightSec;

			outputCX_back = LiveParam.LiveSetting.Width;
			outputCY_back = LiveParam.LiveSetting.Height;
		}
		else
		{
			baseCX = LiveParam.LiveSetting.Width;
			baseCY = LiveParam.LiveSetting.Height;

			outputCX_back = LiveParam.LiveSetting.WidthSec;
			outputCY_back = LiveParam.LiveSetting.HeightSec;
		}
	}
	else
	{
		baseCX = LiveParam.LiveSetting.Width;
		baseCY = LiveParam.LiveSetting.Height;
	}

	baseCX = MIN(MAX(baseCX, 128), 4096);
	baseCY = MIN(MAX(baseCY, 128), 4096);

	scaleCX = double(baseCX);
	scaleCY = double(baseCY);

	//align width to 128bit for fast SSE YUV4:2:0 conversion
	outputCX = scaleCX & 0xFFFFFFFC;
	outputCY = scaleCY & 0xFFFFFFFE;

	outputCX_back &= 0xFFFFFFFC;
	outputCY_back &= 0xFFFFFFFE;


	if (!bLittlePre || (bLittlePre && !bNoPreView)) //只有PGM和PVW有Db
	{
		if (!bLittlePre)
		{
			DbWidth = 10;
		}
		else
		{
			DbWidth = 5;
		}
		LPBYTE pData = new BYTE[DbWidth * (int)PreviewArea.height * 4];
		//生成渐变色
		GradualChangeColor(pData, DbWidth * 4, DbWidth * (int)PreviewArea.height * 4);
		DbTexture.reset(D3DRender->CreateTexture(DbWidth, (int)PreviewArea.height, GS_BGRA, pData, FALSE, FALSE));
		delete [] pData;
	}

	ResizeRenderFrame(true);
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::BulidX264Encoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	if (videoEncoder)
	{
		delete videoEncoder;
		videoEncoder = NULL;
	}

	if (bUseBackInstance)
	{
		if (LiveParam.LiveSetting.Width < LiveParam.LiveSetting.WidthSec || LiveParam.LiveSetting.Height < LiveParam.LiveSetting.HeightSec)
		{
			if (LiveParam.LiveSetting.bUseHardEncoder)
			{
				int HardType = CSLiveManager::GetInstance()->GetHardEncoderType();
				if (HardType == 1 || HardType == 3)
				{
					videoEncoder = CreateNvidiaEncoder(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);

					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
					}
				}
				else if (HardType == 2)
				{
					videoEncoder = CreateRDX264EncoderNew(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);

					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
					}
				}
				
			}
			else
			{
				videoEncoder = CreateX264Encoder(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(),/*L"veryfast",*/ Asic2WChar(LiveParam.LiveSetting.X264Profile).c_str(), false, CSLiveManager::GetInstance()->colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false,false);
			}
			
		}
		else
		{
			if (LiveParam.LiveSetting.bUseHardEncoder)
			{
				int HardType = CSLiveManager::GetInstance()->GetHardEncoderType();
				if (HardType == 1 || HardType == 3)
				{
					videoEncoder = CreateNvidiaEncoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);

					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
					}
				}
				else if (HardType == 2)
				{
					videoEncoder = CreateRDX264EncoderNew(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);
					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
					}
				}
				
			}
			else
			{
				videoEncoder = CreateX264Encoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(),/*L"veryfast",*/ Asic2WChar(LiveParam.LiveSetting.X264Profile).c_str(), false, CSLiveManager::GetInstance()->colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false,false);
			}
		}
	}
	else
	{
		if (LiveParam.LiveSetting.bUseHardEncoder)
		{
			int HardType = CSLiveManager::GetInstance()->GetHardEncoderType();
			if (HardType == 1 || HardType == 3)
			{
				videoEncoder = CreateNvidiaEncoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);

				if (videoEncoder)
				{
					Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
				}
			}
			else if (HardType == 2)
			{
				videoEncoder = CreateRDX264EncoderNew(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false, 1);

				if (videoEncoder)
				{
					Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
				}
			}
			
		}
		else
		{
			videoEncoder = CreateX264Encoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.Quality, Asic2WChar(LiveParam.LiveSetting.X264Preset).c_str(),/*L"veryfast",*/ Asic2WChar(LiveParam.LiveSetting.X264Profile).c_str(), false, CSLiveManager::GetInstance()->colorDesc, LiveParam.LiveSetting.VideoBitRate, LiveParam.LiveSetting.VideoBitRate, false,false);
		}
	}

	if (!videoEncoder)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
		BUTEL_THORWERROR("创建videoEncoder失败");
	}

	if (bUseBackInstance)
	{
		if (videoEncoder_back)
		{
			delete videoEncoder_back;
			videoEncoder_back = NULL;
		}
		if (LiveParam.LiveSetting.Width < LiveParam.LiveSetting.WidthSec || LiveParam.LiveSetting.Height < LiveParam.LiveSetting.HeightSec)
		{
			if (LiveParam.LiveSetting.bUseHardEncoderSec)
			{
				int HardType = CSLiveManager::GetInstance()->GetHardEncoderType();
				if (HardType == 1 || HardType == 3)
				{
					videoEncoder_back = CreateNvidiaEncoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, true, 1);

					if (videoEncoder_back)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
					}
				}
				else if (HardType == 2)
				{
					videoEncoder_back = CreateRDX264EncoderNew(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, false, 1);

					if (videoEncoder_back)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
					}
				}
				
			}
			else
			{
				videoEncoder_back = CreateX264Encoder(LiveParam.LiveSetting.FPS, outputCX, outputCY, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(),/*L"veryfast",*/ Asic2WChar(LiveParam.LiveSetting.X264ProfileSec).c_str(), false, CSLiveManager::GetInstance()->colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, false, true);
			}
		}
		else
		{
			if (LiveParam.LiveSetting.bUseHardEncoderSec)
			{
				int HardType = CSLiveManager::GetInstance()->GetHardEncoderType();
				if (HardType == 1 || HardType == 3)
				{
					videoEncoder_back = CreateNvidiaEncoder(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, true, 1);

					if (videoEncoder_back)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
					}
				}
				else if (HardType == 2)
				{
					videoEncoder_back = CreateRDX264EncoderNew(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(), false, colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, false, 1);

					if (videoEncoder_back)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
					}
				}

			}
			else
			{
				videoEncoder_back = CreateX264Encoder(LiveParam.LiveSetting.FPS, outputCX_back, outputCY_back, LiveParam.LiveSetting.QualitySec, Asic2WChar(LiveParam.LiveSetting.X264PresetSec).c_str(), Asic2WChar(LiveParam.LiveSetting.X264ProfileSec).c_str(), false, CSLiveManager::GetInstance()->colorDesc, LiveParam.LiveSetting.VideoBitRateSec, LiveParam.LiveSetting.VideoBitRateSec, false, true);
			}
			
			
		}
		
		if (!videoEncoder_back)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
			BUTEL_THORWERROR("创建videoEncoder_back失败");
		}
	}
	else
	{
		if (videoEncoder_back)
		{
			delete videoEncoder_back;
			videoEncoder_back = NULL;
		}
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::BulidEncoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	if (audioEncoder)
	{
		delete audioEncoder;
	}

	if (1 == LiveParam.LiveSetting.AudioEncoderType) //选择用AAC还是Mp3
		audioEncoder = CreateAACEncoder(LiveParam.LiveSetting.AudioBitRate, LiveParam.LiveSetting.AudioSampleRate, LiveParam.LiveSetting.AudioChannel);
	else
	{
		audioEncoder = CreateMP3Encoder(LiveParam.LiveSetting.AudioBitRate, LiveParam.LiveSetting.AudioSampleRate, LiveParam.LiveSetting.AudioChannel);
	}

	if (!audioEncoder)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
		BUTEL_THORWERROR("创建audioEncoder失败");
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

String ExpandRecordingFilename(String filename)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	filename.FindReplace(L"$Y", UIntString(st.wYear).Array());
	filename.FindReplace(L"$M", UIntString(st.wMonth).Array());
	filename.FindReplace(L"$0M", FormattedString(L"%02u", st.wMonth).Array());
	filename.FindReplace(L"$D", UIntString(st.wDay).Array());
	filename.FindReplace(L"$0D", FormattedString(L"%02u", st.wDay).Array());
	filename.FindReplace(L"$h", UIntString(st.wHour).Array());
	filename.FindReplace(L"$0h", FormattedString(L"%02u", st.wHour).Array());
	filename.FindReplace(L"$m", UIntString(st.wMinute).Array());
	filename.FindReplace(L"$0m", FormattedString(L"%02u", st.wMinute).Array());
	filename.FindReplace(L"$s", UIntString(st.wSecond).Array());
	filename.FindReplace(L"$0s", FormattedString(L"%02u", st.wSecond).Array());

	filename.FindReplace(L"$T", FormattedString(L"%u-%02u-%02u-%02u%02u-%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond).Array());
	return filename;
}

String CInstanceProcess::GetOutputFilename(bool bBack)
{
	String path = OSGetDefaultVideoSavePath(L"\\.flv");
	String strOutputFile;

	if (bBack)
	{
		strOutputFile = Asic2WChar(LiveParam.LiveSetting.RecoderPathSec).c_str();
	}
	else
	{
		strOutputFile = Asic2WChar(LiveParam.LiveSetting.RecoderPath).c_str();
	}

	strOutputFile.FindReplace(TEXT("\\"), TEXT("/"));

	OSFindData ofd;
	HANDLE hFind = NULL;
	bool bUseDateTimeName = true;
	bool bOverwrite = false;
	strOutputFile = ExpandRecordingFilename(strOutputFile);

	CreatePath(GetPathDirectory(strOutputFile));

	if (!bOverwrite && (hFind = OSFindFirstFile(strOutputFile, ofd)))
	{
		String strFileExtension = GetPathExtension(strOutputFile);
		String strFileWithoutExtension = GetPathWithoutExtension(strOutputFile);

		if (strFileExtension.IsValid() && !ofd.bDirectory)
		{
			String strNewFilePath;
			UINT curFile = 0;

			do
			{
				UINT w_back, h_back;
				GetOutputSize_back(w_back, h_back);

				UINT w, h;
				GetOutputSize(w, h);

				if (bBack)
				{
					if (w_back == w && h_back == h)
					{
						strNewFilePath.Clear() << strFileWithoutExtension << TEXT(" (") << FormattedString(TEXT("%02u"), ++curFile) << TEXT(")Copy.") << strFileExtension;
					}
					else
					{
						strNewFilePath.Clear() << strFileWithoutExtension << TEXT(" (") << FormattedString(TEXT("%02u_%uX%u"), ++curFile, w_back, h_back) << TEXT(").") << strFileExtension;
					}
				}
				else
				{

					if (w_back == w && h_back == h)
					{
						strNewFilePath.Clear() << strFileWithoutExtension << TEXT(" (") << FormattedString(TEXT("%02u"), ++curFile, w, h) << TEXT(").") << strFileExtension;
					}
					else
					{
						strNewFilePath.Clear() << strFileWithoutExtension << TEXT(" (") << FormattedString(TEXT("%02u_%uX%u"), ++curFile, w, h) << TEXT(").") << strFileExtension;
					}
				}

			} while (OSFileExists(strNewFilePath));

			strOutputFile = strNewFilePath;

			bUseDateTimeName = false;
		}

		if (ofd.bDirectory)
			strOutputFile.AppendChar('/');

		OSFindClose(hFind);
	}

	if (bUseDateTimeName)
	{
		String strFileName = GetPathFileName(strOutputFile);

		if (!strFileName.IsValid() || !IsSafeFilename(strFileName))
		{
			SYSTEMTIME st;
			GetLocalTime(&st);

			String strDirectory = GetPathDirectory(strOutputFile);
			String file = strOutputFile.Right(strOutputFile.Length() - strDirectory.Length());
			String extension;

			if (!file.IsEmpty())
				extension = GetPathExtension(file.Array());

			if (extension.IsEmpty())
				extension = TEXT("mp4");
			UINT w, h;
			GetOutputSize(w, h);
			strOutputFile = FormattedString(TEXT("%s/%u-%02u-%02u-%02u%02u-%02u(%uX%u).%s"), strDirectory.Array(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, w, h, extension.Array());
			if (bBack)
			{
				UINT w_back, h_back;
				GetOutputSize_back(w_back, h_back);

				if (w_back == w && h_back == h)
				{
					strOutputFile = FormattedString(TEXT("%s/%u-%02u-%02u-%02u%02u-%02u(%uX%u)Copy.%s"), strDirectory.Array(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, w_back, h_back, extension.Array());
				}
				else
				{
					strOutputFile = FormattedString(TEXT("%s/%u-%02u-%02u-%02u%02u-%02u(%uX%u).%s"), strDirectory.Array(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, w_back, h_back, extension.Array());
				}
			}
		}
	}

	return strOutputFile;
}

String CInstanceProcess::GetOutputRecordFilename(const String &Path, const String &Name, int Width, int Height)
{
	String strOutputFile;

	strOutputFile = Path;


	strOutputFile.FindReplace(TEXT("\\"), TEXT("/"));

	OSFindData ofd;
	HANDLE hFind = NULL;
	bool bUseDateTimeName = true;
	bool bOverwrite = false;
	strOutputFile = ExpandRecordingFilename(strOutputFile);

	CreatePath(GetPathDirectory(strOutputFile));

	if (!bOverwrite && (hFind = OSFindFirstFile(strOutputFile, ofd)))
	{
		String strFileExtension = GetPathExtension(strOutputFile);
		String strFileWithoutExtension = GetPathWithoutExtension(strOutputFile);

		if (strFileExtension.IsValid() && !ofd.bDirectory)
		{
			String strNewFilePath;
			UINT curFile = 0;

			do
			{
				strNewFilePath.Clear() << strFileWithoutExtension << TEXT(" (") << FormattedString(TEXT("%02u_%uX%u"), ++curFile, Width, Height) << TEXT(").") << strFileExtension;

			} while (OSFileExists(strNewFilePath));

			strOutputFile = strNewFilePath;

			bUseDateTimeName = false;
		}

		if (ofd.bDirectory)
			strOutputFile.AppendChar('/');

		OSFindClose(hFind);
	}

	if (bUseDateTimeName)
	{
		String strFileName = GetPathFileName(strOutputFile);

		if (!strFileName.IsValid() || !IsSafeFilename(strFileName))
		{
			SYSTEMTIME st;
			GetLocalTime(&st);

			String strDirectory = GetPathDirectory(strOutputFile);
			String file = strOutputFile.Right(strOutputFile.Length() - strDirectory.Length());
			String extension;

			if (!file.IsEmpty())
				extension = GetPathExtension(file.Array());

			if (extension.IsEmpty())
				extension = TEXT("flv");
			strOutputFile = FormattedString(TEXT("%s/%s-%u-%02u-%02u-%02u%02u-%02u(%uX%u).%s"), strDirectory.Array(), Name.Array(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, Width, Height, extension.Array());

			if (OSFileExists(strOutputFile))
			{
				strOutputFile = FormattedString(TEXT("%s/%s-%u-%02u-%02u-%02u%02u-%02u-%03u(%uX%u).%s"), strDirectory.Array(), Name.Array(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,st.wMilliseconds, Width, Height, extension.Array());
			}
		}
	}

	return strOutputFile;
}

void CInstanceProcess::BulidFileStream()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	if (fileStream)
	{
		fileStream.reset();
	}

	if (fileStream_back)
	{
		fileStream_back.reset();
	}

	if (LiveParam.LiveSetting.bRecoder) //开启录制
	{
		String Path = GetOutputFilename();

		String &Ext = GetPathExtension(Path);

		if (Ext == L"flv")
			fileStream = unique_ptr<VideoFileStream>(CreateFLVFileStream(Path));
		else
		{
			fileStream = unique_ptr<VideoFileStream>(CreateMP4FileStream(Path));
		}

		if (!fileStream)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! CreateFLVFileStream failed!", __FUNCTION__);
		}
	}

	if (LiveParam.LiveSetting.bUseLiveSec && LiveParam.LiveSetting.bRecoderSec) //开启录制
	{
		String Path = GetOutputFilename(true);

		String &Ext = GetPathExtension(Path);

		if (Ext == L"flv")
			fileStream_back = unique_ptr<VideoFileStream>(CreateFLVFileStream(Path,true));
		else
		{
			fileStream_back = unique_ptr<VideoFileStream>(CreateMP4FileStream(Path,true));
		}

		if (!fileStream_back)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! CreateFLVFileStream failed!", __FUNCTION__);
		}
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::RequestKeyframe(int waitTime)
{
	if (bRequestKeyframe && waitTime > keyframeWait)
		return;

	bRequestKeyframe = true;
	keyframeWait = waitTime;
}

void CInstanceProcess::GetVideoHeaders(DataPacket &packet)
{
	videoEncoder->GetHeaders(packet);
}

void CInstanceProcess::GetVideoHeaders_back(DataPacket &packet)
{
	videoEncoder_back->GetHeaders(packet);
}


void CInstanceProcess::GetAudioHeaders(DataPacket &packet)
{
	audioEncoder->GetHeaders(packet);
}

void CInstanceProcess::BulidRtmpNetWork()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	EnterCriticalSection(&NetWorkSection);
	if (network)
	{
		delete network;
		network = NULL;
	}

// 	if (bUseBackInstance && (LiveParam.LiveSetting.Width < LiveParam.LiveSetting.WidthSec || LiveParam.LiveSetting.Height < LiveParam.LiveSetting.HeightSec))
// 	{
// 		if (LiveParam.LiveSetting.DelayTimeSec > 0)
// 		{
// 			network = CreateDelayedPublisher(LiveParam.LiveSetting.DelayTimeSec, this, true);
// 		}
// 		else
// 		{
// 			network = CreateRTMPPublisher(this, true);
// 		}
// 		network->bBack = true;
// 	}
// 	else
	{
		if (LiveParam.LiveSetting.DelayTime > 0)
		{
			network = CreateDelayedPublisher(LiveParam.LiveSetting.DelayTime, this, false);
		}
		else
		{
			network = CreateRTMPPublisher(this, false);
		}
	}

	if (!network)
	{
		LeaveCriticalSection(&NetWorkSection);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
		BUTEL_THORWERROR("创建RtmpNetwork失败");
	}
	LeaveCriticalSection(&NetWorkSection);

	if (bUseBackInstance)
	{
		EnterCriticalSection(&NetWorkSection_back);
		if (network_back)
		{
			delete network_back;
			network_back = NULL;
		}
// 		if (LiveParam.LiveSetting.Width < LiveParam.LiveSetting.WidthSec || LiveParam.LiveSetting.Height < LiveParam.LiveSetting.HeightSec)
// 		{
// 			if (LiveParam.LiveSetting.DelayTime > 0)
// 			{
// 				network_back = CreateDelayedPublisher(LiveParam.LiveSetting.DelayTime, this, false);
// 			}
// 			else
// 			{
// 				network_back = CreateRTMPPublisher(this, false);
// 			}
// 		}
// 		else
		{
			if (LiveParam.LiveSetting.DelayTimeSec > 0)
			{
				network_back = CreateDelayedPublisher(LiveParam.LiveSetting.DelayTimeSec, this, true);
			}
			else
			{
				network_back = CreateRTMPPublisher(this, true);
			}
			network_back->bBack = true;
		}
		
		if (!network_back)
		{
			LeaveCriticalSection(&NetWorkSection_back);
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
			BUTEL_THORWERROR("创建RtmpNetwork失败");
		}
		LeaveCriticalSection(&NetWorkSection_back);
	}
	else
	{
		if (network_back)
		{
			delete network_back;
			network_back = NULL;
		}
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::BulidThread()
{
	if (!bNewD3DSelf)
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s 不是自己创建的D3D不能创建线程",__FUNCTION__);
		return;
	}

	HVideoCapture = CreateThread(NULL, 0, VideoCapture, this, 0, NULL);

	if (!HVideoCapture)
	{
		BUTEL_THORWERROR("%s HVideoCapture 线程创建失败", __FUNCTION__);
	}

	HAudioCapture = CreateThread(NULL, 0, AudioCapture, this, 0, NULL);

	if (!HAudioCapture)
	{
		BUTEL_THORWERROR("%s HAudioCapture 线程创建失败", __FUNCTION__);
	}

}

void CInstanceProcess::StartLive(bool bRecordOnly, bool bCreatRtmp)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	if (bStartLive)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!,已经为开始状态 ", __FUNCTION__);
		return;
	}
	bLiveInstanceRecordOnly = bRecordOnly;
	ListPublisher.clear();


	if (!bLiveInstanceRecordOnly && !bNewD3DSelf && bCreatRtmp) //不是自创建的D3DRender才推流
	{
		String URL = Asic2WChar(LiveParam.LiveSetting.LivePushUrl).c_str();

		if (URL.IsEmpty())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
			BUTEL_THORWERROR("URL为空");
		}


		int Len = URL.Length();
		TSTR StrUrl = URL.Array() + Len - 1;
		int iLoop = Len - 1;
		while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
		{
			if (*StrUrl == '/')
			{
				break;
			}
			--iLoop;
			--StrUrl;
		}

		PushPath0 = URL.Right(Len - iLoop - 1);
		if (PushPath0.IsEmpty())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
			BUTEL_THORWERROR("PushPath为空");
		}
		PushURL0 = URL.Left(iLoop + 1);

		if (PushURL0.IsEmpty())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
			BUTEL_THORWERROR("PushURL为空");
		}


		if (LiveParam.LiveSetting.bUseBackPush)
		{
			URL = Asic2WChar(LiveParam.LiveSetting.LiveBackPushUrl).c_str();
			int Len = URL.Length();
			TSTR StrUrl = URL.Array() + Len - 1;
			int iLoop = Len - 1;
			while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
			{
				if (*StrUrl == '/')
				{
					break;
				}
				--iLoop;
				--StrUrl;
			}

			PushPath1 = URL.Right(Len - iLoop - 1);
			PushURL1 = URL.Left(iLoop + 1);
		}

		if (LiveParam.LiveSetting.bUseLiveSec)
		{
			if (LiveParam.LiveSetting.bUsePushSec)
			{
				URL = Asic2WChar(LiveParam.LiveSetting.LivePushUrlSec).c_str();
				int Len = URL.Length();
				TSTR StrUrl = URL.Array() + Len - 1;
				int iLoop = Len - 1;
				while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
				{
					if (*StrUrl == '/')
					{
						break;
					}
					--iLoop;
					--StrUrl;
				}

				PushPath0_back = URL.Right(Len - iLoop - 1);
				PushURL0_back = URL.Left(iLoop + 1);
			}

			if (LiveParam.LiveSetting.bUseBackPushSec)
			{
				URL = Asic2WChar(LiveParam.LiveSetting.LiveBackPushUrlSec).c_str();
				int Len = URL.Length();
				TSTR StrUrl = URL.Array() + Len - 1;
				int iLoop = Len - 1;
				while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
				{
					if (*StrUrl == '/')
					{
						break;
					}
					--iLoop;
					--StrUrl;
				}

				PushPath1_back = URL.Right(Len - iLoop - 1);
				PushURL1_back = URL.Left(iLoop + 1);
			}

		}

		BulidRtmpNetWork();
	}
	BulidX264Encoder();
	BulidEncoder();
	BulidFileStream();

	lastAudioTimestamp = 0;
	lastAudioTimestamp_back = 0;
	bfirstTimeStamp = true;
	bFristAudioEncode = true;
	IsReject = false;
	bStartLive = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::StopLive(bool bUI)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	//if (bUI)
	{
		if (!bStartLive && bUI)//加了bUI,因为停止录制也会调这里
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!,已经为停止状态 ", __FUNCTION__);
			return;
		}

		bStartLive = false;
		bLiveInstanceRecordOnly = false;
		//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
		//return;
	}

	ListPublisher.clear();

	if (network_back)
	{
		network_back->GetRtmpPublist(ListPublisher);
	}

	EnterCriticalSection(&NetWorkSection);
	if (network)
	{
		network->GetRtmpPublist(ListPublisher);
		delete network;
		network = NULL;
	}


	if (fileStream)
	{
		fileStream.reset();
	}

	LeaveCriticalSection(&NetWorkSection);

	EnterCriticalSection(&NetWorkSection_back);

	if (network_back)
	{
		delete network_back;
		network_back = NULL;
	}
	if (fileStream_back)
	{
		fileStream_back.reset();
	}
	LeaveCriticalSection(&NetWorkSection_back);
	
	CurrentAudioTime = 0;

	for (UINT i = 0; i < bufferedVideo.Num(); ++i)
	{
		VideoSegment &Segment = bufferedVideo[i];
		Segment.Clear();
	}
	bufferedVideo.Clear();

	for (UINT i = 0; i < bufferedVideo_back.Num(); ++i)
	{
		VideoSegment &Segment = bufferedVideo_back[i];
		Segment.Clear();
	}
	bufferedVideo_back.Clear();
	bufferedTimes.Clear();
	bufferedTimes_back.Clear();

	if (LiveParam.TipsCb)
	{
		LiveParam.TipsCb(-100, " ");
	}


	bFristIn = true;
	bFristOut = true;

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::SetParam(const SLiveParam *Param)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! Param Width %d,Param Height %d,Param VideoBitRate %d", __FUNCTION__, Param->LiveSetting.Width, Param->LiveSetting.Height, Param->LiveSetting.VideoBitRate);

	if (Param->Advanced.bChange)
	{
		if (Param->Advanced.BufferTime == 0)
		{
			LiveParam.Advanced.BufferTime = 200;
			LiveParam.Advanced.bUseMultiThread = Param->Advanced.bUseMultiThread;
			LiveParam.Advanced.DeinterlaceType = Param->Advanced.DeinterlaceType;
			LiveParam.Advanced.PriorityID = Param->Advanced.PriorityID;
		}
		else
		{
			memcpy(&LiveParam.Advanced, &Param->Advanced, sizeof AdvancedParam);
		}
	}

	if (Param->LiveSetting.bChange)
	{
		bool bChange = false;
		if (LiveParam.LiveSetting.Width != Param->LiveSetting.Width || LiveParam.LiveSetting.Height != Param->LiveSetting.Height || LiveParam.LiveSetting.bUseLiveSec != Param->LiveSetting.bUseLiveSec)
		{
			bChange = true;
		}

		if (LiveParam.LiveSetting.VideoBitRate != Param->LiveSetting.VideoBitRate)
		{
			if (videoEncoder)
			{
				videoEncoder->SetBitRate(Param->LiveSetting.VideoBitRate, Param->LiveSetting.VideoBitRate);
			}
		}

		if (LiveParam.LiveSetting.bUseLiveSec)
		{
			if (LiveParam.LiveSetting.WidthSec != Param->LiveSetting.WidthSec || LiveParam.LiveSetting.HeightSec != Param->LiveSetting.HeightSec)
			{
				bChange = true;
			}

			//bitrate

			if (LiveParam.LiveSetting.VideoBitRateSec != Param->LiveSetting.VideoBitRateSec)
			{
				if (videoEncoder_back)
				{
					videoEncoder_back->SetBitRate(Param->LiveSetting.VideoBitRateSec, Param->LiveSetting.VideoBitRateSec);
				}
			}
		}

		if (LiveParam.LiveSetting.AudioSampleRate != Param->LiveSetting.AudioSampleRate)
		{
			bReBulidAudio = true;
		}
		//判断推流地址是否更改

		if (IsLiveInstance && bStartLive)
		{
			do 
			{
				if (0 != strcmp(LiveParam.LiveSetting.LivePushUrl, Param->LiveSetting.LivePushUrl))
				{
					//主推流地址改变

					String URL = Asic2WChar(Param->LiveSetting.LivePushUrl).c_str();

					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 推流过程中主直播的主推流地址改变为 %s 原来地址 %s", __FUNCTION__, Param->LiveSetting.LivePushUrl, LiveParam.LiveSetting.LivePushUrl);

					if (URL.IsEmpty())
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Param->LiveSetting.LivePushUrl empty", __FUNCTION__);
						break;
					}


					int Len = URL.Length();
					TSTR StrUrl = URL.Array() + Len - 1;
					int iLoop = Len - 1;
					while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
					{
						if (*StrUrl == '/')
						{
							break;
						}
						--iLoop;
						--StrUrl;
					}

					PushPath0 = URL.Right(Len - iLoop - 1);
					if (PushPath0.IsEmpty())
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushPath0 empty", __FUNCTION__);
						break;
					}
					PushURL0 = URL.Left(iLoop + 1);

					if (PushURL0.IsEmpty())
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushURL0 empty", __FUNCTION__);
						break;
					}

					EnterCriticalSection(&NetWorkSection);

					if (network)
						network->ReConnectPuhlisher(L"Publish0");

					LeaveCriticalSection(&NetWorkSection);

				}
			} while (0);
			

			if (Param->LiveSetting.bUseBackPush)
			{
				do
				{
					if (0 != strcmp(LiveParam.LiveSetting.LivePushUrlSec, Param->LiveSetting.LivePushUrlSec))
					{
						//主推流次流地址改变

						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 推流过程中主直播的次推流地址改变为 %s 原来地址 %s", __FUNCTION__, Param->LiveSetting.LivePushUrlSec, LiveParam.LiveSetting.LivePushUrlSec);

						String URL = Asic2WChar(Param->LiveSetting.LivePushUrlSec).c_str();

						if (URL.IsEmpty())
						{
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Param->LiveSetting.LivePushUrlSec empty", __FUNCTION__);
							break;
						}

						int Len = URL.Length();
						TSTR StrUrl = URL.Array() + Len - 1;
						int iLoop = Len - 1;
						while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
						{
							if (*StrUrl == '/')
							{
								break;
							}
							--iLoop;
							--StrUrl;
						}

						PushPath1 = URL.Right(Len - iLoop - 1);

						if (PushPath1.IsEmpty())
						{
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushPath1 empty", __FUNCTION__);
							break;
						}

						PushURL1 = URL.Left(iLoop + 1);

						if (PushURL1.IsEmpty())
						{
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushURL1 empty", __FUNCTION__);
							break;
						}

						EnterCriticalSection(&NetWorkSection);

						if (network)
							network->ReConnectPuhlisher(L"Publish1");

						LeaveCriticalSection(&NetWorkSection);

					}
				} while (0);

			}

			if (Param->LiveSetting.bUseLiveSec)
			{
				if (Param->LiveSetting.bUsePushSec)
				{
					do 
					{
						if (0 != strcmp(LiveParam.LiveSetting.LiveBackPushUrl, Param->LiveSetting.LiveBackPushUrl))
						{
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 推流过程中次直播的主推流地址改变为 %s 原来地址 %s", __FUNCTION__, Param->LiveSetting.LiveBackPushUrl, LiveParam.LiveSetting.LiveBackPushUrl);
							String URL = Asic2WChar(Param->LiveSetting.LiveBackPushUrl).c_str();

							if (URL.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Param->LiveSetting.LiveBackPushUrl empty", __FUNCTION__);
								break;
							}

							int Len = URL.Length();
							TSTR StrUrl = URL.Array() + Len - 1;
							int iLoop = Len - 1;
							while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
							{
								if (*StrUrl == '/')
								{
									break;
								}
								--iLoop;
								--StrUrl;
							}

							PushPath0_back = URL.Right(Len - iLoop - 1);

							if (PushPath0_back.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushPath0_back empty", __FUNCTION__);
								break;
							}

							PushURL0_back = URL.Left(iLoop + 1);


							if (PushURL0_back.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushURL0_back empty", __FUNCTION__);
								break;
							}

							EnterCriticalSection(&NetWorkSection_back);

							if (network_back)
								network_back->ReConnectPuhlisher(L"Publish0");

							LeaveCriticalSection(&NetWorkSection_back);
						}
					} while (0);
					
				}

				if (Param->LiveSetting.bUseBackPushSec)
				{
					do 
					{
						if (0 != strcmp(LiveParam.LiveSetting.LiveBackPushUrlSec, Param->LiveSetting.LiveBackPushUrlSec))
						{

							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 推流过程中次直播的次推流地址改变为 %s 原来地址 %s", __FUNCTION__, Param->LiveSetting.LiveBackPushUrlSec, LiveParam.LiveSetting.LiveBackPushUrlSec);

							String URL = Asic2WChar(Param->LiveSetting.LiveBackPushUrlSec).c_str();

							if (URL.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Param->LiveSetting.LiveBackPushUrlSec empty", __FUNCTION__);
								break;
							}

							int Len = URL.Length();
							TSTR StrUrl = URL.Array() + Len - 1;
							int iLoop = Len - 1;
							while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
							{
								if (*StrUrl == '/')
								{
									break;
								}
								--iLoop;
								--StrUrl;
							}

							PushPath1_back = URL.Right(Len - iLoop - 1);

							if (PushPath1_back.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushPath1_back empty", __FUNCTION__);
								break;
							}

							PushURL1_back = URL.Left(iLoop + 1);

							if (PushURL1_back.IsEmpty())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PushURL1_back empty", __FUNCTION__);
								break;
							}

							EnterCriticalSection(&NetWorkSection_back);

							if (network_back)
								network_back->ReConnectPuhlisher(L"Publish1");

							LeaveCriticalSection(&NetWorkSection_back);
						}

					} while (0);
					
				}

			}

		}

		memcpy(&LiveParam.LiveSetting, &Param->LiveSetting, sizeof LiveSettingParam);

		if (LiveParam.LiveSetting.VideoBitRate > 100000)
		{
			Log::writeError(LOG_RTSPSERV, 1, "%s BSParam.LiveSetting.VideoBitRate  过大 %d,设置为100000", __FUNCTION__, LiveParam.LiveSetting.VideoBitRate);
			LiveParam.LiveSetting.VideoBitRate = 100000;
		}

		if (LiveParam.LiveSetting.VideoBitRateSec > 100000)
		{
			Log::writeError(LOG_RTSPSERV, 1, "%s BSParam.LiveSetting.VideoBitRateSec  过大 %d,设置为100000", __FUNCTION__, LiveParam.LiveSetting.VideoBitRateSec);
			LiveParam.LiveSetting.VideoBitRateSec = 100000;
		}

		if (LiveParam.LiveSetting.FPS <= 0)
		{
			LiveParam.LiveSetting.FPS = 25;
		}

		if (LiveParam.LiveSetting.FPSSec <= 0)
		{
			LiveParam.LiveSetting.FPSSec = 25;
		}

		bUseBackInstance = LiveParam.LiveSetting.bUseLiveSec;

		if (Param->DeviceSetting.bChange)
		{
			memcpy(&LiveParam.DeviceSetting, &Param->DeviceSetting, sizeof DeviceParam);
		}

		if (bChange)
		{
			int MaxWidth = LiveParam.LiveSetting.Width;
			int MaxHeight = LiveParam.LiveSetting.Height;
			if (LiveParam.LiveSetting.bUseLiveSec && (LiveParam.LiveSetting.WidthSec > LiveParam.LiveSetting.Width))
			{
				MaxWidth = LiveParam.LiveSetting.WidthSec;
				MaxHeight = LiveParam.LiveSetting.HeightSec;
			}

			EnterCriticalSection(&VideoSection);

			for (int i = 0; i < m_VideoList.Num(); ++i)
			{
				VideoStruct &OneVideo = m_VideoList[i];

				//if (0 == strcmp(OneVideo.VideoStream->GainClassName(), "AgentSource"))
				{
					OneVideo.pos.x = MaxWidth * OneVideo.pos.x / baseCX;
					OneVideo.pos.y = MaxHeight * OneVideo.pos.y / baseCY;
					OneVideo.size.x = MaxWidth * OneVideo.size.x / baseCX;
					OneVideo.size.y = MaxHeight * OneVideo.size.y / baseCY;
				}
			}

			LeaveCriticalSection(&VideoSection);

			

			bReBulid = true;
		}

	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::SetHwnd(uint64_t hwnd)
{
	RenderHwnd = (HWND)hwnd;
}

void CInstanceProcess::ClearVideo(bool bRemoveDelay, bool bCut, bool bCanAddAgent, bool bRemoveTop)
{
	std::vector<IBaseVideo *> vAgentListLocal;
	std::vector<shared_ptr<IBaseVideo>> vAgentList;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num();)
	{
		VideoStruct &Video = m_VideoList[i];

		if (!bRemoveTop && Video.bTop)
		{
			++i;
			continue;
		}

		//在ClearAudio时已经SetLiveInstance设置
// 		if (m_VideoList[i].AudioStream)
// 		{
// 			EnterCriticalSection(&AudioSection);
// 
// 			for (UINT j = 0; j < m_AudioList.Num(); ++j)
// 			{
// 				if (m_AudioList[j].AudioStream.get() == m_VideoList[i].AudioStream)
// 				{
// 					if (m_AudioList[j].AudioStream.use_count() == 2)
// 						m_AudioList[j].AudioStream->SetLiveInstance(false);
// 
// 					if (bRemove)
// 					{
// 						m_AudioList[j].AudioStream.reset();
// 						m_AudioList.Remove(j);
// 					}
// 					break;
// 				}
// 			}
// 			LeaveCriticalSection(&AudioSection);
// 		}

		if (Video.VideoStream)
		{
			//如果是区域占位源要在bRender的时候调，如果不是则直接进入
			if (Video.VideoStream->GetGlobalSource())
			{
				if (Video.bRender)
				{
					IBaseVideo *BaseVideo = Video.VideoStream.get();

					if (BaseVideo)
					{
						//为了区域占位源不影响PVW切换做的更改
						if (IsLiveInstance)
						{
							if (bCanAddAgent)
							{
								vAgentList.push_back(Video.VideoStream);
							}
							else
							{
								//为了防止在有特效切换时区域占位源的析构而导致音频被移除
								vEffectAgentList.push_back(Video.VideoStream);
							}
							
						}
						else
						{
							if (bRemoveDelay && bCut)
							{
								vAgentListLocal.push_back(BaseVideo);
							}
							else if (m_VideoListTransForm.Num() == 0 && !bCut)
							{
								vAgentListLocal.push_back(BaseVideo);
							}
							else
							{
								BaseVideo->GlobalSourceLeaveScene();
							}
						}

					}
				}
			}
			else if (Video.bRender)
			{
				if (Video.bGlobalStream && Video.VideoStream.use_count() == 2)
				{
					IBaseVideo *BaseVideo = Video.VideoStream.get();

					if (BaseVideo)
					{
						BaseVideo->GlobalSourceLeaveScene();
						if (Video.VideoDevice)
						{
							Video.VideoDevice->GlobalSourceLeaveScene();
							Video.VideoDevice->SetCanEnterScene(true);
						}
						BaseVideo->SetCanEnterScene(true);
					}
				}
			}

			for (UINT j = 0; j < m_Filter.Num(); ++j)
			{
				Filter &OneFilter = m_Filter[j];

				if (strcmp(Video.VideoStream->GainClassName(), "AgentSource") == 0)
				{
					IBaseVideo *BaseVideo = Video.VideoStream->GetGlobalSource();

					if (BaseVideo && OneFilter.IVideo == BaseVideo)
					{
						m_Filter.Remove(j);
						break;
					}
				}
				else
				{
					if (OneFilter.IVideo == Video.VideoStream.get() || (OneFilter.IVideo == Video.VideoDevice.get()))
					{
						m_Filter.Remove(j);
						break;
					}
				}
				
			}
			
			Video.VideoStream.reset();


			if (Video.Config)
			{
				Video.Config.reset();
			}
		}
		if (Video.VideoDevice)
			Video.VideoDevice.reset();

		m_VideoList.Remove(i);
	}
	//m_VideoList.Clear();
	LeaveCriticalSection(&VideoSection);

	//要在m_VideoList.Clear()之后进行
	//if (!IsLiveInstance)
	{
		for (auto Video : vAgentListLocal)
		{
			Video->GlobalSourceLeaveScene();
		}
	}
	
}

void CInstanceProcess::ClearVideoTransForm()
{
	std::vector<shared_ptr<IBaseVideo>> vAgentList;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoListTransForm.Num(); ++i)
	{
		VideoStruct &Video = m_VideoListTransForm[i];
		if (Video.VideoStream)
		{
			if (strcmp(Video.VideoStream->GainClassName(), "AgentSource") == 0)
			{
				//如果是区域占位源，则让它在自己析构的时候GlobalSourceLeaveScene
				vAgentList.push_back(Video.VideoStream);
			}
			else
			{
				if (Video.bGlobalStream && Video.VideoStream.use_count() == 2)
				{
					IBaseVideo *BaseVideo = Video.VideoStream.get();

					if (BaseVideo)
					{
						BaseVideo->GlobalSourceLeaveScene();
						if (strcmp(BaseVideo->GainClassName(),"DeviceSource") == 0)
						{
							Video.VideoDevice->GlobalSourceLeaveScene();
							Video.VideoDevice->SetCanEnterScene(true);
						}
						BaseVideo->SetCanEnterScene(true);
					}
				}
			}
		

			Video.VideoStream.reset();

			if (Video.Config)
			{
				Video.Config.reset();
			}
		}
		if (Video.VideoDevice)
			Video.VideoDevice.reset();
	
	}
	m_VideoListTransForm.Clear();
	LeaveCriticalSection(&VideoSection);

	ClearFilterTransForm();

	vEffectAgentList.clear();
}

void CInstanceProcess::ClearAudio()
{
	EnterCriticalSection(&AudioSection);
	for (UINT i = 0; i < m_AudioList.Num(); ++i)
	{
		AudioStruct &Audio = m_AudioList[i];

// 		if (IsLiveInstance)
// 		{
// 			//没有移除，稍后会在音频线程里移除
// 			m_AudioList[i].AudioStream->SetLiveInstance(false);
// 		}
// 		else
// 		{
			if (m_AudioList[i].AudioStream.use_count() == 2)
				m_AudioList[i].AudioStream->SetLiveInstance(false);

			if (!IsLiveInstance)//新加的这里
			{
				if (Audio.AudioStream)
					Audio.AudioStream.reset();
			}
//		}

		if (Audio.Config)
			Audio.Config.reset();
	
	}
	
	if (!IsLiveInstance)
		m_AudioList.Clear();

	LeaveCriticalSection(&AudioSection);
}

void CInstanceProcess::CopyNewToVideoList()
{
	for (int i = 0; i < m_VideoListTransForm.Num(); ++i)
	{
		VideoStruct &VSTem = m_VideoListTransForm[i];
		m_VideoList.SetSize(m_VideoList.Num() + 1);
		VideoStruct &VS = m_VideoList[m_VideoList.Num() - 1];
		VS = VSTem;
		VS.bSelect = false;

		if (VS.AudioStream)
		{
			EnterCriticalSection(&AudioSection);
			for (int i = 0; i < m_AudioList.Num(); ++i)
			{
				AudioStruct &OneAudio = m_AudioList[i];

				if (OneAudio.AudioStream.get() == VS.AudioStream)
				{
					if (OneAudio.AudioStream.use_count() == 3 && IsLiveInstance)
						OneAudio.AudioStream->SetLiveInstance(true);
					break;
				}
			}

// 			if (!bFind)
// 			{
// 				//这里是不对的，应该增加引用计数
// 				AudioStruct InAudio;
// 				InAudio.AudioStream = shared_ptr<IBaseAudio>(VS.AudioStream);
// 				InAudio.VideoStream = VS.VideoStream.get();
// 
// 				m_AudioList.SetSize(m_AudioList.Num() + 1);
// 				AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
// 				AS = InAudio;
// 
// 			}
			LeaveCriticalSection(&AudioSection);
		}
	}

	for (int i = 0; i < m_FilterTransForm.Num(); ++i)
	{
		Filter& FTer = m_FilterTransForm[i];

		m_Filter.SetSize(m_Filter.Num() + 1);
		Filter &VS = m_Filter[m_Filter.Num() - 1];
		VS = FTer;
	}
}

void CInstanceProcess::AddVideoStream(VideoStruct& VideoStream)
{
	bool bFind = false;
	for (int i = 0; i < m_VideoList.Num(); ++i)
	{
		if (m_VideoList[i].VideoStream.get() == VideoStream.VideoStream.get())
		{
			bFind = true;
			break;
		}
	}

	if (!bFind)
	{
		m_VideoList.SetSize(m_VideoList.Num() + 1);
		VideoStruct &VS = m_VideoList[m_VideoList.Num() - 1];
		VS = VideoStream;
	}
}

void CInstanceProcess::ClearFilterTransForm()
{
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_FilterTransForm.Num(); ++i)
	{
		Filter& FTer = m_FilterTransForm[i];

		vector<__FilterSturct> &BaseFilter = FTer.BaseFilter;

		for (UINT j = 0; j < BaseFilter.size(); ++j)
		{
			__FilterSturct &OneFilter = BaseFilter[j];

			if (OneFilter._Filter)
				OneFilter._Filter.reset();
		}


		if (FTer.RenderTarget)
			FTer.RenderTarget.reset();
	}

	m_FilterTransForm.Clear();
	LeaveCriticalSection(&VideoSection);
}

void CInstanceProcess::CreateLittleRenderTarget()
{
	RECT Rect;
	GetClientRect((HWND)RenderHwnd, &Rect);

	//LittleRenderTarget = D3DRender->CreateRenderTarget(Rect.right, Rect.bottom, GS_RGBA, FALSE);
}

void CInstanceProcess::DrawLittlePreview(Shader *VertShader, Shader *PixShader)
{
	if (MultiRender /*&& MultiRender->texture*/)
	{
		MultiRender->RenderTexture(VertShader,PixShader);
	}
}

char* CInstanceProcess::EncMetaData(char *enc, char *pend, bool bFLVFile, bool bBack)
{
	VideoEncoder *Video;
	if (bBack)
	{
		Video = GetVideoEncoder_back();
	}
	else
	{
		Video = GetVideoEncoder();
	}

	int Width = RecordWidth;
	int Height = RecordHeight;

	Video->GetWH(Width, Height);
	AudioEncoder *Audio = GetAudioEncoder();
	int    maxBitRate = Video->GetBitRate();
	int    fps = Video->GetFps();
	int    audioBitRate = Audio ? Audio->GetBitRate() : 0;
	CTSTR  lpAudioCodec = Audio ? Audio->GetCodec() : L"";

	const AVal *av_codecFourCC;

#ifdef USE_AAC
	if (scmpi(lpAudioCodec, TEXT("AAC")) == 0)
	{
		av_codecFourCC = &av_mp4a;
		//audioCodecID = 10.0;
	}
	else
#endif
	{
		av_codecFourCC = &av_mp3;
		//audioCodecID = 2.0;
	}

	if (bFLVFile)
	{
		*enc++ = AMF_ECMA_ARRAY;
		enc = AMF_EncodeInt32(enc, pend, 14);
	}
	else
		*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);

	enc = AMF_EncodeNamedNumber(enc, pend, &av_width, double(Width));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_height, double(Height));


	enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//

	enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, double(maxBitRate));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, double(fps));

	if (Audio)
	{
		enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, av_codecFourCC);//audioCodecID);//
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, double(audioBitRate)); //ex. 128kb\s
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, double(Audio->GetsampleRate()));
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
		int numChannels = Audio->GetnumChannels();
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, double(numChannels));
		//enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo,         true);

		enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, numChannels == 2);
	}
	else
	{
		enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//audioCodecID);//
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, double(0)); //ex. 128kb\s
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, double(0));
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, double(0));
		//enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo,         true);

		enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, false);
	}
	const AVal Vision = AVC("SLive");
	enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &Vision);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	return enc;
}

void CInstanceProcess::GetOutputSize(UINT&Width, UINT &Height)
{
	Width = LiveParam.LiveSetting.Width;
	Height = LiveParam.LiveSetting.Height;
}

void CInstanceProcess::GetOutputSize_back(UINT&Width, UINT &Height)
{
	Width = LiveParam.LiveSetting.WidthSec;
	Height = LiveParam.LiveSetting.HeightSec;
}

UINT CInstanceProcess::GetFrameTime()
{
	return 1000 / LiveParam.LiveSetting.FPS;
}

void CInstanceProcess::ChangeToBackup()
{
	EnterCriticalSection(&NetWorkSection);
	if (network)
	{
		network->SetStoped();
		network->ResetNoDelayPublisher(this,false);
	}
	LeaveCriticalSection(&NetWorkSection);

	EnterCriticalSection(&NetWorkSection_back);
	if (network_back)
	{
		network_back->SetStoped();
		network_back->ResetNoDelayPublisher(this, true);
	}
	LeaveCriticalSection(&NetWorkSection_back);
}

void CInstanceProcess::SetForceKillThread()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);
	bForceKilled = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::SetDelayCancel()
{
	DelaySection.Lock();
	for (int i = 0; i < ListPublisher.size(); ++i)
	{
		RTMPPublisher *Publisher = ListPublisher[i];
		Publisher->SetStop(true);

	}
	DelaySection.UnLock();
}

void CInstanceProcess::ProcessRecord(CSampleData *Data)
{
	if (bCanRecord)
	{
		RecordTime = (*Data->UserData)["RecordTime"].asUInt();
		//开启录制
		if (!Data->bAudio)
		{
			string &DeviceName = (*Data->UserData)["audioDeviceName"].asString();
			bool bAudioDevice = strcmp(DeviceName.c_str(), "禁用") != 0;

			if (!bRecord || RecordWidth != Data->cx || RecordHeight != Data->cy || bAudioDevice != bHasAudio || DeviceName.compare(m_DeviceName.c_str()))
			{
				RecordWidth = Data->cx;
				RecordHeight = Data->cy;

				StopRecord();

				bHasAudio = bAudioDevice;
				m_DeviceName = DeviceName;

				RecordBitRate = (*Data->UserData)["RecorderBitRate"].asUInt();
				RecordPath = Asic2WChar((*Data->UserData)["RecordPath"].asString().c_str()).c_str();
				if (fileStream)
					fileStream.reset();

				if (RecordBitRate > 100000)
				{
					Log::writeError(LOG_RTSPSERV, 1, "%s RecordBitRate 过大 %d,设置为100000", __FUNCTION__,RecordBitRate);
					RecordBitRate = 100000;
				}

				RecordFPS = 10000000.0 / (*Data->UserData)["frameInterval"].asUInt();

				if (IsSupportRecord(L"NVIDIA"))
				{
					videoEncoder = CreateNvidiaEncoder(RecordFPS, Data->cx, Data->cy, 8, L"veryfast", true, colorDesc, RecordBitRate, RecordBitRate, false, Data->colorType == DeviceOutputType_I420 ? 2 : 0);

					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use NVIDIA 显卡硬编", __FUNCTION__);
					}
				}
				else if (IsSupportRecord(L"Intel"))
				{
					videoEncoder = CreateRDX264EncoderNew(RecordFPS, Data->cx, Data->cy, 8, L"veryfast", false, colorDesc, RecordBitRate, RecordBitRate, false, Data->colorType == DeviceOutputType_I420 ? 2 : 0);

					if (videoEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s Use Intel 显卡硬编", __FUNCTION__);
					}
				}
				if (!videoEncoder)
				{
					Log::writeError(LOG_RTSPSERV, 1, "%s CreateNvidiaEncoder or CreateRDX264EncoderNew failed", __FUNCTION__);
				}

				String RecordName;

				EnterCriticalSection(&VideoSection);
				if (m_VideoList.Num() > 0)
				{
					RecordName = Asic2WChar((*m_VideoList[0].Config)["Name"].asString()).c_str();
				}
				else
				{
					RecordName = Asic2WChar((*Data->UserData)["Name"].asString().c_str()).c_str();
				}
				LeaveCriticalSection(&VideoSection);

				String &Path = GetOutputRecordFilename(RecordPath, RecordName, Data->cx, Data->cy);

				if (videoEncoder)
				{
					if (bHasAudio && audioEncoder)
					{
						fileStream = unique_ptr<VideoFileStream>(CreateFLVFileStreamNew(Path, true, false, this));
						DataPacket packet;
						audioEncoder->GetHeaders(packet);
						fileStream->AddPacket((const BYTE *)packet.lpPacket, packet.size, (DWORD)0, 0, PacketType_Audio_HEAD);
					}
					else if (!bHasAudio)
					{
						fileStream = unique_ptr<VideoFileStream>(CreateFLVFileStreamNew(Path, false, false, this));
					}

				}
				
				//RTMP

// 				String URL = Asic2WChar(LiveParam.LiveSetting.LivePushUrl).c_str();
// 
// 				if (URL.IsEmpty())
// 				{
// 					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
// 					BUTEL_THORWERROR("URL为空");
// 				}
// 
// 
// 				int Len = URL.Length();
// 				TSTR StrUrl = URL.Array() + Len - 1;
// 				int iLoop = Len - 1;
// 				while (iLoop <= Len - 1 && iLoop >= 0 && *StrUrl != '\0')
// 				{
// 					if (*StrUrl == '/')
// 					{
// 						break;
// 					}
// 					--iLoop;
// 					--StrUrl;
// 				}
// 
// 				PushPath0 = URL.Right(Len - iLoop - 1);
// 				if (PushPath0.IsEmpty())
// 				{
// 					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
// 					BUTEL_THORWERROR("PushPath为空");
// 				}
// 				PushURL0 = URL.Left(iLoop + 1);
// 
// 				if (PushPath0.IsEmpty())
// 				{
// 					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occur", __FUNCTION__);
// 					BUTEL_THORWERROR("PushURL为空");
// 				}

				//network = CreateRTMPPublisher(this, false);

				///////////////////////////

				bRecord = true;
				if (!m_hEncodeThread)
					m_hEncodeThread = OSCreateThread((XTHREAD)VideoEncoderThread, this);
			}
			
			if (videoEncoder)
			{
				Data->AddRef();
				OSEnterMutex(m_hMutexRawV);
				m_listVRawData.push_back(Data);
				OSLeaveMutex(m_hMutexRawV);
			}
		}
		else if (bRecord)
		{
			if (!audioEncoder)
			{
				bool bAudioDevice = strcmp((*Data->UserData)["audioDeviceName"].asString().c_str(), "禁用") != 0;
			
				if (bAudioDevice)
				{
					WAVEFORMATEX *Wave = (WAVEFORMATEX*)Data->pAudioFormat;
					audioEncoder = CreateAACEncoderNew(128, Wave->nSamplesPerSec, Wave->nChannels, Wave->wBitsPerSample / 8);

					if (!audioEncoder)
					{
						Log::writeError(LOG_RTSPSERV, 1, "%s CreateAACEncoderNew failed", __FUNCTION__);
					}


					if (bAudioDevice != bHasAudio)
					{
						bHasAudio = bAudioDevice;
						if (videoEncoder)
						{
							if (fileStream)
							{
								bRecord = false;
								EnterCriticalSection(&NetWorkSection);
								lastAudioTimestamp = 0;
								m_firstFrameTimestamp = -1;

								if (fileStream)
									fileStream.reset();

								if (m_hEncodeThread)
								{
									if (WAIT_TIMEOUT == WaitForSingleObject(m_hEncodeThread, 5000))
									{
										TerminateThread(m_hEncodeThread, 0);
									}

									CloseHandle(m_hEncodeThread);
									m_hEncodeThread = NULL;
								}
								LeaveCriticalSection(&NetWorkSection);

								bRecord = true;
								if (!m_hEncodeThread)
									m_hEncodeThread = OSCreateThread((XTHREAD)VideoEncoderThread, this);
							}

							RecordPath = Asic2WChar((*Data->UserData)["RecordPath"].asString().c_str()).c_str();

							String &Path = GetOutputRecordFilename(RecordPath, Asic2WChar((*Data->UserData)["Name"].asString().c_str()).c_str(), RecordWidth, RecordHeight);

							fileStream = unique_ptr<VideoFileStream>(CreateFLVFileStreamNew(Path, true, false, this));

							DataPacket packet;
							audioEncoder->GetHeaders(packet);
							fileStream->AddPacket((const BYTE *)packet.lpPacket, packet.size, (DWORD)0, 0, PacketType_Audio_HEAD);
						}
					}
					else if (!fileStream)
					{
						RecordPath = Asic2WChar((*Data->UserData)["RecordPath"].asString().c_str()).c_str();

						String &Path = GetOutputRecordFilename(RecordPath, Asic2WChar((*Data->UserData)["Name"].asString().c_str()).c_str(), RecordWidth, RecordHeight);

						fileStream = unique_ptr<VideoFileStream>(CreateFLVFileStreamNew(Path, true, false, this));

						DataPacket packet;
						audioEncoder->GetHeaders(packet);
						fileStream->AddPacket((const BYTE *)packet.lpPacket, packet.size, (DWORD)0, 0, PacketType_Audio_HEAD);
					}
					
				}
				
			}

			if (audioEncoder)
			{
				bHasAudio = true;
			
				Data->AddRef();
				OSEnterMutex(m_hMutexRawA);
				m_listARawData.push_back(Data);
				OSLeaveMutex(m_hMutexRawA);
			}
		}

	}
	else
	{
		StopRecord();
	}
}

DWORD CInstanceProcess::VideoEncoderThread(LPVOID lparam)
{
	CInstanceProcess *pThis = (CInstanceProcess *)lparam;
	Log::writeMessage(LOG_RTSPSERV, 1, "VideoEncoderThread ID = %d Begin", GetCurrentThreadId());
	if (NULL != pThis)
	{
		CoInitialize(0);
		pThis->VideoEncoderLoop();
		CoUninitialize();
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "VideoEncoderThread ID = %d End", GetCurrentThreadId());
	return 0;
}

bool CInstanceProcess::QueryNewAudio(std::deque<CSampleData *> &listAudio)
{
	OSEnterMutex(m_hMutexRawA);
	listAudio.swap(m_listARawData);
	OSLeaveMutex(m_hMutexRawA);
	return listAudio.size() > 0;
}

bool CInstanceProcess::QueryNewVideo(std::deque<CSampleData *> &listVideo)
{
	OSEnterMutex(m_hMutexRawV);
	listVideo.swap(m_listVRawData);
	OSLeaveMutex(m_hMutexRawV);
	return listVideo.size() > 0;
}

void CInstanceProcess::VideoEncoderLoop()
{
	std::deque<CSampleData *> templistVideo;
	std::deque<CSampleData *> templistAuido;
	// 初始化编码缓冲区数据
	x264_picture_t *picIn = new x264_picture_t;
	x264_picture_init(picIn);
	//x264_picture_alloc(picIn, X264_CSP_NV12, m_recordPara.uWidth, m_recordPara.uHeight);

	QWORD streamTimeStart = GetQPCNS(); // current nano second		

	Log::writeMessage(LOG_RTSPSERV, 1, "%llu 录制线程启动", GetQPCMS());

	CircularList<QWORD> bufferedTimes;
	CSampleData *LastSampleData = NULL;
#if SAVE_YUV_TEST
	// 输出文件创建
	FILE *fyuv = fopen("D://Test.yuv", "wb");
#endif
	while (bRecord || m_listVRawData.size())
	{
		if (!RecordFPS)
			RecordFPS = 25;

		QWORD frameTimeNS = 1000000000 / RecordFPS;

		if (!QueryNewVideo(templistVideo) && LastSampleData)
		{
			LastSampleData->timestamp = GetQPCNS() / 1000000;
			templistVideo.push_back(LastSampleData);
		}
		if (templistVideo.size() > 0)
		{
			for (auto pFrame : templistVideo)
			{
				// 转换数据
				picIn->img.plane[0] = pFrame->lpData;
				picIn->img.i_stride[0] = pFrame->dataLength;
				picIn->i_pts = pFrame->timestamp /*/ (10 * 1000)*/;
				List<DataPacket> videoPackets;
				List<PacketType> videoPacketTypes;

				// 需要发送的视频数据
				std::list<std::shared_ptr<VideoSegment>> segmentOuts;

				bufferedTimes << pFrame->timestamp;

				bool bProcessedFrame, bSendFrame = false;
				EnterCriticalSection(&NetWorkSection);

				DWORD pts;
				if(videoEncoder)
					videoEncoder->Encode(picIn, videoPackets, videoPacketTypes, pFrame->timestamp, pts);
#if SAVE_YUV_TEST
				fwrite(pFrame->lpData, 1, pFrame->dataLength, fyuv);
#endif
				bProcessedFrame = (videoPackets.Num() != 0);// encode success if videoPackets have data 

				if (LastSampleData != pFrame)
				{
					if (LastSampleData)
						LastSampleData->Release();

					LastSampleData = pFrame;
				}
				
				if (bHasAudio && QueryNewAudio(templistAuido))
				{
					for (auto pAudioFrame : templistAuido)
					{
						DataPacket packet;
						QWORD timestamp = pAudioFrame->timestamp /*/ (10 * 1000)*/;
						if (audioEncoder)
						{
							if (audioEncoder->Encode(pAudioFrame->lpData, pAudioFrame->dataLength, packet, timestamp))
							{
								EnterCriticalSection(&SoundDataMutex);
								FrameAudio *frameAudio = pendingAudioFrames.CreateNew();
								frameAudio->audioData.CopyArray(packet.lpPacket, packet.size);
								frameAudio->timestamp = timestamp;
								LeaveCriticalSection(&SoundDataMutex);
							}
						}
						pAudioFrame->Release();
					}
					templistAuido.clear();
				}

				// 缓冲视频数据
				
				if (bProcessedFrame && bRecord)
				{
					if (-1 == m_firstFrameTimestamp || fileStream)
					{
						if (-1 == m_firstFrameTimestamp)
							m_firstFrameTimestamp = bufferedTimes[0];

						// 视频数据
						DataPacket packet;
						if (videoEncoder)
						{
							videoEncoder->GetHeaders(packet);
							if (packet.size > 0)
							{
								if (fileStream)
								{
									fileStream->AddPacket((const BYTE *)packet.lpPacket, packet.size, (DWORD)0, 0, PacketType_Video_HEAD);
									videoEncoder->GetSEI(packet);
									fileStream->AddPacket((const BYTE *)packet.lpPacket, packet.size, (DWORD)0, 0, PacketType_Video_SEI);
								}
							}
							else
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "%llu 视频编码线程无法获取视频头", GetQPCMS());
							}
						}

					}
					// 视频送入缓冲中

					if (bHasAudio)
					{
						bSendFrame = BufferVideoDataList(videoPackets, videoPacketTypes, bufferedTimes[0] - m_firstFrameTimestamp, bufferedTimes[0] - m_firstFrameTimestamp, m_firstFrameTimestamp, segmentOuts);
						bufferedTimes.Remove(0);
					}
					else
					{
						VideoSegment &segmentIn = *bufferedVideo.CreateNew();
						segmentIn.timestamp = bufferedTimes[0] - m_firstFrameTimestamp;
						segmentIn.pts = segmentIn.timestamp;

						bufferedTimes.Remove(0);

						segmentIn.packets.SetSize(videoPackets.Num());
						for (UINT i = 0; i < videoPackets.Num(); i++)
						{
							segmentIn.packets[i].data.CopyArray(videoPackets[i].lpPacket, videoPackets[i].size);
							segmentIn.packets[i].type = videoPacketTypes[i];
						}
						auto segmentOut = std::make_shared<VideoSegment>();
						segmentOut->packets.TransferFrom(bufferedVideo[0].packets);
						segmentOut->timestamp = bufferedVideo[0].timestamp;
						segmentOut->pts = bufferedVideo[0].pts;

						segmentOuts.push_back(segmentOut);
						bufferedVideo.Remove(0);

						bSendFrame = segmentOuts.size() > 0;
					}
				}
				else
					nop();

				//send headers before the first frame if not yet sent
				if (bSendFrame)
				{
					for (auto pos : segmentOuts)
					{
						SendFrame(*pos, m_firstFrameTimestamp);
					}
				}

				LeaveCriticalSection(&NetWorkSection);
			}
			templistVideo.clear();
		}

		Sleep2NS(streamTimeStart += frameTimeNS);
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "11111templistVideo.size %d", templistVideo.size());

	int iCount = 0;
	for(auto Vdata : m_listVRawData)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "11111m_listVRawData %d",++iCount);
		Vdata->Release();
	}
	m_listVRawData.clear();

	iCount = 0;
	for (auto Adata : m_listARawData)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "11111m_listARawData %d", ++iCount);
		Adata->Release();
	}
	m_listARawData.clear();

	iCount = 0;
	for (UINT i = 0; i < bufferedVideo.Num(); ++i)
	{
		VideoSegment &Segment = bufferedVideo[i];
		Segment.Clear();
		Log::writeMessage(LOG_RTSPSERV, 1, "11111bufferedVideo %d", ++iCount);
	}
	bufferedVideo.Clear();

	for (UINT i = 0; i < pendingAudioFrames.Num(); ++i)
		pendingAudioFrames[i].audioData.Clear();
	pendingAudioFrames.Clear();

	if(picIn)
		delete picIn;

	if (LastSampleData)
		LastSampleData->Release();

#if SAVE_YUV_TEST
	fclose(fyuv);
#endif
}

void CInstanceProcess::StopRecord()
{
	bHasAudio = false;
	if (bRecord)
	{
		bRecord = false;

		if (m_hEncodeThread)
		{
			if (WAIT_TIMEOUT == WaitForSingleObject(m_hEncodeThread, 5000))
			{
				TerminateThread(m_hEncodeThread, 0);
			}

			CloseHandle(m_hEncodeThread);
			m_hEncodeThread = NULL;
		}

		EnterCriticalSection(&NetWorkSection);
		lastAudioTimestamp = 0;
		m_firstFrameTimestamp = -1;

		LeaveCriticalSection(&NetWorkSection);

		StopLive(false);

		if (videoEncoder)
			delete videoEncoder;
		videoEncoder = NULL;

		if (audioEncoder)
			delete audioEncoder;
		audioEncoder = NULL;
	}
}

UINT CInstanceProcess::GetRenderCount()
{
	UINT iCount = 0;
	EnterCriticalSection(&VideoSection);
	for (int i = 0; i < m_VideoList.Num(); ++i)
	{
		VideoStruct &OneVideo = m_VideoList[i];
		if (OneVideo.bRender)
			++iCount;
	}
	LeaveCriticalSection(&VideoSection);

	return iCount;
}

Filter CInstanceProcess::AddFilter(uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu,FilterName = %s", __FUNCTION__, iStreamID, FilterName);
	bool bFind = false;
	bool bAgent = false;
	Vect2 Size;
	IBaseVideo *StreamDevice = NULL;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			if (strcmp(m_VideoList[i].VideoStream->GainClassName(),"AgentSource") == 0)
			{
				//区域占位不允许添加Filter
				bAgent = true;
				break;
			}
			Size = m_VideoList[i].VideoStream->GetSize();

			if (m_VideoList[i].VideoDevice)
			{
				Size = m_VideoList[i].VideoDevice->GetSize();
				StreamDevice = m_VideoList[i].VideoDevice.get();
			}

			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	Filter SFilter;
	if (bFind)
	{
		CPObject *Stream = CreatStreamObjectFilter(FilterName);

		if (Stream)
		{
			IIBaseFilter *BaseFilter = dynamic_cast<IIBaseFilter*>(Stream);
			BaseFilter->InitFilter(Size.x, Size.y);

			Value  Param;
			Param["width"] = Size.x;
			Param["height"] = Size.y;

			__FilterSturct __Filter;
			__Filter.JsonParam = Param;
			__Filter._Filter = shared_ptr<IIBaseFilter>(BaseFilter);
			
			EnterCriticalSection(&VideoSection);

			bool bFindFilter = false;
			for (UINT i = 0; i < m_Filter.Num(); ++i)
			{
				Filter &OneFilter = m_Filter[i];
				if ((uint64_t)OneFilter.IVideo == iStreamID || OneFilter.IVideo == StreamDevice)
				{
					OneFilter.BaseFilter.push_back(__Filter);
					bFindFilter = true;
					SFilter = OneFilter;
					break;
				}
			}

			if (!bFindFilter)
			{
				SFilter.Width = Size.x;
				SFilter.Height = Size.y;

				if (SFilter.Width == 0 || SFilter.Width == 32)
					SFilter.Width = 1280;
				if (SFilter.Height == 0 || SFilter.Height == 32)
					SFilter.Height = 720;

				SFilter.BaseFilter.push_back(__Filter);

				if (StreamDevice)
				{
					SFilter.IVideo = StreamDevice;
				}
				else
				{
					SFilter.IVideo = (IBaseVideo*)iStreamID;
				}
				
				SFilter.RenderTarget = shared_ptr<Texture>(D3DRender->CreateRenderTarget(SFilter.Width, SFilter.Height, GS_BGRA, FALSE));

				m_Filter.SetSize(m_Filter.Num() + 1);
				Filter &FTter = m_Filter[m_Filter.Num() - 1];
				FTter = SFilter;
			}

			LeaveCriticalSection(&VideoSection);

			*iFilterID = (uint64_t)BaseFilter;
		}
		else
		{
			BUTEL_THORWERROR("没有找到FilterName为 %s 的滤镜", FilterName);
		}
	}
	else
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		if (bAgent)
		{
			BUTEL_THORWERROR("区域占位不允许添加Filter");
		}
		else
		{
			BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
		}
		
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);

	return SFilter;
}

void CInstanceProcess::AddFilter(const Filter &NewFilter)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin!", __FUNCTION__);

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		IBaseVideo *Global = m_VideoList[i].VideoStream->GetGlobalSource();
		if (m_VideoList[i].VideoStream.get() == NewFilter.IVideo || (Global && Global == NewFilter.IVideo) || (m_VideoList[i].VideoDevice && NewFilter.IVideo == m_VideoList[i].VideoDevice.get()))
		{
			bool bFind = false;
			for (UINT j = 0; j < m_Filter.Num(); ++j)
			{
				Filter &OneFilter = m_Filter[j];
				if (OneFilter.IVideo == NewFilter.IVideo)
				{
					vector<__FilterSturct> &BaseFilter = OneFilter.BaseFilter;

					for (UINT m = 0; m < NewFilter.BaseFilter.size(); ++m)
					{
						__FilterSturct &NewOneFilter = const_cast<__FilterSturct&>(NewFilter.BaseFilter[m]);

						bool bFindFilter = false;
						for (UINT k = 0; k < BaseFilter.size(); ++k)
						{
							__FilterSturct &OneFilter = BaseFilter[k];
						
							if (OneFilter._Filter.get() == NewOneFilter._Filter.get())
							{
								bFindFilter = true;
								break;
							}
						}

						if (!bFindFilter)
						{
							OneFilter.BaseFilter.push_back(NewOneFilter);
						}
					}


					bFind = true;
					break;
				}
			}
			
			if (!bFind)
			{
				m_Filter.SetSize(m_Filter.Num() + 1);
				Filter &FTter = m_Filter[m_Filter.Num() - 1];
				FTter = NewFilter;
			}

			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::DeleteFilter(uint64_t iStreamID, uint64_t iFilterID, uint64_t iDeviceID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		IBaseVideo *Global = m_VideoList[i].VideoStream->GetGlobalSource();
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID || (Global && (uint64_t)Global == iStreamID) || (iDeviceID && (uint64_t)(Global) == iDeviceID))
		{
			for (UINT j = 0; j < m_Filter.Num(); ++j)
			{
				Filter &OneFilter = m_Filter[j];

				if (OneFilter.IVideo == m_VideoList[i].VideoStream.get() || OneFilter.IVideo == Global || (m_VideoList[i].VideoDevice && OneFilter.IVideo == m_VideoList[i].VideoDevice.get()))
				{
					vector<__FilterSturct> &BaseFilter = OneFilter.BaseFilter;

					auto Begin = begin(BaseFilter);
					auto End = end(BaseFilter);

					for (Begin; Begin != End; ++Begin)
					{
						if ((uint64_t)Begin->_Filter.get() == iFilterID)
						{
							if (Begin->_Filter)
								Begin->_Filter.reset();
							BaseFilter.erase(Begin);
							break;
						}
					}

					if (BaseFilter.empty())
					{
						if (OneFilter.RenderTarget)
							OneFilter.RenderTarget.reset();

						m_Filter.Remove(j);

					}

					break;
				}
			}

			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::UpdateFilter(uint64_t iStreamID, uint64_t iFilterID, Value &JValue)
{
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			for (UINT j = 0; j < m_Filter.Num(); ++j)
			{
				Filter &OneFilter = m_Filter[j];

				if (OneFilter.IVideo == m_VideoList[i].VideoStream.get() || (m_VideoList[i].VideoDevice && OneFilter.IVideo == m_VideoList[i].VideoDevice.get()))
				{
					vector<__FilterSturct> &BaseFilter = OneFilter.BaseFilter;

					for (auto &__Filter : BaseFilter)
					{
						if ((uint64_t)__Filter._Filter.get() == iFilterID)
						{
							if (__Filter._Filter)
								__Filter._Filter->UpDataSetting(JValue);
							break;
						}
					}

					break;
				}
			}

			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);

	}

}

uint64_t CInstanceProcess::FindDShowDeviceID(uint64_t iStreamID)
{
	uint64_t DeviceId = 0;
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			
			DeviceId = (uint64_t)m_VideoList[i].VideoDevice.get();
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	return DeviceId;
}

void CInstanceProcess::ClearEmptyAgent()
{
	EnterCriticalSection(&VideoSection);

	for (int i = 0; i < m_VideoList.Num();)
	{
		VideoStruct &OneVideo = m_VideoList[i];

		if (strcmp(OneVideo.VideoStream->GainClassName(),"AgentSource") == 0)
		{
			if (!OneVideo.VideoStream->GetGlobalSource())
			{
				OneVideo.VideoStream.reset();
				OneVideo.Config.reset();
				m_VideoList.Remove(i);
			}
			else
			{
				++i;
			}
		}
		else
		{
			++i;
		}
	}

	LeaveCriticalSection(&VideoSection);
}

void CInstanceProcess::ClearVideoTop()
{
	std::vector<shared_ptr<IBaseVideo>> vAgentList;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num();)
	{
		VideoStruct &Video = m_VideoList[i];

		if (Video.bTop && Video.VideoStream)
		{
			//如果是区域占位源要在bRender的时候调，如果不是则直接进入
			if (Video.VideoStream->GetGlobalSource())
			{
				if (Video.bRender)
				{
					IBaseVideo *BaseVideo = Video.VideoStream.get();

					if (BaseVideo)
					{
						//为了区域占位源不影响PVW切换做的更改
						if (IsLiveInstance)
						{
							//为了防止在有特效切换时区域占位源的析构而导致音频被移除
							vEffectAgentList.push_back(Video.VideoStream);
						}
						else
						{
							BaseVideo->GlobalSourceLeaveScene();
						}

					}
				}
			}
			else if (Video.bRender)
			{
				if (Video.bGlobalStream && Video.VideoStream.use_count() == 2)
				{
					IBaseVideo *BaseVideo = Video.VideoStream.get();

					if (BaseVideo)
					{
						BaseVideo->GlobalSourceLeaveScene();
						if (Video.VideoDevice)
						{
							Video.VideoDevice->GlobalSourceLeaveScene();
							Video.VideoDevice->SetCanEnterScene(true);
						}
						BaseVideo->SetCanEnterScene(true);
					}
				}
			}

			for (UINT j = 0; j < m_Filter.Num(); ++j)
			{
				Filter &OneFilter = m_Filter[j];

				if (strcmp(Video.VideoStream->GainClassName(), "AgentSource") == 0)
				{
					IBaseVideo *BaseVideo = Video.VideoStream->GetGlobalSource();

					if (BaseVideo && OneFilter.IVideo == BaseVideo)
					{
						m_Filter.Remove(j);
						break;
					}
				}
				else
				{
					if (OneFilter.IVideo == Video.VideoStream.get() || (OneFilter.IVideo == Video.VideoDevice.get()))
					{
						m_Filter.Remove(j);
						break;
					}
				}

			}

			Video.VideoStream.reset();


			if (Video.Config)
			{
				Video.Config.reset();
			}

			if (Video.VideoDevice)
				Video.VideoDevice.reset();

			m_VideoList.Remove(i);

		}
		else
		{
			++i;
		}
		
	}
	LeaveCriticalSection(&VideoSection);
}

bool CInstanceProcess::CreateD3DRender()
{
	D3DRender = new D3DAPI(LiveParam.DeviceSetting.AdpterID);

	if (!D3DRender)
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s new D3DApi failed!", __FUNCTION__);
		return false;
	}


	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		m_VideoList[i].VideoStream->SetD3DRender(D3DRender);
	}

	mainVertexShader.reset(D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader")));

	if (!mainVertexShader)
	{
		BUTEL_THORWERROR("加载mainVertexShader失败");
	}


	mainPixelShader.reset(D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawTexture.pShader")));

	if (!mainPixelShader)
	{
		BUTEL_THORWERROR("加载mainPixelShader失败");
	}

	yuvScalePixelShader.reset(D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DownscaleBilinear1YUV.pShader")));
	if (!yuvScalePixelShader)
	{
		BUTEL_THORWERROR("加载yuvScalePixelShader失败");
	}


	for (int i = 0; i < 2; ++i)
	{
		mainRenderTextures[i].reset(D3DRender->CreateRenderTarget(baseCX, baseCY, GS_BGRA, FALSE));
	}

	yuvRenderTextures.reset(D3DRender->CreateRenderTarget(outputCX, outputCY, GS_BGRA, FALSE));


	copyTextures.reset(D3DRender->CreateTextureRead(outputCX, outputCY));

	bNewD3DSelf = true;

	return true;
}

DWORD WINAPI CInstanceProcess::VideoCapture(LPVOID Param)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s 线程invode begin!",__FUNCTION__);

	CInstanceProcess *pThis = (CInstanceProcess *)(Param);

	pThis->VideoCaptureLoop();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s 线程invode end!", __FUNCTION__);
	return 0;
}

DWORD WINAPI CInstanceProcess::AudioCapture(LPVOID Param)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s 线程invode begin!", __FUNCTION__);
	CInstanceProcess *pThis = (CInstanceProcess *)(Param);
	pThis->AudioCaptureLoop();
	Log::writeMessage(LOG_RTSPSERV, 1, "%s 线程invode end!", __FUNCTION__);
	return 0;
}

void CInstanceProcess::VideoCaptureLoop()
{
	QWORD frameTimeNS = 1000000000 / LiveParam.LiveSetting.FPS;

	HANDLE hMatrix = yuvScalePixelShader->GetParameterByName(TEXT("yuvMat"));
	HANDLE hScaleVal = yuvScalePixelShader->GetParameterByName(TEXT("baseDimensionI"));

	int CopyI = 0;
	Vect2 baseSize = Vect2(float(baseCX), float(baseCY));
	x264_picture_t* outPics[2] = { NULL };

	for (int i = 0; i < 2; ++i)
	{
		outPics[i] = new x264_picture_t;
		x264_picture_init(outPics[i]);
		x264_picture_alloc(outPics[i], X264_CSP_NV12, outputCX, outputCY);

	}


	int FrameCount = 0;

	QWORD LastVideoTimeNS = 0;
	QWORD lastStreamTime = 0;
	double bpsTime = 0.0;
	int swapIndex = 0;
	QWORD StartStreamTime = GetQPCMS();
	QWORD sleepTargetTime = GetQPCNS();
	QWORD CurrentVideoTime = 0;
	QWORD StartVideoTime = 0;
	int FPSCount = 0;


	bool bFirst = true;
	while (bRunning)
	{
		frameTimeNS = 1000000000 / LiveParam.LiveSetting.FPS;
		if (!bRunning)
			break;

		QWORD StartTime = GetQPCMS();

		Sleep2NS(sleepTargetTime += frameTimeNS / 2);

		LastVideoTimeNS = sleepTargetTime;

		QWORD curStreamTime = LastVideoTimeNS;
		if (!lastStreamTime)
			lastStreamTime = curStreamTime - frameTimeNS;

		QWORD frameDelta = curStreamTime - lastStreamTime;

		double fSeconds = double(frameDelta)*0.000000001;


		EnterCriticalSection(&VideoSection);
		//------------------------------------
		// render the mini render texture

		SetHasPreProcess(false);

		DrawPreProcess(fSeconds);
		//profileOut

		D3DRender->LoadVertexShader(mainVertexShader.get());
		D3DRender->LoadPixelShader(mainPixelShader.get());

		D3DRender->Ortho(0.0f, baseCX, baseCY, 0.0f, -100.0f, 100.0f);
		D3DRender->SetViewport(0, 0, baseCX, baseCY);// 一定要在PreProcess之后

		D3DRender->SetRenderTarget(mainRenderTextures[swapIndex].get());
		DrawRender(mainRenderTextures[swapIndex].get(), mainVertexShader.get(), mainPixelShader.get());

		LeaveCriticalSection(&VideoSection);


		bpsTime += fSeconds;

		if (bpsTime >= 1.0f)
		{
			//FPS = FrameCount;
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
		}

		++FrameCount;
		//++FPSCount;

		HRESULT result;
		try{

			if (bStartLive)
			{
				//profileIn("MainVideoLoop bStartLive")
				D3DRender->LoadVertexShader(mainVertexShader.get());
				D3DRender->LoadPixelShader(yuvScalePixelShader.get());

				D3DRender->SetRenderTarget(yuvRenderTextures.get());

		
				yuvScalePixelShader->SetMatrix(hMatrix, (float*)yuvMat[5]);
				
				yuvScalePixelShader->SetVector2(hScaleVal, 1.0f / baseSize);


				D3DRender->Ortho(0.0f, outputCX, outputCY, 0.0f, -100.0f, 100.0f);
				D3DRender->SetViewport(0.0f, 0.0f, outputCX, outputCY);

				D3DRender->DrawSprite(mainRenderTextures[swapIndex].get(), 0xFFFFFFFF, 0.0f, 0.0f, outputCX, outputCY);


				BYTE *lpData;
				UINT Pitch;
				D3DRender->CopyTexture(copyTextures.get(), yuvRenderTextures.get());

				if (FAILED(result = D3DRender->Map(copyTextures.get(), lpData, Pitch)))
				{
					BUTEL_THORWERROR("Map Failed! ");
				}

				Convert444toNV12((LPBYTE)lpData, outputCX, Pitch, outputCX, outputCY, 0, outputCY, outPics[swapIndex]->img.plane);
				D3DRender->Unmap(copyTextures.get());

				//InterlockedExchangePointer((volatile PVOID*)&Outpic, outPics[swapIndex]);


				FrameProcessInfo ProcessInfo;

				if (bfirstTimeStamp)
				{
					CurrentVideoTime = 0;
					StartVideoTime = GetQPCNS();
					bfirstTimeStamp = false;

					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:第一个视频时间戳1111 %llu", StartVideoTime / 1000000);
				}
				else
				{
					CurrentVideoTime = (GetQPCNS() - StartVideoTime) / 1000000;
				}

				ProcessInfo.firstFrameTime = StartVideoTime / 1000000;
				ProcessInfo.frameTimestamp = CurrentVideoTime;
				if (bStartLive)
				{
					ProcessInfo.pic = outPics[swapIndex];
					ProcessInfo.pic->i_pts = CurrentVideoTime;
				}
				else
				{
					ProcessInfo.pic = NULL;
				}


				EnterCriticalSection(&NetWorkSection);
				ProcessFrame(ProcessInfo);
				LeaveCriticalSection(&NetWorkSection);

			}
		}
		catch (...)
		{
			Log::writeError(LOG_RTSPSERV, 1, "Map Failed!");
		}


		D3DRender->Flush();

		lastStreamTime = curStreamTime;

		++swapIndex %= 2;

		Sleep2NS(sleepTargetTime += frameTimeNS / 2);

	}

	EnterCriticalSection(&NetWorkSection);
	bOutPicDel = true;
	for (int i = 0; i < 2; ++i)
	{
		if (outPics[i])
		{
			delete outPics[i];
			outPics[i] = NULL;
		}
	}
	LeaveCriticalSection(&NetWorkSection);

}

void CInstanceProcess::AudioCaptureLoop()
{
	unsigned int audioSamplesPerSec = LiveParam.LiveSetting.AudioSampleRate;
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
		if (QueryNewAudio())
		{
			QWORD &timestamp = bufferedAudioTimes[0];
			bufferedAudioTimes.Remove(0);


			zero(mixBuffer.Array(), audioSampleSize * 2 * sizeof(float));
			zero(rightaudioData.Array(), audioSampleSize * sizeof (float));
			zero(leftaudioData.Array(), audioSampleSize * sizeof (float));

			//profileIn("MainAudioLoop MixAudio")

			EnterCriticalSection(&AudioSection);

			for (UINT i = 0; i < m_AudioList.Num(); ++i) {
				float *auxBuffer;
				if (m_AudioList[i].PreviewAudio && m_AudioList[i].PreviewAudio->GetBuffer(&auxBuffer, timestamp, IsLiveInstance))
				{
					MixAudio(mixBuffer.Array(), auxBuffer, audioSampleSize * 2, false);
				}
			}

			LeaveCriticalSection(&AudioSection);

			if (bStartLive)
			{
				EncodeAudioSegment(mixBuffer.Array(), audioSampleSize, timestamp);
				if (bFristAudioEncode)
				{
					bFristAudioEncode = false;
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:第一个音频时间戳1111 %llu", timestamp);
				}
			}
			else
			{
				EnterCriticalSection(&SoundDataMutex);
				for (UINT i = 0; i < pendingAudioFrames.Num(); ++i)
					pendingAudioFrames[i].audioData.Clear();
				pendingAudioFrames.Clear();
				LeaveCriticalSection(&SoundDataMutex);
			}
		}
		else
		{
			Sleep(5);
		}

	}


		EnterCriticalSection(&SoundDataMutex);
		for (UINT i = 0; i < pendingAudioFrames.Num(); ++i)
			pendingAudioFrames[i].audioData.Clear();
		pendingAudioFrames.Clear();
		LeaveCriticalSection(&SoundDataMutex);
	
}

void CInstanceProcess::StopThread()
{
	if (HVideoCapture)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待窗口视频采集线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HVideoCapture, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:窗口视频采集线程等待退出超过5000,强杀!");
			TerminateThread(HVideoCapture, 0);
		}
		CloseHandle(HVideoCapture);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:窗口视频采集线程退出!");
	}

	if (HAudioCapture)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待窗口音频采集线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HAudioCapture, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:窗口音频采集线程等待退出超过5000,强杀!");
			TerminateThread(HAudioCapture, 0);
		}
		CloseHandle(HAudioCapture);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:窗口音频采集线程退出!");
	}
}

void CInstanceProcess::SetAudioNeed(bool bNeedPVMAudio, bool bNeedPGMAudio)
{
	bNeedPVW = bNeedPVMAudio;
	bNeedPGM = bNeedPGMAudio;
}

void CInstanceProcess::DoMixOpenAndFollowOpen(AudioStruct& Audio)
{
	bool bInSences = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].AudioStream == (uint64_t)Audio.AudioStream.get())
		{
			bInSences = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (bInSences)
	{
		//加入音频
		DoMixOpen(Audio);
	}
	else
	{
		//去掉音频
		DoMixClose(Audio);
	}
}

void CInstanceProcess::DoMixOpen(AudioStruct& Audio)
{
	EnterCriticalSection(&AudioSection);
	//加入音频
	bool bFind = false;

	for (UINT j = 0; j < m_AudioList.Num(); ++j)
	{
		if ((uint64_t)m_AudioList[j].AudioStream.get() == (uint64_t)Audio.AudioStream.get())
		{
			bFind = true;
			break;
		}
	}

	if (!bFind)
	{
		m_AudioList.SetSize(m_AudioList.Num() + 1);
		AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
		AS = Audio;

		if (IsLiveInstance)
		{
			AS.AudioStream->SetLiveInstance(true);
		}
	}
	LeaveCriticalSection(&AudioSection);
}

void CInstanceProcess::DoMixClose(AudioStruct& Audio)
{
	EnterCriticalSection(&AudioSection);

	for (UINT j = 0; j < m_AudioList.Num(); ++j)
	{
		if ((uint64_t)m_AudioList[j].AudioStream.get() == (uint64_t)Audio.AudioStream.get())
		{

			if (IsLiveInstance)
			{
				m_AudioList[j].AudioStream->SetLiveInstance(false);
			}

			m_AudioList[j].AudioStream.reset();

			if (m_AudioList[j].Config)
				m_AudioList[j].Config.reset();

			m_AudioList.Remove(j);
			break;
		}
	}

	LeaveCriticalSection(&AudioSection);
}

void CInstanceProcess::RemoveMixOpenFollowoOpenAndNoInSences()
{
	EnterCriticalSection(&AudioSection);

	for (UINT j = 0; j < m_AudioList.Num();)
	{
		AudioStruct &Audio = m_AudioList[j];
		if (Audio.MixOpen && Audio.FollowOpen)
		{
			bool bInSences = false;
			EnterCriticalSection(&VideoSection);
			for (UINT i = 0; i < m_VideoList.Num(); ++i)
			{
				if ((uint64_t)m_VideoList[i].AudioStream == (uint64_t)Audio.AudioStream.get())
				{
					bInSences = true;
					break;
				}
			}
			LeaveCriticalSection(&VideoSection);

			if (!bInSences)
			{
				if (IsLiveInstance)
				{
					m_AudioList[j].AudioStream->SetLiveInstance(false);
				}

				m_AudioList[j].AudioStream.reset();

				if (m_AudioList[j].Config)
					m_AudioList[j].Config.reset();

				m_AudioList.Remove(j);
			}
			else
			{
				++j;
			}
			
		}
		else
		{
			++j;
		}
	}

	LeaveCriticalSection(&AudioSection);
}

