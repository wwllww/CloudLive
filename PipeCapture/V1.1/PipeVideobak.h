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
#include "AudioWaveOut.h"
#include "VirtualCameraCommand.h"
#include "PCMRecord.h"
#include "Pipe1.1.h"
#include "AsynIoPub.h"
#define MESSAGEBUFFER 1024*4
void PackPlanar(LPBYTE convertBuffer, LPBYTE lpPlanar, UINT renderCX, UINT renderCY, UINT pitch, UINT startY, UINT endY, UINT linePitch, UINT lineShift);

enum DeviceColorType
{
    DeviceOutputType_RGB,

    //planar 4:2:0
    DeviceOutputType_I420,
    DeviceOutputType_YV12,

    //packed 4:2:2
    DeviceOutputType_YVYU,
    DeviceOutputType_YUY2,
    DeviceOutputType_UYVY,
    DeviceOutputType_HDYC,

	//RGB24
	DeviceOutputType_RGB24,
};

struct ISampleData {
	
	unsigned char* lpData;
	bool bAudio;

	union
	{
		AudioDataInfo* AInfo;
		VideoDataInfo* VInfo;
	};

	volatile long refs;

	inline ISampleData() :lpData(NULL), bAudio(false), AInfo(NULL), VInfo(NULL) { refs = 1; }
	inline ~ISampleData()
	{
		if (bAudio)
		{
			if (AInfo)
			{
				delete AInfo;
				AInfo = NULL;
			}
		}
		else
		{
			if (VInfo)
			{
				delete VInfo;
				VInfo = NULL;
			}
		}

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


class PipeVideo;

class PipeAudioSource : public AudioSource
{
	PipeVideo *m_PipeVideo;

	UINT sampleSegmentSize, sampleFrameCount;

	HANDLE hAudioMutex;
	List<BYTE> sampleBuffer;
	List<BYTE> outputBuffer;
	
protected:
	virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp);
	virtual void ReleaseBuffer();
	virtual CTSTR GetDeviceName() const;

public:
	bool Initialize(PipeVideo *parent);
	PipeAudioSource(){}
	~PipeAudioSource();

	void ReceiveAudio(LPBYTE lpData, UINT dataLength);
	void FlushSamples();
};

class Server;
class BitmapImage;

class PipeVideo : public ImageSource
{
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
    XElement        *data;
    UINT            texturePitch;
    bool            bCapturing;
    Shader          *colorConvertShader;
    Shader          *drawShader;

    HANDLE          hSampleMutex;
	//Server          *m_Server;
	VirtualCameraControl  m_cmdSend;
    ISampleData      *latestVideoSample;

    //---------------------------------
    LPBYTE          lpImageBuffer;
    ConvertData     *convertData;
    HANDLE          *hConvertThreads;
	//---------------------------------
	AudioWaveOut  *m_pAudioWaveOut;
	PipeAudioSource* audioOut;
	HANDLE hAudioMutex;
	WAVEFORMATEX            audioFormat;
	//---------------------------------

	FILE*	fdumppcm;

    void ChangeSize(bool bSucceeded = true, bool bForce = false);
    void KillThreads();

    String ChooseShader();

    void Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY);

    void FlushSamples()
    {
        OSEnterMutex(hSampleMutex);
        SafeRelease(latestVideoSample);
        OSLeaveMutex(hSampleMutex);
    }
	void ReceiveMediaSample(ISampleData *sample, bool bAudio);// IMediaSample *sample, bool bAudio);
	void ReceiveState(bool connected);
	
    bool LoadFilters();
    void UnloadFilters();

	bool SetParaToIneraction();

    void Start();
    void Stop();
	// 默认图片为证
	char * m_pDefaultImgbuffer;
	int   m_nDefaultImgSize;

	//=====================
	//int    m_iListIndex;
	AIOID m_aioId;
	bool b_flag;
	char name[24];
	//=====================
public:
    bool Init(XElement *data);
	PipeVideo();
    ~PipeVideo();

    void UpdateSettings();
	void Tick(float fSeconds); //设置默认图片刷新
    void Preprocess();
    void Render(const Vect2 &pos, const Vect2 &size);

    void BeginScene();
    void EndScene();

    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();

	Vect2 GetSize() const 
	{ 
		int cx = data->GetParent()->GetFloat(TEXT("cx"), 32.0f);
		int cy = data->GetParent()->GetFloat(TEXT("cy"), 32.0f);
		
		return Vect2(cx, cy);
	}

protected:
	void AddCreateInfo(String& info) 
	{
		bool find = false;
		int num = CreateInfo.Num();
		for (int i = 0; i < num; ++i)
		{
			if (CreateInfo[i] == info)
			{
				find = true;
			}
		}

		if (find == false)
		{
			CreateInfo.Add(info);
		}
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
struct VideoContext
{
	VideoContext()
	{
		refs = 0;
		pSampleData = NULL;
		//Buffer = NULL;
	}
	inline void AddRef() { ++refs; }
	inline void Release()
	{
		if (!InterlockedDecrement(&refs))
		{
			Buffer.Clear();
			pSampleData = NULL;
			delete this;
		}
	}
	volatile long refs;
	CSampleData* pSampleData;
	List<char> Buffer;
};

struct AIOIDContext
{
	MsgType type;
};

//struct HostModerUsing
//{
//	bool b_flag;
//	char name[24];
//};


class CPipeServer : public IPipeControlCallback, public  VideoCustomer
{

private:
	CPipeServer();
	virtual ~CPipeServer();
	static CPipeServer* instance;

private:
	FILE *fp;
	String    m_strInteractionClientPara;
	PipeControl* m_pPipeControl;
	std::wstring PipeClientName;
	AsynIoErr st;
	PCMRecord* DirectorAudioRecord;
	PCMRecord* HostAudioRecord;
	unsigned long SendSize;
	HANDLE m_thread;
	int m_iRequestNum; //暂缓处理。。。。
	DeviceParam *oldDeviceParam;

	int m_nWith;
	int m_nHeight;

	std::list<AIOID> DirectorList;
	std::list<AIOID> PresenterList;
	std::list<PipeVideo*> PipeRenders;
	std::map<AIOID, PipeVideo*> RenderMap;
	//HostModerUsing *VideoRender;
	HANDLE          hPipeVideoMutex;
	HANDLE          hPresenterVideoMutex;
	HANDLE          hDirectorVideoMutex;
	HANDLE          hAIOIDMutex;
	bool  blive;
	bool  bStop;
	AIOID bliveId;
protected:
	static DWORD STDCALL Thread_RUN(LPVOID lpUnused);

	void Exec();//接收线程处理函数

public:
	static CPipeServer* GetInstance();

	static void ReceiveMediaSample(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data);

	virtual int SetVideoData(CSampleData * data,
		unsigned long long nCtx);

	void Start();

	void Stop();

	int AddVideoRender(PipeVideo*);

	void RemoveVideoRender(PipeVideo*);

	bool MonitorHeartBeat();

	void AudioProcess(std::wstring  DirectorAudioCapture, std::wstring  PresenterAudioCapture);

	void ChangeSourceRegester(VideoCustomer* pCustomer, const String& OldDeviceName, const String& OldDeviceNameID, const String& NewDeviceName, const String& NewDeviceNameID,
		unsigned long long ctx1, unsigned long long ntx2);

	bool AnalyzeCommand(AIOID id, char *pcommand, int msglen, char * buf, int& len, ULL64 ctx1, ULL64 ctx2);

	int RegisterDevice(DeviceParam & oDeviceParam);

	int CheckDeviceByName(const String & DeviceName, const String & DeviceNameID,int flag);

	void ImgResizeYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void ImgResizeRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH);

	void RGB24ToYUV420(int Width, int Height, unsigned char* RgbBuffer, unsigned char*YuvBuffer);

	void RGB32toYUV420P(unsigned char *pSrcRGB32,unsigned char *pDstYUV420P,int width, int height);

	virtual void on_pipe_accept(AsynIoErr st, const char *pipename, AIOID id, ULL64 ctx1, ULL64 ctx2);

	virtual void on_pipe_write(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

	virtual void on_pipe_read(AsynIoErr st, AIOID id, const char *pipename, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);
};
