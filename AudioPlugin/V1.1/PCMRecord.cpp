#include "PCMRecord.h"
#define INP_BUFFER_SIZE (8 * 1024)

PCMRecord::PCMRecord(std::wstring auidoName, void* data)
{
	clientData = data;
	m_record = false;
	m_callBack = NULL;
	pWaveHdr1 = reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR)));
	pWaveHdr2 = reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR))); //�������ļ�ͷ�����ڴ�ռ�

	pBuffer1 = (PBYTE)malloc(INP_BUFFER_SIZE);
	pBuffer2 = (PBYTE)malloc(INP_BUFFER_SIZE);    //������������ռ�

	waveform.wFormatTag = WAVE_FORMAT_PCM;   //PCM����
	waveform.nChannels = 1;                //������
	waveform.nSamplesPerSec = 8000;         //����Ƶ�ʣ�ÿ��ɼ�����
	waveform.wBitsPerSample = 8;           //����λ��ģ���ź�ת�����źŵľ�׼��

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
					//����λ��ģ���ź�ת�����źŵľ�׼��
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_4S16 | WAVE_FORMAT_4M16) ? 16 : 8; 
					waveform.nSamplesPerSec = 44100;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16 | WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08))
				{
					//����λ��ģ���ź�ת�����źŵľ�׼��
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16) ? 16 : 8;
					waveform.nSamplesPerSec = 48000;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16 | WAVE_FORMAT_96M08 | WAVE_FORMAT_96S08))
				{
					//����λ��ģ���ź�ת�����źŵľ�׼��
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16) ? 16 : 8;
					waveform.nSamplesPerSec = 96000;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08))
				{
					//����λ��ģ���ź�ת�����źŵľ�׼��
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16) ? 16 : 8;
					waveform.nSamplesPerSec = 22050;
				}
				else if (caps.dwFormats & (WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 | WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08))
				{
					//����λ��ģ���ź�ת�����źŵľ�׼��
					waveform.wBitsPerSample = caps.dwFormats & (WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16) ? 16 : 8;
					waveform.nSamplesPerSec = 11025;
				}

				break;
			}
		}
	}
	
	waveform.nAvgBytesPerSec = waveform.nSamplesPerSec * sizeof(unsigned short);
	waveform.nBlockAlign = waveform.nChannels * waveform.wBitsPerSample / 8;
	waveform.cbSize = 0;       //PCM����ʱ���˴�Ϊ0

	//�����̴߳�����
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

void PCMRecord::RecordStart()     //¼��׼��
{
	if (m_record)
		return;
	m_record = true;

	if (int Ret = waveInOpen(&hWaveIn, AudioId, &waveform, M_PCMThreadID, NULL, CALLBACK_THREAD)) { //�������豸
		free(pBuffer1);
		pBuffer1 = NULL;
		free(pBuffer2);
		pBuffer2 = NULL;
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(NULL, TEXT("Audio can not be open!"), TEXT("YES"), MB_OK);
		return;
	}
	//��ʼ�������ļ�ͷ
	pWaveHdr1->lpData = (LPSTR)pBuffer1;   //���û�����
	pWaveHdr1->dwBufferLength = INP_BUFFER_SIZE; //��������С
	pWaveHdr1->dwBytesRecorded = 0;
	pWaveHdr1->dwUser = 0;
	pWaveHdr1->dwFlags = WHDR_PREPARED;
	pWaveHdr1->dwLoops = 1;
	pWaveHdr1->lpNext = NULL;
	pWaveHdr1->reserved = 0;
	waveInPrepareHeader(hWaveIn, pWaveHdr1, sizeof(WAVEHDR));  //����������Ϣ�������豸����
	waveInAddBuffer(hWaveIn, pWaveHdr1, sizeof (WAVEHDR)); //����������ַ��ӵ������豸��

	pWaveHdr2->lpData = (LPSTR)pBuffer2;
	pWaveHdr2->dwBufferLength = INP_BUFFER_SIZE;
	pWaveHdr2->dwBytesRecorded = 0;
	pWaveHdr2->dwUser = 0;
	pWaveHdr2->dwFlags = WHDR_PREPARED;
	pWaveHdr2->dwLoops = 1;
	pWaveHdr2->lpNext = NULL;
	pWaveHdr2->reserved = 0;
	waveInPrepareHeader(hWaveIn, pWaveHdr2, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn, pWaveHdr2, sizeof (WAVEHDR)); //ͬ��

	dwDataLength = 0;
	waveInStart(hWaveIn); //�������豸����ʼ¼��
}

void PCMRecord::RecordStop()
{
	if (m_record)
	{
		m_record = false;
		waveInReset(hWaveIn); //ֹͣ¼�����ر������豸
	}
}

LRESULT PCMRecord::OnMM_WIM_OPEN(UINT wParam, LONG lParam) //��ʼ¼��
{
	// TODO: Add your message handler code here and/or call default
	m_record = TRUE;
	return 0;
}

LRESULT PCMRecord::OnMM_WIM_DATA(WPARAM wParam, LPARAM lParam) //����������ʱ�򣬶�Ӧ�������ļ�ͷ��pWaveHdr1��ΪlParam���ݽ���
{
	//// TODO: Add your message handler code here and/or call default
	//// Reallocate save buffer memory

	WAVEHDR& Wavehdr = *(reinterpret_cast<PWAVEHDR>(lParam));

	dwDataLength += Wavehdr.dwBytesRecorded;//�ӳ�pSaveBuffer��ʵ�����ݳ���

	if (m_record == false)
	{
		waveInClose(hWaveIn);//ֹͣ¼�����ر������豸
		return 0;
	}

	m_mutex.lock();
	if (m_callBack)
	{
		m_callBack(Wavehdr.lpData, Wavehdr.dwBytesRecorded, waveform, clientData);
	}
	m_mutex.unlock();

	////����Ƶд�뵽�ļ���
	//FILE* fp = fopen("ecord.pcm", "ab+");
	//if (fp == NULL)
	//{
	//	printf("fopen error,%d", __LINE__);
	//}
	//fwrite(((PWAVEHDR)lParam)->lpData, ((PWAVEHDR)lParam)->dwBytesRecorded, 1, fp);
	//fclose(fp);

	// Send out a new buffer
	waveInAddBuffer(hWaveIn, &Wavehdr, sizeof (WAVEHDR));//����������ӻص��豸��
	//����������pWaveHdr1���ˣ�lParam����pWaveHdr1�������Ǳ���pWaveHdr1������ʱ��pWaveHdr2����¼����������pWaveHdr1���ٰ�pWaveHdr1��ӻص��豸�У������ﵽ��������������ʹ��
	return 0;
}

LRESULT PCMRecord::OnMM_WIM_CLOSE(UINT wParam, LONG lParam) //ֹͣ¼��ʱ
{
	// TODO: Add your message handler code here and/or call default
	m_record = false;
	if (0 == dwDataLength) {   //û�����ݣ�����Ϊ0
		return 0;
	}
	waveInUnprepareHeader(hWaveIn, pWaveHdr1, sizeof (WAVEHDR));//ȡ�������豸��pWaveHdr1�Ĺ���
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
