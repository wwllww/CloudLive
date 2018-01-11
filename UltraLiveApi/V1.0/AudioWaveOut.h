#ifndef __AUDIOWAVEOUT_H__
#define __AUDIOWAVEOUT_H__
#include "BaseAfx.h"
//#include <tchar.h>
#include <MMSystem.h>
#include <queue>
#define MAX_WAVEOUT_QUEUE_LEN	20
#define MAX_SUPPORT_PCMPKG_LEN	90960
#pragma warning(disable:4273)
class BASE_EXPORT AudioWaveOut
{
public:
	AudioWaveOut(void);
	~AudioWaveOut(void);
	
	int Initialize( TCHAR* devicename, int channels, int samplerate, int sampleperbits );

	int push_pcm_data(char* pbuf, int nlen);

	int Uninitialize();
private:
	static void CALLBACK write_callback(HWAVEOUT outdev, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

	int awresample( char* pbuf, int nlen, char* dstbuf, int* dstlen );
private:
		int			m_outdevid;
	HWAVEOUT	m_outdev;
	bool		m_bInit;
	bool        bCanInPCM;
	bool		m_bConver32t16;
	typedef struct _WavePerIo{
		WAVEHDR	 hdr;
		PBYTE	 buf;
		DWORD	 len;
	}WAVEPERIO, *PWAVEPERIO;
	std::queue<PWAVEPERIO> m_wavequeue;
	CRITICAL_SECTION	 m_lock;
	std::wstring         m_DeviceName;
	int                  FailedCount;
	int                  iChannels;
	int                  iSamplerate;
	int                  iSampleperbits;
};

#endif
