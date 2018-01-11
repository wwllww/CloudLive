#include "MediaAudio.h"
#include "VideoSource.h"

IMPLEMENT_DYNIC(CDemandMediaAudio, "DemandMediaAudio", "1.0.0.1")

const float dbMinus3 = 0.7071067811865476f;
const float dbMinus6 = 0.5f;
const float dbMinus9 = 0.3535533905932738f;

//not entirely sure if these are the correct coefficients for downmixing,
//I'm fairly new to the whole multi speaker thing
const float surroundMix = dbMinus3;
const float centerMix = dbMinus6;
const float lowFreqMix = dbMinus3;

const float surroundMix4 = dbMinus6;

const float attn5dot1 = 1.0f / (1.0f + centerMix + surroundMix);
const float attn4dotX = 1.0f / (1.0f + surroundMix4);

union TripleToLong
{
	LONG val;
	struct
	{
		WORD wVal;
		BYTE tripleVal;
		BYTE lastByte;
	};
};


CDemandMediaAudio::CDemandMediaAudio()
{
	InitializeCriticalSection(&sampleBufferLock);
	lastTimestamp = 0;
	fVolume = 1.0f;
	bLiveInstance = false;
	bSameDevice = false;
	m_pAudioWaveOut = NULL;
	m_pSecWaveOut = NULL;
}

CDemandMediaAudio::~CDemandMediaAudio()
{
	DeleteCriticalSection(&sampleBufferLock);
	if (m_pAudioWaveOut)
		delete m_pAudioWaveOut;
	if (m_pSecWaveOut)
		delete m_pSecWaveOut;
}

/**************************************************
重置音频参数函数
参数：
sAudioParam ：设置音频参数，采样率，采样位，声道
***************************************************/
void CDemandMediaAudio::ResetAudioParam(const AudioParam & sAudioParam)
{
	EnterCriticalSection(&sampleBufferLock);
	m_sAudioParam = sAudioParam;
	m_bisFloat = false;
	bSameDevice = false;
	m_uBlockSize = sAudioParam.iChannel * sAudioParam.iBitPerSample / 8;

	if (m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Uninitialize();
	}

	String strReanderName = GetDirectorMonitorDevices();
	if (NULL == m_pAudioWaveOut)
	{
		if (!strReanderName.Compare(TEXT("停用")) && !m_pAudioWaveOut)
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	else
	{
		if ((strReanderName.Compare(TEXT("Disable")) || strReanderName.Compare(TEXT("停用"))))
		{
			delete m_pAudioWaveOut;
			m_pAudioWaveOut = NULL;
		}
	}

	if (NULL != m_pAudioWaveOut)
	{
		if (sAudioParam.iChannel > 2)
		{
			m_bisFloat = true;
			m_pAudioWaveOut->Initialize(strReanderName.Array(), 2, sAudioParam.iSamplesPerSec, sAudioParam.iBitPerSample * 2);
		}
		else
		{
			m_pAudioWaveOut->Initialize(strReanderName.Array(), 2, sAudioParam.iSamplesPerSec, sAudioParam.iBitPerSample);
		}
	}

	String SecRenderName = GetSecMonitorDevices();


	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Uninitialize();
	}

	if (SecRenderName.CompareI(strReanderName.Array()) && (!SecRenderName.Compare(TEXT("Disable")) || !SecRenderName.Compare(TEXT("停用"))))
	{
		bSameDevice = true;
	}
	else if (!SecRenderName.Compare(TEXT("停用")) && !m_pSecWaveOut)
	{
		m_pSecWaveOut = new AudioWaveOut;
	}
	else if (m_pSecWaveOut)
	{
		if ((SecRenderName.Compare(TEXT("Disable")) || SecRenderName.Compare(TEXT("停用"))))
		{
			delete m_pSecWaveOut;
			m_pSecWaveOut = NULL;
		}
	}
	if (m_pSecWaveOut)
	{
		if (sAudioParam.iChannel > 2)
		{
			m_bisFloat = true;
			m_pSecWaveOut->Initialize(SecRenderName.Array(), 2, sAudioParam.iSamplesPerSec, sAudioParam.iBitPerSample * 2);
		}
		else
		{
			m_pSecWaveOut->Initialize(SecRenderName.Array(), 2, sAudioParam.iSamplesPerSec, sAudioParam.iBitPerSample);
		}
	}

	sampleFrameCount = m_sAudioParam.iSamplesPerSec / 100;
	if (m_bisFloat)
	{
		InitAudioData(m_bisFloat, 2, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample * 2, m_uBlockSize, m_uchannelMask);
		sampleSegmentSize = 4 * sampleFrameCount * sizeof(float) * 8 / m_sAudioParam.iBitPerSample;//4 又声道的BlockSize,2为转为short转float后扩大2倍
	}
	else
	{
		InitAudioData(m_bisFloat, m_sAudioParam.iChannel, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample, m_uBlockSize, m_uchannelMask);
		sampleSegmentSize = this->m_uBlockSize * sampleFrameCount;
	}


	outputBuffer.SetSize(sampleSegmentSize);
	
	if (!sampleSegmentSize)
	{
		sampleBuffer.RemoveRange(0, sampleBuffer.Num());
	}

	LeaveCriticalSection(&sampleBufferLock);
}

/**************************************************
输出音频渲染数据函数
参数：
buffer ：输出数据的内存
numFrames：输出数据的长度
timestamp：输出数据的时间戳
***************************************************/
bool CDemandMediaAudio::GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp)
{
	
	if (sampleBuffer.Num() >= sampleSegmentSize && sampleSegmentSize)
	{
		EnterCriticalSection(&sampleBufferLock);

		if (!sampleBuffer.Num())
		{
			LeaveCriticalSection(&sampleBufferLock);
			return false;
		}

		int64_t pts; 
		int samplesProcessed = 0;
		while (samplesProcessed != sampleFrameCount) {
			int remaining = sampleFrameCount - samplesProcessed;
			AudioTimestamp &ts = sampleBufferPts[0];
			ts.count -= remaining;
			samplesProcessed += remaining;
			if (ts.count < 0) {
				samplesProcessed += ts.count;
				sampleBufferPts.pop_front();
			}
		}

		mcpy(outputBuffer.Array(), sampleBuffer.Array(), sampleSegmentSize);
		sampleBuffer.RemoveRange(0, sampleSegmentSize);

		LeaveCriticalSection(&sampleBufferLock);

		*buffer = outputBuffer.Array();
		*numFrames = sampleFrameCount;
		*timestamp = 0;//赋值时间戳
		if (!lastTimestamp || LastTimeTimeStamp > lastTimestamp + 10) {
			*timestamp = LastTimeTimeStamp;
		}
		else {
			*timestamp = lastTimestamp + 10;
		}
		//Log(TEXT("hhhhhhhBLiveGetAudioTime() = %llu,*timestamp = %llu"), BLiveGetAudioTime(), *timestamp);
		lastTimestamp = *timestamp;
		//Log::writeMessage(LOG_RTSPSERV, 1, "audioDataNum = %d ", sampleBuffer.Num());
		return true;
	}

	return false;
}

/**************************************************
释放内存函数
***************************************************/
void CDemandMediaAudio::ReleaseBuffer()
{

}

/**************************************************
获取音频数据函数
参数：
lpData ：输入数据的内存
size：输入数据的长度
pts：输入数据的时间戳
***************************************************/
void CDemandMediaAudio::PushAudio(const void *lpData, unsigned int size, int64_t pts, IBaseVideo *Video, bool bCanPlay)
{
	VideoLiveSource *Source = dynamic_cast<VideoLiveSource*>(Video);

	if (!m_uBlockSize || !Source)
		return;
	
	if (m_sAudioParam.iChannel <= 2)
	{
		if (fVolume != 1.0f)
		{
			short *Tem = (short*)lpData;
			for (int i = 0; i < size; i += 2)
			{
				long sVolume = Tem[i / 2];

				sVolume *= fVolume;

				if (sVolume > 0x7fff)
				{
					sVolume = 0x7fff;
				}
				else if (sVolume < -0x8000)
				{
					sVolume = -0x8000;
				}

				Tem[i / 2] = (short)sVolume;
			}
		}

		Source->PlayCallBackAudio((LPBYTE)lpData, size);
	}
	else
	{
		UINT totalSamples = size * 8 / m_sAudioParam.iBitPerSample;
		if (TemconvertBuffer.Num() < totalSamples)
			TemconvertBuffer.SetSize(totalSamples);

		OutputconvertBuffer.SetSize(totalSamples / m_sAudioParam.iChannel * 2);


		if (m_sAudioParam.iBitPerSample == 8)
		{
			float *tempConvert = TemconvertBuffer.Array();
			char *tempSByte = (char*)lpData;

			while (totalSamples--)
			{
				*(tempConvert++) = float(*(tempSByte++)) / 127.0f;
			}
		}
		else if (m_sAudioParam.iBitPerSample == 16)
		{
			float *tempConvert = TemconvertBuffer.Array();
			short *tempShort = (short*)lpData;

			while (totalSamples--)
			{
				*(tempConvert++) = float(*(tempShort++)) / 32767.0f;
			}
		}
		else if (m_sAudioParam.iBitPerSample == 24)
		{
			float *tempConvert = TemconvertBuffer.Array();
			BYTE *tempTriple = (BYTE*)lpData;
			TripleToLong valOut;

			while (totalSamples--)
			{
				TripleToLong &valIn = (TripleToLong&)tempTriple;

				valOut.wVal = valIn.wVal;
				valOut.tripleVal = valIn.tripleVal;
				if (valOut.tripleVal > 0x7F)
					valOut.lastByte = 0xFF;

				*(tempConvert++) = float(double(valOut.val) / 8388607.0);
				tempTriple += 3;
			}
		}
		else if (m_sAudioParam.iBitPerSample == 32)
		{
			float *tempConvert = TemconvertBuffer.Array();
			long *tempShort = (long*)lpData;

			while (totalSamples--)
			{
				*(tempConvert++) = float(double(*(tempShort++)) / 2147483647.0);
			}
		}

		float *inputTemp = TemconvertBuffer.Array();
		float *outputTemp = OutputconvertBuffer.Array();

		UINT numFloats = size * 8 / m_sAudioParam.iBitPerSample;

		if (m_sAudioParam.iChannel == 3)
		{
			float *endTemp = inputTemp + numFloats;

			while (inputTemp < endTemp)
			{
				float left = inputTemp[0];
				float right = inputTemp[1];

				// Drop LFE since we don't need it
				//float lfe       = inputTemp[2]*lowFreqMix;

				*(outputTemp++) = left;
				*(outputTemp++) = right;

				inputTemp += 3;
			}
		}
		else if (m_sAudioParam.iChannel == 4)
		{
			float *endTemp = inputTemp + numFloats;

			while (inputTemp < endTemp)
			{
				float left = inputTemp[0];
				float right = inputTemp[1];
				float frontCenter = inputTemp[2];
				float lowFreq = inputTemp[3];

				*(outputTemp++) = left;
				*(outputTemp++) = right;

				inputTemp += 4;
			}
		}
		else if (m_sAudioParam.iChannel == 5)
		{
			float *endTemp = inputTemp + numFloats;

			while (inputTemp < endTemp)
			{
				float left = inputTemp[0];
				float right = inputTemp[1];

				// Skip LFE , we don't really need it.
				//float lfe       = inputTemp[2];

				float rearLeft = inputTemp[3] * surroundMix4;
				float rearRight = inputTemp[4] * surroundMix4;

				// Same idea as with 5.1 downmix

				*(outputTemp++) = (left + rearLeft)  * attn4dotX;
				*(outputTemp++) = (right + rearRight) * attn4dotX;

				inputTemp += 5;
			}
		}
		else if (m_sAudioParam.iChannel == 6)
		{
			float *endTemp = inputTemp + numFloats;

			while (inputTemp < endTemp)
			{
				float left = inputTemp[0];
				float right = inputTemp[1];
				float center = inputTemp[2] * centerMix;


				float rearLeft = inputTemp[4] * surroundMix;
				float rearRight = inputTemp[5] * surroundMix;


				*(outputTemp++) = (left + center + rearLeft) * attn5dot1;
				*(outputTemp++) = (right + center + rearRight) * attn5dot1;

				inputTemp += 6;
			}
		}

		else if (m_sAudioParam.iChannel == 8)
		{
			float *endTemp = inputTemp + numFloats;

			while (inputTemp < endTemp)
			{
				float left = inputTemp[0];
				float right = inputTemp[1];

				float center = inputTemp[2] * centerMix;

				// Drop LFE since we don't need it
				//float lowFreq       = inputTemp[3]*lowFreqMix;

				float rearLeft = inputTemp[4] * surroundMix;
				float rearRight = inputTemp[5] * surroundMix;

				// Drop SPEAKER_FRONT_LEFT_OF_CENTER , SPEAKER_FRONT_RIGHT_OF_CENTER
				//float centerLeft    = inputTemp[6];
				//float centerRight   = inputTemp[7];

				// Downmix from 5.1 to stereo
				*(outputTemp++) = (left + center + rearLeft)  * attn5dot1;
				*(outputTemp++) = (right + center + rearRight) * attn5dot1;

				inputTemp += 8;
			}
		}

		if (fVolume != 1.0f)
			MultiplyAudioBuffer(OutputconvertBuffer.Array(), OutputconvertBuffer.Num(),fVolume);

		Source->PlayCallBackAudio((LPBYTE)OutputconvertBuffer.Array(), OutputconvertBuffer.Num() * 4);

		if (bCanPlay)
		{
			bool bPlayLive = false;

			if (bLiveInstance)
			{
				AudioTimestamp audioTimestamp;
				EnterCriticalSection(&sampleBufferLock);
				sampleBuffer.AppendArray((BYTE *)(OutputconvertBuffer.Array()), OutputconvertBuffer.Num() * 4);
				audioTimestamp.count = size / m_uBlockSize;
				audioTimestamp.pts = pts;
				sampleBufferPts.push_back(audioTimestamp);
				LeaveCriticalSection(&sampleBufferLock);
				bPlayLive = m_bPlayPcmLive;
			}
			else
			{
				sampleBuffer.RemoveRange(0, sampleBuffer.Num());
			}

			int Len = OutputconvertBuffer.Num();
			char *OutBuffer;
			CaculateVolume((LPVOID)OutputconvertBuffer.Array(), Len, (void**)&OutBuffer);

			EnterCriticalSection(&sampleBufferLock);

			if (m_pAudioWaveOut && (m_bPlayPcmLocal || bPlayLive))
			{
				m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len * 4);

				if (!bSameDevice && bProjector && m_pSecWaveOut)
					m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len * 4);

			}
			else if (bProjector)
			{
				if (bSameDevice && m_pAudioWaveOut)
				{
					m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len * 4);
				}
				else if (m_pSecWaveOut)
				{
					m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len * 4);
				}
			}
			LeaveCriticalSection(&sampleBufferLock);
		}
		else
		{
			int Len = OutputconvertBuffer.Num();
			char *OutBuffer;
			CaculateVolume((LPVOID)OutputconvertBuffer.Array(), Len, (void**)&OutBuffer);
		}
		return;
	}

	if (bCanPlay)
	{
		bool bPlayLive = false;
		size = size / m_uBlockSize;
		if (bLiveInstance)
		{
			AudioTimestamp audioTimestamp;
			EnterCriticalSection(&sampleBufferLock);
			sampleBuffer.AppendArray(static_cast<const BYTE *>(lpData), size * m_uBlockSize);
			audioTimestamp.count = size;
			audioTimestamp.pts = pts;
			sampleBufferPts.push_back(audioTimestamp);
			LeaveCriticalSection(&sampleBufferLock);
			bPlayLive = m_bPlayPcmLive;
		}
		else
		{
			sampleBuffer.RemoveRange(0, sampleBuffer.Num());
		}
		int Len = size  * m_uBlockSize;
		char *OutBuffer;
		CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer);
		EnterCriticalSection(&sampleBufferLock);
		
		if (m_pAudioWaveOut && (m_bPlayPcmLocal || bPlayLive))
		{
			m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);

			if (!bSameDevice && bProjector && m_pSecWaveOut)
				m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);

		}
		else if (bProjector)
		{
			if (bSameDevice && m_pAudioWaveOut)
			{
				m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);
			}
			else if (m_pSecWaveOut)
			{
				m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);
			}
		}
		LeaveCriticalSection(&sampleBufferLock);
	}
	else
	{
		int Len = size;
		char *OutBuffer;
		CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer, true);
	}
}

void CDemandMediaAudio::UpdateSettings(Value &JsonParam)
{
	fVolume = JsonParam["Volume"].asDouble();
}

void CDemandMediaAudio::SetVolumef(float Volume)
{
	fVolume = Volume;
}

void CDemandMediaAudio::SetLiveInstance(bool bLiveInstance)
{
	this->bLiveInstance = bLiveInstance;

// 	if (!bLiveInstance)
// 	{
// 		EnterCriticalSection(&sampleBufferLock);
// 		sampleBuffer.RemoveRange(0, sampleBuffer.Num());
// 		LeaveCriticalSection(&sampleBufferLock);
// 	}
}

bool CDemandMediaAudio::IsNeedRemove() const
{
	return !bLiveInstance && !audioSegments.Num();
}

void CDemandMediaAudio::OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor)
{

	if (this->MonitorDevices.Compare(MonitorDevices.Array()) && this->SecMonitor.Compare(SecMonitor.Array()))
	{
		return;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
	EnterCriticalSection(&sampleBufferLock);

	m_bisFloat = false;
	bSameDevice = false;

	if (m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Uninitialize();
	}

	if (NULL == m_pAudioWaveOut)
	{
		if (!MonitorDevices.Compare(TEXT("停用")) && !m_pAudioWaveOut)
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	else
	{
		if ((MonitorDevices.Compare(TEXT("Disable")) || MonitorDevices.Compare(TEXT("停用"))))
		{
			delete m_pAudioWaveOut;
			m_pAudioWaveOut = NULL;
		}
	}
	if (NULL != m_pAudioWaveOut)
	{
		if (m_sAudioParam.iChannel > 2)
		{
			m_bisFloat = true;
			m_pAudioWaveOut->Initialize(MonitorDevices.Array(), 2, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample * 2);
		}
		else
		{
			m_pAudioWaveOut->Initialize(MonitorDevices.Array(), 2, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample);
		}
	}

	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Uninitialize();
	}

	if (SecMonitor.CompareI(MonitorDevices.Array()) && (!SecMonitor.Compare(TEXT("Disable")) || !SecMonitor.Compare(TEXT("停用"))))
	{
		bSameDevice = true;
	}
	else if (!SecMonitor.Compare(TEXT("停用")) && !m_pSecWaveOut)
	{
		m_pSecWaveOut = new AudioWaveOut;
	}
	else if (m_pSecWaveOut)
	{
		if ((SecMonitor.Compare(TEXT("Disable")) || SecMonitor.Compare(TEXT("停用"))))
		{
			delete m_pSecWaveOut;
			m_pSecWaveOut = NULL;
		}
	}

	if (m_pSecWaveOut)
	{
		if (m_sAudioParam.iChannel > 2)
		{
			m_bisFloat = true;
			m_pSecWaveOut->Initialize(SecMonitor.Array(), 2, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample * 2);
		}
		else
		{
			m_pSecWaveOut->Initialize(SecMonitor.Array(), 2, m_sAudioParam.iSamplesPerSec, m_sAudioParam.iBitPerSample);
		}
	}

	LeaveCriticalSection(&sampleBufferLock);

	this->MonitorDevices = MonitorDevices;
	this->SecMonitor = SecMonitor;

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}