#ifndef BASEAUDIO_H
#define BASEAUDIO_H
#include "BaseAfx.h"

#ifdef ULTRALIVEAPI_EXPORTS
typedef unsigned long long  uint64_t;
typedef void(*AudioDBCallBack)(uint64_t StreamID, float leftDb, float rightDb);
#endif
 enum
 {
 	NoAudioAvailable,
 	AudioAvailable,
 };

 struct AudioSegment
 {
 	List<float> audioData;
 	QWORD timestamp;

	inline AudioSegment(float *data, UINT numFloats, QWORD timestamp) : timestamp(timestamp)
 	{
 		audioData.CopyArray(data, numFloats);
	}

	inline void ClearData()
	{
		audioData.Clear();
	}
};


class API_EXPORT IBaseAudio : public CPObject
{
public:
	IBaseAudio();
	virtual ~IBaseAudio();
	void SetSampleRateHz(UINT sampleRateHz);
	void SetPlayLocal(bool bPlay);
	void SetAudioDBCallBack(AudioDBCallBack DBCb);
	void StartRenderAStream(const char* cRenderAudioDevice);
	void StopRenderAStream();
	void SetLastTimeStamp(QWORD &TimeStamp);
	void SetLastTimeStamp_Local(QWORD &TimeStamp);

	virtual bool GetBuffer(float **buffer, QWORD targetTimestamp,bool bLiveGet);
	virtual bool GetLatestTimestamp(QWORD &timestamp);
	virtual bool GetStreamInfo(Value &JsonInfo){ return false; }
	virtual bool Init(Value &JsonParam){ return false; }
	virtual void UpdateSettings(Value &JsonParam) {}
	virtual void SetLiveInstance(bool bLiveInstance);
	virtual bool IsLiveInstance() const { return false; }
	virtual bool IsNeedRemove() const = 0;
	virtual void OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor){};

	UINT QueryAudio(float curVolume,bool LiveQuery, bool bCanBurst = false);
	void SortAudio(QWORD timestamp);

	int  GetTimeOffset() const;
	void SetTimeOffset(int newOffset);

	void SetVolume(float fVal);
	float GetVolume() const;

	void SetAudioParam(
		float LVolumeQuotiety,                     //left volume quotiety
		float RVolumeQuotiety,                     //right volume quotiety
		float PVolumeQuotiety,                      //PGMvolume quotiety
		bool  bPlayPcmLocal, bool  bPlayPcmLive, float quotietyVolume, bool Projector);
	void CaculateVolume(LPVOID pBuffer, int& numAudioFrames, void **OutBuffer,bool bOnlyCallBack = false);
	void VolumeCaculate(char* buf, UINT32 size, double vol);
	void ResetAudioDB();
	void SetPlayPreview(bool bPlay);
	void GetDb(float &LeftDb, float &RightDb);

protected:
	void InitAudioData(bool bFloat, UINT channels, UINT samplesPerSec, UINT bitsPerSample, UINT blockSize, DWORD channelMask, bool bCheck = false);

	virtual bool GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp,bool bLiveGet) = 0;
	virtual void ReleaseBuffer() = 0;
	void MultiplyAudioBuffer(float *buffer, int totalFloats, float mulVal);
	void CalculateVolumeLevelsShort(char *buffer, int totalFloats, float &RMS);
	void CalculateVolumeLevelsFloat(float *buffer, int totalFloats, float &RMS);
	float toDB(float RMS);
	
protected:
	bool bPlayLocal;
	AudioDBCallBack AudioDbCB;
	String RenderDevice;
	QWORD LastTimeTimeStamp;
	QWORD LastTimeTimeStamp_Local;
	float leftdesktopVol;                      //left volume quotiety
	float rightdesktopVol;                      //right volume quotiety
	float desktopVol;                      //PGMvolume quotiety

	bool  m_bPlayPcmLocal;
	bool  m_bPlayPcmLive;
	bool  m_bPlayPre;
	float m_quotietyVolume;

	List<char> leftaudioData;
	List<char> rightaudioData;
	List<char> OutputaudioData;
	List<AudioSegment*> audioSegments;
	List<AudioSegment*> audioSegmentsLoacl;

	List<float> leftaudioDataf;
	List<float> rightaudioDataf;
	List<float> OutputaudioDataf;
	String MonitorDevices;
	String SecMonitor;

	bool bProjector;
private:
	void AddAudioSegment(AudioSegment *segment, float curVolume,bool bLiveAdd);
private:
	UINT OutputsampleRateHz;
	bool bFloat;
	
	int OffsetTime;
	float Volume;

	bool bResample;
	LPVOID resampler;
	double resampleRatio;

	QWORD lastUsedTimestamp;
	QWORD lastUsedTimestampLocal = 0;
	QWORD lastSentTimestamp;
	QWORD lastSentTimestampLocal = 0;
	QWORD lastGetTimestamp;
	bool  bAudioError;
	int timeOffset;

	//-----------------------------------------

	List<float> storageBuffer;

	//-----------------------------------------

	List<float> outputBuffer;
	List<float> outputBufferLocal;
	List<float> convertBuffer;
	List<float> convertBufferLocal;
	List<float> tempBuffer;
	List<float> tempBufferLocal;
	List<float> tempResampleBuffer;
	List<float> tempResampleBufferLocal;
	List<float>  inputBuffer;
	List<float>  inputBufferLocal;

	//-----------------------------------------
	UINT  inputChannels;
	UINT  inputSamplesPerSec;
	UINT  inputBitsPerSample;
	UINT  inputBlockSize;
	DWORD inputChannelMask;
	int   nAudioCnt;

	//-----------------------------------------

	float sourceVolume;
	UINT audioFramesUpdate;
	float LeftDb = -96, RightDb = -96;
};

#endif