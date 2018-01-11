#pragma once

#include "inc/NvCodecDef.h"

#ifdef  NVENCODE_EXPORTS
#define  EXTERN_DLLEXPORT  extern "C" __declspec(dllexport)
#else
#define  EXTERN_DLLEXPORT  extern "C" __declspec(dllimport)
#endif

EXTERN_DLLEXPORT void* NvEncodeCreate(EncodeConfig* pEncodeConfig);

EXTERN_DLLEXPORT int NvEncodeDestroy(void* hEncode);

EXTERN_DLLEXPORT int NvEncodeFrame(void* hEncode, VCodecBuffer* pIn, VCodecBuffer** pOut);

EXTERN_DLLEXPORT unsigned int NvGetBufferedCount(void* hEncode);