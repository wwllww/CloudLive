
#include "inc/nvCPUOPSys.h"
#include "inc/nvEncodeAPI.h"
#include "inc/nvUtils.h"
#include "NvEncoder.h"
#include "inc/nvFileIO.h"
#include <new>

#include <dxgi1_2.h>
#include <vector>
#include <array>
#include "inc/mfx/mfxdefs.h"
#include "inc/mfx/mfxcommon.h"
#include "inc/mfx/mfxvideo++.h"
//#pragma comment(lib, "libmfx.lib")

#include "LogDeliver.h"

#pragma comment(lib,"Log_writer.lib")

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif // !LOG_RTSPSERV

HINSTANCE g_hinstLib;
NV_ENCODE_API_FUNCTION_LIST*  g_pEncodeAPI = nullptr;

volatile int  g_CodecLibRefCount = 0;

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

void convertYUVpitchtoNV12(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
	unsigned char *nv12_luma, unsigned char *nv12_chroma,
	int width, int height, int srcStride, int dstStride)
{
	int y;
	int x;
	if (srcStride == 0)
		srcStride = width;
	if (dstStride == 0)
		dstStride = width;

	for (y = 0; y < height; y++)
	{
		memcpy(nv12_luma + (dstStride*y), yuv_luma + (srcStride*y), width);
	}
	for (y = 0; y < height / 2; y++)
	{
		for (x = 0; x < width; x = x + 2)
		{
			nv12_chroma[(y*dstStride) + x] = yuv_cb[((srcStride / 2)*y) + (x >> 1)];
			nv12_chroma[(y*dstStride) + (x + 1)] = yuv_cr[((srcStride / 2)*y) + (x >> 1)];
		}
	}
}

CNvEncoder::CNvEncoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "CNvEncoder 构造");
    m_pNvHWEncoder = new CNvHWEncoder;
    m_pDevice = NULL;
    m_cuContext = NULL;

    m_uEncodeBufferCount = 0;
	memset(&m_stEncoderConfig, 0, sizeof(m_stEncoderConfig));
    memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));
    memset(&m_stMVBuffer, 0, sizeof(m_stMVBuffer));
    memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
	m_pTempBuffer = nullptr;
	m_bFlushComplete = false;
}

CNvEncoder::~CNvEncoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "CNvEncoder 析构");

    if (m_pNvHWEncoder)
    {
        delete m_pNvHWEncoder;
        m_pNvHWEncoder = NULL;
    }
}

NVENCSTATUS CNvEncoder::InitCuda(uint32_t deviceID)
{
    CUresult cuResult;
    CUdevice device;
    CUcontext cuContextCurr;
    int  deviceCount = 0;
    int  SMminor = 0, SMmajor = 0;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void *CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    cuResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuInit error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceGetCount(&deviceCount);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuDeviceGetCount error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    if ((int)deviceID < 0)
        deviceID = 0;

    if (deviceID >(unsigned int)deviceCount - 1)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    cuResult = cuDeviceGet(&device, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuDeviceGet error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuDeviceComputeCapability error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    if (((SMmajor << 4) + SMminor) < 0x30)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "GPU %d does not have NVENC capabilities exiting\n", deviceID);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuCtxCreate error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxPopCurrent(&cuContextCurr);
    if (cuResult != CUDA_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "cuCtxPopCurrent error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }
    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "InitD3D11 CreateDXGIFactory1 error Device Id = %d\n", deviceID);
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
        if (FAILED(hr))
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "Problem while creating %d D3d11 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "InitD3D11 Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, NV_ENC_BUFFER_FORMAT inputFormat)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, inputFormat);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "NvEncCreateInputBuffer error:%d\n", nvStatus);
			return nvStatus;
		}

        m_stEncodeBuffer[i].stInputBfr.bufferFmt = inputFormat;
        m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
        m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;
        nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "NvEncCreateBitstreamBuffer error:0x%x\n", nvStatus);
			return nvStatus;
		}
         m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;
		 if (m_stEncoderConfig.enableAsyncMode)
        {
            nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			if (nvStatus != NV_ENC_SUCCESS)
				return nvStatus;
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
        }
        else
            m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
    }

    m_stEOSOutputBfr.bEOSFlag = TRUE;

	if (m_stEncoderConfig.enableAsyncMode)
    {
        nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;
    }
    else
        m_stEOSOutputBfr.hOutputEvent = NULL;

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::AllocateMVIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, NV_ENC_BUFFER_FORMAT inputFormat)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    m_MVBufferQueue.Initialize(m_stMVBuffer, m_uEncodeBufferCount);
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        for (uint32_t j = 0; j < 2; j++)
        {
            nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stMVBuffer[i].stInputBfr[j].hInputSurface, inputFormat);
			if (nvStatus != NV_ENC_SUCCESS)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "NvEncCreateInputBuffer error:0x%x\n", nvStatus);
				return nvStatus;
			}
            m_stMVBuffer[i].stInputBfr[j].bufferFmt = inputFormat;
            m_stMVBuffer[i].stInputBfr[j].dwWidth = uInputWidth;
            m_stMVBuffer[i].stInputBfr[j].dwHeight = uInputHeight;
        }
        //Allocate output surface
        uint32_t encodeWidthInMbs = (uInputWidth + 15) >> 4;
        uint32_t encodeHeightInMbs = (uInputHeight + 15) >> 4;
        uint32_t dwSize = encodeWidthInMbs * encodeHeightInMbs * 64;
        nvStatus = m_pNvHWEncoder->NvEncCreateMVBuffer(dwSize, &m_stMVBuffer[i].stOutputBfr.hBitstreamBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "nvEncCreateMVBuffer error:0x%x\n", nvStatus);
            return nvStatus;
        }
        m_stMVBuffer[i].stOutputBfr.dwBitstreamBufferSize = dwSize;
		if (m_stEncoderConfig.enableAsyncMode)
        {
            nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stMVBuffer[i].stOutputBfr.hOutputEvent);
			if (nvStatus != NV_ENC_SUCCESS)
				return nvStatus;
            m_stMVBuffer[i].stOutputBfr.bWaitOnEvent = true;
        }
        else
            m_stMVBuffer[i].stOutputBfr.hOutputEvent = NULL;
    }
    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
{
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
        m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;
        m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
        m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
		if (m_stEncoderConfig.enableAsyncMode)
        {
            m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
            nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
            m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
        }
    }

    if (m_stEOSOutputBfr.hOutputEvent)
    {
		if (m_stEncoderConfig.enableAsyncMode)
        {
            m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
            nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
            m_stEOSOutputBfr.hOutputEvent = NULL;
        }
    }

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::ReleaseMVIOBuffers()
{
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        for (uint32_t j = 0; j < 2; j++)
        {
            m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stMVBuffer[i].stInputBfr[j].hInputSurface);
            m_stMVBuffer[i].stInputBfr[j].hInputSurface = NULL;
        }
        m_pNvHWEncoder->NvEncDestroyMVBuffer(m_stMVBuffer[i].stOutputBfr.hBitstreamBuffer);
        m_stMVBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
		if (m_stEncoderConfig.enableAsyncMode)
        {
            m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stMVBuffer[i].stOutputBfr.hOutputEvent);
            nvCloseFile(m_stMVBuffer[i].stOutputBfr.hOutputEvent);
            m_stMVBuffer[i].stOutputBfr.hOutputEvent = NULL;
        }
    }

    return NV_ENC_SUCCESS;
}

void CNvEncoder::FlushMVOutputBuffer()
{
    MotionEstimationBuffer *pMEBufer = m_MVBufferQueue.GetPending();
    while (pMEBufer)
    {
		m_pNvHWEncoder->ProcessMVOutput(pMEBufer);
		pMEBufer = m_MVBufferQueue.GetPending();
    }
}

NVENCSTATUS CNvEncoder::FlushEncoder(VCodecBuffer** pOut)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	if (m_EncodeBufferQueue.GetPendingCount() == m_uEncodeBufferCount)
	{
		nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "NvEncFlushEncoderQueue error:0x%x\n", nvStatus);
			return nvStatus;
		}
	}

	EncodeBuffer *pEncodeBuffer = m_EncodeBufferQueue.GetPending();
	if (pEncodeBuffer)
    {
		if (m_pTempBuffer)
		{
			if (m_pTempBuffer->pFrame)
			{
				delete[] m_pTempBuffer->pFrame;
				m_pTempBuffer->pFrame = nullptr;
			}
			delete m_pTempBuffer;
			m_pTempBuffer = nullptr;
		}

		VCodecBuffer buffer;
		m_pNvHWEncoder->ProcessOutput(pEncodeBuffer, &buffer.pFrame, &buffer.frameLen, &buffer.is_key, &buffer.ts, &buffer.pictureType);

		m_pTempBuffer = new VCodecBuffer;
		if (m_pTempBuffer)
		{
			m_pTempBuffer->pFrame = new unsigned char[buffer.frameLen];
			if (m_pTempBuffer->pFrame)
			{
				memcpy(m_pTempBuffer->pFrame, buffer.pFrame, buffer.frameLen);
				m_pTempBuffer->frameLen = buffer.frameLen;
				m_pTempBuffer->ts = buffer.ts;
				m_pTempBuffer->is_key = buffer.is_key;
				m_pTempBuffer->pictureType = buffer.pictureType;
				*pOut = m_pTempBuffer;
			}
		}
	}

	int nPendingCount = m_EncodeBufferQueue.GetPendingCount();
	Log::writeMessage(LOG_RTSPSERV, 1, "FlushEncoder, GetPendingCount:%d", nPendingCount);

	if (nPendingCount == 0)
	{
#if defined(NV_WINDOWS)
		if (m_stEncoderConfig.enableAsyncMode)
		{
			if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) error:0x%x\n", nvStatus);
// 				assert(0);
				nvStatus = NV_ENC_ERR_GENERIC;
			}
		}
		m_bFlushComplete = true;
#endif  
	}

    return nvStatus;
}

NVENCSTATUS CNvEncoder::LoadCodecLib()
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	MYPROC nvEncodeAPICreateInstance; // function pointer to create instance in nvEncodeAPI

	Log::writeMessage(LOG_RTSPSERV, 1, "调用LoadCodecLib!\n");

#if defined (_WIN64)
	g_hinstLib = LoadLibrary(TEXT("nvEncodeAPI64.dll"));
#else
	g_hinstLib = LoadLibrary(TEXT("nvEncodeAPI.dll"));
#endif
	if (g_hinstLib == NULL)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LoadLibrary nvEncodeAPI64.dll error!\n");
		return NV_ENC_ERR_OUT_OF_MEMORY;
	}

	nvEncodeAPICreateInstance = (MYPROC)GetProcAddress(g_hinstLib, "NvEncodeAPICreateInstance");
	if (nvEncodeAPICreateInstance == NULL)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "GetProcAddress nvEncodeAPI64.dll error!\n");
		return NV_ENC_ERR_OUT_OF_MEMORY;
	}

	if (nullptr == g_pEncodeAPI)
	{
		g_pEncodeAPI = new NV_ENCODE_API_FUNCTION_LIST;
		if (g_pEncodeAPI == NULL)
			return NV_ENC_ERR_OUT_OF_MEMORY;

		memset(g_pEncodeAPI, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
		g_pEncodeAPI->version = NV_ENCODE_API_FUNCTION_LIST_VER;
		nvStatus = nvEncodeAPICreateInstance(g_pEncodeAPI);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "nvEncodeAPICreateInstance error!\n");
			return nvStatus;
		}
	}

	return NV_ENC_SUCCESS;
}

void CNvEncoder::UnLoadCodecLib()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "调用UnLoadCodecLib!\n");

	if (g_pEncodeAPI)
	{
		delete g_pEncodeAPI;
		g_pEncodeAPI = NULL;
	}
	if (g_hinstLib)
	{
		FreeLibrary(g_hinstLib);
		g_hinstLib = NULL;
	}
}

void* CNvEncoder::NvEncodeCreate(EncodeConfig* pEncodeConfig)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "调用NvEncodeCreate");

	g_CodecLibRefCount++;
	if (NULL == g_pEncodeAPI)
	{
		NVENCSTATUS status = LoadCodecLib();
		if (NV_ENC_ERR_OUT_OF_MEMORY == status)
			return nullptr;
	}

	if (pEncodeConfig)
		memcpy(&m_stEncoderConfig, pEncodeConfig, sizeof(EncodeConfig));

// 	switch (pEncodeConfig->deviceType)
// 	{
// 	case NV_ENC_DX11:
// 		InitD3D11(pEncodeConfig->deviceID);
// 		break;
// 	case NV_ENC_CUDA:
		InitCuda(0);
// 		break;
// 	}

	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
// 	if (pEncodeConfig->deviceType != NV_ENC_CUDA)
// 		nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
// 	else
		nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeCreate  Initialize error: %d!\n", nvStatus);
		return nullptr;
	}

	pEncodeConfig->presetGUID = m_pNvHWEncoder->GetPresetGUID(pEncodeConfig->encoderPreset, pEncodeConfig->codec);
	nvStatus = m_pNvHWEncoder->CreateEncoder(pEncodeConfig);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeCreate  CreateEncoder error: %d, bitrate: %d\
			codec : %d,\
			deviceID : %d,\
			deviceType: %d \
			encoderPreset: %s \
			fps :%d \
			gopLength :%d \
			width :%d \
			height :%d \
			maxWidth :%d \
			maxHeight : %d \
			numB : %d \
			rcMode :%d\
			pictureStruct:%d!\n",
			nvStatus,
			pEncodeConfig->bitrate,
			pEncodeConfig->codec,
			pEncodeConfig->deviceID,
			pEncodeConfig->deviceType,
			pEncodeConfig->encoderPreset,
			pEncodeConfig->fps,
			pEncodeConfig->gopLength,
			pEncodeConfig->width,
			pEncodeConfig->height,
			pEncodeConfig->maxWidth,
			pEncodeConfig->maxHeight,
			pEncodeConfig->numB,
			pEncodeConfig->rcMode,
			pEncodeConfig->pictureStruct);
		return nullptr;
	}
	pEncodeConfig->maxWidth = pEncodeConfig->maxWidth ? pEncodeConfig->maxWidth : pEncodeConfig->width;
	pEncodeConfig->maxHeight = pEncodeConfig->maxHeight ? pEncodeConfig->maxHeight : pEncodeConfig->height;

	m_stEncoderConfig.enableAsyncMode = pEncodeConfig->enableAsyncMode;

	if (pEncodeConfig->enableExternalMEHint && (m_stEncoderConfig.enableMEOnly ||
		pEncodeConfig->codec != NV_ENC_H264 || pEncodeConfig->numB > 0))
		return nullptr;

	if (pEncodeConfig->numB > 0)
	{
		m_uEncodeBufferCount = pEncodeConfig->numB + 4; // min buffers is numb + 1 + 3 pipelining
	}
	else
	{
		int numMBs = ((pEncodeConfig->maxHeight + 15) >> 4) * ((pEncodeConfig->maxWidth + 15) >> 4);
		int NumIOBuffers;
		if (numMBs >= 32768) //4kx2k
			NumIOBuffers = MAX_ENCODE_QUEUE / 8;
		else if (numMBs >= 16384) // 2kx2k
			NumIOBuffers = MAX_ENCODE_QUEUE / 4;
		else if (numMBs >= 8160) // 1920x1080
			NumIOBuffers = MAX_ENCODE_QUEUE / 2;
		else
			NumIOBuffers = MAX_ENCODE_QUEUE;
		m_uEncodeBufferCount = NumIOBuffers;
	}
	m_uPicStruct = pEncodeConfig->pictureStruct;
	if (m_stEncoderConfig.enableMEOnly)
	{
		// Struct MotionEstimationBuffer has capacity to store two inputBuffer in single object.
		m_uEncodeBufferCount = m_uEncodeBufferCount / 2;
		nvStatus = AllocateMVIOBuffers(pEncodeConfig->width, pEncodeConfig->height, pEncodeConfig->inputFormat);
	}
	else
	{
		nvStatus = AllocateIOBuffers(pEncodeConfig->width, pEncodeConfig->height, pEncodeConfig->inputFormat);
	}
	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeCreate  AllocateIOBuffers error: %d!\n", nvStatus);
		return nullptr;
	}
	return this;
}

int CNvEncoder::NvEncodeFrame(VCodecBuffer* pIn, VCodecBuffer** pOut)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	if (NULL == pIn)
	{
		FlushEncoder(pOut);
		return NV_ENC_SUCCESS;
	}

	EncodeBuffer *pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
	if (!pEncodeBuffer)
	{
		if (m_pTempBuffer)
		{
			if (m_pTempBuffer->pFrame)
			{
				delete[] m_pTempBuffer->pFrame;
				m_pTempBuffer->pFrame = nullptr;
			}
			delete m_pTempBuffer;
			m_pTempBuffer = nullptr;
		}

		VCodecBuffer buffer;
		m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending(), &buffer.pFrame, &buffer.frameLen, &buffer.is_key, &buffer.ts, &buffer.pictureType);

		m_pTempBuffer = new VCodecBuffer;
		if (m_pTempBuffer)
		{
			m_pTempBuffer->pFrame = new unsigned char[buffer.frameLen];
			if (m_pTempBuffer->pFrame)
			{
				memcpy(m_pTempBuffer->pFrame, buffer.pFrame, buffer.frameLen);
				m_pTempBuffer->frameLen = buffer.frameLen;
				m_pTempBuffer->ts = buffer.ts;
				m_pTempBuffer->is_key = buffer.is_key;
				m_pTempBuffer->pictureType = buffer.pictureType;
				*pOut = m_pTempBuffer;
			}
		}

		pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
	}

	uint32_t lockedPitch = 0;
	unsigned char *pInputSurface;
	nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeFrame  NvEncLockInputBuffer error: %d!\n", nvStatus);
		return nvStatus;
	}

	int width = m_stEncoderConfig.width;
	int height = m_stEncoderConfig.height;

	if (m_stEncoderConfig.RealInputFormat == NV_ENC_BUFFER_FORMAT_YV12)
	{
		uint8_t *y = (uint8_t*)pIn->pFrame;
		uint8_t *v = (uint8_t*)pIn->pFrame + width * height;
		uint8_t *u = (uint8_t*)pIn->pFrame + width * height * 5 / 4;
		unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
		convertYUVpitchtoNV12(y, u, v, pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
	}
	else if (m_stEncoderConfig.RealInputFormat == NV_ENC_BUFFER_FORMAT_IYUV)
	{
		uint8_t *y = (uint8_t*)pIn->pFrame;
		uint8_t *u = (uint8_t*)pIn->pFrame + width * height;
		uint8_t *v = (uint8_t*)pIn->pFrame + width * height * 5 / 4;
		unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
		convertYUVpitchtoNV12(y, u, v, pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
	}
	else if (m_stEncoderConfig.RealInputFormat == NV_ENC_BUFFER_FORMAT_NV12)
	{
		uint8_t *y = (uint8_t*)pIn->pFrame;
		uint8_t *uv = (uint8_t*)pIn->pFrame + width * height;
		unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
		for (int i = 0; i < height; i++)
		{
			memcpy(pInputSurface + i*lockedPitch, y + i*width, width);
		}
		for (int i = 0; i < height / 2; i++)
		{
			memcpy(pInputSurfaceCh + i*lockedPitch, uv + i*width, width);
		}
	}
	nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeFrame  NvEncUnlockInputBuffer error: %d!\n", nvStatus);
		return nvStatus;
	}

	nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct);

	return nvStatus;
}

int CNvEncoder::NvEncodeDestroy()
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	Log::writeMessage(LOG_RTSPSERV, 1, "调用NvEncodeDestroy!\n");

// 	while (!m_bFlushComplete)
// 	{
// 		Sleep(1);
// 		if (m_EncodeBufferQueue.GetPendingCount() == 0)
// 			m_bFlushComplete = true;
// 	}

	for (size_t i = 0; i < m_uEncodeBufferCount; i++)
	{
		VCodecBuffer* pOut = nullptr;
		FlushEncoder(&pOut);
	}

	if (m_stEncoderConfig.enableMEOnly)
		ReleaseMVIOBuffers();
	else
		ReleaseIOBuffers();

	nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

	if (m_pDevice)
	{
		CUresult cuResult = CUDA_SUCCESS;
		cuResult = cuCtxDestroy((CUcontext)m_pDevice);
		if (cuResult != CUDA_SUCCESS)
			Log::writeMessage(LOG_RTSPSERV, 1, "cuCtxDestroy error:0x%x\n", cuResult);
		m_pDevice = NULL;
	}

	g_CodecLibRefCount--;
	if (g_CodecLibRefCount == 0)
	{
		UnLoadCodecLib();
	}
	return nvStatus;
}

unsigned int CNvEncoder::NvGetBufferedCount()
{
	return m_EncodeBufferQueue.GetPendingCount();
}

static void GetCpuInfo(int *isPentium)
{
	std::array<int, 4> _cpui;
	std::vector<std::array<int, 4>> data_;
	std::vector<std::array<int, 4>> extdata_;
	bool _isintel = false;
	bool _isamd = false;
	__cpuid(_cpui.data(), 0);
	int _nids = _cpui[0];
	for (int i = 0; i <= _nids; ++i)
	{
		__cpuidex(_cpui.data(), i, 0);
		data_.push_back(_cpui);
	}
	char vendor[32];
	memset(vendor, 0, sizeof(vendor));
	*reinterpret_cast<int*>(vendor) = data_[0][1];
	*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
	*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
	if (strstr(vendor, "GenuineIntel") != NULL)
		_isintel = true;
	else if (strstr(vendor, "AuthenticAMD") != NULL)
		_isamd = true;

	__cpuid(_cpui.data(), 0x80000000);
	int _nExids = _cpui[0];
	for (int i = 0x80000000; i <= _nExids; ++i)
	{
		__cpuidex(_cpui.data(), i, 0);
		extdata_.push_back(_cpui);
	}
	char brand[64];
	memset(brand, 0, sizeof(brand));
	if (_nExids >= 0x80000004)
	{
		memcpy(brand, extdata_[2].data(), sizeof(_cpui));
		memcpy(brand + 16, extdata_[3].data(), sizeof(_cpui));
		memcpy(brand + 32, extdata_[4].data(), sizeof(_cpui));
	}
	if (strstr(brand, "Pentium") != NULL || strstr(brand, "Celeron") != NULL)
		*isPentium = 1;
}

static bool QueryIntelSupport()
{
	bool _bsupport = true;
	mfxStatus sts;
	MFXVideoSession _session;
	mfxVersion _version = { 0, 1 };
	mfxIMPL _impl = MFX_IMPL_AUTO_ANY;
	sts = _session.Init(_impl, &_version);
	if (sts == MFX_ERR_NONE)
	{
		sts = _session.QueryIMPL(&_impl);
		if ((_impl & MFX_IMPL_HARDWARE) || (_impl & MFX_IMPL_HARDWARE_ANY))
		{
			int isPentium = 0;
			GetCpuInfo(&isPentium);
			if (isPentium)
				_bsupport = false;
		}
		else
			_bsupport = false;
	}
	else
		_bsupport = false;
	_session.Close();
	return _bsupport;
}

int  OSGetVersion()
{
	OSVERSIONINFO osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	GetVersionEx(&osVersionInfo);
	if (osVersionInfo.dwMajorVersion > 6)
		return 8;
	if (osVersionInfo.dwMajorVersion == 6)
	{
		//Windows 8
		if (osVersionInfo.dwMinorVersion >= 2)
			return 8;
		//Windows 7
		if (osVersionInfo.dwMinorVersion == 1)
			return 7;
		//Vista
		if (osVersionInfo.dwMinorVersion == 0)
			return 6;
	}
	return 0;
}

int CNvEncoder::QueryNvidia()
{
	int ret = -1;
	HRESULT err;
	REFIID iidVal = OSGetVersion() >= 8 ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);

	IDXGIFactory1 *factory;
	if (SUCCEEDED(err = CreateDXGIFactory1(iidVal, (void**)&factory)))
	{
		UINT i = 0;
		IDXGIAdapter1 *giAdapter;
		while (factory->EnumAdapters1(i++, &giAdapter) == S_OK)
		{
			DXGI_ADAPTER_DESC adapterDesc;
			if (SUCCEEDED(err = giAdapter->GetDesc(&adapterDesc)))
			{
				if (adapterDesc.DedicatedVideoMemory != 0)
				{
					if (wcsstr(adapterDesc.Description, L"NVIDIA"))
					{
						err = D3D11CreateDevice(giAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
							NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
						if (FAILED(err))
						{
							Log::writeMessage(LOG_RTSPSERV, 1, "Problem while creating %d D3d11 device \n", i);
							return NV_ENC_ERR_OUT_OF_MEMORY;
						}
						ret = i;
						giAdapter->Release();
						break;
					}
				}
			}
			giAdapter->Release();
		}
		factory->Release();
	}
	return ret;
}

int CNvEncoder::QueryHardEncodeSupport()
{
	int nRet = -1;
	int nRetNvidia = 0, nRetIntel = 0;

	if (QueryIntelSupport())
		nRetIntel = 2;

	if (QueryNvidia() != -1)
	{
		NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
		nvStatus = LoadCodecLib();
		if (nvStatus != NV_ENC_SUCCESS)
		{
			if (nRetNvidia || nRetIntel)
				nRet = nRetIntel | nRetNvidia;
			return nRet;
		}

		nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "NvEncOpenEncodeSessionEx error: %d!\n", nvStatus);
			if (m_pDevice)
			{
				((ID3D11Device*)(m_pDevice))->Release();
				m_pDevice = NULL;
			}
			UnLoadCodecLib();
		}
		else
		{
			nRetNvidia = 1;
		}
	}

	if (nRetNvidia || nRetIntel)
		nRet = nRetIntel | nRetNvidia;

	return nRet;
}

UINT BGRA2ARGB(UINT bgra)
{
	BYTE b = (bgra >> 24) & 0xff;
	BYTE g = (bgra >> 16) & 0xff;
	BYTE r = (bgra >> 8) & 0xff;
	BYTE a = (bgra >> 0) & 0xff;
	return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}
