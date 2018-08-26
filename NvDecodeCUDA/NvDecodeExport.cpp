#include "NvDecodeExport.h"
#include "NvDecode.h"

EXTERN_DLLEXPORT void* NvDecodeCreate(int nWidth, int nHeight)
{
	NvidiaDecode* pNvDecode = new NvidiaDecode;
	if (pNvDecode)
		return pNvDecode->NvDecodeCreate(nWidth, nHeight);
	else
		return nullptr;
}

EXTERN_DLLEXPORT int NvDecodeDestroy(void* hDecode)
{
	int status = ((NvidiaDecode*)hDecode)->NvDecodeDestroy();
	if (hDecode)
	{
		delete (NvidiaDecode*)hDecode;
		hDecode = nullptr;
	}
	return status;
}

EXTERN_DLLEXPORT void NvSendPacket(void* hDecode, InputPacket* pInputPacket)
{
	return ((NvidiaDecode*)hDecode)->NvSendPacket(pInputPacket);
}

EXTERN_DLLEXPORT void NvReceiveFrame(void* hDecode, OutFrame** ppOutFrame)
{
	return ((NvidiaDecode*)hDecode)->NvReceiveFrame(ppOutFrame);
}

EXTERN_DLLEXPORT int QueryHardDecodeSupport()
{
	int nRet = -1;
	NvidiaDecode* pNvDecode = new NvidiaDecode;
	if (pNvDecode)
	{
		nRet = pNvDecode->QueryHardDecodeSupport();
		delete pNvDecode;
		pNvDecode = nullptr;
	}
	return nRet;
}

