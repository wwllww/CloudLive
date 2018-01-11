#ifndef INSTANCE_H
#define INSTANCE_H
#include "BaseAfx.h"
#include "SLiveApi.h"
#include "BaseVideo.h"
#include "BaseAudio.h"
#include "BaseFilter.h"
#include <list>
#include "MultimediaRender.h"
extern "C"
{
#include "../x264/x264.h"
}

//----------------------------
enum ColorPrimaries
{
	ColorPrimaries_BT709 = 1,
	ColorPrimaries_Unspecified,
	ColorPrimaries_BT470M = 4,
	ColorPrimaries_BT470BG,
	ColorPrimaries_SMPTE170M,
	ColorPrimaries_SMPTE240M,
	ColorPrimaries_Film,
	ColorPrimaries_BT2020
};

enum ColorTransfer
{
	ColorTransfer_BT709 = 1,
	ColorTransfer_Unspecified,
	ColorTransfer_BT470M = 4,
	ColorTransfer_BT470BG,
	ColorTransfer_SMPTE170M,
	ColorTransfer_SMPTE240M,
	ColorTransfer_Linear,
	ColorTransfer_Log100,
	ColorTransfer_Log316,
	ColorTransfer_IEC6196624,
	ColorTransfer_BT1361,
	ColorTransfer_IEC6196621,
	ColorTransfer_BT202010,
	ColorTransfer_BT202012
};

enum ColorMatrix
{
	ColorMatrix_GBR = 0,
	ColorMatrix_BT709,
	ColorMatrix_Unspecified,
	ColorMatrix_BT470M = 4,
	ColorMatrix_BT470BG,
	ColorMatrix_SMPTE170M,
	ColorMatrix_SMPTE240M,
	ColorMatrix_YCgCo,
	ColorMatrix_BT2020NCL,
	ColorMatrix_BT2020CL
};

struct ColorDescription
{
	int fullRange;
	int primaries;
	int transfer;
	int matrix;
};

struct DataPacket
{
	LPBYTE lpPacket;
	UINT size;
};

struct FrameAudio
{
	List<BYTE> audioData;
	QWORD timestamp;
};

struct FrameProcessInfo
{
	x264_picture_t * pic;

	QWORD frameTimestamp;
	QWORD firstFrameTime;
	FrameProcessInfo()
	{
		pic = NULL;

		frameTimestamp = 0;
		firstFrameTime = 0;
	}
};

struct ServiceIdentifier
{
	int id;
	String file;

	ServiceIdentifier(int id, String file) : id(id), file(file) {}
	bool operator==(const ServiceIdentifier &sid) { return id == sid.id && file == sid.file; }
	bool operator!=(const ServiceIdentifier &sid) { return !(*this == sid); }
};


using namespace std;
class IBaseVideo;
class IBaseAudio;
class VideoEncoder;
class AudioEncoder;
class RTMPPublisherVectorBase;
class VideoFileStream;

enum PacketType
{
	PacketType_VideoDisposable,
	PacketType_VideoLow,
	PacketType_VideoHigh,
	PacketType_VideoHighest,
	PacketType_Audio,
	PacketType_Video_SEI,
	PacketType_Video_HEAD,
	PacketType_Audio_HEAD
};

struct VideoPacketData
{
	List<BYTE> data;
	PacketType type;

	inline void Clear() { data.Clear(); }
};

struct VideoSegment
{
	List<VideoPacketData> packets;
	DWORD timestamp;
	DWORD pts;

	inline VideoSegment() : timestamp(0), pts(0) {}
	inline ~VideoSegment() { Clear(); }
	inline void Clear()
	{
		for (UINT i = 0; i < packets.Num(); i++)
			packets[i].Clear();
		packets.Clear();
	}
};

typedef struct VideoStruct
{
	shared_ptr<IBaseVideo> VideoStream;
	IBaseAudio *AudioStream;//如果存在
	shared_ptr<IBaseVideo> VideoDevice;//增加真实摄像头
	shared_ptr<Value> Config;
	bool bSelect;
	bool bRender;
	bool bScale;
	Vect2 pos, size;
	Vect4 Crop;
	bool bGlobalStream;

	VideoStruct()
	{
		AudioStream = NULL;
		bSelect = false;
		bRender = true;
		bGlobalStream = false;
		bScale = true;
		pos = Vect2(0, 0);
		size = Vect2(640, 480);
		Crop = Vect4(0, 0, 0, 0);
	}
	VideoStruct(const VideoStruct & VStruct)
	{
		if (this != &VStruct)
		{
			VideoStream = VStruct.VideoStream;
			AudioStream = VStruct.AudioStream;
			VideoDevice = VStruct.VideoDevice;
			Config = VStruct.Config;
			bSelect = VStruct.bSelect;
			bRender = VStruct.bRender;
			bScale = VStruct.bScale;
			pos = VStruct.pos;
			size = VStruct.size;
			bGlobalStream = VStruct.bGlobalStream;
			Crop = VStruct.Crop;
		}
	}

	VideoStruct &operator = (const VideoStruct & VStruct)
	{
		if (this != &VStruct)
		{
			VideoStream = VStruct.VideoStream;
			AudioStream = VStruct.AudioStream;
			VideoDevice = VStruct.VideoDevice;
			Config = VStruct.Config;
			bSelect = VStruct.bSelect;
			bRender = VStruct.bRender;
			bScale = VStruct.bScale;
			pos = VStruct.pos;
			size = VStruct.size;
			bGlobalStream = VStruct.bGlobalStream;
			Crop = VStruct.Crop;
		}

		return *this;
	}

	Vect2 GetScale()
	{
		return VideoStream ? (VideoStream->GetSize() / size) : Vect2(1.0f, 1.0f);
	}

	Vect4 GetCrop()
	{
		Vect4 scaledCrop = Crop;
// 		Vect2 scale = GetScale();
// 		scaledCrop.x /= scale.x; scaledCrop.y /= scale.y;
// 		scaledCrop.z /= scale.y; scaledCrop.w /= scale.x;
		return scaledCrop;
	}

	~VideoStruct()
	{
		if (VideoStream)
			VideoStream.reset();
		if (Config)
			Config.reset();
		if (VideoDevice)
			VideoDevice.reset();
	}

}VideoStruct; 

typedef struct __FilterSturct
{
	Value JsonParam;
	shared_ptr<IIBaseFilter> _Filter;

	__FilterSturct()
	{
	}
	__FilterSturct(const __FilterSturct & VStruct)
	{
		if (this != &VStruct)
		{
			JsonParam = VStruct.JsonParam;
			_Filter = VStruct._Filter;
		}
	}

	__FilterSturct &operator = (const __FilterSturct & VStruct)
	{
		if (this != &VStruct)
		{
			JsonParam = VStruct.JsonParam;
			_Filter = VStruct._Filter;
		}

		return *this;
	}

	~__FilterSturct()
	{
		if (_Filter)
			_Filter.reset();
	}
}__FilterSturct;

typedef struct Filter
{
	vector<__FilterSturct> BaseFilter;
	IBaseVideo *IVideo;
	shared_ptr<Texture> RenderTarget;
	UINT Width;
	UINT Height;
	Filter()
	{
		IVideo = NULL;
		Width = 640;
		Height = 480;
		RenderTarget = NULL;
	}
	Filter(const Filter & VStruct)
	{
		if (this != &VStruct)
		{
			//std::copy_backward<vector<shared_ptr<IIBaseFilter>>::const_iterator, vector<shared_ptr<IIBaseFilter>>>(VStruct.BaseFilter.begin(), VStruct.BaseFilter.end(), BaseFilter);

			for (UINT i = 0; i < VStruct.BaseFilter.size(); ++i)
			{
				__FilterSturct Struct = VStruct.BaseFilter[i];
				BaseFilter.push_back(Struct);
			}
			IVideo = VStruct.IVideo;
			RenderTarget = VStruct.RenderTarget;
			Width = VStruct.Width;
			Height = VStruct.Height;
		}
	}

	Filter &operator = (const Filter & VStruct)
	{
		if (this != &VStruct)
		{
			for (UINT i = 0; i < VStruct.BaseFilter.size(); ++i)
			{
				__FilterSturct Struct = VStruct.BaseFilter[i];
				BaseFilter.push_back(Struct);
			}
			IVideo = VStruct.IVideo;
			RenderTarget = VStruct.RenderTarget;
			Width = VStruct.Width;
			Height = VStruct.Height;
		}

		return *this;
	}

	~Filter()
	{
		for (UINT i = 0; i < BaseFilter.size(); ++i)
		{
			__FilterSturct &OneFilter = BaseFilter[i];

			if (OneFilter._Filter)
				OneFilter._Filter.reset();
		}
		RenderTarget.reset();
	}

}Filter;

typedef struct AudioStruct
{
	IBaseVideo *VideoStream; //如果存在
	shared_ptr<IBaseAudio> AudioStream;
	shared_ptr<Value> Config;
	bool bSelect;
	bool bRender;
	bool bMustDel;
	bool bGlobalStream;
	AudioStruct()
	{
		AudioStream = NULL;
		VideoStream = NULL;
		bSelect = false;
		bRender = true;
		bMustDel = false;
		bGlobalStream = false;
	}

	AudioStruct(const AudioStruct & AStruct)
	{
		if (this != &AStruct)
		{
			VideoStream = AStruct.VideoStream;
			AudioStream = AStruct.AudioStream;
			Config = AStruct.Config;
			bSelect = AStruct.bSelect;
			bRender = AStruct.bRender;
			bMustDel = AStruct.bMustDel;
			bGlobalStream = AStruct.bGlobalStream;
		}
	}

	AudioStruct &operator = (const AudioStruct & AStruct)
	{
		if (this != &AStruct)
		{
			VideoStream = AStruct.VideoStream;
			AudioStream = AStruct.AudioStream;
			Config = AStruct.Config;
			bSelect = AStruct.bSelect;
			bRender = AStruct.bRender;
			bMustDel = AStruct.bMustDel;
			bGlobalStream = AStruct.bGlobalStream;
		}

		return *this;
	}

	~AudioStruct()
	{
		AudioStream.reset();
		Config.reset();
	}
}AudioStruct;

class CInstanceProcess : public CPObject
{
	friend class RTMPPublisher;
	friend class CSLiveManager;
	friend struct SharedDevice;
	friend class BlackMagic;

public:
	CInstanceProcess(const SLiveParam *Param);
	~CInstanceProcess();
	const char* GainClassName();
	void CreateStream(const Value &Jvalue, VideoArea *Area, uint64_t *StreamID1, uint64_t *StreamID2 = NULL);
	void AddVideoStream(VideoStruct& VideoStream);
	void SetHwnd(uint64_t hwnd);
	void SetParam(const SLiveParam *Param);

	void DeleteStream(uint64_t StreamId);
	void GetStreamInfo(uint64_t SteamId, Value &JValue);
	void GetStreamStatus(uint64_t iStreamID, DBOperation *Status);
	void SetStreamPlayPos(uint64_t iStreamID, UINT Pos);
	void GetStreamPlayPos(uint64_t iStreamID, UINT& Pos);
	void OperaterStream(uint64_t iStreamID, DBOperation OperType);
	void UpdateStream(uint64_t iStreamID, const char* cJsonParam);
	void UpdateStreamPosition(uint64_t iStreamID, VideoArea *Area, bool bScale);
	void MoveStream(uint64_t iStreamID, StreamMoveOperation Type);
	void MoveStream(StreamMoveOperation Type, int Index);
	void SetRenderStream(uint64_t iStreamID, bool bRender,const AudioStruct &_Audio);
	void SelectStream(uint64_t iStreamID, bool bSelect);
	void ConfigStream(uint64_t iStreamID, const char *cJson);
	void SetCrop(uint64_t iStreamID, float left, float top, float right, float bottom);
	string ReNameStream(uint64_t iStreamID, const char *NewName);
	void ReNameStreamSec(uint64_t iStreamID, const char *NewName);
	void GetStreamSize(uint64_t StreamID, UINT *Width, UINT *Height);
	void SetPlayPreAudio(uint64_t iStreamID, bool *bRet);
	void ChangeToBackup();
	void SetDelayCancel();
	Filter AddFilter(uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID);
	void   AddFilter(const Filter &NewFilter);
	void DeleteFilter(uint64_t iStreamID, uint64_t iFilterID);
	void UpdateFilter(uint64_t iStreamID, uint64_t iFilterID, Value &JValue);
	static void StreamCallBack(void *Context, CSampleData* Data);
	static void RecordCallBack(void *Context, CSampleData* Data);
	static DWORD VideoEncoderThread(LPVOID lparam);
	void VideoEncoderLoop();

	void StartRenderAStream(uint64_t iStreamID, const char* cRenderAudioDevice);
	void StopRenderAStream(uint64_t iStreamID);
	void SetAudioStreamDBCallBack(uint64_t iStreamID, AudioDBCallBack DBCallBack);
	void StartResize(bool bDragResize);
	void StopResize();
	void CreateLittleRenderTarget();

	void StartLive(bool bRecordOnly);
	void StopLive(bool bUI = true);
	void ClearVideo(bool bRemoveDelay = false,bool bCut = false);
	void ClearVideoTransForm();
	void ClearFilterTransForm();
	void ClearAudio();
	UINT GetRenderCount();

	void CopyNewToVideoList();
	void SetForceKillThread();

	void DrawPreview(Texture* Prev, const Vect2 &renderSize, const Vect2 &renderOffset, const Vect2 &renderCtrlSize, Shader *solidVertexShader, Shader *solidPixelShader, Texture* SDITexture, bool HasOutSDI,bool bStart);
	void DrawLittlePreview();
	void DrawPreProcess(float fSeconds);
	void DrawTransFormProcess(float fSeconds);
	void DrawRender(Texture *PreTexture, Shader *VertexShader, Shader *PixShader);
	void DrawTransFormRender(Texture *PreTexture, Shader *VertexShader, Shader *PixShader);
	void SetHasPreProcess(bool bPrePro);
	void ResizeRenderFrame(bool bRedrawRenderFrame);
	void BulidD3D();
	void BulidX264Encoder();
	void BulidEncoder();
	void BulidRtmpNetWork();
	void BulidFileStream();
	String GetOutputFilename(bool bBack = false);
	String GetOutputRecordFilename(const String &Path, const String &Name,int Width, int Height);
	void ProcessRecord(CSampleData *Data);
	void StopRecord();

	//处理函数
	void MixAudio(float *bufferDest, float *bufferSrc, UINT totalFloats, bool bForceMono);
	bool QueryNewAudio();
	bool QueryNewAudio(std::deque<CSampleData *> &listAudio);
	bool QueryNewVideo(std::deque<CSampleData *> &listVideo);
	void QueryAudioBuffers();
	void EncodeAudioSegment(float *buffer, UINT numFrames, QWORD timestamp);
	bool ProcessFrame(FrameProcessInfo &frameInfo);
	bool ProcessFrame_back(FrameProcessInfo &frameInfo);
	void SendFrame(VideoSegment &curSegment, QWORD firstFrameTime);
	void SendFrame_back(VideoSegment &curSegment, QWORD firstFrameTime);
	bool BufferVideoDataList(const List<DataPacket> &inputPackets, const List<PacketType> &inputTypes, QWORD timestamp, DWORD out_pts, QWORD firstFrameTime, std::list<std::shared_ptr<VideoSegment>>& segmentOuts);
	bool BufferVideoDataList_back(const List<DataPacket> &inputPackets, const List<PacketType> &inputTypes, QWORD timestamp, DWORD out_pts, QWORD firstFrameTime, std::list<std::shared_ptr<VideoSegment>>& segmentOuts);

	void GetVideoHeaders(DataPacket &packet);
	void GetVideoHeaders_back(DataPacket &packet);
	void GetAudioHeaders(DataPacket &packet);

	VideoEncoder* GetVideoEncoder() const { return videoEncoder; }
	AudioEncoder* GetAudioEncoder() const { return audioEncoder; }

	VideoEncoder* GetVideoEncoder_back() const { return videoEncoder_back; }
	char* CInstanceProcess::EncMetaData(char *enc, char *pend, bool bFLVFile, bool bBack);

	void GetOutputSize(UINT&Width, UINT &Height);
	void GetOutputSize_back(UINT&Width, UINT &Height);
	UINT GetFrameTime();

	void RequestKeyframe(int waitTime);
public:
	List<VideoStruct> m_VideoList;
	bool bLittlePre;
	bool   bNoPreView;
	SLiveParam LiveParam;
	std::vector<RTMPPublisher*> ListPublisher;
	CRITICAL_SECTION AudioSection;
	bool bStartLive;
	bool bLiveInstanceRecordOnly;
private:
	
	List<AudioStruct> m_AudioList;
	List<VideoStruct> m_VideoListTransForm;
	List<FrameAudio> pendingAudioFrames, pendingAudioFrames_back;
	List<Filter> m_Filter;
	List<Filter> m_FilterTransForm;

	CRITICAL_SECTION VideoSection;
	CRITICAL_SECTION SoundDataMutex,SoundDataMutex_back;
	CRITICAL_SECTION NetWorkSection, NetWorkSection_back;

	CircularList<QWORD> bufferedAudioTimes;
	List<VideoSegment> bufferedVideo;
	List<VideoSegment> bufferedVideo_back;
	CircularList<QWORD> bufferedTimes;
	CircularList<QWORD> bufferedTimes_back;

	
	bool bRunning;
	bool bResizeRenderView;
	HWND RenderHwnd;
	
	bool bReBulid;
	bool bReBulidAudio;
	bool bRecord;
	bool bCanRecord;
	bool bfirstTimeStamp;
	bool bFristAudioEncode;
	bool bOutPicDel;
	bool IsReject;

	//------
	bool IsLiveInstance;
	bool bForceKilled;
	//------

	UINT    baseCX, baseCY;
	UINT    scaleCX, scaleCY;
	UINT    outputCX, outputCY,outputCX_back,outputCY_back;
	ColorDescription colorDesc;

	Vect2 renderFrameSize;
	Vect2 renderFrameOffset;
	Vect2 renderFrameCtrlSize;

	//timestamp
	QWORD CurrentAudioTime;
	QWORD lastAudioTimestamp, lastAudioTimestamp_back;

	//编码
	VideoEncoder            *videoEncoder,*videoEncoder_back;
	AudioEncoder            *audioEncoder;
	x264_picture_t * volatile Outpic;

	bool bRequestKeyframe, bRequestKeyframe_back;
	int  keyframeWait;

	//RtmpPush
	RTMPPublisherVectorBase* network,*network_back;
	String PushURL0;
	String PushURL1;
	String PushURL0_back;
	String PushURL1_back;
	String PushPath0;
	String PushPath1;
	String PushPath0_back;
	String PushPath1_back;

	//SwapRender

	Texture *SwapRender;
	bool   bShutDown;
	ConfigCallBack ConfigCB;
	CMultimediaRender *MultiRender;
	bool bUseBackInstance;

	//录制文件
	std::unique_ptr<VideoFileStream> fileStream;
	std::unique_ptr<VideoFileStream> fileStream_back;
	UINT  RecordBitRate;
	String RecordPath;
	int  RecordWidth;
	int  RecordHeight;
	int  RecordFPS;
	HANDLE  m_hEncodeThread;
	int64_t m_firstFrameTimestamp;
	std::deque<CSampleData *> m_listARawData;
	std::deque<CSampleData *> m_listVRawData;
	HANDLE              m_hMutexRawA;
	HANDLE              m_hMutexRawV;
	bool                bHasAudio;
	D3DAPI              *D3DRender;
};

#endif