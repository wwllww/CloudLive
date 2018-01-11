#include "NvEncoderExport.h"
#include "NvEncoder.h"

EXTERN_DLLEXPORT void* NvEncodeCreate(EncodeConfig* pEncodeConfig)
{
	CNvEncoder* pCNvEncoder = new CNvEncoder;
	if (pCNvEncoder)
		return pCNvEncoder->NvEncodeCreate(pEncodeConfig);
	else
		return nullptr;
}

EXTERN_DLLEXPORT int NvEncodeFrame(void* hEncode, VCodecBuffer* pIn, VCodecBuffer** pOut)
{
	return ((CNvEncoder*)hEncode)->NvEncodeFrame(pIn, pOut);
}

EXTERN_DLLEXPORT int NvEncodeDestroy(void* hEncode)
{
	int status = ((CNvEncoder*)hEncode)->NvEncodeDestroy();
	if (hEncode)
	{
		delete (CNvEncoder*)hEncode;
		hEncode = nullptr;
	}
	return status;
}

EXTERN_DLLEXPORT unsigned int NvGetBufferedCount(void* hEncode)
{
	return ((CNvEncoder*)hEncode)->NvGetBufferedCount();
}

