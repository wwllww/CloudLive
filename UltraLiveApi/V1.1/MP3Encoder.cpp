#include "Encoder.h"
#include "OperatNew.h"
#include "Error.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

#include "../../lame/include/lame.h"
const int audioBlockSize = 4; //output is 2 16bit channels


struct AACDataPacket
{
	List<BYTE> Packet;

	inline void FreeData() { Packet.Clear(); }
};

//lame is not lame..  it's godly.
class MP3Encoder : public AudioEncoder
{
	lame_global_flags *lgf;

	List<BYTE> MP3OutputBuffer;
	List<BYTE> header;
	DWORD dwMP3MaxSize;
	bool bFirstPacket;

	UINT outputFrameSize;
	UINT curBitRate;
	UINT curSampRate;
	UINT curNumChannel;

	List<QWORD> bufferedTimestamps;
	QWORD curEncodeTimestamp;
	DWORD frameCounter;
	bool bFirstFrame;

public:
	MP3Encoder(UINT bitRate, UINT SampRate, UINT NumChannel)
	{
		curBitRate = bitRate;
		curSampRate = SampRate;
		curNumChannel = NumChannel;

		lgf = lame_init();
		if (!lgf)
			CrashError(TEXT("Unable to open mp3 encoder"));

		lame_set_in_samplerate(lgf, curSampRate);
		lame_set_out_samplerate(lgf, curSampRate);
		lame_set_num_channels(lgf, curNumChannel);
		lame_set_disable_reservoir(lgf, TRUE); //bit reservoir has to be disabled for seamless streaming
		lame_set_quality(lgf, 2);
		lame_set_VBR(lgf, vbr_off);
		lame_set_brate(lgf, bitRate);
		lame_init_params(lgf);

		outputFrameSize = lame_get_framesize(lgf); //1152 usually
		dwMP3MaxSize = DWORD(1.25*double(outputFrameSize*audioBlockSize) + 7200.0);
		MP3OutputBuffer.SetSize(dwMP3MaxSize + 1);
		if (curNumChannel == 1)
		{
			MP3OutputBuffer[0] = 0x2e;
		}
		else
		{
			MP3OutputBuffer[0] = 0x2f;
		}
		

		bFirstPacket = true;
		bFirstFrame = true;
		frameCounter = 0;
		curEncodeTimestamp = 0;

		Log::writeMessage(LOG_RTSPSERV,1,"------------------------------------------");
		Log::writeMessage(LOG_RTSPSERV, 1, "%s", WcharToAnsi(GetInfoString().Array()).c_str());
	}

	~MP3Encoder()
	{
		lame_close(lgf);
	}

	bool Encode(float *input, UINT numInputFrames, DataPacket &packet, QWORD &timestamp)
	{
		if (bFirstFrame)
		{
			curEncodeTimestamp = timestamp;
			bFirstFrame = false;
		}

		//------------------------------------------------

		UINT lastSampleSize = frameCounter;

		frameCounter += numInputFrames;
		if (frameCounter > outputFrameSize)
		{
			frameCounter -= outputFrameSize;

			bufferedTimestamps << curEncodeTimestamp;
			curEncodeTimestamp = timestamp + ((outputFrameSize - lastSampleSize) * 1000 / curSampRate);
		}

		int ret = lame_encode_buffer_interleaved_ieee_float(lgf, (float*)input, numInputFrames, MP3OutputBuffer.Array() + 1, dwMP3MaxSize);

		if (ret < 0)
		{
			Log::writeError(LOG_RTSPSERV, 1, "MP3 encode failed");
			return false;
		}

		if (ret > 0)
		{
			if (bFirstPacket)
			{
				header.CopyArray(MP3OutputBuffer.Array(), ret);
				bFirstPacket = false;
				ret = 0;
			}
			else
			{
				packet.lpPacket = MP3OutputBuffer.Array();
				packet.size = ret + 1;

				timestamp = bufferedTimestamps[0];
				bufferedTimestamps.Remove(0);
			}
		}

		return ret > 0;
	}

	void SetFirstEnoder()
	{
		bFirstFrame = true;
		MP3OutputBuffer.Clear();
		bufferedTimestamps.Clear();
	}

	UINT GetFrameSize() const
	{
		return outputFrameSize;
	}

	void GetHeaders(DataPacket &packet)
	{
		packet.lpPacket = header.Array();
		packet.size = header.Num();
	}

	int GetBitRate() const { return curBitRate; }
	CTSTR GetCodec() const { return TEXT("MP3"); }
	virtual int  GetsampleRate() const { return curSampRate; }
	virtual int  GetnumChannels() const { return curNumChannel; }

	String GetInfoString() const
	{
		String strInfo;
		strInfo << TEXT("Audio Encoding: MP3") <<
			TEXT("\r\n    bitrate: ") << IntString(curBitRate);

		return strInfo;
	}
};


AudioEncoder* CreateMP3Encoder(UINT bitRate, UINT SampRate, UINT NumChannel)
{
	return new MP3Encoder(bitRate, SampRate, NumChannel);
}
