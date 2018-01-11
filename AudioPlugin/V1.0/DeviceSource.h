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
#include <memory>
#include "BaseAudio.h"
#include "BaseVideo.h"

class DSource;
class AudioWaveOut;

class DAudioSource : public IBaseAudio
{
	DYNIC_DECLARE();
    DSource *device;

    UINT sampleSegmentSize, sampleFrameCount;

    HANDLE hAudioMutex;
    List<BYTE> sampleBuffer;
    List<BYTE> outputBuffer;

    int offset;
	UINT  inputBlockSize;
	AudioWaveOut *m_pAudioWaveOut;
	AudioWaveOut  *m_pSecWaveOut;
	bool          bSameDevice;
	QWORD         lastTimestamp;
	bool          bLiveInstance;

protected:
    virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp);
    virtual void ReleaseBuffer();

    virtual CTSTR GetDeviceName() const;

public:
	DAudioSource();
    bool Initialize(DSource *parent);
	~DAudioSource();

    void ReceiveAudio(LPBYTE lpData, UINT dataLength,float volume);

    void FlushSamples();

    inline void SetAudioOffset(int offset) {this->offset = offset; SetTimeOffset(offset);}

	virtual void SetLiveInstance(bool bLiveInstance);
	virtual bool IsNeedRemove() const;
	virtual void OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor);
};

class DSource : public IBaseVideo
{
	DYNIC_DECLARE();
	friend class DAudioSource;
	friend class CapturePin;

	IGraphBuilder           *graph;
	ICaptureGraphBuilder2   *capture;
	IMediaControl           *control;
	IMediaEventEx           *m_pEvent;
	IBaseFilter             *audioDeviceFilter;
	IBaseFilter             *audioFilter; // Audio renderer filter

    WAVEFORMATEX            audioFormat;
	DAudioSource       *audioOut;

	bool bFiltersLoaded;
	//volatile  bool            m_bChecking;
	HANDLE          hThreadCheckDevice;
	HANDLE  m_hCheckExitEvent;

    bool bRequestVolume;
    float fNewVol;

	// 上次音频来的时间
	QWORD m_qwrdAudioTime;

	// 上次设备重启时间
	//QWORD m_qwrdDeviceResetTime;

    UINT enteredSceneCount;
	String strAudioName;
	String strAudioID;
    Value   data;
    bool            bCapturing;

	bool bHasPre;
    bool bCanEnter;

	//bool m_bCheckAudioDevice;

	int soundOutputType = 0;
    HANDLE   hSampleMutex;

	bool LoadAudioInputDevice();
	void UnloadAudioInputDevice();
	void SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType);

	bool ReStartCaptrue();
	DWORD CheckDevice();
	static DWORD STDCALL CheckDeviceThread(LPVOID *pData);
    void Start();
    void Stop();

	void FlushSamples() {}
	void ReceiveMediaSample(IMediaSample *sample, bool bAudio);
public:
	DSource();
    bool Init(Value &data);
	~DSource();

	void UpdateSettings(Value &data);

    void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture*texture, bool bScaleFull, bool bIsLiveC);

	virtual const char* GetAduioClassName() const;
	virtual IBaseAudio * GetAudioRender();

    void BeginScene();
    void EndScene();

    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();

    void SetInt(CTSTR lpName, int iVal);
    void SetFloat(CTSTR lpName, float fValue);

    Vect2 GetSize() const {return Vect2(float(32), float(32));}
	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
	virtual void SetCanEnterScene(bool bCanEnter);
	virtual bool CanEnterScene() const;
};

