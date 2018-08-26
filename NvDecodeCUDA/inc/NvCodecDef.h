#pragma once

#include "nvEncodeAPI.h"
#include <d3d11.h>

typedef struct _EncodeConfig
{
	int              width;
	int              height;
	int              maxWidth;
	int              maxHeight;
	int              fps;
	int              bitrate;
	int              vbvMaxBitrate;
	int              vbvSize;
	int              rcMode;
	int              qp;
	float            i_quant_factor;
	float            b_quant_factor;
	float            i_quant_offset;
	float            b_quant_offset;
	GUID             presetGUID;
	int              codec;
	int              invalidateRefFramesEnableFlag;
	int              intraRefreshEnableFlag;
	int              intraRefreshPeriod;
	int              intraRefreshDuration;
	int              deviceType;
	int              startFrameIdx;
	int              endFrameIdx;
	int              gopLength;
	int              numB;
	int              pictureStruct;
	int              deviceID;
	NV_ENC_BUFFER_FORMAT inputFormat;
	NV_ENC_BUFFER_FORMAT RealInputFormat;
	int              generateQpDeltaMap;
	char            *qpDeltaMapFile;
	char* inputFileName;
	char* outputFileName;
	char* externalHintInputFile;
	char* encoderPreset;
	char* inputFilePath;
	char* encCmdFileName;
	int   enableMEOnly;
	int   enableAsyncMode;
	int   preloadedFrameCount;
	int   enableTemporalAQ;
	int   enableExternalMEHint;
}EncodeConfig;

typedef struct _VCodecBuffer
{
	ID3D11Texture2D	 *pTexture;
	unsigned char *pFrame;
	int frameLen;							
	uint64_t ts;							
	bool is_key;	
	NV_ENC_PIC_TYPE pictureType;
}VCodecBuffer;

typedef struct _InputPacket
{
	unsigned char *pFrame;
	int frameLen;
	uint64_t dts;
	uint64_t pts;
	bool is_key;
	bool bTS;
	unsigned char* pSpspps;
	int nLenSpspps;

	int avg_frame_rate_num;
	int avg_frame_rate_den;
	int r_frame_rate_num;
	int r_frame_rate_den;
	int time_base_num;
	int time_base_den;
	bool bSeek;
	int seek_timestamp;
	uint64_t start_time;
}InputPacket;

typedef struct _OutFrame{
	int   media_type; 
	char* frame_data[8]; 
	unsigned int frame_size[8]; 
	int   pts; 
	int   duration;
	int   key_frame; 
	int   width;
	int   height;
	int   channels;
	int   nb_samples;
	int   is_resampled;
	void* av_frame;
	int   deviceHeight;
	void* hShare;
}OutFrame;
