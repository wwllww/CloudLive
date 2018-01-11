#ifndef _MEDIAAUDIO_H_
#define _MEDIAAUDIO_H_
#include "BaseAfx.h"
#include "BaseAudio.h"
#include "BaseVideo.h"
#include "AudioWaveOut.h"

/***************************************
音频参数结构
***************************************/
struct AudioParam
{
	UINT iBitPerSample = 0;   //采样位
	UINT iSamplesPerSec = 0;  //采样率
	UINT iChannel = 0;        //声道

	void operator =(const AudioParam & audioParam)
	{
		iBitPerSample = audioParam.iBitPerSample;
		iSamplesPerSec = audioParam.iSamplesPerSec;
		iChannel = audioParam.iChannel;
	}
};

/***************************************
音频时戳结构
***************************************/
struct AudioTimestamp {
	int count;
	int64_t pts;
};

/***************************************
音频处理类
***************************************/
class CDemandMediaAudio : public IBaseAudio
{
	DYNIC_DECLARE();
public:
	CDemandMediaAudio(const AudioParam & sAudioParam);
	CDemandMediaAudio();
	~CDemandMediaAudio();
public:
	virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp);
	virtual void ReleaseBuffer();
	virtual void UpdateSettings(Value &JsonParam);

	void ResetAudioParam(const AudioParam & sAudioParam);
	void PushAudio(const void *lpData, unsigned int size, int64_t pts,IBaseVideo *Video,bool bCanPlay);
	void SetVolumef(float Volume);
	virtual void SetLiveInstance(bool bLiveInstance);
	virtual bool IsNeedRemove() const;
	virtual void OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor);
protected:
private:
	AudioParam m_sAudioParam;
	bool m_bisFloat = false;
	UINT m_uBlockSize = 0;
	UINT m_uchannelMask = 0;
	List<BYTE> sampleBuffer;
	List<BYTE> outputBuffer;
	List<float> TemconvertBuffer;
	List<float> OutputconvertBuffer;
	CRITICAL_SECTION sampleBufferLock;
	std::deque<AudioTimestamp> sampleBufferPts;
	unsigned int sampleSegmentSize;
	unsigned int sampleFrameCount;
	AudioWaveOut  *m_pAudioWaveOut;
	AudioWaveOut  *m_pSecWaveOut;
	QWORD lastTimestamp;

	float fVolume;
	bool bLiveInstance;
	bool  bSameDevice;
};

#endif