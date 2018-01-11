#pragma once
#include<string>
#include<mutex>

typedef void(*AudioDataCallback)(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data);

class PCMRecord
{

	friend class CapturePin;

	IGraphBuilder           *graph = NULL;
	ICaptureGraphBuilder2   *capture = NULL;
	IMediaControl           *control = NULL;
	IMediaEventEx           *m_pEvent = NULL;
	IBaseFilter             *audioDeviceFilter = NULL;
	IBaseFilter             *audioFilter = NULL; // Audio renderer filter

	WAVEFORMATEX            audioFormat;

	bool bFiltersLoaded;
	HANDLE  hThreadCheckDevice;
	HANDLE  m_hCheckExitEvent;

	// 上次音频来的时间
	QWORD m_qwrdAudioTime;

	// 上次设备重启时间
	//QWORD m_qwrdDeviceResetTime;

	bool     bCapturing;


	int soundOutputType = 0;

	bool LoadAudioInputDevice();
	void UnloadAudioInputDevice();
	void SetAudioInfo(AM_MEDIA_TYPE *audioMediaType, GUID &expectedAudioType, IAMStreamConfig *audioConfig);

	bool ReStartCaptrue();
	DWORD CheckDevice();
	static DWORD STDCALL CheckDeviceThread(LPVOID *pData);
	void Start();
	void Stop();

	void ReceiveMediaSample(IMediaSample *sample, bool bAudio);
public:
	PCMRecord(std::wstring auidoName, void* data);
	~PCMRecord();
	void RecordStart();
	void RecordStop();
	void GetAudioInfo(WAVEFORMATEX& other)
	{
		other = audioFormat;
	}

	void SetCallback(AudioDataCallback callback);
	int InitParam(int nSamplesRate, int nAudioChannel, int nBitPerSample);
	std::wstring GetAuidoName() const;
private:
	static DWORD WINAPI PCMCallThread(PVOID param);
	

private:
	WAVEFORMATEX waveform;    //WAV文件头包含音频格式
	AudioDataCallback m_callBack;
	std::mutex  m_mutex;
	void* clientData;
	std::wstring m_auidoName;
};
