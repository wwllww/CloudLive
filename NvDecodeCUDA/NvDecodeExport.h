#pragma once

#include "NvCodecDef.h"

#ifdef  NVDECODE_EXPORTS
#define  EXTERN_DLLEXPORT  extern "C" __declspec(dllexport)
#else
#define  EXTERN_DLLEXPORT  extern "C" __declspec(dllimport)
#endif

EXTERN_DLLEXPORT void* NvDecodeCreate(int nWidth, int hHeight);

EXTERN_DLLEXPORT int NvDecodeDestroy(void* hDecode);

EXTERN_DLLEXPORT void NvSendPacket(void* hEncode, InputPacket *pInputPacket);

EXTERN_DLLEXPORT void NvReceiveFrame(void* hDecode, OutFrame **ppOutFrame);

EXTERN_DLLEXPORT int QueryHardDecodeSupport();

