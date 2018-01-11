#ifndef ENCODER_H
#define ENCODER_H
#include "common.h"
#include "Instance.h"

struct TimedPacket
{
	List<BYTE> data;
	DWORD timestamp;
	PacketType type;
};

class AudioEncoder
{

public:
	virtual bool    Encode(float *input, UINT numInputFrames, DataPacket &packet, QWORD &timestamp) = 0;
	virtual bool    Encode(const unsigned char *input, int inputSize, DataPacket &packet, QWORD &timestamp){ return false; }
	virtual void    GetHeaders(DataPacket &packet) = 0;
public:
	virtual ~AudioEncoder() {}

	virtual UINT    GetFrameSize() const = 0;

	virtual int     GetBitRate() const = 0;
	virtual CTSTR   GetCodec() const = 0;

	virtual String  GetInfoString() const = 0;
	virtual int  GetsampleRate() const = 0;
	virtual int  GetnumChannels() const = 0;
	virtual void  SetFirstEnoder() = 0;
};

//-------------------------------------------------------------------

class VideoEncoder
{
protected:

	virtual void RequestBuffers(LPVOID buffers) {}

public:
	virtual ~VideoEncoder() {}
	virtual bool Encode(LPVOID picIn, List<DataPacket> &packets, List<PacketType> &packetTypes, QWORD timestamp, DWORD &out_pts) = 0;
	virtual int  GetBitRate() const = 0;
	virtual bool DynamicBitrateSupported() const = 0;
	virtual bool SetBitRate(DWORD maxBitrate, DWORD bufferSize) = 0;
	virtual int  GetFps() const = 0;

	virtual void GetHeaders(DataPacket &packet) = 0;
	virtual void GetSEI(DataPacket &packet) {}

	virtual void RequestKeyframe() {}

	virtual String GetInfoString() const = 0;

	virtual bool isQSV() { return false; }

	virtual bool HasBufferedFrames() { return false; }
	virtual bool InitEncoder() { return false; }
	virtual void GetWH(int& Width, int& Height){};
};

VideoEncoder* CreateX264Encoder(int fps, int width, int height, int quality, CTSTR preset, CTSTR ProFile, bool bUse444, ColorDescription &colorDesc, int maxBitRate, int bufferSize, bool bUseCFR, bool bUesBackConfig);
AudioEncoder* CreateAACEncoder(UINT bitRate, UINT SampRate, UINT NumChannel);
AudioEncoder* CreateMP3Encoder(UINT bitRate, UINT SampRate, UINT NumChannel);
VideoEncoder* CreateRDX264EncoderNew(int fps, int width, int height, int quality, CTSTR preset, bool bUse444, ColorDescription &colorDesc, int maxBitRate, int bufferSize, bool bUseCFR, int ColorT = 0);
VideoEncoder* CreateNvidiaEncoder(int fps, int width, int height, int quality, CTSTR preset, bool bUseDefaultConfig, ColorDescription &colorDesc, int maxBitRate, int bufferSize, bool bUseBack, int ColorT = 0);
AudioEncoder* CreateAACEncoderNew(UINT bitRate, UINT SampRate, UINT NumChannel, UINT BitsPerSample);

#endif // !ENCODER_H
