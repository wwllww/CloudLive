/**
* John Bradley (jrb@turrettech.com)
*/
#include "VideoAudioSource.h"

VideoAudioSource::VideoAudioSource(unsigned int bitsPerSample, unsigned int blockSize, unsigned int channelMask, unsigned int rate, unsigned int channels, bool global)
{
    InitializeCriticalSection(&sampleBufferLock);
	m_bPlayMusic = true;
	//fpYUV = fopen("D:\\test2_640x480.pcm", "wb");
    this->bitsPerSample = bitsPerSample;
    this->blockSize = blockSize;
    this->channelMask = channelMask;
    this->rate = rate;
    this->channels = channels;
    this->lastTimestamp = 0;

    sampleFrameCount   = this->rate / 100;
    sampleSegmentSize  = this->blockSize * sampleFrameCount;

    outputBuffer.SetSize(sampleSegmentSize);

    InitAudioData(false, channels, rate, bitsPerSample, blockSize, channelMask);
    
	//GlobalAudo = global;
    BLiveAddAudioSource(this);	
	//m_bPalyLocal = !global;

	m_pAudioWaveOut = NULL;
	String strReanderName = BLiveGetRendAudioDevice(true,true);
	if (NULL == m_pAudioWaveOut)
	{
		if (!strReanderName.Compare(TEXT("Disable")))
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	if (NULL != m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Initialize(strReanderName.Array(), 2, rate, bitsPerSample);
	}

	m_pSecWaveOut = NULL;
	bSameDevice = false;
	String SecRenderName = BLiveGetSecondRendAudioDevice(true, true);

	if (SecRenderName.CompareI(strReanderName.Array()) && !SecRenderName.Compare(TEXT("Disable")))
	{
		bSameDevice = true;
	}
	else if (!SecRenderName.Compare(TEXT("Disable")))
	{
		m_pSecWaveOut = new AudioWaveOut;

		m_pSecWaveOut->Initialize(SecRenderName.Array(), 2, rate, bitsPerSample);
	}

	m_frameTime = BLiveGetFrameTime();
	
}

VideoAudioSource::~VideoAudioSource()
{
	while (m_audioDataVec.Num() > 0)
	{		
		delete m_audioDataVec[0];;
		m_audioDataVec.Remove(0);	
	}
	if (NULL != m_pAudioWaveOut)
	{
		delete m_pAudioWaveOut;
		m_pAudioWaveOut = NULL;
	}
	if (NULL != m_pSecWaveOut)
	{
		delete m_pSecWaveOut;
		m_pSecWaveOut = NULL;
	}

    BLiveRemoveAudioSource(this);
    DeleteCriticalSection(&sampleBufferLock);
}

bool VideoAudioSource::GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp)
{
	if (sampleBuffer.Num() >= sampleSegmentSize)
	{
		EnterCriticalSection(&sampleBufferLock);
		int64_t pts;
		int samplesProcessed = 0;
		while (samplesProcessed != sampleFrameCount) {
			int remaining = sampleFrameCount - samplesProcessed;
			AudioTimestamp &ts = sampleBufferPts[0];
			ts.count -= remaining;
			samplesProcessed += remaining;
			if (ts.count < 0) {
				samplesProcessed += ts.count;
				//Log(TEXT("how many times it goes,pts = %I64d"), ts.pts);
				sampleBufferPts.pop_front();
			}
			//pts = ts.pts;
		}

		mcpy(outputBuffer.Array(), sampleBuffer.Array(), sampleSegmentSize);
		sampleBuffer.RemoveRange(0, sampleSegmentSize);

		LeaveCriticalSection(&sampleBufferLock);

		*buffer = outputBuffer.Array();
		*numFrames = sampleFrameCount;
		/*CurrentBLiveGetAudioTime = BLiveGetAudioTime();
		if (lastBLiveGetAudioTime != CurrentBLiveGetAudioTime)
		{
			lastBLiveGetAudioTime = CurrentBLiveGetAudioTime;
		}
		else
		{
			return false;
		}*/
		if (!lastTimestamp || BLiveGetAudioTime() > lastTimestamp + 10) {
			*timestamp = BLiveGetAudioTime();
		}
		else {
			*timestamp = lastTimestamp + 10;
		}
		//Log(TEXT("hhhhhhhBLiveGetAudioTime() = %llu,*timestamp = %llu"), BLiveGetAudioTime(), *timestamp);
		lastTimestamp = *timestamp;
		//if (pts >= *timestamp) {
		//	*timestamp = pts;
		//}
		return true;
	}
	
    return false;
}

void VideoAudioSource::ReleaseBuffer()
{

}

CTSTR VideoAudioSource::GetDeviceName() const
{
    return NULL;
}

void VideoAudioSource::PushAudio(const void *lpData, unsigned int size, int64_t pts)
{
	
    if(lpData)
    {
		if (m_bPlayMusic)
		{
			size = size / blockSize;
			//size = size / 4;
			AudioTimestamp audioTimestamp;
			EnterCriticalSection(&sampleBufferLock);
			sampleBuffer.AppendArray(static_cast<const BYTE *>(lpData), size * blockSize);
			audioTimestamp.count = size;
			audioTimestamp.pts = pts;
			sampleBufferPts.push_back(audioTimestamp);
			//Log(TEXT("sampleBufferPts.size = %d"), sampleBufferPts.size());
			LeaveCriticalSection(&sampleBufferLock);

			if (m_pAudioWaveOut && m_bPlayPcmLocal)
			{
				char *OutBuffer;
				int Len = size  * blockSize;
				CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer);

				m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);

				if (!bSameDevice && bProjector && m_pSecWaveOut)
					m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);

			}
			else if (bProjector)
			{
				char *OutBuffer;
				int Len = size  * blockSize;
				CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer);

				if (bSameDevice && m_pAudioWaveOut)
				{
					m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);
				}
				else if (m_pSecWaveOut)
				{
					m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);
				}
			}
		}		
    }	 
}

void VideoAudioSource::SetPlayMuisc(bool bPlay /*= false*/)
{
	m_bPlayMusic = bPlay;
}
