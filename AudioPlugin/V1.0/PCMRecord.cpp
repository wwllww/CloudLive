#include "PCMRecord.h"
#define INP_BUFFER_SIZE (8 * 1024)

PCMRecord::PCMRecord(std::wstring auidoName, void* data)
{
	clientData = data;
	m_record = false;
	m_callBack = NULL;
	pWaveHdr1 = reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR)));
	pWaveHdr2 = reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR))); //给声音文件头分配内存空间

	pBuffer1 = (PBYTE)malloc(INP_BUFFER_SIZE);
	pBuffer2 = (PBYTE)malloc(INP_BUFFER_SIZE);    //给缓冲区分配空间

	waveform.wFormatTag = WAVE_FORMAT_PCM;   //PCM编码
	waveform.nChannels = 1;                //单声道
	waveform.nSamplesPerSec = 8000;         //采样频率，每秒采集次数
	waveform.wBitsPerSample = 8;           //采样位，模拟信号转数字信号的精准度

	AudioId = WAVE_MAPPER;
	MMRESULT mr = MMSYSERR_NOERROR;
	unsigned int numIn = waveInGetNumDevs();
	for (int item = 0; item < numIn; item++)
	{
		WAVEINCAPS caps;
		mr = waveInGetDevCaps(item, &caps, sizeof (WAVEINCAPS));
		if (mr == MMSYSERR_NOERROR)
		{
			if (auidoName == caps.szPname)
			{
				AudioId = item;

				waveform.nChannels = caps.wChannels;

				if (caps.dwFormats & (WAVE_FORMAT_4S16 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M08))
				{
					//采样位，模拟信号转数字信号的精准度
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_4S16 | WAVE_FORMAT_4M16) ? 16 : 8; 
					waveform.nSamplesPerSec = 44100;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16 | WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08))
				{
					//采样位，模拟信号转数字信号的精准度
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16) ? 16 : 8;
					waveform.nSamplesPerSec = 48000;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16 | WAVE_FORMAT_96M08 | WAVE_FORMAT_96S08))
				{
					//采样位，模拟信号转数字信号的精准度
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16) ? 16 : 8;
					waveform.nSamplesPerSec = 96000;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08))
				{
					//采样位，模拟信号转数字信号的精准度
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16) ? 16 : 8;
					waveform.nSamplesPerSec = 22050;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 | WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08))
				{
					//采样位，模拟信号转数字信号的精准度
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16) ? 16 : 8;
					waveform.nSamplesPerSec = 11025;
				}

				break;
			}
		}
	}
	
	waveform.nAvgBytesPerSec = waveform.nSamplesPerSec * sizeof(unsigned short);
	waveform.nBlockAlign = waveform.nChannels * waveform.wBitsPerSample / 8;
	waveform.cbSize = 0;       //PCM编码时，此处为0

	//创建线程处理函数
	M_PCMThread = ::CreateThread(NULL, 0, PCMCallThread, this, 0,	&M_PCMThreadID);
}

PCMRecord::~PCMRecord()
{
	if (pWaveHdr1)
	{
		free(pWaveHdr1);
		pWaveHdr1 = NULL;
	}
	if (pWaveHdr2)
	{
		free(pWaveHdr2);
		pWaveHdr2 = NULL;
	}

	if (pBuffer1)
	{
		free(pBuffer1);
		pBuffer1 = NULL;
	}
	if (pBuffer2)
	{
		free(pBuffer2);
		pBuffer2 = NULL;
	}
}

void PCMRecord::SetCallback(AudioDataCallback callback)
{
	m_mutex.lock();
	m_callBack = callback;
	m_mutex.unlock();
}

void PCMRecord::RecordStart()     //录音准备
{
	if (m_record)
		return;
	m_record = true;

	if (int Ret = waveInOpen(&hWaveIn, AudioId, &waveform, M_PCMThreadID, NULL, CALLBACK_THREAD)) { //打开输入设备
		free(pBuffer1);
		pBuffer1 = NULL;
		free(pBuffer2);
		pBuffer2 = NULL;
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(NULL, TEXT("Audio can not be open!"), TEXT("YES"), MB_OK);
		return;
	}
	//初始化声音文件头
	pWaveHdr1->lpData = (LPSTR)pBuffer1;   //设置缓冲区
	pWaveHdr1->dwBufferLength = INP_BUFFER_SIZE; //缓冲区大小
	pWaveHdr1->dwBytesRecorded = 0;
	pWaveHdr1->dwUser = 0;
	pWaveHdr1->dwFlags = WHDR_PREPARED;
	pWaveHdr1->dwLoops = 1;
	pWaveHdr1->lpNext = NULL;
	pWaveHdr1->reserved = 0;
	waveInPrepareHeader(hWaveIn, pWaveHdr1, sizeof(WAVEHDR));  //将缓冲区信息和输入设备关联
	waveInAddBuffer(hWaveIn, pWaveHdr1, sizeof (WAVEHDR)); //将缓冲区地址添加到输入设备中

	pWaveHdr2->lpData = (LPSTR)pBuffer2;
	pWaveHdr2->dwBufferLength = INP_BUFFER_SIZE;
	pWaveHdr2->dwBytesRecorded = 0;
	pWaveHdr2->dwUser = 0;
	pWaveHdr2->dwFlags = WHDR_PREPARED;
	pWaveHdr2->dwLoops = 1;
	pWaveHdr2->lpNext = NULL;
	pWaveHdr2->reserved = 0;
	waveInPrepareHeader(hWaveIn, pWaveHdr2, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn, pWaveHdr2, sizeof (WAVEHDR)); //同上

	dwDataLength = 0;
	waveInStart(hWaveIn); //打开输入设备，开始录音
}

void PCMRecord::RecordStop()
{
	if (m_record)
	{
		m_record = false;
		waveInReset(hWaveIn); //停止录音，关闭输入设备
	}
}

LRESULT PCMRecord::OnMM_WIM_OPEN(UINT wParam, LONG lParam) //开始录音
{
	// TODO: Add your message handler code here and/or call default
	m_record = TRUE;
	return 0;
}

LRESULT PCMRecord::OnMM_WIM_DATA(WPARAM wParam, LPARAM lParam) //缓冲区满的时候，对应的声音文件头如pWaveHdr1作为lParam传递进来
{
	//// TODO: Add your message handler code here and/or call default
	//// Reallocate save buffer memory

	WAVEHDR& Wavehdr = *(reinterpret_cast<PWAVEHDR>(lParam));

	dwDataLength += Wavehdr.dwBytesRecorded;//加长pSaveBuffer的实际数据长度

	if (m_record == false)
	{
		waveInClose(hWaveIn);//停止录音，关闭输入设备
		return 0;
	}

	m_mutex.lock();
	if (m_callBack)
	{
		m_callBack(Wavehdr.lpData, Wavehdr.dwBytesRecorded, waveform, clientData);
	}
	m_mutex.unlock();

	////将音频写入到文件中
	//FILE* fp = fopen("ecord.pcm", "ab+");
	//if (fp == NULL)
	//{
	//	printf("fopen error,%d", __LINE__);
	//}
	//fwrite(((PWAVEHDR)lParam)->lpData, ((PWAVEHDR)lParam)->dwBytesRecorded, 1, fp);
	//fclose(fp);

	// Send out a new buffer
	waveInAddBuffer(hWaveIn, &Wavehdr, sizeof (WAVEHDR));//将缓冲区添加回到设备中
	//假如现在是pWaveHdr1满了，lParam就是pWaveHdr1，在我们保存pWaveHdr1的数据时，pWaveHdr2正在录音，保存完pWaveHdr1，再把pWaveHdr1添加回到设备中，这样达到两个缓冲区交替使用
	return 0;
}

LRESULT PCMRecord::OnMM_WIM_CLOSE(UINT wParam, LONG lParam) //停止录音时
{
	// TODO: Add your message handler code here and/or call default
	m_record = false;
	if (0 == dwDataLength) {   //没有数据，长度为0
		return 0;
	}
	waveInUnprepareHeader(hWaveIn, pWaveHdr1, sizeof (WAVEHDR));//取消输入设备和pWaveHdr1的关联
	waveInUnprepareHeader(hWaveIn, pWaveHdr2, sizeof (WAVEHDR));

	return 0;
}

DWORD WINAPI PCMRecord::PCMCallThread(PVOID param)
{
	PCMRecord* _This = (PCMRecord*)param;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch (msg.message)
		{
		case MM_WIM_OPEN:
			_This->OnMM_WIM_OPEN(msg.wParam, msg.lParam);
			break;
		case MM_WIM_DATA:
			if (_This)
			{
				_This->OnMM_WIM_DATA(msg.wParam, msg.lParam);
			}
			break;
		case MM_WIM_CLOSE:
			_This->OnMM_WIM_CLOSE(msg.wParam, msg.lParam);
			delete _This;
			_This = NULL;
			return 0;
		}
	}

	return 0;
}
