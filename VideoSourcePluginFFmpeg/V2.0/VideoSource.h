/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "BaseAfx.h"
#include "MediaProcessSDK.h"
#include "VideoSourceConfigDialog.h"
#include "BaseVideo.h"
#include "concurrent_queue.h"
#include<list>
#include <mmreg.h>
#define CHROMA "RV32"

class CDemandMediaAudio;

enum MediaState { MediaUnknow, MediaInit, MediaPalying, MediaPause, MediaStop };
enum VideoMessageType { VideoUnknow, VideoReset, VideoPlay, VideoExit };

struct HintInfo {
	int Index;
	String FileName;
};

#define  PATH_MAX_ 1024
#define OUTTEX 0
class VideoSource : public IBaseVideo
{
	DYNIC_DECLARE();
public:
	VideoSource();
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
	HANDLE          m_hMediaProcess;
	MPMediaInfo     m_pMPMediaInfo;
    Vect2           m_vectVideoSize;
    Texture         *m_pTexture;
	volatile BOOL   Captureing;
	MediaState      m_MediaState;
	Shader          *m_pColorConvertShader;
	Shader          *m_pColorFieldConvertShader;
	Shader          *m_pRGBFieldShader;
	DeviceColorType m_ColorType;
	UINT			m_nMediaDuration;
    Vect2           m_vectPreviousRenderSize;
    Vect2           m_vectMediaOffset;
    Vect2           m_vectMediaSize;
	CSampleData       *m_pLatestVideoSample;
	CDemandMediaAudio *m_pDemandMediaAudio;
	unsigned int m_height;
	unsigned int m_width;
	int m_nfileLoopPlayUseTsp = 0;
	int m_nFirstTsTimeStamp = -1;
	int m_nFileIndex = 0;
	volatile int m_nPts = 0;
	volatile int m_nPtsLast = 0;
	unsigned int m_nFrameLines = 0;
	unsigned int m_nFramePitchs = 0;
	char m_playPath[PATH_MAX_];
	bool m_bReadyDraw = false;
	bool m_bisOpen = false;
	bool m_bisLastFileStop = false;
	bool m_bisAutoNextFile = true;
	bool m_bScaleFull = false;
	bool m_bSwitch = false;
	bool m_bHasPreProcess = false;
	bool m_bIsFieldSignal = true;
	String             m_strShaderOld;
	CDeinterlacer      *m_pDeinterlacer = NULL;
	DeinterlacerConfig m_DeinterConfig;
	D3DAPI             *m_pD3DRender;

	//SDK使用变量
	char              m_pPlayParam[PATH_MAX_];

	//同步使用变量
	MPFrameInfo       *m_pCurrentMPFrameInfo = NULL;
	MPFrameInfo       *m_pLastMPFrameInfo = NULL;
	QWORD             m_nSyncTimeStart = 0;
	bool              m_bFirstFrame = false;                                  //渲染第一帧
	bool              m_bExit = false;
	bool              m_bSeekFlag = false;                                     //当seek后，有些文件长时间单独返回音频或视频，以此标记处理
	bool			  m_bSeekClearVideoLastFrame = true;
	bool			  m_bSeekClearVideoLastFrameSecond = false;
	CSampleData       *m_pCurrentVideoSample = NULL;
	CSampleData       *m_pCurrentAudioSample = NULL;
	HANDLE            m_hPauseSyncThreadEvent = NULL;                          //需要挂起同步线程
	bool              m_bFileIsError = false;                                  //文件打开错误
	tbb::concurrent_bounded_queue<VideoMessageType> m_pMessageQueue;                  //消息处理队列
	DWORD opacity255 = 255;

public:
	void AudioPlayBack(const void *samples, unsigned int count, int64_t pts);
	void InitAudioCSampleData();
	void DestoryAuidoCSampleData();
	void InitCSampleData();
	void DestoryCSampleData();
private:
	CSampleData* m_pAudioSample;
	WAVEFORMATEX  m_wavAudioFormat;
public:
    VideoSourceConfig *m_pConfig;
    CRITICAL_SECTION m_secDataLock;
	CRITICAL_SECTION m_secAudioDataLock;
	CRITICAL_SECTION m_secTextureDataLock;
	CRITICAL_SECTION m_secCallBackLock;
    unsigned int m_nMediaWidth;
    unsigned int m_nMediaHeight;
    unsigned int m_nMediaWidthOffset;
    unsigned int m_nMediaHeightOffset;

	UINT m_nEnteredSceneCount;        
public:
    Texture *GetTexture() { return m_pTexture; }

public:
    // ImageSource
	HANDLE m_hSyncThread = NULL;
	static DWORD STDCALL SyncThread(LPVOID lpUnused)
	{
		VideoSource* _This = (VideoSource*)lpUnused;
		_This->Synchronization();
		return 0;
	}

	HANDLE m_hMsgProcessThread = NULL;
	static DWORD STDCALL MsgProcessThread(LPVOID lpUnused)
	{
		VideoSource* _This = (VideoSource*)lpUnused;
		_This->MessageLoop();
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

	void Synchronization();
	void Monitor();
	void MessageLoop();
	void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC);

	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();

	void SetHasPreProcess(bool bPre);
	bool GetHasPreProcess() const;

    void GlobalSourceLeaveScene();
    void GlobalSourceEnterScene();

	virtual void SetOpacity(DWORD Opacity);

	virtual bool IsFieldSignal() const;
	Vect2 GetSize() const;

    void ChangeScene();
	void UpdateSettings(Value &JsonParam);
	bool ChangePos();
	bool ChangePlay();
	bool ChangeReset();
	bool ChangeNext(bool bLoop = true);
	bool ChangePrev();
	bool ChangeNextUI();
	void GetPlayList(StringList &);
	int NeedChange();
	void OpenFileFailed();
	bool PauseMedia();
	virtual bool  SetStreamPos(UINT nPos);
	virtual bool  GetStreamPos(UINT &nPos);
	int64_t GetMediaDuration();

	bool PlayMedia();
	bool ReSetMedia();
	bool PlayNextFile();
	bool PlayPrevFile();
	void ColseFile();
	bool OpenStartFile();
	unsigned int VideoFormatCallback( unsigned *width, unsigned *height);

	//同步操作犀利函数
	bool InitQPCTimer();
	void SyncVideoFrame();
	void GetVideoFrame();
	void SyncAudioFrame();
	void GetAudioFrame();
    
};
