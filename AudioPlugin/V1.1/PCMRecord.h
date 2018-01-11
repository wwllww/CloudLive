#pragma once
#include<string>
#include <Windows.h>  
#include<mutex>
#pragma comment(lib, "winmm.lib")  

typedef void(*AudioDataCallback)(LPSTR ptr, DWORD len, const WAVEFORMATEX& info, void* data);

class PCMRecord
{

public:
	PCMRecord(std::wstring auidoName, void* data);
	
	void RecordStart();
	void RecordStop();
	void GetAudioInfo(WAVEFORMATEX& other)
	{
		other = waveform;
	}

	LRESULT OnMM_WIM_OPEN(UINT wParam, LONG lParam);
	LRESULT OnMM_WIM_DATA(WPARAM wParam, LPARAM lParam);
	LRESULT OnMM_WIM_CLOSE(UINT wParam, LONG lParam);

	void SetCallback(AudioDataCallback callback);
private:
	static DWORD WINAPI PCMCallThread(PVOID param);
	~PCMRecord();

private:
	DWORD M_PCMThreadID;
	HANDLE M_PCMThread;
	bool m_record;    //m_record表示是否正在录音
	WAVEFORMATEX waveform;    //WAV文件头包含音频格式
	DWORD dwDataLength; //dwDataLength已有的数据长度
	HWAVEIN hWaveIn;     //输入设备句柄
	PBYTE pBuffer1, pBuffer2;   //保存输入数据的两个缓冲区。

	PWAVEHDR pWaveHdr1, pWaveHdr2;  //声音文件头

	AudioDataCallback m_callBack;
	std::mutex  m_mutex;
	void* clientData;
	unsigned int AudioId;
};
