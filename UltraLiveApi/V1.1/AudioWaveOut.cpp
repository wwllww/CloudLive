#include "AudioWaveOut.h"
#include "OperatNew.h"


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

AudioWaveOut::AudioWaveOut(void)
{	
	m_outdevid = -1;
	m_outdev = NULL;
	m_bInit = false;
	bCanInPCM = false;
	FailedCount = 0;
	InitializeCriticalSection(&m_lock);
	int nprehdrcnt = MAX_WAVEOUT_QUEUE_LEN;
	while( nprehdrcnt-- )
	{
		PWAVEPERIO whdr = (PWAVEPERIO)malloc(sizeof(WAVEPERIO));
		whdr->hdr.dwFlags = 0;
		whdr->buf = (PBYTE)malloc(MAX_SUPPORT_PCMPKG_LEN);
		whdr->len = MAX_SUPPORT_PCMPKG_LEN;
		m_wavequeue.push(whdr);
	}
	m_bConver32t16 = false;
}

AudioWaveOut::~AudioWaveOut(void)
{
	if(m_outdev)
	{
		waveOutReset(m_outdev);
		waveOutClose(m_outdev);
		m_outdev = NULL;
	}
	DeleteCriticalSection(&m_lock);
	while( !m_wavequeue.empty() )
	{
		PWAVEPERIO whdr = m_wavequeue.front();
		m_wavequeue.pop();
		free(whdr->buf);
		free(whdr);
	}
	if (!m_DeviceName.empty())
	Log::writeMessage(LOG_RTSPSERV,1,"waveout 析构 devicename %s", WcharToAnsi(m_DeviceName.c_str()).c_str());
}
int AudioWaveOut::Uninitialize()
{
	m_bInit = false;
	if(m_outdev)
	{
		waveOutReset(m_outdev);
		waveOutClose(m_outdev);
		m_outdev = NULL;
	}
	m_outdevid = -1;
	return 0;
}
int AudioWaveOut::Initialize( TCHAR* devicename, int channels, int samplerate, int sampleperbits )
{	
	if( m_bInit || !sampleperbits)
		return -1;
	m_DeviceName = devicename;
	iChannels = channels;
	iSamplerate = samplerate;
	iSampleperbits = sampleperbits;

	MMRESULT mr = MMSYSERR_NOERROR;
	Log::writeMessage(LOG_RTSPSERV, 1, "devname:%s ch:%d sr:%d bits:%d", WcharToAnsi(devicename).c_str(), channels, samplerate, sampleperbits);
	if (wcscmp(L"Default", devicename) == 0 || wcscmp(L"默认", devicename) == 0)
	{
		m_outdevid = WAVE_MAPPER;
	}
	else
	{
		int nOutDevices = waveOutGetNumDevs();
		Log::writeMessage(LOG_RTSPSERV, 1, "waveOutGetNumDevs ret %d", nOutDevices);
		for (int item = 0; item < nOutDevices; item++)
		{
			WAVEOUTCAPS caps;
			mr = waveOutGetDevCaps(item, &caps, sizeof (WAVEOUTCAPS));
			Log::writeMessage(LOG_RTSPSERV, 1, "waveOutGetDevCaps ret %d", mr);
			if (mr == MMSYSERR_NOERROR)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "device id %d, device name %s", item, WcharToAnsi(caps.szPname).c_str());
				if (wcsstr(devicename, caps.szPname))
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "found device id %d, device name %s", item, WcharToAnsi(devicename).c_str());
					m_outdevid = item;
					break;
				}
			}
		}
		if (m_outdevid == -1)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "not found waveout devices devicename %s", WcharToAnsi(devicename).c_str());
			return -2;
		}
	}	
	if( sampleperbits == 32 )
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "AudioWaveOut Enable 32bits Conver 16 short");
		m_bConver32t16 = true;
		sampleperbits = 16;
	}
	else
	{
		m_bConver32t16 = false;
	}
	WAVEFORMATEX outwfx={0};
	DWORD dwoFlag = WAVE_MAPPED | CALLBACK_FUNCTION;
	outwfx.wFormatTag = WAVE_FORMAT_PCM;
	outwfx.cbSize = 0;
	outwfx.nChannels = channels;
	outwfx.nSamplesPerSec = samplerate;
	outwfx.wBitsPerSample = sampleperbits;

	outwfx.nBlockAlign=outwfx.nChannels * outwfx.wBitsPerSample / 8;
	outwfx.nAvgBytesPerSec=outwfx.nSamplesPerSec * outwfx.nBlockAlign;	
	mr = waveOutOpen (&m_outdev, m_outdevid, &outwfx,(DWORD_PTR)write_callback, (DWORD)this, dwoFlag);
	if (mr != MMSYSERR_NOERROR)
	{
		mr = waveOutOpen (&m_outdev, WAVE_MAPPER, &outwfx,(DWORD_PTR)write_callback, (DWORD)this, CALLBACK_FUNCTION);
		if (mr != MMSYSERR_NOERROR)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "waveOutOpen error!devicename %s", WcharToAnsi(devicename).c_str());
			return -3;
		}
	}
	m_bInit = true;
	bCanInPCM = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "waveout init devicename %s 初始化成功!", WcharToAnsi(devicename).c_str());
	return 0;
}

int AudioWaveOut::push_pcm_data(char* pbuf, int nlen)
{
	if (pbuf == NULL || nlen <= 0)
		return -1;
	if (!m_bInit && !bCanInPCM)
	{
		return -2;
	}
	MMRESULT mr = MMSYSERR_NOERROR;
	PWAVEPERIO hdr = m_wavequeue.front();
	if( hdr->hdr.dwFlags & WHDR_DONE || hdr->hdr.dwFlags == 0 )
	{
		mr = waveOutUnprepareHeader(m_outdev, &hdr->hdr, sizeof(WAVEHDR));
		if (mr != MMSYSERR_NOERROR)
			Log::writeMessage(LOG_RTSPSERV, 1, "waveOutUnprepareHeader ERROR devicename %s ", WcharToAnsi(m_DeviceName.c_str()).c_str());
	}
	else
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "overrun reset devicename %s ", WcharToAnsi(m_DeviceName.c_str()).c_str());
		waveOutReset(m_outdev);
	}

	hdr->hdr.dwFlags = 0;
	hdr->hdr.dwUser = 0;
	if( nlen > MAX_SUPPORT_PCMPKG_LEN )
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "PCM package len > MAX_SUPPORT_PCMPKG_LEN not play devicename %s ", WcharToAnsi(m_DeviceName.c_str()));
		return -5;
	}
	int dlen = nlen;
	if( m_bConver32t16 )
	{
		if( awresample( pbuf, nlen, (char*)hdr->buf, &dlen ) != 0 )
			Log::writeMessage(LOG_RTSPSERV, 1, "AudioWaveOut resample fail  devicename %s ", WcharToAnsi(m_DeviceName.c_str()));
	}
	else
		memcpy(hdr->buf, pbuf, nlen );
	hdr->hdr.lpData = (LPSTR)hdr->buf;
	hdr->hdr.dwBufferLength = dlen;

	mr = waveOutPrepareHeader(m_outdev, &hdr->hdr, sizeof(WAVEHDR));
	if (mr != MMSYSERR_NOERROR){
		if (FailedCount++ >= 100)
		{
			FailedCount = 0;
			Log::writeMessage(LOG_RTSPSERV, 1, "waveOutPrepareHeader fail devicename %s ", WcharToAnsi(m_DeviceName.c_str()).c_str());
			Uninitialize();
			Initialize(const_cast<TCHAR*>(m_DeviceName.c_str()), iChannels, iSamplerate, iSampleperbits);
		}
		return -3;
	}
	mr = waveOutWrite(m_outdev, &hdr->hdr, sizeof(WAVEHDR));
	if (mr != MMSYSERR_NOERROR){
		if (FailedCount++ >= 100)
		{
			FailedCount = 0;
			Log::writeMessage(LOG_RTSPSERV, 1, "waveOutWrite fail devicename %s ", WcharToAnsi(m_DeviceName.c_str()).c_str());
			Uninitialize();
			Initialize(const_cast<TCHAR*>(m_DeviceName.c_str()), iChannels, iSamplerate, iSampleperbits);
		}
		return -4;
	}
	m_wavequeue.pop();
	m_wavequeue.push(hdr);
	return 0;
}
void CALLBACK AudioWaveOut::write_callback(HWAVEOUT outdev, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	AudioWaveOut* pthis = (AudioWaveOut*)(dwInstance);
	WAVEHDR *hdr = (WAVEHDR *)dwParam1;

	switch (uMsg){
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		break;
	case WOM_DONE:
		break;
	}
}

int AudioWaveOut::awresample( char* pbuf, int nlen, char* dstbuf, int* dstlen )
{
	int k = 0;
	for (int i = 0; i<nlen; i += 4, k += 2)
	{
		float ftmp = *((float*)(pbuf + i));
		ftmp *= (short)(((unsigned short)-1) >> 1) + 1.;//1U<<31;
		short nmax = (short)(((unsigned short)-1) >> 1);
		short nmin = (short)(1 << 15);
		long itmp1 = (ftmp >= 0 ? floor(ftmp + 0.5) : ceil(ftmp - 0.5));
		if (itmp1 > nmax)
			itmp1 = nmax;
		if (itmp1 < nmin)
			itmp1 = nmin;
		*((short*)(dstbuf + k)) = (short)itmp1;
		//*(dstbuf + k) = *((float*)(pbuf + i))%0x7fff;
 	}
	*dstlen = nlen / sizeof(float)*sizeof(short);
	return 0;
}
