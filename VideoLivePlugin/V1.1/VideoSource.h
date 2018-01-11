/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "BaseAfx.h"
#include "MediaProcessSDK.h"
#include "VideoSourceConfigDialog.h"
#include "BaseVideo.h"
#include <mmreg.h>
#include <deque>
#include<list>
#include "concurrent_queue.h"
#define CHROMA "RV32"

class AudioOutputStreamHandler;
enum URLStreamStatus{
	STOP_STREAM,
	H264_AAC_STREAM,
	PLAY_STREAM,
	PCM_STREAM,
	YUV_STREAM,
	UNINIT,
};
struct TimestampWithCount {
	int count;
	int64_t pts;
};
class CDemandMediaAudio;
class VideoLiveSource :  public IBaseVideo
{
	DYNIC_DECLARE();
public:
	VideoLiveSource();
	VideoLiveSource(Value& data);
	~VideoLiveSource();
	virtual bool Init(Value &JsonParam);
	//virtual bool GetStreamInfo(Value &JsonInfo);
	//virtual bool OperaterStream(DBOperation OperType);
	virtual void SetCanEnterScene(bool bCanEnter);
	virtual bool CanEnterScene() const;
	//virtual void SetHasSwitchSences(bool bSwitch);
	//virtual void SwitchSences();
	virtual void RegisterDataCallBack(void *Context, DataCallBack pCb);
	virtual void UnRegisterDataCallBack(void *Context);
	void BeginScene();
	void EndScene();
	String ChooseShader(bool bNeedField = true);
	void   ChangeShader();
	void   PlayCallBackAudio(LPBYTE lpData, UINT len);
	
private:
	HANDLE HMediaProcess;
	MPMediaInfo m_pMPMediaInfo;
    Vect2 videoSize;
    Texture *texture;
	unsigned int CallBackWidth;
	unsigned int CallBackHight;
	bool Captureing;
	Shader          *colorConvertShader;
	Shader          *colorFieldConvertShader;
	Shader          *RGBFieldShader;
	DeviceColorType colorType;
	QWORD m_mediaDuration;
    Vect2 previousRenderSize;
    Vect2 mediaOffset;
    Vect2 mediaSize;
	std::list<CSampleData*> m_VideoYuvBuffer;
	CSampleData      *latestVideoSample;
	std::list<CSampleData*> m_AudioAACBuffer;
	CDemandMediaAudio *m_pDemandMediaAudio;
	tbb::concurrent_queue<CSampleData*> m_YUVBuffer;
	tbb::concurrent_queue<CSampleData*> m_PCMuffer;
	BYTE *m_convertShader = NULL;
	//bool m_bGlobal;
	bool m_stop = false;
	int m_height;
	int m_width;
	int m_nWarnTime;
	int m_nBufferTime;
	int64_t m_pos;
	volatile int m_pts;
	volatile int m_ptsLast;
	int m_interval = 0;
	char m_choma[256];
	unsigned int m_FrameWidth;
	unsigned int m_FrameHeight;
	unsigned int m_FrameLines;
	unsigned int m_FramePitchs;
	char *m_playPath;
	int Shaderpts = 0;
	bool m_bclear = true;
	bool m_bReadyDraw = false;
	bool m_bReconnect = false;
	int m_iReconnectPts = 0;
	int m_iLastChannel = 0;
	int m_iLastPerSecSamples = 0;
	int m_iVolume = 100;
	DWORD m_uCurrentTime = 0;
	DWORD m_uLastTime = 0;
	HANDLE m_hUpdataEvent = NULL; //用于更新设置
	HANDLE m_hAllowCloseEvent = NULL; //用于关闭文件设置
	HANDLE m_hPauseThreadEvent = NULL; //用于关闭文件设置
	HANDLE m_hCloseMoinitorThreadEvent = NULL; //Release Monitor Thread
	HANDLE m_hH264AACBufferEvent = NULL;
	HANDLE m_hYUVBufferEvent = NULL;
	HANDLE m_hPCMBufferEvent = NULL;
	HANDLE m_hRenderSleepEvent = NULL;
	volatile int URLState = 0;
	bool m_bisReBuffer = true;
	bool m_bVframeisChange = false;
	bool m_bTotalBufferPlay = false;
	bool m_bisReadyVideoPlay = false;
	bool m_bisReadyAudioPlay = false;
	unsigned int  m_fVideoWarnning = 0.0;
	unsigned int  m_fAudioWarnning = 0.0;
	int m_iFPS = 0;
	URLStreamStatus m_StatusStream;
	unsigned int m_iWaitTimeOut;
	unsigned int m_iPCMCount = 0;
	unsigned int m_iYUVCount = 0;
	bool m_bisForceOpenURL = false;
	int m_iLastBufferTime = -1;
	int m_iPcmBufferNumber = 0;
	int m_iLastVideoFrameTimestamp = 0;
	int m_iYUVBufferTotalTime = 0;
	int m_iYUVBufferCurTime = 0;
	CSampleData* Last_inf_Audio = NULL;
	CSampleData* Last_inf_Video = NULL;
	bool m_bScaleFull = false;
	long long m_DiffQpcTime = 0;
	QWORD m_FirstQpcTime = 0;
	QWORD m_CurrentQpcTime = 0;
	long long m_DiffVideoFrameTime = 0;
	QWORD m_FirstVideoFrameTime = 0;
	QWORD m_CurrentVideoFrameTime = 0;
	bool m_bResetDiffMode = false;
	bool m_bAllowClose = true;
	bool bHasPreProcess = false;
	bool bIsFieldSignal = false;
	String strShaderOld;
	D3DAPI *D3DRender;
public:
	void AudioPlayBack(const void *samples, unsigned int count, int64_t pts);
	void InitAudioCSampleData();
	void DestoryAuidoCSampleData();
	void InitCSampleData();
	void DestoryCSampleData();
private:
	CSampleData* audioSample;
	WAVEFORMATEX  audioFormat;
    
public:
    bool isRenderingWithoutRef;
    volatile bool isInScene;
    bool hasSetAsyncProperties;

public:
    VideoSourceConfig *config;
	CRITICAL_SECTION DataLock;
	CRITICAL_SECTION TextureDataLock;
	CRITICAL_SECTION CallBackLock;
    unsigned int mediaWidth;
    unsigned int mediaHeight;
    unsigned int mediaWidthOffset;
    unsigned int mediaHeightOffset;

    bool isRendering;
	bool m_bTextureNeedClear;
	bool m_bNeedSetImage;
    int remainingVideos;
	UINT enteredSceneCount;        
public:
    Texture *GetTexture() { return texture; }
    void ClearTexture();

public:
    // ImageSource
	HANDLE m_SyncThread = NULL;
	static DWORD STDCALL SynchronizationThread(LPVOID lpUnused)
	{
		VideoLiveSource* _This = (VideoLiveSource*)lpUnused;
		_This->Synchronization();

		return 0;
	}

	HANDLE m_MonitorThread = NULL;
	DWORD m_MonitorThreadID = 0;
	static DWORD STDCALL MonitorThread(LPVOID lpUnused)
	{
		VideoLiveSource* _This = (VideoLiveSource*)lpUnused;
		_This->Monitor();
		return 0;
	}

	int m_iConvertNum;
	char *m_pYUVData;
	void Synchronization();
	void Monitor();
    void Tick(float fSeconds);
	void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC);
	void SetHasPreProcess(bool bPre);
	bool GetHasPreProcess() const;
    void GlobalSourceLeaveScene();
    void GlobalSourceEnterScene();

	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();
	virtual bool IsFieldSignal() const;
    void ChangeScene();
	void UpdateSettings(Value &JsonParam);
	bool Reconnect();
	void StartMonitor();
	void StopMonitor();
	void UpdateSet();
    Vect2 GetSize() const;
	virtual bool GetStreamPos(UINT& Pos);

	static void FrameCallBackFunc(void* frame, int frame_type, const void* ctx, int status);
	static void SetParamCallBackFunc(int , void* ctx);
public:
    // Vlc

    static unsigned videoFormatProxy(
        void **opaque, 
        char *chroma,
        unsigned *width, 
        unsigned *height,
        unsigned *pitches, 
        unsigned *lines)
    { 
		return reinterpret_cast<VideoLiveSource *>(*opaque)->VideoFormatCallback(chroma, width, height, pitches, lines);
    }

    static void videoCleanupProxy(void *opaque)
    { 
		reinterpret_cast<VideoLiveSource *>(opaque)->VideoFormatCleanup();
    };

    unsigned int VideoFormatCallback(char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines);
    void VideoFormatCleanup();
};