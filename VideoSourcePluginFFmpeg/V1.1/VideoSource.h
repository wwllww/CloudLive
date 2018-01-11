/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "BaseAfx.h"
#include "MediaProcessSDK.h"
#include "VideoSourceConfigDialog.h"
#include "BaseVideo.h"
#include<list>
#include <mmreg.h>
#define CHROMA "RV32"

class CDemandMediaAudio;

enum MediaState { MediaUnknow, MediaInit, MediaPalying, MediaPause, MediaStop };

struct TimestampWithCount {
	int count;
	int64_t pts;
};

struct HintInfo {
	int Index;
	String FileName;
};

class VideoSource : public IBaseVideo
{
	DYNIC_DECLARE();
public:
	VideoSource();
    VideoSource(Value& data);
    ~VideoSource();

	virtual bool Init(Value &JsonParam);
	virtual bool GetStreamInfo(Value &JsonInfo);
	virtual bool OperaterStream(DBOperation OperType);

	virtual void SetHasSwitchSences(bool bSwitch);
	virtual void SwitchSences();
	virtual void SetCanEnterScene(bool bCanEnter);
	virtual bool CanEnterScene() const;
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
	volatile BOOL Captureing;
	MediaState m_MediaState;
	Shader          *colorConvertShader;
	Shader          *colorFieldConvertShader;
	Shader          *RGBFieldShader;
	DeviceColorType colorType;
    // be careful when accessing these
	UINT m_mediaDuration;
    Vect2 previousRenderSize;
    Vect2 mediaOffset;
    Vect2 mediaSize;
	std::list<CSampleData*> m_VideoYuvBuffer;
	CSampleData      *latestVideoSample;
	std::list<CSampleData*> m_AudioAACBuffer;
	CDemandMediaAudio *m_pDemandMediaAudio;
	bool  bCanResetAudio = true;
	BYTE *m_convertShader = NULL;
	//bool m_bGlobal;
	bool m_stop = false;
	int m_height;
	int m_width;
	int64_t m_pos;
	volatile int m_pts;
	volatile int m_ptsLast;
	int m_interval = 0;
	char m_choma[256];
	unsigned int m_FrameWidth;
	unsigned int m_FrameHeight;
	unsigned int m_FrameLines;
	unsigned int m_FramePitchs;
	char m_playPath[256];
	int Shaderpts = 0;
	int m_WidthChange = 0;
	bool m_bPlay = false;
	bool m_bclear = true;
	bool m_bFirstLoadFile = true;
	bool m_bFirstVideo = false;
	bool m_bNeedUpdate = false;
	bool m_bChangePos = false;
	int m_ChangePosPts = 0;
	bool m_bReadyDraw = false;
	int m_iVolume = 100;
	HANDLE m_hCloseSyncThreadEvent = NULL;
	bool m_bVframeisChange = false;
	int m_iLastChannel = 0;
	int m_iLastPerSecSamples = 0;
	int m_bFirstTsTimeStamp = -1;
	bool m_bisLastFile = false;
	int m_iFPS = 25;
	bool m_bisOpen = false;
	bool m_bisLastFileStop = false;
	int FileIndex = 0;
	bool m_bisAutoNextFile = true;
	bool m_bFileIsError = false;
	bool m_bScaleFull = false;
	int fileLoopPlayUseTsp = 0;
	bool bInitSuc = false;
	bool m_bSwitch = false;
	bool bHasPreProcess = false;
	bool bIsFieldSignal = true;
	String strShaderOld;

	CDeinterlacer *Deinterlacer = NULL;
	DeinterlacerConfig DeinterConfig;
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
public:
    VideoSourceConfig *config;
    CRITICAL_SECTION DataLock;
	CRITICAL_SECTION AudioDataLock;
	CRITICAL_SECTION TextureDataLock;
	CRITICAL_SECTION textureLock3s;
	CRITICAL_SECTION CallBackLock;
    unsigned int mediaWidth;
    unsigned int mediaHeight;
    unsigned int mediaWidthOffset;
    unsigned int mediaHeightOffset;

	UINT enteredSceneCount;        
public:
    Texture *GetTexture() { return texture; }
   // void ClearTexture();

public:
    // ImageSource
	HANDLE timeThread = NULL;
	volatile bool bShutdownTickThread = false;
	static DWORD STDCALL TickThread(LPVOID lpUnused)
	{
		VideoSource* _This = (VideoSource*)lpUnused;
		_This->YUV420_2_RGB32();

		return 0;
	}

	HANDLE m_Buffer3sThread = NULL;
	volatile bool bShutdownBuffer3sThread = false;
	static DWORD STDCALL Buffer3sThread(LPVOID lpUnused)
	{
		VideoSource* _This = (VideoSource*)lpUnused;
		_This->StoreBuffer3sThread();
		return 0;
	}

	//文件不支持提示框
	HANDLE m_hFileHint = NULL;
	static DWORD STDCALL FileHintThread(LPVOID lpUnused)
	{
		HintInfo* _This = (HintInfo*)lpUnused;
		CFileNotSupportHintDialog *m_pFileNotSupportHintDialog = new CFileNotSupportHintDialog;
		m_pFileNotSupportHintDialog->FileName = _This->FileName;
		m_pFileNotSupportHintDialog->index = _This->Index;
		m_pFileNotSupportHintDialog->Show();
		delete m_pFileNotSupportHintDialog;
		m_pFileNotSupportHintDialog = NULL;
		delete _This;
		_This = NULL;
		return 0;
	}


	int m_iConvertNum;
	char *m_pYUVData;
	void YUV420_2_RGB32();
	void StoreBuffer3sThread();
    void Tick(float fSeconds);
	void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC);

	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();

	void SetHasPreProcess(bool bPre);
	bool GetHasPreProcess() const;

    void GlobalSourceLeaveScene();
    void GlobalSourceEnterScene();

	virtual bool IsFieldSignal() const;
    
    void ChangeScene();
	void UpdateSettings(Value &JsonParam);
	bool ChangePos();
	bool ChangeStop();
	bool ChangePlay();
	bool ChangeReset();
	bool ChangeNext();
	bool ChangePrev();
	bool ChangeNext_API();
	void GetPlayList(StringList &);
	void SetDirectPlay(const String);
	int NeedChange();
	void OpenFileFailed();
    Vect2 GetSize() const;
	// 播放控制
	bool PauseMedia();
	// 设置进度,单位，毫秒
	virtual bool  SetStreamPos(UINT nPos);
	// 获取进度，单位，毫秒,-1表示获取进度失败
	virtual bool  GetStreamPos(UINT &nPos);
	int64_t GetMediaDuration();

	bool PlayMedia();
	bool StopMedia();
	bool ReSetMedia();
	bool PlayNextFile();
	bool PlayPrevFile();
	static void FrameCallBackFunc(void* frame, int frame_type, const void* ctx, int status);
	void ColseFile();
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
        return reinterpret_cast<VideoSource *>(*opaque)->VideoFormatCallback(chroma, width, height, pitches, lines); 
    }

    static void videoCleanupProxy(void *opaque)
    { 
        reinterpret_cast<VideoSource *>(opaque)->VideoFormatCleanup(); 
    };

    unsigned int VideoFormatCallback(char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines);
    void VideoFormatCleanup();
};
