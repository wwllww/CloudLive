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
	bool m_record;    //m_record��ʾ�Ƿ�����¼��
	WAVEFORMATEX waveform;    //WAV�ļ�ͷ������Ƶ��ʽ
	DWORD dwDataLength; //dwDataLength���е����ݳ���
	HWAVEIN hWaveIn;     //�����豸���
	PBYTE pBuffer1, pBuffer2;   //�����������ݵ�������������

	PWAVEHDR pWaveHdr1, pWaveHdr2;  //�����ļ�ͷ

	AudioDataCallback m_callBack;
	std::mutex  m_mutex;
	void* clientData;
	unsigned int AudioId;
};
