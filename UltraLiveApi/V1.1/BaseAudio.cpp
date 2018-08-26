#include "BaseAudio.h"
#include "OperatNew.h"
#include "samplerate.h"
#include "Error.h"
#include <Audioclient.h>
#include "SLiveManager.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

#define KSAUDIO_SPEAKER_4POINT1     (KSAUDIO_SPEAKER_QUAD|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_3POINT1     (KSAUDIO_SPEAKER_STEREO|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_2POINT1     (KSAUDIO_SPEAKER_STEREO|SPEAKER_LOW_FREQUENCY)

struct NotAResampler
{
	SRC_STATE *resampler;
	SRC_STATE *resamplerLocal;
	QWORD     jumpRange;
};

#define MoreVariables static_cast<NotAResampler*>(resampler)

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

// inline QWORD GetQWDif(QWORD val1, QWORD val2)
// {
// 	return (val1 > val2) ? (val1 - val2) : (val2 - val1);
// }

void IBaseAudio::MultiplyAudioBuffer(float *buffer, int totalFloats, float mulVal)
{
	float sum = 0.0f;
	int totalFloatsStore = totalFloats;

	if ((UPARAM(buffer) & 0xF) == 0)
	{
		UINT alignedFloats = totalFloats & 0xFFFFFFFC;
		__m128 sseMulVal = _mm_set_ps1(mulVal);

		__m128 maxVal = _mm_set_ps1(1.0f);
		__m128 minVal = _mm_set_ps1(-1.0f);

		for (UINT i = 0; i < alignedFloats; i += 4)
		{
			__m128 sseScaledVals = _mm_mul_ps(_mm_load_ps(buffer + i), sseMulVal);

			sseScaledVals = _mm_min_ps(sseScaledVals, maxVal);
			sseScaledVals = _mm_max_ps(sseScaledVals, minVal);

			_mm_store_ps(buffer + i, sseScaledVals);
		}

		buffer += alignedFloats;
		totalFloats -= alignedFloats;
	}

	for (int i = 0; i < totalFloats; ++i)
	{
		buffer[i] *= mulVal;

		if (buffer[i] > 1.0f)
		{
			buffer[i] = 1.0f;
		}
		else if (buffer[i] < -1.0f)
		{
			buffer[i] = -1.0f;
		}
	}
}

IBaseAudio::IBaseAudio()
{
	bAudioError = false;
	sourceVolume = 1.0f;
	lastGetTimestamp = 0;
	lastUsedTimestamp = 0;
	AudioDbCB = NULL;
	LastTimeTimeStamp = 0;
	LastTimeTimeStamp_Local = 0;
	Volume = 1.0f;
	resampler = (void*)new NotAResampler;
	MoreVariables->jumpRange = 10;
	MoreVariables->resampler = NULL;
	MoreVariables->resamplerLocal = NULL;
	bResample = false;

	bFloat = false;
	inputChannels = 2;
	inputSamplesPerSec = 44100;
	inputBitsPerSample = 16;
	inputBlockSize = 4;
	inputChannelMask = 0;

	leftdesktopVol = 1.0f;
	rightdesktopVol = 1.0f;
	desktopVol = 1.0f;              

	m_bPlayPcmLocal = false;
	m_bPlayPcmLive = false;
	m_quotietyVolume = 3.0f;
	bProjector = false;
	m_bPlayPre = false;
	audioFramesUpdate = 0;
}

IBaseAudio::~IBaseAudio()
{
	if (MoreVariables->resampler)
		src_delete(MoreVariables->resampler);

	if (MoreVariables->resamplerLocal)
		src_delete(MoreVariables->resamplerLocal);

	for (UINT i = 0; i < audioSegments.Num(); i++)
		delete audioSegments[i];

	for (UINT i = 0; i < audioSegmentsLoacl.Num(); i++)
		delete audioSegmentsLoacl[i];

	delete (NotAResampler*)resampler;
}


void IBaseAudio::InitAudioData(bool bFloat, UINT channels, UINT samplesPerSec, UINT bitsPerSample, UINT blockSize, DWORD channelMask,bool bCheck)
{
	this->bFloat = bFloat;
	this->inputChannels = channels;
	this->inputSamplesPerSec = samplesPerSec;
	this->inputBitsPerSample = bitsPerSample;
	this->inputBlockSize = blockSize;
	this->inputChannelMask = channelMask;

	this->OutputsampleRateHz = GetOutputAudioSampleRate();

	UINT sampleRateHz = this->OutputsampleRateHz;

	if (inputSamplesPerSec != sampleRateHz)
	{
		int errVal;

		if (!MoreVariables->resampler)
		{
			int converterType = SRC_SINC_FASTEST;
			MoreVariables->resampler = src_new(converterType, 2, &errVal);
			MoreVariables->resamplerLocal = src_new(converterType, 2, &errVal);
		}
		
		if (!MoreVariables->resampler)
		{
			BUTEL_THORWERROR("AudioSource::InitAudioData: Could not initiate resampler");
		}

		resampleRatio = double(OutputsampleRateHz) / double(inputSamplesPerSec);
		bResample = true;

		//----------------------------------------------------
		// hack to get rid of that weird first quirky resampled packet size

		SRC_DATA data;
		data.src_ratio = resampleRatio;

		List<float> blankBuffer;
		blankBuffer.SetSize(inputSamplesPerSec / 100 * 2);

		data.data_in = blankBuffer.Array();
		data.input_frames = inputSamplesPerSec / 100;

		UINT frameAdjust = UINT((double(data.input_frames) * resampleRatio) + 1.0);
		UINT newFrameSize = frameAdjust * 2;

		tempResampleBuffer.SetSize(newFrameSize);
		tempResampleBufferLocal.SetSize(newFrameSize);

		data.data_out = tempResampleBuffer.Array();
		data.output_frames = frameAdjust;

		data.end_of_input = 0;

		int err = src_process(MoreVariables->resampler, &data);

		data.data_out = tempResampleBufferLocal.Array();
		src_process(MoreVariables->resamplerLocal,&data);
	}
	else
	{
		bResample = false;
	}

	//-------------------------------------------------------------------------

	if (inputChannels > 2)
	{
		switch (inputChannelMask)
		{
		case KSAUDIO_SPEAKER_QUAD:             /* Log(TEXT("Using quad speaker setup"));*/                         break; //ocd anyone?
		case KSAUDIO_SPEAKER_2POINT1:          /* Log(TEXT("Using 2.1 speaker setup"));*/                           break;
		case KSAUDIO_SPEAKER_3POINT1:           /*Log(TEXT("Using 3.1 speaker setup"));*/                          break;
		case KSAUDIO_SPEAKER_4POINT1:           /*Log(TEXT("Using 4.1 speaker setup"));*/                           break;
		case KSAUDIO_SPEAKER_SURROUND:          /*Log(TEXT("Using basic surround speaker setup"));*/                break;
		case KSAUDIO_SPEAKER_5POINT1:           /*Log(TEXT("Using 5.1 speaker setup"));*/                           break;
		case KSAUDIO_SPEAKER_5POINT1_SURROUND:  /*Log(TEXT("Using 5.1 surround speaker setup"));*/                  break;
		case KSAUDIO_SPEAKER_7POINT1:           /*Log(TEXT("Using 7.1 speaker setup"));*/                           break;
		case KSAUDIO_SPEAKER_7POINT1_SURROUND:  /*Log(TEXT("Using 7.1 surround speaker setup"));*/                  break;
		default:
			//Log(TEXT("Using unknown speaker setup: 0x%lX, %d channels"), inputChannels, inputChannelMask);
			inputChannelMask = 0;
			break;
		}

		if (inputChannelMask == 0)
		{
			switch (inputChannels)
			{
			case 3: inputChannelMask = KSAUDIO_SPEAKER_2POINT1; break;
			case 4: inputChannelMask = KSAUDIO_SPEAKER_QUAD;    break;
			case 5: inputChannelMask = KSAUDIO_SPEAKER_4POINT1; break;
			case 6: inputChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
			case 8: inputChannelMask = KSAUDIO_SPEAKER_7POINT1; break;
			default:
				//BUTEL_THORWERROR("Unknown speaker setup, no downmixer available.");
				break;
			}
		}
	}
}


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

void IBaseAudio::AddAudioSegment(AudioSegment *newSegment, float curVolume, bool bLiveAdd)
{
	if (newSegment && curVolume != 1.0f)
		MultiplyAudioBuffer(newSegment->audioData.Array(), newSegment->audioData.Num(), curVolume*sourceVolume);

	if (newSegment)
	{
		if (bLiveAdd)
			audioSegments << newSegment;
		else
		{
			audioSegmentsLoacl << newSegment;
		}
	}
}

void IBaseAudio::SortAudio(QWORD timestamp)
{
	QWORD jumpAmount = 0;

	if (audioSegments.Num() <= 1)
		return;

	lastUsedTimestamp = lastSentTimestamp = audioSegments.Last()->timestamp = timestamp;
	bool bReprot = false;
	bool bReprotNomal = false;
	for (UINT i = audioSegments.Num() - 1; i > 0; i--)
	{
		AudioSegment *segment = audioSegments[i - 1];
		UINT frames = segment->audioData.Num() / 2;
		double totalTime = double(frames) / double(OutputsampleRateHz)*1000.0;//-----
		QWORD newTime = timestamp - QWORD(totalTime);

		if (newTime < segment->timestamp)
		{
			QWORD newAmount = (segment->timestamp - newTime);
			if (newAmount > jumpAmount)
			{
				jumpAmount = newAmount;
				if (!bReprotNomal)
				{
					//Log(L"%s 设备时间戳跳动值加大到%llu 计算时间戳%llu ,当前时间戳%llu", GetDeviceName(), newAmount, newTime, segment->timestamp);
				}
				bReprotNomal = true;
			}
			if (newAmount > 20000)
			{
				if (!bReprot)
				{
					//Log(L"%s 设备时间戳跳动值加大到%llu 计算时间戳%llu ,当前时间戳%llu，值大于20000，不正常，返回70", GetDeviceName(), newAmount, newTime, segment->timestamp);
				}
				newAmount = 70;
				bReprot = true;
			}
			segment->timestamp = newTime;
		}

		timestamp = segment->timestamp;
	}

	//if (jumpAmount && sstri(GetDeviceName(), L"avermedia") != NULL)
	//    Log(L"sorted, lastUsedTimestamp is now %llu", lastUsedTimestamp);

	if (jumpAmount > MoreVariables->jumpRange)
	{
		//Log(L"ooh, more variables, I mean, device %s, range %llu", GetDeviceName(), jumpAmount);
		MoreVariables->jumpRange = jumpAmount;
	}
}

void IBaseAudio::SetSampleRateHz(UINT sampleRateHz)
{
	OutputsampleRateHz = sampleRateHz;

	InitAudioData(bFloat, inputChannels, inputSamplesPerSec, inputBitsPerSample, inputBlockSize, inputChannelMask, true);
}

void IBaseAudio::SetPlayLocal(bool bPlay)
{
	bPlayLocal = bPlay;
}

int IBaseAudio::GetTimeOffset() const
{
	return OffsetTime;
}

void IBaseAudio::SetTimeOffset(int newOffset)
{
	OffsetTime = newOffset;
}

void IBaseAudio::SetVolume(float fVal)
{
	Volume = fVal;
}

float IBaseAudio::GetVolume() const
{
	return Volume;
}

UINT IBaseAudio::QueryAudio(float curVolume, bool LiveQuery, bool bCanBurstHack /*= false*/)
{
	LPVOID buffer;
	UINT numAudioFrames;
	QWORD newTimestamp;

	if (GetNextBuffer((void**)&buffer, &numAudioFrames, &newTimestamp, LiveQuery))
	{
		//------------------------------------------------------------
		// convert to float
		float *captureBuffer;
		List<float> &TemcoverBuffer = LiveQuery ? convertBuffer : convertBufferLocal;
		if (!bFloat)
		{
			UINT totalSamples = numAudioFrames*inputChannels;
			if (TemcoverBuffer.Num() < totalSamples)
				TemcoverBuffer.SetSize(totalSamples);

			if (inputBitsPerSample == 8)
			{
				float *tempConvert = TemcoverBuffer.Array();
				char *tempSByte = (char*)buffer;

				while (totalSamples--)
				{
					*(tempConvert++) = float(*(tempSByte++)) / 127.0f;
				}
			}
			else if (inputBitsPerSample == 16)
			{
				float *tempConvert = TemcoverBuffer.Array();
				short *tempShort = (short*)buffer;

				while (totalSamples--)
				{
					*(tempConvert++) = float(*(tempShort++)) / 32767.0f;
				}
			}
			else if (inputBitsPerSample == 24)
			{
				float *tempConvert = TemcoverBuffer.Array();
				BYTE *tempTriple = (BYTE*)buffer;
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
			else if (inputBitsPerSample == 32)
			{
				float *tempConvert = TemcoverBuffer.Array();
				long *tempShort = (long*)buffer;

				while (totalSamples--)
				{
					*(tempConvert++) = float(double(*(tempShort++)) / 2147483647.0);
				}
			}

			captureBuffer = TemcoverBuffer.Array();
		}
		else
			captureBuffer = (float*)buffer;

		//------------------------------------------------------------
		// channel upmix/downmix

		List<float> &TemBuff = LiveQuery ? tempBuffer : tempBufferLocal;

		if (TemBuff.Num() < numAudioFrames * 2)
			TemBuff.SetSize(numAudioFrames * 2);

		float *dataOutputBuffer = TemBuff.Array();
		float *tempOut = dataOutputBuffer;

		if (inputChannels == 1)
		{
			UINT  numFloats = numAudioFrames;
			float *inputTemp = (float*)captureBuffer;
			float *outputTemp = dataOutputBuffer;

			if ((UPARAM(inputTemp) & 0xF) == 0 && (UPARAM(outputTemp) & 0xF) == 0)
			{
				UINT alignedFloats = numFloats & 0xFFFFFFFC;
				for (UINT i = 0; i < alignedFloats; i += 4)
				{
					__m128 inVal = _mm_load_ps(inputTemp + i);

					__m128 outVal1 = _mm_unpacklo_ps(inVal, inVal);
					__m128 outVal2 = _mm_unpackhi_ps(inVal, inVal);

					_mm_store_ps(outputTemp + (i * 2), outVal1);
					_mm_store_ps(outputTemp + (i * 2) + 4, outVal2);
				}

				numFloats -= alignedFloats;
				inputTemp += alignedFloats;
				outputTemp += alignedFloats * 2;
			}

			while (numFloats--)
			{
				float inputVal = *inputTemp;
				*(outputTemp++) = inputVal;
				*(outputTemp++) = inputVal;

				inputTemp++;
			}
		}
		else if (inputChannels == 2) //straight up copy
		{
			memcpy(dataOutputBuffer, captureBuffer, numAudioFrames * 2 * sizeof(float));
		}
		else
		{
			//todo: downmix optimization, also support for other speaker configurations than ones I can merely "think" of.  ugh.
			float *inputTemp = (float*)captureBuffer;
			float *outputTemp = dataOutputBuffer;

			if (inputChannelMask == KSAUDIO_SPEAKER_QUAD)
			{
				UINT numFloats = numAudioFrames * 4;
				float *endTemp = inputTemp + numFloats;

				while (inputTemp < endTemp)
				{
					float left = inputTemp[0];
					float right = inputTemp[1];
					float rearLeft = inputTemp[2] * surroundMix4;
					float rearRight = inputTemp[3] * surroundMix4;

					// When in doubt, use only left and right .... and rear left and rear right :) 
					// Same idea as with 5.1 downmix

					*(outputTemp++) = (left + rearLeft)  * attn4dotX;
					*(outputTemp++) = (right + rearRight) * attn4dotX;

					inputTemp += 4;
				}
			}
			else if (inputChannelMask == KSAUDIO_SPEAKER_2POINT1)
			{
				UINT numFloats = numAudioFrames * 3;
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
			else if (inputChannelMask == KSAUDIO_SPEAKER_3POINT1)
			{
				UINT numFloats = numAudioFrames * 4;
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
			else if (inputChannelMask == KSAUDIO_SPEAKER_4POINT1)
			{
				UINT numFloats = numAudioFrames * 5;
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
			else if (inputChannelMask == KSAUDIO_SPEAKER_SURROUND)
			{
				UINT numFloats = numAudioFrames * 4;
				float *endTemp = inputTemp + numFloats;

				while (inputTemp < endTemp)
				{
					float left = inputTemp[0];
					float right = inputTemp[1];
					float frontCenter = inputTemp[2];
					float rearCenter = inputTemp[3];

					// When in doubt, use only left and right :) Seriously.
					// THIS NEEDS TO BE PROPERLY IMPLEMENTED!

					*(outputTemp++) = left;
					*(outputTemp++) = right;

					inputTemp += 4;
				}
			}
			// Both speakers configs share the same format, the difference is in rear speakers position 
			// See: http://msdn.microsoft.com/en-us/library/windows/hardware/ff537083(v=vs.85).aspx
			// Probably for KSAUDIO_SPEAKER_5POINT1_SURROUND we will need a different coefficient for rear left/right
			else if (inputChannelMask == KSAUDIO_SPEAKER_5POINT1 || inputChannelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)
			{
				UINT numFloats = numAudioFrames * 6;
				float *endTemp = inputTemp + numFloats;

				while (inputTemp < endTemp)
				{
					float left = inputTemp[0];
					float right = inputTemp[1];
					float center = inputTemp[2] * centerMix;

					//We don't need LFE channel so skip it (see below)
					//float lowFreq   = inputTemp[3]*lowFreqMix;

					float rearLeft = inputTemp[4] * surroundMix;
					float rearRight = inputTemp[5] * surroundMix;

					// According to ITU-R  BS.775-1 recommendation, the downmix from a 3/2 source to stereo
					// is the following:
					// L = FL + k0*C + k1*RL
					// R = FR + k0*C + k1*RR
					// FL = front left
					// FR = front right
					// C  = center
					// RL = rear left
					// RR = rear right
					// k0 = centerMix   = dbMinus3 = 0.7071067811865476 [for k0 we can use dbMinus6 = 0.5 too, probably it's better]
					// k1 = surroundMix = dbMinus3 = 0.7071067811865476

					// The output (L,R) can be out of (-1,1) domain so we attenuate it [ attn5dot1 = 1/(1 + centerMix + surroundMix) ]
					// Note: this method of downmixing is far from "perfect" (pretty sure it's not the correct way) but the resulting downmix is "okayish", at least no more bleeding ears.
					// (maybe have a look at http://forum.doom9.org/archive/index.php/t-148228.html too [ 5.1 -> stereo ] the approach seems almost the same [but different coefficients])


					// http://acousticsfreq.com/blog/wp-content/uploads/2012/01/ITU-R-BS775-1.pdf
					// http://ir.lib.nctu.edu.tw/bitstream/987654321/22934/1/030104001.pdf

					*(outputTemp++) = (left + center + rearLeft) * attn5dot1;
					*(outputTemp++) = (right + center + rearRight) * attn5dot1;

					inputTemp += 6;
				}
			}

			// According to http://msdn.microsoft.com/en-us/library/windows/hardware/ff537083(v=vs.85).aspx
			// KSAUDIO_SPEAKER_7POINT1 is BLiveolete and no longer supported in Windows Vista and later versions of Windows
			// Not sure what to do about it, meh , drop front left of center/front right of center -> 5.1 -> stereo; 

			else if (inputChannelMask == KSAUDIO_SPEAKER_7POINT1)
			{
				UINT numFloats = numAudioFrames * 8;
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

			// Downmix to 5.1 (easy stuff) then downmix to stereo as done in KSAUDIO_SPEAKER_5POINT1
			else if (inputChannelMask == KSAUDIO_SPEAKER_7POINT1_SURROUND)
			{
				UINT numFloats = numAudioFrames * 8;
				float *endTemp = inputTemp + numFloats;

				while (inputTemp < endTemp)
				{
					float left = inputTemp[0];
					float right = inputTemp[1];
					float center = inputTemp[2] * centerMix;

					// Skip LFE we don't need it
					//float lowFreq   = inputTemp[3]*lowFreqMix;

					float rearLeft = inputTemp[4];
					float rearRight = inputTemp[5];
					float sideLeft = inputTemp[6];
					float sideRight = inputTemp[7];

					// combine the rear/side channels first , baaam! 5.1
					rearLeft = (rearLeft + sideLeft)  * 0.5f;
					rearRight = (rearRight + sideRight) * 0.5f;

					// downmix to stereo as in 5.1 case
					*(outputTemp++) = (left + center + rearLeft  * surroundMix) * attn5dot1;
					*(outputTemp++) = (right + center + rearRight * surroundMix) * attn5dot1;

					inputTemp += 8;
				}
			}
		}

		ReleaseBuffer();

		//------------------------------------------------------------
		// resample
		List<float> &TemResampleBuff = LiveQuery ? tempResampleBuffer : tempResampleBufferLocal;
		if (bResample)
		{
			UINT frameAdjust = UINT((double(numAudioFrames) * resampleRatio) + 1.0);
			UINT newFrameSize = frameAdjust * 2;

			if (TemResampleBuff.Num() < newFrameSize)
				TemResampleBuff.SetSize(newFrameSize);

			SRC_DATA data;
			data.src_ratio = resampleRatio;

			data.data_in = TemBuff.Array();
			data.input_frames = numAudioFrames;

			data.data_out = TemResampleBuff.Array();
			data.output_frames = frameAdjust;

			data.end_of_input = 0;

			//是否需要加锁？
			int err = 0;
			if (LiveQuery)
				err = src_process(MoreVariables->resampler, &data);
			else
			{
				err = src_process(MoreVariables->resamplerLocal, &data);
			}
			if (err)
			{
				//RUNONCE AppWarning(TEXT("AudioSource::QueryAudio: Was unable to resample audio for device '%s'"), GetDeviceName());
				return NoAudioAvailable;
			}

			if (data.input_frames_used != numAudioFrames)
			{
				//RUNONCE AppWarning(TEXT("AudioSource::QueryAudio: Failed to downsample buffer completely, which shouldn't actually happen because it should be using 10ms of samples"));
				return NoAudioAvailable;
			}

			numAudioFrames = data.output_frames_gen;
		}

		//------------------------------------------------------
		// timestamp smoothing (keep audio within 70ms of target time)

		QWORD &TemTimestamp = LiveQuery ? lastUsedTimestamp : lastUsedTimestampLocal;

		if (!TemTimestamp)
			TemTimestamp = newTimestamp;
		else
			TemTimestamp += 10;

		QWORD difVal = GetQWDif(newTimestamp, TemTimestamp);

		if (difVal >= MoreVariables->jumpRange) {
			TemTimestamp = newTimestamp;
		}
		lastGetTimestamp = newTimestamp;

		float *newBuffer = (bResample) ? TemResampleBuff.Array() : TemBuff.Array();

		QWORD &TemLastTimestamp = LiveQuery ? lastSentTimestamp : lastSentTimestampLocal;

		bool overshotAudio = (TemTimestamp < TemLastTimestamp + 10);
		if (bCanBurstHack || !overshotAudio)
		{
			AudioSegment *newSegment = new AudioSegment(newBuffer, numAudioFrames * 2, TemTimestamp);

			AddAudioSegment(newSegment, curVolume*sourceVolume,LiveQuery);
			TemLastTimestamp = TemTimestamp;
		}
		else
		{
			//Log(L"11111111111 %s音频丢弃 lastSentTimestamp《%d》newTimestamp《%d》 ，个数%d,", GetDeviceName(), lastSentTimestamp, newTimestamp, audioSegments.Num());
		}

		//-----------------------------------------------------------------------------

		return AudioAvailable;
	}

	return NoAudioAvailable;
}


bool IBaseAudio::GetBuffer(float **buffer, QWORD targetTimestamp, bool bLiveGet)
{
	bool bSuccess = false;
	bool bDeleted = false;

	List<float> &outputBufferTem = bLiveGet ? outputBuffer : outputBufferLocal;
	outputBufferTem.Clear();

	bool bReportedOnce = false;
	QWORD timestamp = 0;
	int  nDeletedCnt = 0;
	QWORD timestampDel = 0;

	List<AudioSegment*> &TmeAudioSegment = bLiveGet ? audioSegments : audioSegmentsLoacl;


	while (TmeAudioSegment.Num())
	{
		timestamp = TmeAudioSegment[0]->timestamp;
		if (TmeAudioSegment[0]->timestamp < targetTimestamp)
		{
			QWORD diff = targetTimestamp - TmeAudioSegment[0]->timestamp;

			if (!bReportedOnce)
			{
				bReportedOnce = true;
			}

			if (0 == timestampDel)
			{
				timestampDel = timestamp;
			}
			delete TmeAudioSegment[0];
			TmeAudioSegment.Remove(0);
			nDeletedCnt++;
			bDeleted = true;
		}
		else
			break;
	}
	if (nDeletedCnt > 0)
	{
		Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:音频设备落后音频编码时钟%llu,删除时间戳开始%llu，删除个数 %d 当前音频帧个数%d,当前编码时钟%llu",
			targetTimestamp,
			timestampDel,
			nDeletedCnt,
			TmeAudioSegment.Num(),
			LastTimeTimeStamp
			);
	}
	if (TmeAudioSegment.Num())
	{
		bool bUseSegment = false;

		AudioSegment *segment = TmeAudioSegment[0];

		QWORD difference = (segment->timestamp - targetTimestamp);
		if (bDeleted || difference <= 11)
		{
			//Log(TEXT("segment.timestamp: %llu, targetTimestamp: %llu"), segment.timestamp, targetTimestamp);
			outputBufferTem.TransferFrom(segment->audioData);

			delete segment;
			TmeAudioSegment.Remove(0);

			bSuccess = true;
		}
	}

	outputBufferTem.SetSize(OutputsampleRateHz / 100 * 2);

	*buffer = outputBufferTem.Array();
	return bSuccess;
}

void IBaseAudio::SetAudioDBCallBack(AudioDBCallBack DBCb)
{
	AudioDbCB = DBCb;
}

bool IBaseAudio::GetLatestTimestamp(QWORD &timestamp)
{
	if (audioSegments.Num())
	{
		timestamp = audioSegments.Last()->timestamp;
		return true;
	}

	return false;
}

void IBaseAudio::StartRenderAStream(const char* cRenderAudioDevice)
{
	RenderDevice = cRenderAudioDevice;
	bPlayLocal = true;
}

void IBaseAudio::StopRenderAStream()
{
	bPlayLocal = false;
}

void IBaseAudio::SetLastTimeStamp(QWORD &TimeStamp)
{
	LastTimeTimeStamp = TimeStamp;
}


void IBaseAudio::SetLastTimeStamp_Local(QWORD &TimeStamp)
{
	LastTimeTimeStamp_Local = TimeStamp;
}

void IBaseAudio::SetAudioParam(float LVolumeQuotiety, float RVolumeQuotiety, float PVolumeQuotiety, bool bPlayPcmLocal, bool bPlayPcmLive, float quotietyVolume, bool Projector)
{
	leftdesktopVol = LVolumeQuotiety;
	rightdesktopVol = RVolumeQuotiety;
	desktopVol = PVolumeQuotiety;
	m_bPlayPcmLocal = bPlayPcmLocal;
	m_bPlayPcmLive = bPlayPcmLive;
	m_quotietyVolume = quotietyVolume;
	bProjector = Projector;
}

void IBaseAudio::VolumeCaculate(char* buf, UINT32 size, double vol)
{
	if (!size || !buf)
	{
		return;
	}

	short *pShort = (short*)buf;
	signed long dwData;
	for (int i = 0; i < size; i += 2)
	{
		dwData = pShort[i / 2];

		dwData *= vol;
		if (dwData < -0x8000)
		{
			dwData = -0x8000;
		}
		else if (dwData > 0x7FFF)
		{
			dwData = 0x7FFF;
		}

		*(short*)(buf + i) = (short)dwData;
		//memcpy(&buf[i], &dwData, sizeof(short));
	}
}


void IBaseAudio::CaculateVolume(LPVOID pBuffer, int& numAudioFrames, void **OutBuffer,bool bOnlyCallBack)
{
	float desktopVolGain = 0, leftdesktopVolGain = 0, rightdesktopVolGain = 0;

	if (bFloat)
	{
		if (leftaudioDataf.Num() < numAudioFrames)
		{
			leftaudioDataf.SetSize(numAudioFrames);
		}

		if (rightaudioDataf.Num() < numAudioFrames)
		{
			rightaudioDataf.SetSize(numAudioFrames);
		}
	}
	else
	{
		if (leftaudioData.Num() < numAudioFrames)
		{
			leftaudioData.SetSize(numAudioFrames);
		}

		if (rightaudioData.Num() < numAudioFrames)
		{
			rightaudioData.SetSize(numAudioFrames);
		}
	}
	

	char *TemChar = NULL;
	float *TemFloat = NULL;
// 	if (1 == inputChannels)
// 	{
// 		if (OutputaudioData.Num() < numAudioFrames * 2)
// 		{
// 			OutputaudioData.SetSize(numAudioFrames * 2);
// 		}
// 
// 		char *pTemChar = (char*)pBuffer;
// 
// 		for (int index = 0, iCount = 0; index < numAudioFrames; index += 2, iCount += 4)
// 		{
// 			memcpy(&OutputaudioData[iCount], &pTemChar[index], 2);
// 			memcpy(&OutputaudioData[iCount + 2], &pTemChar[index], 2);
// 		}
// 
// 		TemChar = OutputaudioData.Array();
// 		numAudioFrames *= 2;
// 	}
// 	else
	{
		if (bFloat)
		{
			TemFloat = (float*)pBuffer;
		}
		else
		{
			TemChar = (char*)pBuffer;
		}
		
	}

	UINT SampleSize = inputSamplesPerSec;

	//if (AudioDbCB)
	{
		if (bFloat)
		{
			SampleSize = SampleSize * 4 / 1000;
			audioFramesUpdate += numAudioFrames * sizeof(float) / 2;
		}
		else
		{
			SampleSize = SampleSize * 2 / 1000;
			audioFramesUpdate += numAudioFrames / 2;
		}
	}
	
	if (/*AudioDbCB && */audioFramesUpdate >= SampleSize * 40)//40ms计算一次
	{
		float LDb = -96, RDb = -96;
		if (bFloat)
		{
			//将左右声道拆开
			for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 2, iCount += 1)
			{
				memcpy(&leftaudioDataf[iCount], &TemFloat[iIndex], 4);
				memcpy(&rightaudioDataf[iCount], &TemFloat[iIndex + 1], 4);
			}
			//计算左右声道分贝值
			CalculateVolumeLevelsFloat(leftaudioDataf.Array(), numAudioFrames / 2, LDb);
			CalculateVolumeLevelsFloat(rightaudioDataf.Array(), numAudioFrames / 2, RDb);
		}
		else
		{
			if (2 == inputChannels)
			{
				//将左右声道拆开
				for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 4, iCount += 2)
				{
					memcpy(&leftaudioData[iCount], &TemChar[iIndex], 2);
					memcpy(&rightaudioData[iCount], &TemChar[iIndex + 2], 2);
				}
				//计算左右声道分贝值
				CalculateVolumeLevelsShort(leftaudioData.Array(), numAudioFrames / 2, LDb);
				CalculateVolumeLevelsShort(rightaudioData.Array(), numAudioFrames / 2, RDb);
			}
			else
			{
				CalculateVolumeLevelsShort(TemChar, numAudioFrames, LDb);
				RDb = LDb;
			}
		}

		//AudioDbCB((uint64_t)this, toDB(LeftDb), toDB(RightDb));

		LeftDb = toDB(LDb);
		RightDb = toDB(RDb);

		audioFramesUpdate = 0;
	}

	if (bOnlyCallBack)
		return;

	if (desktopVol > 1.0)
	{
		desktopVolGain = desktopVol - 1.0;
	}

	if (desktopVol + desktopVolGain*m_quotietyVolume != 1)
	{
		if (bFloat)
		{
			MultiplyAudioBuffer(TemFloat, numAudioFrames, desktopVol + desktopVolGain*m_quotietyVolume);

// 			for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 2, iCount += 1){
// 				memcpy(&leftaudioDataf[iCount], &TemFloat[iIndex], 4);
// 				memcpy(&rightaudioDataf[iCount], &TemFloat[iIndex + 1], 4);
// 			}
		}
		else
		{
			VolumeCaculate(TemChar, numAudioFrames, desktopVol + desktopVolGain*m_quotietyVolume);

// 			for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 4, iCount += 2){
// 				memcpy(&leftaudioData[iCount], &TemChar[iIndex], 2);
// 				memcpy(&rightaudioData[iCount], &TemChar[iIndex + 2], 2);
// 			}
		}

	}

	if (bFloat)
	{
// 		for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 2, iCount += 1){
// 			memcpy(&TemFloat[iIndex], &leftaudioDataf[iCount], 4);
// 			memcpy(&TemFloat[iIndex + 1], &rightaudioDataf[iCount], 4);
// 		}

		*OutBuffer = TemFloat;
	}
	else
	{
// 		for (int iIndex = 0, iCount = 0; iIndex < numAudioFrames; iIndex += 4, iCount += 2){
// 			memcpy(&TemChar[iIndex], &leftaudioData[iCount], 2);
// 			memcpy(&TemChar[iIndex + 2], &rightaudioData[iCount], 2);
// 		}

		*OutBuffer = TemChar;
	}

}

void IBaseAudio::CalculateVolumeLevelsShort(char *buffer, int totalFloats, float &RMS)
{
	float sum = 0.0f;
	int totalFloatsStore = totalFloats;

	float Max = 0.0f;

	if ((UPARAM(buffer) & 0xF) == 0)
	{
		UINT alignedFloats = totalFloats & 0xFFFFFFF8;
		__m128 sseValue;
		__m128 sseSquares;
		for (UINT i = 0; i < alignedFloats; i += 8)
		{
			short *tempShort = (short*)&buffer[i];

			sseValue = _mm_set_ps(*tempShort / 32767.0f, *(++tempShort) / 32767.0f, *(++tempShort) / 32767.0f, *(++tempShort) / 32767.0f);

			/*compute squares and add them to the sum*/
			sseSquares = _mm_mul_ps(sseValue, sseValue);
			sum += sseSquares.m128_f32[0] + sseSquares.m128_f32[1] + sseSquares.m128_f32[2] + sseSquares.m128_f32[3];
		}

		buffer += alignedFloats;
		totalFloats -= alignedFloats;
	}

	for (int i = 0; i < totalFloats; i += 2)
	{
		float val = ((short)buffer[i]) / 32767.0f;
		sum += val * val;
	}

	RMS = sqrt(sum / totalFloatsStore / 2);
}

void IBaseAudio::CalculateVolumeLevelsFloat(float *buffer, int totalFloats, float &RMS)
{
	float sum = 0.0f;
	int totalFloatsStore = totalFloats;

	float Max = 0.0f;

	if ((UPARAM(buffer) & 0xF) == 0)
	{
		UINT alignedFloats = totalFloats & 0xFFFFFFFC;
		__m128 sseScaledVals;
		__m128 sseSquares;
		for (UINT i = 0; i<alignedFloats; i += 4)
		{
			sseScaledVals = _mm_load_ps(buffer + i);

			/*compute squares and add them to the sum*/
			sseSquares = _mm_mul_ps(sseScaledVals, sseScaledVals);
			sum += sseSquares.m128_f32[0] + sseSquares.m128_f32[1] + sseSquares.m128_f32[2] + sseSquares.m128_f32[3];
		}

		buffer += alignedFloats;
		totalFloats -= alignedFloats;
	}

	for (int i = 0; i<totalFloats; i++)
	{
		float val = buffer[i];
		float pow2Val = val * val;
		sum += pow2Val;
		Max = max(Max, pow2Val);
	}

	RMS = sqrt(sum / totalFloatsStore);
}

float IBaseAudio::toDB(float RMS)
{
	float db = 20.0f * log10(RMS);
	if (!_finite(db))
		return VOL_MIN;
	return db;
}

void IBaseAudio::ResetAudioDB()
{
	if (AudioDbCB)
		AudioDbCB((uint64_t)this, -96.0f, -96.0f);
}

void IBaseAudio::SetLiveInstance(bool bLiveInstance)
{
	if (bLiveInstance)
	{
		CSLiveManager::GetInstance()->AddPreviewInstanceAudio(this);
	}
	else
	{
		CSLiveManager::GetInstance()->RemovePreviewInstanceAudio(this);
	}
}

void IBaseAudio::SetPlayPreview(bool bPlay)
{
	m_bPlayPre = bPlay;
}

void IBaseAudio::GetDb(float &LeftDb, float &RightDb)
{
	LeftDb = this->LeftDb;
	RightDb = this->RightDb;
}

