#include "BaseAfx.h"

#include "faac.h"
#include "OperatNew.h"
#include "Encoder.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

//AAC is pretty good, I changed my  mind
class CAACEncoder : public AudioEncoder
	{
		UINT curBitRate;

		bool bFirstPacket;
		UINT uChannel;
		UINT uSampleRateHz;
		faacEncHandle faac;
		DWORD numReadSamples;
		DWORD outputSize;
		UINT BitsPerSample;
		List<BYTE> inputBuffer;

		List<BYTE>  aacBuffer;
		List<BYTE>  header;

		List<QWORD> bufferedTimestamps;
		QWORD curEncodeTimestamp;
		bool bFirstFrame;

	public:
		CAACEncoder(UINT bitRate, UINT uSampleRateHz = 44100, UINT uChannelNums = 2, UINT BitsPerSample = 2)
		{
			curBitRate = bitRate;
			uChannel = uChannelNums;
			this->BitsPerSample = BitsPerSample;
			this->uSampleRateHz = uSampleRateHz;
			faac = faacEncOpen(uSampleRateHz, uChannelNums, &numReadSamples, &outputSize);

			//Log(TEXT("numReadSamples: %d"), numReadSamples);
			aacBuffer.SetSize(outputSize + 2);
			aacBuffer[0] = 0xaf;
			aacBuffer[1] = 0x1;

			faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(faac);
			config->bitRate = (bitRate * 1000) / uChannelNums;
			config->quantqual = 100;
			config->inputFormat = FAAC_INPUT_16BIT;
			config->mpegVersion = MPEG4;
			config->aacObjectType = LOW;
			config->useLfe = 0;
			config->outputFormat = 0;

			int ret = faacEncSetConfiguration(faac, config);
			if (!ret)
				CrashError(TEXT("aac configuration failed"));

			BYTE *tempHeader;
			DWORD len;

			header.SetSize(2);
			header[0] = 0xaf;
			header[1] = 0x00;

			faacEncGetDecoderSpecificInfo(faac, &tempHeader, &len);
			header.AppendArray(tempHeader, len);
			free(tempHeader);

			bFirstPacket = true;
			bFirstFrame = true;

			Log::writeMessage(LOG_RTSPSERV,1,"------------------------------------------");
			Log::writeMessage(LOG_RTSPSERV, 1, "%s", WcharToAnsi(GetInfoString().Array()).c_str());
		}

		~CAACEncoder()
		{
			faacEncClose(faac);
		}

		bool Encode(const unsigned char *input,int inputSize, DataPacket &packet, QWORD &timestamp)
		{
			if (bFirstFrame)
			{
				curEncodeTimestamp = timestamp;
				bFirstFrame = false;
			}

			//------------------------------------------------
			
			QWORD curTimestamp = timestamp;

			UINT lastSampleSize = inputBuffer.Num() / BitsPerSample;
			inputBuffer.AppendArray(input, inputSize);
			int ret = 0;
			
			if (inputBuffer.Num() / BitsPerSample >= numReadSamples)
			{				
				ret = faacEncEncode(faac, (int32_t*)inputBuffer.Array(), numReadSamples, aacBuffer.Array() + 2, outputSize);
				if (ret > 0)
				{
					if (bFirstPacket)
					{
						bFirstPacket = false;
						ret = 0;
					}
					else
					{
						packet.lpPacket = aacBuffer.Array();
						packet.size = ret + 2;
						timestamp = bufferedTimestamps[0];
						bufferedTimestamps.Remove(0);					
					}
				}
				else if (ret < 0)
					Log::writeError(LOG_RTSPSERV, 1, "aac encode error");

				inputBuffer.RemoveRange(0, numReadSamples * BitsPerSample);

				bufferedTimestamps << curEncodeTimestamp;
				curEncodeTimestamp = curTimestamp + (((numReadSamples - lastSampleSize) / uChannel) * 1000 / uSampleRateHz);
			}

			return ret > 0;
		}

		virtual bool Encode(float *input, UINT numInputFrames, DataPacket &packet, QWORD &timestamp)
		{
			return false;
		}
		UINT GetFrameSize() const
		{
			return 1024;
		}

		void GetHeaders(DataPacket &packet)
		{
			packet.lpPacket = header.Array();
			packet.size = header.Num();
		}

		int GetBitRate() const { return curBitRate; }
		CTSTR GetCodec() const { return TEXT("AAC"); }

		String GetInfoString() const
		{
			String strInfo;
			strInfo << TEXT("Audio Encoding: AAC") <<
				TEXT("\r\n    bitrate: ") << IntString(curBitRate);

			return strInfo;
		}

		virtual int  GetsampleRate() const
		{
			return uSampleRateHz;
		}
		virtual int  GetnumChannels() const
		{
			return uChannel;
		}
		virtual void  SetFirstEnoder(){
			bFirstFrame = true;
			inputBuffer.Clear();
			bufferedTimestamps.Clear();
		}
	};


	AudioEncoder* CreateAACEncoderNew(UINT bitRate, UINT SampRate, UINT NumChannel, UINT BitsPerSample)
	{
		return new CAACEncoder(bitRate, SampRate, NumChannel, BitsPerSample);
	}

