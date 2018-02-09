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


#pragma once

#include <memory>
#include<iostream>
#include<list>
#include<string>
#include"PipeProtocol.h"
#include"TThread.h"
#include <MMSystem.h>
#include <algorithm>
#include "PCMRecord.h"
#include "Pipe.h"
#include "AsynIoPub.h"
#include <deque>
#include "BaseAudio.h"
#include "BaseVideo.h"
#define MESSAGEBUFFER 1024*4
#define  BUFFERSIZE_VIDEO 1920*1080*3/2 + 1024*4*2
#define  BUFFERSIZE_AUDIO 2408*4          //pcm

struct ISampleData {
	
	unsigned char* lpData;
	bool bAudio;

	union
	{
		AudioDataInfo AInfo;
		VideoDataInfo VInfo;
	};

	volatile long refs;

	inline ISampleData() :lpData(NULL), bAudio(false){ refs = 1; }
	inline ~ISampleData()
	{
		if (lpData)
		{
			delete[] lpData;
			lpData = NULL;
		}

	} 

	inline void AddRef() { ++refs; }
	inline void Release()
	{
		if (!InterlockedDecrement(&refs))
		{
			delete this;
		}
	}
};

struct ConvertData
{
	LPBYTE input, output;
	ISampleData *sample;
	HANDLE hSignalConvert, hSignalComplete;
	bool   bKillThread;
	UINT   width, height;
	UINT   pitch;
	UINT   startY, endY;
	UINT   linePitch, lineShift;
};

struct AudioParam
{
	int bitsPerSample;
	int channels;
	int samplesPerSec;
};

struct ListParam
{
	ISampleData *pData;
	int len;
	long long TimeStamp;
};

class PipeVideo;
class AudioWaveOut;

struct AudioTimestamp {
	int count;
	int64_t pts;
};

class PipeAudioSource : public IBaseAudio
{
	DYNIC_DECLARE()
	PipeVideo *m_PipeVideo;
	int             LimitGetData;
	int             Times = 0;
	UINT sampleSegmentSize, sampleFrameCount;

	HANDLE hAudioMutex;
	List<BYTE> sampleBuffer;
	List<BYTE> outputBuffer;

	AudioParam Param;
	AudioWaveOut  *m_pAudioWaveOut;
	AudioWaveOut  *m_pSecWaveOut;
	UINT  inputSamplesPerSec;
	UINT  inputBitsPerSample;

	bool          bSameDevice;
	std::deque<AudioTimestamp> sampleBufferPts;
	bool m_isFirstDiffTimeWithAPI;
	QWORD m_lTimeDiffWithAPI;
	int m_iLastTimeStamp;
	int m_iBlockSize = 0;
	long long m_iLastPts;
	bool bLiveIntance;
	QWORD lastTimestamp;
	float fVolume;
protected:
	virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp);
	virtual void ReleaseBuffer();
	virtual CTSTR GetDeviceName() const;

public:
	bool Initialize(PipeVideo *parent, const AudioParam& param);
	const AudioParam& GetAudioParam();
	PipeAudioSource();
	~PipeAudioSource();
	void UpdateSettings(Value &JsonParam);
	void ReceiveAudio(LPBYTE lpData, UINT dataLength, long long pts,bool bCanPlay );
	void FlushSamples();

	void SetLiveInstance(bool bLiveIntance);
	virtual bool IsNeedRemove() const;
	virtual void OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor);
};

class Server;
class BitmapImage;

class PipeVideo : public IBaseVideo
{
	DYNIC_DECLARE()
	friend class Server;
	friend class CPipeServer;
	
	BitmapImage* bitmapImage; //在没有视频图像时用默认的图像代替视频
	Vect2 m_size; //图像渲染区域大小
	volatile unsigned int FileInfo;

    UINT enteredSceneCount;
    DeviceColorType colorType;

    UINT            renderCX, renderCY;
    UINT            newCX, newCY;
    UINT            linePitch, lineShift, lineSize;
	
    bool            bFirstFrame;
    bool            bUseThreadedConversion;
    bool            bReadyToDraw;
	bool bLoadSucceed;

	Texture         *texture;
    Value           data;
    UINT            texturePitch;
    bool            bCapturing;
    Shader          *colorConvertShader;
	Shader          *colorFieldConvertShader;
	Shader          *RGBFieldShader;
    Shader          *drawShader;

    HANDLE          hSampleMutex;
	HANDLE          hListMutex;
    ISampleData      *latestVideoSample;

    //---------------------------------
    LPBYTE          lpImageBuffer;
    ConvertData     *convertData;
    HANDLE          *hConvertThreads;
	HANDLE          hAudioThreads;
	//---------------------------------
	//AudioWaveOut  *m_pAudioWaveOut;
	PipeAudioSource* audioOut;
	AudioParam Param;
	bool AudioParamInit = false;
	HANDLE hAudioMutex;
	WAVEFORMATEX            audioFormat;
	//---------------------------------

	FILE*	fdumppcm;

    void ChangeSize(bool bSucceeded = true, bool bForce = false);
    void KillThreads();

    String ChooseShader(bool bNeedField = true);
	void   ChangeShader();

    void Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY);

    void FlushSamples()
    {
        OSEnterMutex(hSampleMutex);
        SafeRelease(latestVideoSample);
		std::for_each(ListSample.begin(), ListSample.end(), [](ListParam& data){ if (data.pData) data.pData->Release(); });
		ListSample.clear();
        OSLeaveMutex(hSampleMutex);

		OSEnterMutex(hAudioMutex);
		std::for_each(ListAudioSample.begin(), ListAudioSample.end(), [](ListParam& data){ if (data.pData) data.pData->Release(); });
		ListAudioSample.clear();
		OSLeaveMutex(hAudioMutex);
    }
	void ReceiveMediaSample(ISampleData *sample, bool bAudio);// IMediaSample *sample, bool bAudio);
	void ReceiveState(bool connected);
	
    bool LoadFilters();
    void UnloadFilters();

	static DWORD AudioThread(LPVOID Param);

	//bool SetParaToIneraction();

    void Start();
    void Stop();
	// 默认图片为证
	//char * m_pDefaultImgbuffer;
	char *m_pDefaultImgYUVbuffer;
	int   m_nDefaultImgSize;

	//=====================
	//int    m_iListIndex;
	AIOID m_aioId;
	bool b_flag;
	char name[24];
	String m_NickName;
	//=====================

	bool bHasPre;
	bool bInterlaceSignal;
	String strShaderOld;
	std::list<ListParam> ListSample;
	std::list<ListParam> ListAudioSample;
	bool bThreadRuning;

	D3DAPI *D3DRender;
	QWORD  m_qwrdAudioTime = 0;

public:
	bool Init(Value &data);
	PipeVideo();
    ~PipeVideo();
	const AudioParam& GetAudioParam() { return Param; }
	void ResetAudioParam(const AudioParam& param);
    void UpdateSettings(Value &data);
	void Tick(float fSeconds); //设置默认图片刷新
    void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC);
    void BeginScene();
    void EndScene();

	virtual void SetCanEnterScene(bool bCanEnter);
	virtual bool CanEnterScene() const;

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
	virtual bool IsFieldSignal() const;

    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();
	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();

	virtual void RegisterDataCallBack(void *Context, DataCallBack pCb);
	virtual void UnRegisterDataCallBack(void *Context);

	Vect2 GetSize() const 
	{ 
		return renderCX > renderCY ? Vect2(renderCX, renderCY) : Vect2(renderCY, renderCX);
	}
};

struct PipeAgent
{
	HANDLE hPipe;
	PipeAgent() :hPipe(NULL){}
	~PipeAgent()
	{
		if (hPipe)
		{
			CloseHandle(hPipe);
			hPipe = NULL;
		}
	}
};

#include<map>

typedef struct tagSetAudioMode
{
	MsgType type;

}StSetAudioMode;

typedef struct tagDataTransMode
{
	bool bflag = false;
	bool bFirst = true;
	bool bFirstSetAudioParam = true;
	bool bFirstSetVideoParam = true;
	volatile long refs = 0;
	bool bHasAPipeVideoRender = true;
	bool bFirstRefsMoreThan100Log = true;
	MsgType type;
	MsgType Datatype;
	AIOID id = -1;
	int recive_len = BUFFERSIZE_VIDEO;
	int send_len = BUFFERSIZE_VIDEO;
	char receive_buffer[BUFFERSIZE_VIDEO];
	char send_buffer[BUFFERSIZE_VIDEO];
	HANDLE mutex_lock;
	long LRefs = 0;
	void AddRef()
	{
		InterlockedIncrement(&LRefs);
	}
	void SubRef()
	{
		InterlockedDecrement(&LRefs);
	}
	//此系数为0时再删除
}StModerDataTransMode;

class CPipeServer : public IPipeControlCallback
{

private:
	CPipeServer();
	virtual ~CPipeServer();
	static CPipeServer* m_instance;

private:
	HANDLE timeThread = NULL;
	volatile bool bShutdownTickThread = false;
	static DWORD STDCALL TickThread(LPVOID lpUnused)
	{
		CPipeServer* _This = (CPipeServer*)lpUnused;
		_This->Tick();

		return 0;
	}
	void Tick();

private:

	PipeControl* m_pPipeControl;
	PCMRecord* DirectorAudioRecord;
	PCMRecord* HostAudioRecord;
	PCMRecord* ThirdAudioRecord;
	StSetAudioMode *m_pSetAudioDirectorMode;
	StSetAudioMode *m_pSetAudioHostMode;
	StSetAudioMode *m_pSetAudioThird;
	StModerDataTransMode* m_pAudioDirectorTransMode;
	StModerDataTransMode* m_pAudioHostTransMode;
	StModerDataTransMode* m_pVideoDirectorTransMode;
	StModerDataTransMode* m_pVideoHostTransMode;
	StModerDataTransMode* m_pAudioThirdTransMode;
	StModerDataTransMode* m_pVideoThirdTransMode;
	char m_cNickName[64];
	unsigned long SendSize;
	DeviceParam *oldDeviceParam;
	int m_nSamplesRate; //采样率
	int m_nAudioChannel;//声道数
	int m_nBitPerSample;//采样精度
	int m_iRequestNum;
	int m_nWithDirect = 0;
	int m_nHeightDirect = 0;
	int m_nWithHost = 0;
	int m_nHeightHost = 0;
	int m_nWithThird = 0;
	int m_nHeightThird = 0;
	bool m_bDirectorStop;
	bool m_bHostStop;
	bool m_bStop;
	bool m_bRestartInteractive = true;
	bool m_bFirstLogDirectVideo;        //每次开启摄像头都打印一次格式log
	bool m_bFirstLogDirectAudio;
	bool m_bFirstLogThirdAudio;
	bool m_bFirstLogThirdVideo;
	bool m_bFirstLogHostVideo;
	bool m_bFirstLogHostAudio;
	bool m_bFirstStart;
	bool m_bDirectorHostSameName = false;
	bool m_bDirectorThirdSameName = false;
	bool m_bHostThirdSameName = false;
	AIOID m_DirectorID;
	AIOID m_HostID;
	AIOID m_ThirdID;
	AIOID m_processID;
	std::wstring m_ProcessName;
	std::list<PipeVideo*> PipeRenders;
	std::map<AIOID, PipeVideo*> RenderMap;
	HANDLE          hPipeVideoMutex;
	HANDLE          m_hRegisterMutex;
	HANDLE          m_hDataStListMutex;
	HANDLE          m_RealeaseThread;
	HANDLE          m_RestartClientThread;
	HANDLE          HClientName;
	String          m_NabeNumber;
	String          InteractiveNumber;
	String          Appkey;
	String          NickName;
	String          strIneractionPara;
	String          exePath;
	std::list<AIOID> m_pModerDataTransModeList;      //删除所有场景时析构调用
	DWORD   m_th32ProcessID;
	DWORD   DirectTimeLast = 0;
	DWORD   DirectTimeCurrent = 0;
	DWORD   HostTimeLast = 0;
	DWORD   HostTimeCurrent = 0;
	DWORD   SendDirectTimeLast = 0;
	DWORD   SendDirectTimeCurrent = 0;
	DWORD   SentHostTimeLast = 0;
	DWORD   SentHostTimeCurrent = 0;
	bool m_bLogFlag = false;
	int  m_iCountFrame = 0;
	HANDLE m_hEvent = NULL; //用于释放线程
	HANDLE m_hEventTick = NULL;
	HANDLE m_hCloseRestartClientThreadEvent = NULL;
	HANDLE m_hCloseTickEvent = NULL;
	HANDLE m_hRegisterEvent = NULL; //用于等待反注册，禁止在注册未执行完成时，执行反注册
	long m_iLRefs = 0;

	SharedDevice DeviceDirector;
	SharedDevice DeviceHost;
	SharedDevice DeviceCenter;

	void AddRef()
	{
		InterlockedIncrement(&m_iLRefs);
	}
	void SubRef()
	{
		InterlockedDecrement(&m_iLRefs);
	}
	//此系数为0时再删除
protected:
	static DWORD STDCALL Thread_RUN(LPVOID lpUnused);

	void Exec();//接收线程处理函数

public:

	static CPipeServer* GetInstance();

	static void DestroyInstance();

	static void ReceiveMediaSample(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data);

	virtual int SetVideoData(CSampleData * data,
		unsigned long long nCtx);

	static void ReceiveVideoData(void *context, CSampleData * RGBdata);

	int Start(Value &data);

	void Stop();

	void Restart();

	bool CheckProcess( const WCHAR* CheckName);

	HANDLE GetProcessHandle(int nID);//通过进程ID获取进程句柄

	static DWORD STDCALL ReleaseThread(LPVOID lpUnused);

	static DWORD STDCALL RestartClientThread(LPVOID lpUnused);

	int AddVideoRender(PipeVideo*);

	void RemoveVideoRender(PipeVideo*);

	bool MonitorHeartBeat();

	void AudioProcess(const std::wstring&  DirectorAudioCapture, const std::wstring&  PresenterAudioCapture, const std::wstring&  ThirdAudioCapture);

	/*void ChangeSourceRegester(VideoCustomer* pCustomer, const String& OldDeviceName, const String& OldDeviceNameID, const String& NewDeviceName, const String& NewDeviceNameID,
		unsigned long long ctx1, unsigned long long ntx2);*/

	bool AnalyzeCommand(AIOID id, char *pcommand, int msglen, char * buf, int& len, StModerDataTransMode * pModerDataTransMode);

	bool AnalyzeHostModeCommand(AIOID id, char *pcommand, int msglen, StModerDataTransMode * pModerDataTransMode);

	int RegisterDevice(DeviceParam & oDeviceParam);

	int CheckDeviceByName(const String & DeviceName, const String & DeviceNameID,int flag);

	void ImgResizeYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeYV12(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeYUV422(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeRGB32(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeRGB32R(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeRGB24(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void RGB24ToYUV420(int Width, int Height, unsigned char* RgbBuffer, unsigned char*YuvBuffer);

	void RGB32toYUV420P(unsigned char *pSrcRGB32,unsigned char *pDstYUV420P,int width, int height);

	void ImgResizeUYVY(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeYUY2(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	virtual void on_pipe_accept(AsynIoErr st, const char *pipename, AIOID id, ULL64 ctx1, ULL64 ctx2);

	virtual void on_pipe_write(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

	virtual void on_pipe_read(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);
};
