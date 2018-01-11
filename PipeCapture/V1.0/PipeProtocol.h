#ifndef PIPEPROTOCOL_H
#define PIPEPROTOCOL_H
#include"DataStream.h"

#define DATAPIPENAME TEXT("\\\\.\\pipe\\Pipe_Butel_InteractionClient_DATA_")

#define	VIDEODATA        0x01
#define	AUDIODATA        0x02
#define	CONNECTEDSTATE   0x03

struct MsgHeader
{
	unsigned int Type;
};

struct VideoDataInfo
{
	int Width;
	int Height;
	int FrameRate;
	VideoType VType;
	int Rotate;
	long long Timestamp;
	int Datalen;
	int Bpp;
};

struct AudioDataInfo
{
	int channels;
	int samplerate;
	int ampleperbits;
	AudioType AType;
	long long Timestamp;
	int Datalen;
};

#define PIPECLIENT  0x01

struct PipeMessage
{
	unsigned int type;
	WCHAR PipeName[64];
};

#endif