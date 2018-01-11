#ifndef SDLDRAW_H
#define SDLDRAW_H

#include "BaseAfx.h"
#include "AudioWaveOut.h"
#include "TThread.h"
#include <list>
#include "BlackMagic.h"


class CMultimediaRender
{
public:
	CMultimediaRender();
	~CMultimediaRender();
	void InitD3DReSize();
	bool SetVideoRender(HWND Hwnd, const Vect2 &pos, const Vect2 &size);
	bool SetAudioRender(int channels, int samplerate, int sampleperbits);
	bool SetAudioRender();		
	bool CanPlayAudio();
	String ChooseShader(DeviceColorType ColorType);
	void AudioEable(bool eable) 
	{
		m_lock.Lock();
		CanAudio = eable;
		m_lock.UnLock();
	}
	bool AudioRendering() { return CanAudio;	}
	void Render(CSampleData* data,bool bAudioDisabled);
	void ResetShaderYV12();

	bool HaveSDIOut();
	void PushOrUpdateSIDId(const SDIID& id);
	bool FindAndRemoveId(int id);
	void SetIsInteraction(bool bInteraction);
	void GetDisPlayMode(SDIOUT_DISPLAYMODE mode,int &Width, int& Height);
	void RenderTexture();

private:
	void VideoRender(unsigned char* Buffer, int width, int height, int pixformat);
	inline void AudioRender(unsigned char* Buffer, unsigned int len);
	void VideoDestroy();
	void AudioDestroy();
private:
	int Width;
	int Height; 
	int Pitch;
	AudioWaveOut* AudioOut;
	bool	CanAudio;
	TLock	m_lock;
	bool    bUseYV12;
	bool    bUseI420;

	std::list<SDIID> SIDIDs;
	TLock SDIMutex;
	int FirstSDIRender = -1000;
	bool m_bInteraction;

	unsigned char *LpConverData;
	unsigned char *LpTemData;
	D3DAPI *D3DRender;
	Texture *SwapRender;
	HWND    m_hwnd;
	CD3DReszie     *SDID3DResize;
	bool     bReadyToDraw;

public:
	Vect2			m_pos;
	Vect2			m_size;
	int				Pixformat;
	Texture			*texture;
	unsigned char*  m_pDes;
	TLock			m_previewlock;
	void			UpLoad();
	Texture			*Oldtexture;
	Shader          *colorConvertShader;
	HANDLE          colorHandle;
	WAVEFORMATEX    WaveFormat;
	
};


#endif
