
#include "../inc/NvHWEncoder.h"

#include "LogDeliver.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV ((long long)1<<49)
#endif // !LOG_RTSPSERV

extern NV_ENCODE_API_FUNCTION_LIST*  g_pEncodeAPI;

NVENCSTATUS CNvHWEncoder::NvEncOpenEncodeSession(void* device, uint32_t deviceType)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncOpenEncodeSession(device, deviceType, &m_hEncoder);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeGUIDCount(uint32_t* encodeGUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeGUIDCount(m_hEncoder, encodeGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeProfileGUIDCount(GUID encodeGUID, uint32_t* encodeProfileGUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeProfileGUIDCount(m_hEncoder, encodeGUID, encodeProfileGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeProfileGUIDs(GUID encodeGUID, GUID* profileGUIDs, uint32_t guidArraySize, uint32_t* GUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeProfileGUIDs(m_hEncoder, encodeGUID, profileGUIDs, guidArraySize, GUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeGUIDs(GUID* GUIDs, uint32_t guidArraySize, uint32_t* GUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeGUIDs(m_hEncoder, GUIDs, guidArraySize, GUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetInputFormatCount(GUID encodeGUID, uint32_t* inputFmtCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetInputFormatCount(m_hEncoder, encodeGUID, inputFmtCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetInputFormats(GUID encodeGUID, NV_ENC_BUFFER_FORMAT* inputFmts, uint32_t inputFmtArraySize, uint32_t* inputFmtCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetInputFormats(m_hEncoder, encodeGUID, inputFmts, inputFmtArraySize, inputFmtCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeCaps(GUID encodeGUID, NV_ENC_CAPS_PARAM* capsParam, int* capsVal)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeCaps(m_hEncoder, encodeGUID, capsParam, capsVal);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodePresetCount(GUID encodeGUID, uint32_t* encodePresetGUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetCount(m_hEncoder, encodeGUID, encodePresetGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodePresetGUIDs(GUID encodeGUID, GUID* presetGUIDs, uint32_t guidArraySize, uint32_t* encodePresetGUIDCount)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetGUIDs(m_hEncoder, encodeGUID, presetGUIDs, guidArraySize, encodePresetGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodePresetConfig(GUID encodeGUID, GUID  presetGUID, NV_ENC_PRESET_CONFIG* presetConfig)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetConfig(m_hEncoder, encodeGUID, presetGUID, presetConfig);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncCreateInputBuffer(uint32_t width, uint32_t height, void** inputBuffer, NV_ENC_BUFFER_FORMAT inputFormat)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_CREATE_INPUT_BUFFER createInputBufferParams;

    memset(&createInputBufferParams, 0, sizeof(createInputBufferParams));
    SET_VER(createInputBufferParams, NV_ENC_CREATE_INPUT_BUFFER);

    createInputBufferParams.width = width;
    createInputBufferParams.height = height;
    createInputBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
    createInputBufferParams.bufferFmt = inputFormat;

    nvStatus = g_pEncodeAPI->nvEncCreateInputBuffer(m_hEncoder, &createInputBufferParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    *inputBuffer = createInputBufferParams.inputBuffer;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncDestroyInputBuffer(NV_ENC_INPUT_PTR inputBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    if (inputBuffer)
    {
        nvStatus = g_pEncodeAPI->nvEncDestroyInputBuffer(m_hEncoder, inputBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
        }
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncCreateMVBuffer(uint32_t size, void** bitstreamBuffer)
{
    NVENCSTATUS status;
    NV_ENC_CREATE_MV_BUFFER stAllocMVBuffer;
    memset(&stAllocMVBuffer, 0, sizeof(stAllocMVBuffer));
    SET_VER(stAllocMVBuffer, NV_ENC_CREATE_MV_BUFFER);
    status = g_pEncodeAPI->nvEncCreateMVBuffer(m_hEncoder, &stAllocMVBuffer);
    if (status != NV_ENC_SUCCESS)
    {
        assert(0);
    }
    *bitstreamBuffer = stAllocMVBuffer.mvBuffer;
    return status;
}

NVENCSTATUS CNvHWEncoder::NvEncDestroyMVBuffer(NV_ENC_OUTPUT_PTR bitstreamBuffer)
{
    NVENCSTATUS status;
    NV_ENC_CREATE_MV_BUFFER stAllocMVBuffer;
    memset(&stAllocMVBuffer, 0, sizeof(stAllocMVBuffer));
    SET_VER(stAllocMVBuffer, NV_ENC_CREATE_MV_BUFFER);
    status = g_pEncodeAPI->nvEncDestroyMVBuffer(m_hEncoder, bitstreamBuffer);
    if (status != NV_ENC_SUCCESS)
    {
        assert(0);
    }
    bitstreamBuffer = NULL;
    return status;
}

NVENCSTATUS CNvHWEncoder::NvRunMotionEstimationOnly(MotionEstimationBuffer *pMEBuffer, MEOnlyConfig *pMEOnly)
{
    NVENCSTATUS nvStatus;
    NV_ENC_MEONLY_PARAMS stMEOnlyParams;
    SET_VER(stMEOnlyParams,NV_ENC_MEONLY_PARAMS);
    stMEOnlyParams.referenceFrame = pMEBuffer->stInputBfr[0].hInputSurface;
    stMEOnlyParams.inputBuffer = pMEBuffer->stInputBfr[1].hInputSurface;
    stMEOnlyParams.bufferFmt = pMEBuffer->stInputBfr[1].bufferFmt;
    stMEOnlyParams.inputWidth = pMEBuffer->stInputBfr[1].dwWidth;
    stMEOnlyParams.inputHeight = pMEBuffer->stInputBfr[1].dwHeight;
    stMEOnlyParams.mvBuffer = pMEBuffer->stOutputBfr.hBitstreamBuffer;
    stMEOnlyParams.completionEvent = pMEBuffer->stOutputBfr.hOutputEvent;
    nvStatus = g_pEncodeAPI->nvEncRunMotionEstimationOnly(m_hEncoder, &stMEOnlyParams);
    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncCreateBitstreamBuffer(uint32_t size, void** bitstreamBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBufferParams;

    memset(&createBitstreamBufferParams, 0, sizeof(createBitstreamBufferParams));
    SET_VER(createBitstreamBufferParams, NV_ENC_CREATE_BITSTREAM_BUFFER);

    createBitstreamBufferParams.size = size;
    createBitstreamBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

    nvStatus = g_pEncodeAPI->nvEncCreateBitstreamBuffer(m_hEncoder, &createBitstreamBufferParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    *bitstreamBuffer = createBitstreamBufferParams.bitstreamBuffer;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncDestroyBitstreamBuffer(NV_ENC_OUTPUT_PTR bitstreamBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    if (bitstreamBuffer)
    {
        nvStatus = g_pEncodeAPI->nvEncDestroyBitstreamBuffer(m_hEncoder, bitstreamBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
        }
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncLockBitstream(NV_ENC_LOCK_BITSTREAM* lockBitstreamBufferParams)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncLockBitstream(m_hEncoder, lockBitstreamBufferParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncUnlockBitstream(NV_ENC_OUTPUT_PTR bitstreamBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncUnlockBitstream(m_hEncoder, bitstreamBuffer);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncLockInputBuffer(void* inputBuffer, void** bufferDataPtr, uint32_t* pitch)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_LOCK_INPUT_BUFFER lockInputBufferParams;
    memset(&lockInputBufferParams, 0, sizeof(lockInputBufferParams));
    SET_VER(lockInputBufferParams, NV_ENC_LOCK_INPUT_BUFFER);
    lockInputBufferParams.inputBuffer = inputBuffer;
    nvStatus = g_pEncodeAPI->nvEncLockInputBuffer(m_hEncoder, &lockInputBufferParams);

    *bufferDataPtr = lockInputBufferParams.bufferDataPtr;
    *pitch = lockInputBufferParams.pitch;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncUnlockInputBuffer(NV_ENC_INPUT_PTR inputBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncUnlockInputBuffer(m_hEncoder, inputBuffer);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetEncodeStats(NV_ENC_STAT* encodeStats)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeStats(m_hEncoder, encodeStats);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncGetSequenceParams(NV_ENC_SEQUENCE_PARAM_PAYLOAD* sequenceParamPayload)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncGetSequenceParams(m_hEncoder, sequenceParamPayload);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncRegisterAsyncEvent(void** completionEvent)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_EVENT_PARAMS eventParams;
    memset(&eventParams, 0, sizeof(eventParams));
    SET_VER(eventParams, NV_ENC_EVENT_PARAMS);
    eventParams.completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    nvStatus = g_pEncodeAPI->nvEncRegisterAsyncEvent(m_hEncoder, &eventParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncRegisterAsyncEvent error: %d!\n", nvStatus);
    }
    *completionEvent = eventParams.completionEvent;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncUnregisterAsyncEvent(void* completionEvent)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_EVENT_PARAMS eventParams;

    if (completionEvent)
    {
        memset(&eventParams, 0, sizeof(eventParams));
        SET_VER(eventParams, NV_ENC_EVENT_PARAMS);

        eventParams.completionEvent = completionEvent;

        nvStatus = g_pEncodeAPI->nvEncUnregisterAsyncEvent(m_hEncoder, &eventParams);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
        }
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncMapInputResource(void* registeredResource, void** mappedResource)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_MAP_INPUT_RESOURCE mapInputResParams;

    memset(&mapInputResParams, 0, sizeof(mapInputResParams));
    SET_VER(mapInputResParams, NV_ENC_MAP_INPUT_RESOURCE);

    mapInputResParams.registeredResource = registeredResource;

    nvStatus = g_pEncodeAPI->nvEncMapInputResource(m_hEncoder, &mapInputResParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    *mappedResource = mapInputResParams.mappedResource;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncUnmapInputResource(NV_ENC_INPUT_PTR mappedInputBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    
    if (mappedInputBuffer)
    {
        nvStatus = g_pEncodeAPI->nvEncUnmapInputResource(m_hEncoder, mappedInputBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
        }
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncDestroyEncoder()
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	Log::writeMessage(LOG_RTSPSERV, 1, "调用NvEncDestroyEncoder!\n");

    if (m_bEncoderInitialized)
    {
        nvStatus = g_pEncodeAPI->nvEncDestroyEncoder(m_hEncoder);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			assert(0);
		}
        m_bEncoderInitialized = false;
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncInvalidateRefFrames(const NvEncPictureCommand *pEncPicCommand)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    for (uint32_t i = 0; i < pEncPicCommand->numRefFramesToInvalidate; i++)
    {
        nvStatus = g_pEncodeAPI->nvEncInvalidateRefFrames(m_hEncoder, pEncPicCommand->refFrameNumbers[i]);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncOpenEncodeSessionEx(void* device, NV_ENC_DEVICE_TYPE deviceType)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS openSessionExParams;

    memset(&openSessionExParams, 0, sizeof(openSessionExParams));
    SET_VER(openSessionExParams, NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS);

    openSessionExParams.device = device;
    openSessionExParams.deviceType = deviceType;
    openSessionExParams.apiVersion = NVENCAPI_VERSION;

    nvStatus = g_pEncodeAPI->nvEncOpenEncodeSessionEx(&openSessionExParams, &m_hEncoder);
    if (nvStatus != NV_ENC_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "nvEncOpenEncodeSessionEx Error nvStatus = %d\n", nvStatus);
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncRegisterResource(NV_ENC_INPUT_RESOURCE_TYPE resourceType, void* resourceToRegister,
                                                uint32_t width, uint32_t height, uint32_t pitch,
                                                void** registeredResource, NV_ENC_BUFFER_FORMAT bufFormat)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_REGISTER_RESOURCE registerResParams;

    memset(&registerResParams, 0, sizeof(registerResParams));
    SET_VER(registerResParams, NV_ENC_REGISTER_RESOURCE);

    registerResParams.resourceType = resourceType;
    registerResParams.resourceToRegister = resourceToRegister;
    registerResParams.width = width;
    registerResParams.height = height;
    registerResParams.pitch = pitch;
    registerResParams.bufferFormat = bufFormat;

    nvStatus = g_pEncodeAPI->nvEncRegisterResource(m_hEncoder, &registerResParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    *registeredResource = registerResParams.registeredResource;

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncUnregisterResource(NV_ENC_REGISTERED_PTR registeredRes)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    nvStatus = g_pEncodeAPI->nvEncUnregisterResource(m_hEncoder, registeredRes);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::NvEncReconfigureEncoder(const NvEncPictureCommand *pEncPicCommand)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    if (pEncPicCommand->bBitrateChangePending || pEncPicCommand->bResolutionChangePending)
    {
        if (pEncPicCommand->bResolutionChangePending)
        {
            m_uCurWidth = pEncPicCommand->newWidth;
            m_uCurHeight = pEncPicCommand->newHeight;
            if ((m_uCurWidth > m_uMaxWidth) || (m_uCurHeight > m_uMaxHeight))
            {
                return NV_ENC_ERR_INVALID_PARAM;
            }
            m_stCreateEncodeParams.encodeWidth = m_uCurWidth;
            m_stCreateEncodeParams.encodeHeight = m_uCurHeight;
            m_stCreateEncodeParams.darWidth = m_uCurWidth;
            m_stCreateEncodeParams.darHeight = m_uCurHeight;
        }

        if (pEncPicCommand->bBitrateChangePending)
        {
            m_stEncodeConfig.rcParams.averageBitRate = pEncPicCommand->newBitrate;
            m_stEncodeConfig.rcParams.maxBitRate = pEncPicCommand->newBitrate;
            m_stEncodeConfig.rcParams.vbvBufferSize = pEncPicCommand->newVBVSize != 0 ? pEncPicCommand->newVBVSize : (pEncPicCommand->newBitrate * m_stCreateEncodeParams.frameRateDen) / m_stCreateEncodeParams.frameRateNum;
            m_stEncodeConfig.rcParams.vbvInitialDelay = m_stEncodeConfig.rcParams.vbvBufferSize;
        }

        NV_ENC_RECONFIGURE_PARAMS stReconfigParams;
        memset(&stReconfigParams, 0, sizeof(stReconfigParams));
        memcpy(&stReconfigParams.reInitEncodeParams, &m_stCreateEncodeParams, sizeof(m_stCreateEncodeParams));
        stReconfigParams.version = NV_ENC_RECONFIGURE_PARAMS_VER;
        stReconfigParams.forceIDR = pEncPicCommand->bResolutionChangePending ? 1 : 0;

        nvStatus = g_pEncodeAPI->nvEncReconfigureEncoder(m_hEncoder, &stReconfigParams);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
        }
    }

    return nvStatus;
}

CNvHWEncoder::CNvHWEncoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "CNvHWEncoder 构造");

    m_hEncoder = NULL;
    m_bEncoderInitialized = false;
    m_EncodeIdx = 0;
    m_uCurWidth = 0;
    m_uCurHeight = 0;
    m_uMaxWidth = 0;
    m_uMaxHeight = 0;

    memset(&m_stCreateEncodeParams, 0, sizeof(m_stCreateEncodeParams));
    SET_VER(m_stCreateEncodeParams, NV_ENC_INITIALIZE_PARAMS);

    memset(&m_stEncodeConfig, 0, sizeof(m_stEncodeConfig));
    SET_VER(m_stEncodeConfig, NV_ENC_CONFIG);
}

CNvHWEncoder::~CNvHWEncoder()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "CNvHWEncoder 析构");
}

NVENCSTATUS CNvHWEncoder::ValidateEncodeGUID (GUID inputCodecGuid)
{
    unsigned int i, codecFound, encodeGUIDCount, encodeGUIDArraySize;
    NVENCSTATUS nvStatus;
    GUID *encodeGUIDArray;

    nvStatus = g_pEncodeAPI->nvEncGetEncodeGUIDCount(m_hEncoder, &encodeGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        return nvStatus;
    }

    encodeGUIDArray = new GUID[encodeGUIDCount];
    memset(encodeGUIDArray, 0, sizeof(GUID)* encodeGUIDCount);

    encodeGUIDArraySize = 0;
    nvStatus = g_pEncodeAPI->nvEncGetEncodeGUIDs(m_hEncoder, encodeGUIDArray, encodeGUIDCount, &encodeGUIDArraySize);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        delete[] encodeGUIDArray;
        assert(0);
        return nvStatus;
    }

    assert(encodeGUIDArraySize <= encodeGUIDCount);

    codecFound = 0;
    for (i = 0; i < encodeGUIDArraySize; i++)
    {
        if (inputCodecGuid == encodeGUIDArray[i])
        {
            codecFound = 1;
            break;
        }
    }

    delete[] encodeGUIDArray;

    if (codecFound)
        return NV_ENC_SUCCESS;
    else
        return NV_ENC_ERR_INVALID_PARAM;
}

NVENCSTATUS CNvHWEncoder::ValidatePresetGUID(GUID inputPresetGuid, GUID inputCodecGuid)
{
    uint32_t i, presetFound, presetGUIDCount, presetGUIDArraySize;
    NVENCSTATUS nvStatus;
    GUID *presetGUIDArray;

    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetCount(m_hEncoder, inputCodecGuid, &presetGUIDCount);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        return nvStatus;
    }

    presetGUIDArray = new GUID[presetGUIDCount];
    memset(presetGUIDArray, 0, sizeof(GUID)* presetGUIDCount);

    presetGUIDArraySize = 0;
    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetGUIDs(m_hEncoder, inputCodecGuid, presetGUIDArray, presetGUIDCount, &presetGUIDArraySize);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        delete[] presetGUIDArray;
        return nvStatus;
    }

    assert(presetGUIDArraySize <= presetGUIDCount);

    presetFound = 0;
    for (i = 0; i < presetGUIDArraySize; i++)
    {
        if (inputPresetGuid == presetGUIDArray[i])
        {
            presetFound = 1;
            break;
        }
    }

    delete[] presetGUIDArray;

    if (presetFound)
        return NV_ENC_SUCCESS;
    else
        return NV_ENC_ERR_INVALID_PARAM;
}

NVENCSTATUS CNvHWEncoder::CreateEncoder(EncodeConfig *pEncCfg)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    if (pEncCfg == NULL)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "CreateEncoder pEncCfg == NULL");
        return NV_ENC_ERR_INVALID_PARAM;
    }

    m_uCurWidth = pEncCfg->width;
    m_uCurHeight = pEncCfg->height;

    m_uMaxWidth = (pEncCfg->maxWidth > 0 ? pEncCfg->maxWidth : pEncCfg->width);
    m_uMaxHeight = (pEncCfg->maxHeight > 0 ? pEncCfg->maxHeight : pEncCfg->height);

    if ((m_uCurWidth > m_uMaxWidth) || (m_uCurHeight > m_uMaxHeight)) {
		Log::writeMessage(LOG_RTSPSERV, 1, "CreateEncoder m_uCurWidth > m_uMaxWidth");
        return NV_ENC_ERR_INVALID_PARAM;
    }

    if (!pEncCfg->width || !pEncCfg->height)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "CreateEncoder pEncCfg->width==0");
        return NV_ENC_ERR_INVALID_PARAM;
    }

    if ((pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT || pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT) && (pEncCfg->codec == NV_ENC_H264))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "10 bit is not supported with H264 \n");
        return NV_ENC_ERR_INVALID_PARAM;
    }

    GUID inputCodecGUID = pEncCfg->codec == NV_ENC_H264 ? NV_ENC_CODEC_H264_GUID : NV_ENC_CODEC_HEVC_GUID;
    nvStatus = ValidateEncodeGUID(inputCodecGUID);
    if (nvStatus != NV_ENC_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "codec not supported \n");
        return nvStatus;
    }

    codecGUID = inputCodecGUID;

    m_stCreateEncodeParams.encodeGUID = inputCodecGUID;
    m_stCreateEncodeParams.presetGUID = pEncCfg->presetGUID;
    m_stCreateEncodeParams.encodeWidth = pEncCfg->width;
    m_stCreateEncodeParams.encodeHeight = pEncCfg->height;

    m_stCreateEncodeParams.darWidth = pEncCfg->width;
    m_stCreateEncodeParams.darHeight = pEncCfg->height;
    m_stCreateEncodeParams.frameRateNum = pEncCfg->fps;
    m_stCreateEncodeParams.frameRateDen = 1;
    m_stCreateEncodeParams.enableEncodeAsync = 0;

    m_stCreateEncodeParams.enablePTD = 1;
    m_stCreateEncodeParams.reportSliceOffsets = 0;
    m_stCreateEncodeParams.enableSubFrameWrite = 0;
    m_stCreateEncodeParams.encodeConfig = &m_stEncodeConfig;
    m_stCreateEncodeParams.maxEncodeWidth = m_uMaxWidth;
    m_stCreateEncodeParams.maxEncodeHeight = m_uMaxHeight;

	uint32_t nInputFormatCount = 0;
	uint32_t nRealCount = 0;
	NvEncGetInputFormatCount(inputCodecGUID, &nInputFormatCount);
	Log::writeMessage(LOG_RTSPSERV, 1, "NvEncGetInputFormatCount = %d!\n", nInputFormatCount);

	NV_ENC_BUFFER_FORMAT   arr[9];
	NvEncGetInputFormats(inputCodecGUID, arr, 9, &nRealCount);
	Log::writeMessage(LOG_RTSPSERV, 1, "NvEncGetInputFormats = %x,%x,%x,%x,%x,%x,%x,%x,%x!\n", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8]);

    // apply preset
    NV_ENC_PRESET_CONFIG stPresetCfg;
    memset(&stPresetCfg, 0, sizeof(NV_ENC_PRESET_CONFIG));
    SET_VER(stPresetCfg, NV_ENC_PRESET_CONFIG);
    SET_VER(stPresetCfg.presetCfg, NV_ENC_CONFIG);

    if (pEncCfg->enableExternalMEHint)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "pEncCfg->enableExternalMEHint is true!\n");

        m_stCreateEncodeParams.enableExternalMEHints = 1;
        for (int predictorCount = 0; predictorCount < 2; predictorCount++)
        {
            m_stCreateEncodeParams.maxMEHintCountsPerBlock[predictorCount].numCandsPerBlk16x16 = 1;
            m_stCreateEncodeParams.maxMEHintCountsPerBlock[predictorCount].numCandsPerBlk16x8 = 1;
            m_stCreateEncodeParams.maxMEHintCountsPerBlock[predictorCount].numCandsPerBlk8x16 = 1;
            m_stCreateEncodeParams.maxMEHintCountsPerBlock[predictorCount].numCandsPerBlk8x8 = 1;
        }
    }

    nvStatus = g_pEncodeAPI->nvEncGetEncodePresetConfig(m_hEncoder, m_stCreateEncodeParams.encodeGUID, m_stCreateEncodeParams.presetGUID, &stPresetCfg);
    if (nvStatus != NV_ENC_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "nvEncGetEncodePresetConfig returned failure \n");
        return nvStatus;
    }
    memcpy(&m_stEncodeConfig, &stPresetCfg.presetCfg, sizeof(NV_ENC_CONFIG));

    m_stEncodeConfig.gopLength = pEncCfg->gopLength;
    m_stEncodeConfig.frameIntervalP = pEncCfg->numB + 1;
    if (pEncCfg->pictureStruct == NV_ENC_PIC_STRUCT_FRAME)
    {
        m_stEncodeConfig.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
    }
    else
    {
        m_stEncodeConfig.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FIELD;
    }

    m_stEncodeConfig.mvPrecision = NV_ENC_MV_PRECISION_QUARTER_PEL;

    if (pEncCfg->bitrate || pEncCfg->vbvMaxBitrate)
    {
        m_stEncodeConfig.rcParams.rateControlMode = (NV_ENC_PARAMS_RC_MODE)pEncCfg->rcMode;
        m_stEncodeConfig.rcParams.averageBitRate = pEncCfg->bitrate;
        m_stEncodeConfig.rcParams.maxBitRate = pEncCfg->vbvMaxBitrate;
        m_stEncodeConfig.rcParams.vbvBufferSize = pEncCfg->vbvSize;
        m_stEncodeConfig.rcParams.vbvInitialDelay = pEncCfg->vbvSize * 9 / 10;
    }
    else
    {
        m_stEncodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    }

    if (pEncCfg->rcMode == 0)
    {
        m_stEncodeConfig.rcParams.constQP.qpInterP = pEncCfg->presetGUID == NV_ENC_PRESET_LOSSLESS_HP_GUID? 0 : pEncCfg->qp;
        m_stEncodeConfig.rcParams.constQP.qpInterB = pEncCfg->presetGUID == NV_ENC_PRESET_LOSSLESS_HP_GUID? 0 : pEncCfg->qp;
        m_stEncodeConfig.rcParams.constQP.qpIntra = pEncCfg->presetGUID == NV_ENC_PRESET_LOSSLESS_HP_GUID? 0 : pEncCfg->qp;
    }

    // set up initial QP value
    if (pEncCfg->rcMode == NV_ENC_PARAMS_RC_VBR || pEncCfg->rcMode == NV_ENC_PARAMS_RC_VBR_MINQP ||
        pEncCfg->rcMode == NV_ENC_PARAMS_RC_2_PASS_VBR) {
        m_stEncodeConfig.rcParams.enableInitialRCQP = 1;
        m_stEncodeConfig.rcParams.initialRCQP.qpInterP  = pEncCfg->qp;
        if(pEncCfg->i_quant_factor > 0.0 && pEncCfg->b_quant_factor > 0.0) {
            m_stEncodeConfig.rcParams.initialRCQP.qpIntra = (int)(pEncCfg->qp * pEncCfg->i_quant_factor + pEncCfg->i_quant_offset);
            m_stEncodeConfig.rcParams.initialRCQP.qpInterB = (int)(pEncCfg->qp * pEncCfg->b_quant_factor + pEncCfg->b_quant_offset);
        } else {
            m_stEncodeConfig.rcParams.initialRCQP.qpIntra = pEncCfg->qp;
            m_stEncodeConfig.rcParams.initialRCQP.qpInterB = pEncCfg->qp;
        }

    }

    if (pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV444 || pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT)
    {
        if (pEncCfg->codec == NV_ENC_HEVC) {
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.chromaFormatIDC = 3;
        } else {
            m_stEncodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC = 3;
        }
    }
    else
    {
        if (pEncCfg->codec == NV_ENC_HEVC) {
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.chromaFormatIDC = 1;
        } else {
            m_stEncodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC = 1;
        }
    }

    if (pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT || pEncCfg->inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT)
    {
        if (pEncCfg->codec == NV_ENC_HEVC) {
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
        }
    }

    if (pEncCfg->intraRefreshEnableFlag)
    {
        if (pEncCfg->codec == NV_ENC_HEVC)
        {
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.enableIntraRefresh = 1;
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.intraRefreshPeriod = pEncCfg->intraRefreshPeriod;
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.intraRefreshCnt = pEncCfg->intraRefreshDuration;
        }
        else
        {
            m_stEncodeConfig.encodeCodecConfig.h264Config.enableIntraRefresh = 1;
            m_stEncodeConfig.encodeCodecConfig.h264Config.intraRefreshPeriod = pEncCfg->intraRefreshPeriod;
            m_stEncodeConfig.encodeCodecConfig.h264Config.intraRefreshCnt = pEncCfg->intraRefreshDuration;
        }
    }

    if (pEncCfg->invalidateRefFramesEnableFlag)
    {
        if (pEncCfg->codec == NV_ENC_HEVC)
        {
            m_stEncodeConfig.encodeCodecConfig.hevcConfig.maxNumRefFramesInDPB = 16;
        }
        else
        {
            m_stEncodeConfig.encodeCodecConfig.h264Config.maxNumRefFrames = 16;
        }
    }

    if (pEncCfg->qpDeltaMapFile)
    {
        m_stEncodeConfig.rcParams.enableExtQPDeltaMap = 1;
    }
    if (pEncCfg->codec == NV_ENC_H264)
    {
        m_stEncodeConfig.encodeCodecConfig.h264Config.idrPeriod = pEncCfg->gopLength;
    }
    else if (pEncCfg->codec == NV_ENC_HEVC)
    {
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = pEncCfg->gopLength;
    }

    NV_ENC_CAPS_PARAM stCapsParam;
    int asyncMode = 0;
    memset(&stCapsParam, 0, sizeof(NV_ENC_CAPS_PARAM));
    SET_VER(stCapsParam, NV_ENC_CAPS_PARAM);

    stCapsParam.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
    g_pEncodeAPI->nvEncGetEncodeCaps(m_hEncoder, m_stCreateEncodeParams.encodeGUID, &stCapsParam, &asyncMode);
    m_stCreateEncodeParams.enableEncodeAsync = asyncMode;

    pEncCfg->enableAsyncMode = asyncMode;

    if (pEncCfg->enableMEOnly == 1 || pEncCfg->enableMEOnly == 2)
    {
        stCapsParam.capsToQuery = NV_ENC_CAPS_SUPPORT_MEONLY_MODE;
        m_stCreateEncodeParams.enableMEOnlyMode =  true;
        int meonlyMode = 0;
        nvStatus = g_pEncodeAPI->nvEncGetEncodeCaps(m_hEncoder, m_stCreateEncodeParams.encodeGUID, &stCapsParam, &meonlyMode);
        if (nvStatus != NV_ENC_SUCCESS)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "Encode Session Initialization failed \n");
            return nvStatus;
        }
        else
        {
            if (meonlyMode == 1)
            {
				Log::writeMessage(LOG_RTSPSERV, 1, "NV_ENC_CAPS_SUPPORT_MEONLY_MODE  supported\n");
            }
            else
            {
				Log::writeMessage(LOG_RTSPSERV, 1, "NV_ENC_CAPS_SUPPORT_MEONLY_MODE not supported\n");
                return NV_ENC_ERR_UNSUPPORTED_DEVICE;
            }
        } 
    }

    if (pEncCfg->enableTemporalAQ == 1)
    {
        NV_ENC_CAPS_PARAM stCapsParam;
        memset(&stCapsParam, 0, sizeof(NV_ENC_CAPS_PARAM));
        SET_VER(stCapsParam, NV_ENC_CAPS_PARAM);
        stCapsParam.capsToQuery = NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ;
        int temporalAQSupported = 0;
        nvStatus = g_pEncodeAPI->nvEncGetEncodeCaps(m_hEncoder, m_stCreateEncodeParams.encodeGUID, &stCapsParam, &temporalAQSupported);
        if (nvStatus != NV_ENC_SUCCESS)
        {
			Log::writeMessage(LOG_RTSPSERV, 1, "Encode Session Initialization failed\n");
            return nvStatus;
        }
        else
        {
            if (temporalAQSupported == 1)
            {
                m_stEncodeConfig.rcParams.enableTemporalAQ = 1;
            }
            else
            {
				Log::writeMessage(LOG_RTSPSERV, 1, "NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ not supported\n");
                return NV_ENC_ERR_UNSUPPORTED_DEVICE;
            }
        }
    }

			Log::writeMessage(LOG_RTSPSERV, 1, "CreateEncoder,darWidth: %d\
			darHeight : %d\
			encodeWidth : %d\
			encodeHeight: %d \
			frameRateNum :%d \
			frameRateDen :%d \
			maxEncodeWidth :%d \
			maxEncodeHeight :%d \
			enableEncodeAsync :%d \
			encodeConfig->frameFieldMode :%d \
			encodeConfig->frameIntervalP : %d \
			encodeConfig->gopLength : %d \
			encodeConfig->rcParams.maxBitRate :%d\
			encodeConfig->rcParams.rateControlMode:%d!\n",
			m_stCreateEncodeParams.darWidth,
			m_stCreateEncodeParams.darHeight,
			m_stCreateEncodeParams.encodeWidth,
			m_stCreateEncodeParams.encodeHeight,
			m_stCreateEncodeParams.frameRateNum,
			m_stCreateEncodeParams.frameRateDen,
			m_stCreateEncodeParams.maxEncodeWidth,
			m_stCreateEncodeParams.maxEncodeHeight,
			m_stCreateEncodeParams.enableEncodeAsync,
			m_stCreateEncodeParams.encodeConfig->frameFieldMode,
			m_stCreateEncodeParams.encodeConfig->frameIntervalP,
			m_stCreateEncodeParams.encodeConfig->gopLength,
			m_stCreateEncodeParams.encodeConfig->rcParams.maxBitRate,
			m_stCreateEncodeParams.encodeConfig->rcParams.rateControlMode);

    nvStatus = g_pEncodeAPI->nvEncInitializeEncoder(m_hEncoder, &m_stCreateEncodeParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "Encode Session Initialization failed, nvStatus = %d\n", nvStatus);
        return nvStatus;
    }
    m_bEncoderInitialized = true;

    return nvStatus;
}

GUID CNvHWEncoder::GetPresetGUID(char* encoderPreset, int codec)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    GUID presetGUID = NV_ENC_PRESET_DEFAULT_GUID;

    if (encoderPreset && (stricmp(encoderPreset, "hq") == 0))
    {
        presetGUID = NV_ENC_PRESET_HQ_GUID;
    }
    else if (encoderPreset && (stricmp(encoderPreset, "lowLatencyHP") == 0))
    {
        presetGUID = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
    }
    else if (encoderPreset && (stricmp(encoderPreset, "hp") == 0))
    {
        presetGUID = NV_ENC_PRESET_HP_GUID;
    }
    else if (encoderPreset && (stricmp(encoderPreset, "lowLatencyHQ") == 0))
    {
        presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    }
    else if (encoderPreset && (stricmp(encoderPreset, "lossless") == 0))
    {
        presetGUID = NV_ENC_PRESET_LOSSLESS_HP_GUID;
    }
    else
    {
        if (encoderPreset)
			Log::writeMessage(LOG_RTSPSERV, 1, "Unsupported preset guid %s\n", encoderPreset);
        presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
    }

    GUID inputCodecGUID = codec == NV_ENC_H264 ? NV_ENC_CODEC_H264_GUID : NV_ENC_CODEC_HEVC_GUID;
    nvStatus = ValidatePresetGUID(presetGUID, inputCodecGUID);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
		Log::writeMessage(LOG_RTSPSERV, 1, "Unsupported preset guid %s\n", encoderPreset);
    }

    return presetGUID;
}

NVENCSTATUS CNvHWEncoder::ProcessOutput(const EncodeBuffer *pEncodeBuffer, unsigned char **pbuffer, int *framelen, bool *is_key, uint64_t *timestamp, NV_ENC_PIC_TYPE *pictureType)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

	if (pEncodeBuffer == NULL || pbuffer == NULL || framelen == NULL || is_key == NULL || timestamp == NULL)
		return NV_ENC_ERR_INVALID_PARAM;
	if ((pEncodeBuffer->stOutputBfr.hBitstreamBuffer == NULL && pEncodeBuffer->stOutputBfr.bEOSFlag == FALSE) || pbuffer == NULL)
		return NV_ENC_ERR_INVALID_PARAM;

	if (pEncodeBuffer->stOutputBfr.bWaitOnEvent == TRUE)
	{
		if (!pEncodeBuffer->stOutputBfr.hOutputEvent)
		{
			return NV_ENC_ERR_INVALID_PARAM;
		}
#if defined(NV_WINDOWS)
		WaitForSingleObject(pEncodeBuffer->stOutputBfr.hOutputEvent, INFINITE);
#endif
	}
	if (pEncodeBuffer->stOutputBfr.bEOSFlag)
		return NV_ENC_SUCCESS;

	nvStatus = NV_ENC_SUCCESS;
	NV_ENC_LOCK_BITSTREAM lockBitstreamData;
	memset(&lockBitstreamData, 0, sizeof(lockBitstreamData));
	SET_VER(lockBitstreamData, NV_ENC_LOCK_BITSTREAM);
	lockBitstreamData.outputBitstream = pEncodeBuffer->stOutputBfr.hBitstreamBuffer;
	lockBitstreamData.doNotWait = false;

	nvStatus = g_pEncodeAPI->nvEncLockBitstream(m_hEncoder, &lockBitstreamData);
	if (nvStatus == NV_ENC_SUCCESS)
	{
		*framelen = lockBitstreamData.bitstreamSizeInBytes;
		*pbuffer = (unsigned char*)lockBitstreamData.bitstreamBufferPtr;
		*timestamp = lockBitstreamData.outputTimeStamp;
		*is_key = lockBitstreamData.pictureType == NV_ENC_PIC_TYPE_IDR ? true : false;
		*pictureType = lockBitstreamData.pictureType;
		nvStatus = g_pEncodeAPI->nvEncUnlockBitstream(m_hEncoder, pEncodeBuffer->stOutputBfr.hBitstreamBuffer);
	}
	else
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s : %s Lock BitStream Failed, line:%d", __FILE__, __FUNCTION__, __LINE__);
	}

	return nvStatus;
}

NVENCSTATUS CNvHWEncoder::ProcessMVOutput(const MotionEstimationBuffer *pMEBuffer)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    if (pMEBuffer->stOutputBfr.hBitstreamBuffer == NULL && pMEBuffer->stOutputBfr.bEOSFlag == FALSE)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }

    if (pMEBuffer->stOutputBfr.bWaitOnEvent == TRUE)
    {
        if (!pMEBuffer->stOutputBfr.hOutputEvent)
        {
            return NV_ENC_ERR_INVALID_PARAM;
        }
#if defined(NV_WINDOWS)
        WaitForSingleObject(pMEBuffer->stOutputBfr.hOutputEvent, INFINITE);
#endif
    }

    if (pMEBuffer->stOutputBfr.bEOSFlag)
        return NV_ENC_SUCCESS;

    nvStatus = NV_ENC_SUCCESS;
    NV_ENC_LOCK_BITSTREAM lockBitstreamData;
    memset(&lockBitstreamData, 0, sizeof(lockBitstreamData));
    SET_VER(lockBitstreamData, NV_ENC_LOCK_BITSTREAM);
    lockBitstreamData.outputBitstream = pMEBuffer->stOutputBfr.hBitstreamBuffer;
    lockBitstreamData.doNotWait = false;

    nvStatus = g_pEncodeAPI->nvEncLockBitstream(m_hEncoder, &lockBitstreamData);
    if (nvStatus == NV_ENC_SUCCESS)
    {
        if (codecGUID == NV_ENC_CODEC_H264_GUID)
        {
            unsigned int numMBs = ((m_uMaxWidth + 15) >> 4) * ((m_uMaxHeight + 15) >> 4);
            NV_ENC_H264_MV_DATA *outputMV = (NV_ENC_H264_MV_DATA *)lockBitstreamData.bitstreamBufferPtr;
        }
        else
        {
            unsigned int numCTBs = ((m_uMaxWidth + 31) >> 5) * ((m_uMaxHeight + 31) >> 5);
            NV_ENC_HEVC_MV_DATA *outputMV = (NV_ENC_HEVC_MV_DATA *)lockBitstreamData.bitstreamBufferPtr;
            bool lastCUInCTB = false;
            for (unsigned int i = 0; i < numCTBs; i++)
            {
                do
                {
                    lastCUInCTB = outputMV->lastCUInCTB ? true : false;
                    outputMV += 1;
                } while (!lastCUInCTB);
            }
        }
        nvStatus = g_pEncodeAPI->nvEncUnlockBitstream(m_hEncoder, pMEBuffer->stOutputBfr.hBitstreamBuffer);
    }
    else
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "lock bitstream function failed \n");
    }

    return nvStatus;
}

NVENCSTATUS CNvHWEncoder::Initialize(void* device, NV_ENC_DEVICE_TYPE deviceType)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    nvStatus = NvEncOpenEncodeSessionEx(device, deviceType);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncOpenEncodeSessionEx error!\n");
		return nvStatus;
	}
    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvHWEncoder::NvEncEncodeFrame(EncodeBuffer *pEncodeBuffer, NvEncPictureCommand *encPicCommand,
                                           uint32_t width, uint32_t height, NV_ENC_PIC_STRUCT ePicStruct,
                                           int8_t *qpDeltaMapArray, uint32_t qpDeltaMapArraySize, 
                                           NVENC_EXTERNAL_ME_HINT *meExternalHints, NVENC_EXTERNAL_ME_HINT_COUNTS_PER_BLOCKTYPE meHintCountsPerBlock[])
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_PIC_PARAMS encPicParams;

    memset(&encPicParams, 0, sizeof(encPicParams));
    SET_VER(encPicParams, NV_ENC_PIC_PARAMS);

    encPicParams.inputBuffer = pEncodeBuffer->stInputBfr.hInputSurface;
    encPicParams.bufferFmt = pEncodeBuffer->stInputBfr.bufferFmt;
    encPicParams.inputWidth = width;
    encPicParams.inputHeight = height;
    encPicParams.outputBitstream = pEncodeBuffer->stOutputBfr.hBitstreamBuffer;
    encPicParams.completionEvent = pEncodeBuffer->stOutputBfr.hOutputEvent;
    encPicParams.inputTimeStamp = m_EncodeIdx;
    encPicParams.pictureStruct = ePicStruct;
    encPicParams.qpDeltaMap = qpDeltaMapArray;
    encPicParams.qpDeltaMapSize = qpDeltaMapArraySize;

    // reconfigure encoder if hints are available and not set for hints, or no hints and is set for hints.
    if ((meExternalHints && !m_stCreateEncodeParams.enableExternalMEHints)
        || (!meExternalHints && m_stCreateEncodeParams.enableExternalMEHints))
    {
        NV_ENC_RECONFIGURE_PARAMS stReconfigParams;
        memset(&stReconfigParams, 0, sizeof(stReconfigParams));
        memcpy(&stReconfigParams.reInitEncodeParams, &m_stCreateEncodeParams, sizeof(m_stCreateEncodeParams));
        stReconfigParams.version = NV_ENC_RECONFIGURE_PARAMS_VER;

        if (meExternalHints)
        {
            m_stCreateEncodeParams.enableExternalMEHints = 1;
            stReconfigParams.reInitEncodeParams.enableExternalMEHints = 1;
            memcpy(stReconfigParams.reInitEncodeParams.maxMEHintCountsPerBlock, 
                   m_stCreateEncodeParams.maxMEHintCountsPerBlock, sizeof(m_stCreateEncodeParams.maxMEHintCountsPerBlock));
        }
        else
        {
            m_stCreateEncodeParams.enableExternalMEHints = 0;
            stReconfigParams.reInitEncodeParams.enableExternalMEHints = 0;
        }
        nvStatus = g_pEncodeAPI->nvEncReconfigureEncoder(m_hEncoder, &stReconfigParams); // no reset/idr since streaming is infinite IDR
        if (nvStatus != NV_ENC_SUCCESS)
        {
            assert(0);
            return nvStatus;
        }
    }

    if (meExternalHints)
    {
        encPicParams.meHintCountsPerBlock[0] = meHintCountsPerBlock[0];
        encPicParams.meExternalHints = meExternalHints;
    }

    if (encPicCommand)
    {
        if (encPicCommand->bForceIDR)
        {
            encPicParams.encodePicFlags |= NV_ENC_PIC_FLAG_FORCEIDR;
        }

        if (encPicCommand->bForceIntraRefresh)
        {
            if (codecGUID == NV_ENC_CODEC_HEVC_GUID)
            {
                encPicParams.codecPicParams.hevcPicParams.forceIntraRefreshWithFrameCnt = encPicCommand->intraRefreshDuration;
            }
            else
            {
                encPicParams.codecPicParams.h264PicParams.forceIntraRefreshWithFrameCnt = encPicCommand->intraRefreshDuration;
            }
        }
    }

    nvStatus = g_pEncodeAPI->nvEncEncodePicture(m_hEncoder, &encPicParams);
    if (nvStatus != NV_ENC_SUCCESS && nvStatus != NV_ENC_ERR_NEED_MORE_INPUT)
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "NvEncEncodeFrame error: %d!\n", nvStatus);
        return nvStatus;
    }

    m_EncodeIdx++;

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvHWEncoder::NvEncFlushEncoderQueue(void *hEOSEvent)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    NV_ENC_PIC_PARAMS encPicParams;
    memset(&encPicParams, 0, sizeof(encPicParams));
    SET_VER(encPicParams, NV_ENC_PIC_PARAMS);
    encPicParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    encPicParams.completionEvent = hEOSEvent;
    nvStatus = g_pEncodeAPI->nvEncEncodePicture(m_hEncoder, &encPicParams);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
    }
    return nvStatus;
}

