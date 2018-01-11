#pragma once

#include "nvEncodeAPI.h"

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
	unsigned char *pFrame;						
	int frameLen;							
	uint64_t ts;							
	bool is_key;	
	NV_ENC_PIC_TYPE pictureType;
}VCodecBuffer;
