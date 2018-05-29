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
#include "BaseAfx.h"
#include "BaseAudio.h"
#include "BaseVideo.h"
#include <memory>

struct ConvertData
{
    LPBYTE input, output;
    CSampleData *sample;
    HANDLE hSignalConvert, hSignalComplete;
    bool   bKillThread;
    UINT   width, height;
    UINT   pitch;
    UINT   startY, endY;
    UINT   linePitch, lineShift;
};

class DeviceSource;
class AudioWaveOut;

class DeviceAudioSource : public IBaseAudio
{
	DYNIC_DECLARE();
    DeviceSource *device;

    UINT sampleSegmentSize, sampleFrameCount;

    HANDLE hAudioMutex;
    List<BYTE> sampleBuffer;
    List<BYTE> outputBuffer;
	QWORD      m_timeStart;
	bool       m_bErrorLog;
    int offset;
	FILE*	audiosource;
	AudioWaveOut  *m_pAudioWaveOut;
	UINT  inputBlockSize;
	AudioWaveOut  *m_pSecWaveOut;
	bool          bSameDevice;
	QWORD         lastTimestamp;
	bool  bLiveInstance;
protected:
    virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp);
    virtual void ReleaseBuffer();

    virtual CTSTR GetDeviceName() const;

public:
	DeviceAudioSource();
    bool Initialize(DeviceSource *parent);
    ~DeviceAudioSource();

    void ReceiveAudio(LPBYTE lpData, UINT dataLength,float volume);

    void FlushSamples();

    inline void SetAudioOffset(int offset) {this->offset = offset; SetTimeOffset(offset);}
	virtual void SetLiveInstance(bool bLiveInstance);
	virtual bool IsNeedRemove() const;
	virtual void OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor);
	//void SetGlobalTag(bool g)
	//{
	//	GlobalAudo = g;
	//}
};

//class Transform;
class DeviceSource : public IBaseVideo
{
	DYNIC_DECLARE()
    friend class DeviceAudioSource;
    friend class CapturePin;

    IGraphBuilder           *graph;
    ICaptureGraphBuilder2   *capture;
    IMediaControl           *control;
	IMediaEventEx           *m_pEvent;
    IBaseFilter             *deviceFilter;
    IBaseFilter             *audioDeviceFilter;
    CaptureFilter           *captureFilter;
    IBaseFilter             *audioFilter; // Audio renderer filter

    //---------------------------------
    WAVEFORMATEX            audioFormat;
    DeviceAudioSource       *audioOut;
	HANDLE hAudioMutex;

    bool bRequestVolume;
    float fNewVol;

    UINT enteredSceneCount;

    //---------------------------------

    DeviceColorType colorType;
	DeviceColorType oldType;

    String          strDevice, strDeviceName, strDeviceID;
	String          strAudioDevice, strAudioName, strAudioID, strAudioRenderName, strAudioRenderID;
    bool            bFlipVertical, bFlipHorizontal, bDeviceHasAudio, bUsePointFiltering, bUseAudioRender;
    UINT64          frameInterval;
    UINT            renderCX, renderCY;
    UINT            newCX, newCY;
    UINT            imageCX, imageCY;
    UINT            linePitch, lineShift, lineSize;
    BOOL            bUseCustomResolution;
    int            preferredOutputType;
	//void            *m_pWaveOut;
	//String m_strReanderName; //外放设备名称
    struct {
        int                         type; //DeinterlacingType
        char                        fieldOrder; //DeinterlacingFieldOrder
        char                        processor; //DeinterlacingProcessor
        bool                        curField, bNewFrame;
        bool                        doublesFramerate;
        bool                        needsPreviousFrame;
        bool                        isReady;
        std::unique_ptr<Texture>    texture;
        UINT                        imageCX, imageCY;
        std::unique_ptr<Shader>     vertexShader;
        FuturePixelShader           pixelShaderAsync;
		std::unique_ptr<Shader>     pixelShader;
    } deinterlacer;

	int oldDeinterlacerType;

    bool            bFirstFrame;
    bool            bUseThreadedConversion;
    bool            bReadyToDraw;

    int             soundOutputType;
    bool            bOutputAudioToDesktop;

    Texture         *texture, *previousTexture;
    Value           data;
    UINT            texturePitch;
	volatile bool            bCapturing;
	bool bFiltersLoaded;
    Shader          *colorConvertShader;
	Shader          *colorFieldConvertShader;
	Shader          *RGBFieldShader;
    Shader          *drawShader;

    bool            bUseBuffering;
    HANDLE          hStopSampleEvent;
    HANDLE          hSampleMutex;
    HANDLE          hSampleThread;
	HANDLE          colorMutex;
	HANDLE          ListCallBackMutex;
    UINT            bufferTime;
    CSampleData      *latestVideoSample;
    List<CSampleData*> samples;

    UINT            opacity;
    int             gamma;

    //---------------------------------

    LPBYTE          lpImageBuffer;
    ConvertData     *convertData;
    HANDLE          *hConvertThreads;
	HANDLE          hThreadCheckDevice;
	volatile  bool            m_bCheckExit;

    //---------------------------------

    bool            bUseChromaKey;
    DWORD           keyColor;
    Color4          keyChroma;
    Color4          keyBaseColor;
    int             keySimilarity;
    int             keyBlend;
    int             keySpillReduction;
	bool bLoadSucceed;
	bool bHasPre;
	bool bIsFieldSignal;
    //---------------------------------

	FILE*	fdumppcm;

    void ChangeSize(bool bSucceeded = true, bool bForce = false);
    void KillThreads();

    String ChooseShader(bool bNeedField = true);
	void   ChangeShader();
    String ChooseDeinterlacingShader();

    void Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY);

    void FlushSamples()
    {
        OSEnterMutex(hSampleMutex);
        for (UINT i=0; i<samples.Num(); i++)
            delete samples[i];
        samples.Clear();
        SafeRelease(latestVideoSample);
        OSLeaveMutex(hSampleMutex);
    }

    void SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType);

    UINT GetSampleInsertIndex(LONGLONG timestamp);
    void ReceiveMediaSample(IMediaSample *sample, bool bAudio);

    bool LoadFilters();
    void UnloadFilters();

    void Start();
    void Stop();

    static DWORD WINAPI SampleThread(DeviceSource *source);

	// 重启设备
	bool ReStartCaptrue();

	// 是否坚持音频设备
	bool m_bCheckAudioDevice;

	// 上次音频来的时间
	QWORD m_qwrdAudioTime;

	// 上次设备重启时间
	QWORD m_qwrdDeviceResetTime;

	HANDLE  m_hCheckExitEvent;

	String   m_strLastError;

	String  strShaderOld;

	D3DAPI  *D3DRender;
	
	bool    bNeedCheck;
	void* operator new(size_t size){
		void* ptr = _aligned_malloc(size, 16);
		return ptr;
	}

	void* operator new[](size_t size) {
		return operator new(size);
	}

	void operator delete(void* ptr) {
		if (ptr)
		{
			_aligned_free(ptr);
		}
	}

	void operator delete[](void* ptr) {
		if (ptr)
			operator delete(ptr);
	}

public:
	bool Init(Value &data);
	DeviceSource();
    ~DeviceSource();

	void UpdateSettings(Value &data);

    void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC);

    void BeginScene();
    void EndScene();

    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();

    void SetInt(CTSTR lpName, int iVal);
    void SetFloat(CTSTR lpName, float fValue);

    Vect2 GetSize() const {return Vect2(float(newCX), float(newCY));}

	DWORD CheckDevice();

	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();
	virtual bool IsFieldSignal() const;
	virtual void RenameSource(const char *NewName);

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
	void RegisterDataCallBack(void *Context, DataCallBack pCb);
	void UnRegisterDataCallBack(void *Context);
	void SetCanEnterScene(bool bCanEnter);
	bool CanEnterScene() const;
	virtual const char* GetDeviceName() const;
	virtual const char* GetDeviceID() const;
};

