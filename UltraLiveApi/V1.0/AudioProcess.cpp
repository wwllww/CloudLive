#include "Instance.h"
#include "OperatNew.h"
#include "Error.h"
#include "Encoder.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

void CInstanceProcess::MixAudio(float *bufferDest, float *bufferSrc, UINT totalFloats, bool bForceMono)
{
	UINT floatsLeft = totalFloats;
	float *destTemp = bufferDest;
	float *srcTemp = bufferSrc;

	if ((UPARAM(destTemp) & 0xF) == 0 && (UPARAM(srcTemp) & 0xF) == 0)
	{
		UINT alignedFloats = floatsLeft & 0xFFFFFFFC;

		if (bForceMono)
		{
			__m128 halfVal = _mm_set_ps1(0.5f);
			for (UINT i = 0; i < alignedFloats; i += 4)
			{
				float *micInput = srcTemp + i;
				__m128 val = _mm_load_ps(micInput);
				__m128 shufVal = _mm_shuffle_ps(val, val, _MM_SHUFFLE(2, 3, 0, 1));

				_mm_store_ps(micInput, _mm_mul_ps(_mm_add_ps(val, shufVal), halfVal));
			}
		}

		__m128 maxVal = _mm_set_ps1(1.0f);
		__m128 minVal = _mm_set_ps1(-1.0f);

		for (UINT i = 0; i < alignedFloats; i += 4)
		{
			float *pos = destTemp + i;

			__m128 mix;
			mix = _mm_add_ps(_mm_load_ps(pos), _mm_load_ps(srcTemp + i));
			mix = _mm_min_ps(mix, maxVal);
			mix = _mm_max_ps(mix, minVal);

			_mm_store_ps(pos, mix);
		}

		floatsLeft &= 0x3;
		destTemp += alignedFloats;
		srcTemp += alignedFloats;
	}

	if (floatsLeft)
	{
		if (bForceMono)
		{
			for (UINT i = 0; i < floatsLeft; i += 2)
			{
				srcTemp[i] += srcTemp[i + 1];
				srcTemp[i] *= 0.5f;
				srcTemp[i + 1] = srcTemp[i];
			}
		}

		for (UINT i = 0; i < floatsLeft; ++i)
		{
			float val = destTemp[i] + srcTemp[i];

			if (val < -1.0f)     val = -1.0f;
			else if (val > 1.0f) val = 1.0f;

			destTemp[i] = val;
		}
	}
}

void CInstanceProcess::QueryAudioBuffers()
{
	if (!CurrentAudioTime)
	{
		CurrentAudioTime = GetQPCMS();
	}

	if (CurrentAudioTime > GetQPCMS())
	{
		return;
	}

	CurrentAudioTime += 10;
	bufferedAudioTimes << CurrentAudioTime;

	EnterCriticalSection(&AudioSection);
	for (UINT i = 0; i < m_AudioList.Num(); ++i)
	{
		m_AudioList[i].AudioStream->SetLastTimeStamp(CurrentAudioTime);
		m_AudioList[i].AudioStream->QueryAudio(m_AudioList[i].AudioStream->GetVolume(), true);
	}

	LeaveCriticalSection(&AudioSection);

}

bool CInstanceProcess::QueryNewAudio()
{
	bool bAudioBufferFilled = false;

	QueryAudioBuffers();

	bAudioBufferFilled = bufferedAudioTimes.Num() >= LiveParam.Advanced.BufferTime / 10;

	if (!bAudioBufferFilled)
	{	
		EnterCriticalSection(&AudioSection);
		for (UINT i = 0; i < m_AudioList.Num();)
		{
			if (m_AudioList[i].AudioStream->IsNeedRemove())
			{
				m_AudioList[i].AudioStream.reset();
				if (m_AudioList[i].Config)
					m_AudioList[i].Config.reset();
				m_AudioList.Remove(i);
				continue;
			}
			else
			{
				while (m_AudioList[i].bRender && m_AudioList[i].AudioStream->QueryAudio(m_AudioList[i].AudioStream->GetVolume(), true) != NoAudioAvailable);
				++i;
			}
		}

		LeaveCriticalSection(&AudioSection);
	}
	return bAudioBufferFilled;
}

void CInstanceProcess::EncodeAudioSegment(float *buffer, UINT numFrames, QWORD timestamp)
{
	DataPacket packet;
	if (audioEncoder->Encode(buffer, numFrames, packet, timestamp))
	{
		EnterCriticalSection(&SoundDataMutex);
		FrameAudio *frameAudio = pendingAudioFrames.CreateNew();
		frameAudio->audioData.CopyArray(packet.lpPacket, packet.size);
		frameAudio->timestamp = timestamp;
		LeaveCriticalSection(&SoundDataMutex);

		if (bUseBackInstance)
		{
			EnterCriticalSection(&SoundDataMutex_back);
			FrameAudio *frameAudio_back = pendingAudioFrames_back.CreateNew();
			frameAudio_back->audioData.CopyArray(packet.lpPacket, packet.size);
			frameAudio_back->timestamp = timestamp;
			LeaveCriticalSection(&SoundDataMutex_back);
		}

	}
}