#include "NvDecode.h"
#include "LogDeliver.h"

#include <string>
#pragma warning(disable : 4996)

//#include <d3d11.h>
//#include "D3DX11.h"

using namespace std;
#define PTX_FILE "NV12ToARGB_drvapi_x64.ptx"

#define        LOG_VIDEO_NVIDIA_DECODE            ((long long)1<<49)
#define        LOG_VIDEO_NVIDIA_PROCESS            100

const char g_start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
const int g_start_code_len = 4;
const char g_start_code_thr[3] = { 0x00, 0x00, 0x01 };
const int g_start_code_len_thr = 3;
const char g_start_code_six[6] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10 };
const int g_start_code_len_six = 6;

static const char *last_strstr(const char *src, const char *needle, int len, int startcode)
{
	const char *q = src + startcode;
	const char *result = NULL;
	for (int i = 0; i < len - startcode; i++)
	{
		if (memcmp(q, needle, startcode) == 0)
		{
			result = q;
			break;
		}
		q++;
	}
	return result;
}

NvidiaDecode::NvidiaDecode()
{
	memset(m_sps, 0, MAX_SPS_PPS_LEN);
	memset(m_pps, 0, MAX_SPS_PPS_LEN);
	memset(aDisplayQueue_, 0, cnMaximumSize * sizeof(CUVIDPARSERDISPINFO));
	memset(aIsFrameInUse_, 0, cnMaximumSize);
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : NvidiaDecode construction success", __FILE__);
}

NvidiaDecode::~NvidiaDecode()
{
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : NvidiaDecode deconstruction success", __FILE__);
}

int NvidiaDecode::Init()
{
	int ret = ERROR_NONE;
	if (m_b_initialize)
		return ret;
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Begin", __FILE__, __FUNCTION__);
	if (m_width <= 0 || m_height <= 0 || m_width % 2 != 0 || m_height % 2 != 0)
		return ERROR_INVALID_PARAM;

	bool bSuc = InitD3D11();
	if (!bSuc)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s InitD3D11 error, line: %d", __FILE__, __FUNCTION__, __LINE__);
		return ERROR_INIT;
	}
	ret = InitCuda(0);
	if (ret != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s InitCuda error, line: %d", __FILE__, __FUNCTION__, __LINE__);
		Release();
		return ERROR_INIT;
	}

	m_lock = new CRITICAL_SECTION;
	if (m_lock == NULL)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s get lock failed, line: %d", __FILE__, __FUNCTION__, __LINE__);
		Release();
		return ERROR_MALLOC_FAIL;
	}
	InitializeCriticalSection(m_lock);

	m_b_initialize = true;
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s End", __FILE__, __FUNCTION__, __LINE__);
	return ERROR_NONE;
}

int NvidiaDecode::Release()
{
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s, %s Begin", __FILE__, __FUNCTION__, __LINE__);
	FlushDecoder();

	if (m_pU)
	{
		free(m_pU);
		m_pU = nullptr;
	}
	if (m_pV)
	{
		free(m_pV);
		m_pV = nullptr;
	}

	if (OUTTEX)
	{
// 		CUresult result = cuCtxPushCurrent(m_cuContext);
// 		cuGraphicsUnregisterResource(m_CudaResource);
// 		cuCtxPopCurrent(NULL);
// 
// 		if (m_pTexture)
// 			m_pTexture->Release();
// 		if (m_pTextureOutput)
// 			m_pTextureOutput->Release();
	}

	m_b_initialize = false;
	UninitCuda();

	DeleteCriticalSection(m_lock);
	if (m_lock)
	{
		delete m_lock;
		m_lock = nullptr;
	}
	if (m_pD3DDevice)
	{
		m_pD3DDevice->Release();
		m_pD3DDevice = NULL;
	}
	if (m_pD3DContext)
	{
		m_pD3DContext->Release();
		m_pD3DContext = NULL;
	}

	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s, %s End", __FILE__, __FUNCTION__, __LINE__);
	return ERROR_NONE;
}

bool NvidiaDecode::InitD3D11()
{
	bool bRet = false;
	HRESULT err;
	IDXGIFactory1 *factory;
	if (SUCCEEDED(err = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)))
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
							NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pD3DDevice), NULL, (&m_pD3DContext));
						if (FAILED(err))
						{
							Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "Problem while creating %d D3d11 device \n", i);
							return false;
						}
						bRet = true;
						giAdapter->Release();
						break;
					}
				}
			}
			giAdapter->Release();
		}
		factory->Release();
	}

	return bRet;
}

int NvidiaDecode::InitCuda(uint32_t deviceID)
{
	CUresult cresult = CUDA_SUCCESS;
	typedef HMODULE CUDADRIVER;
	CUDADRIVER hHandleDriver = 0;
	CUdevice cuda_device;
	int _deviceCount = 0;
	int  SMminor = 0, SMmajor = 0;
	CUcontext cuContextCurr = NULL;
	cresult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuInit error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return ERROR_INIT;
	}
	cresult = cuvidInit(0);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidInit error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return ERROR_INIT;
	}

	cresult = cuDeviceGetCount(&_deviceCount);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuDeviceGetCount error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}
	for (int i = 0; i < _deviceCount; i++)
	{
		CUdevice tmpdev;
		if (cuDeviceGet(&tmpdev, i) == CUDA_SUCCESS)
		{
			char name[1024];
			if (cuDeviceGetName(name, 1024, tmpdev) == CUDA_SUCCESS)
			{
				Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Cuda device[%d], name : %s", __FILE__, __FUNCTION__, i, name);
			}
		}
	}

	if ((int)deviceID < 0)
		deviceID = 0;
	if (deviceID >(unsigned int)_deviceCount - 1)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Invalid Device id = %d, line:%d", __FILE__, __FUNCTION__, deviceID, __LINE__);
		return ERROR_INVALID_PARAM;
	}

	cresult = cuDeviceGet(&cuda_device, deviceID);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuDeviceGet error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}
	char name[1024];
	if (cuDeviceGetName(name, 1024, cuda_device) == CUDA_SUCCESS)
	{
		Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Using Cuda device : %s", __FILE__, __FUNCTION__, name);
	}
	size_t totalGlobalMem;
	if (cuDeviceTotalMem(&totalGlobalMem, cuda_device) == CUDA_SUCCESS)
	{
		Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Total amount of global memory : %4.4fMB", __FILE__, __FUNCTION__, (float)totalGlobalMem / (1024 * 1024));
	}
	cresult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuDeviceComputeCapability error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}
	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s Device compute capability : %d.%d", __FILE__, __FUNCTION__, SMmajor, SMminor);
	if (((SMmajor << 4) + SMminor) < 0x30)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s GPU %d does not have NVENC capabilities exiting, line:%d", __FILE__, __FUNCTION__, deviceID, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}
	cresult = cuD3D11CtxCreate(&m_cuContext, &cuda_device, CU_CTX_BLOCKING_SYNC, m_pD3DDevice);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuCtxCreate error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}

	// in this branch we use compilation with parameters
	const unsigned int jitNumOptions = 3;
	CUjit_option *jitOptions = new CUjit_option[jitNumOptions];
	void **jitOptVals = new void *[jitNumOptions];

	// set up size of compilation log buffer
	jitOptions[0] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
	int jitLogBufferSize = 1024;
	jitOptVals[0] = (void *)(size_t)jitLogBufferSize;

	// set up pointer to the compilation log buffer
	jitOptions[1] = CU_JIT_INFO_LOG_BUFFER;
	char *jitLogBuffer = new char[jitLogBufferSize];
	jitOptVals[1] = jitLogBuffer;

	// set up pointer to set the Maximum # of registers for a particular kernel
	jitOptions[2] = CU_JIT_MAX_REGISTERS;
	int jitRegCount = 32;
	jitOptVals[2] = (void *)(size_t)jitRegCount;

	string ptx_source;
	char szPath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szPath, MAX_PATH);
	(strrchr(szPath, '\\'))[0] = 0;
	string ptx_path = szPath;
	ptx_path.append("\\");
	ptx_path.append(PTX_FILE);

	FILE *fp = fopen(ptx_path.c_str(), "rb");
	if (!fp)
	{
		Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "Unable to read ptx file %s\n", PTX_FILE);
		return CUDA_ERROR_FILE_NOT_FOUND;
	}
	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	char *buf = new char[file_size + 1];
	fseek(fp, 0, SEEK_SET);
	fread(buf, sizeof(char), file_size, fp);
	fclose(fp);
	buf[file_size] = '\0';
	ptx_source = buf;
	delete[] buf;

	CUresult  cuResult = CUDA_SUCCESS;
	cuResult = cuModuleLoadDataEx(&m_cuModule, ptx_source.c_str(), jitNumOptions, jitOptions, (void **)jitOptVals);
	if (cuResult != CUDA_SUCCESS)
	{
		Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "cuModuleLoadDataEx error:0x%x\n", cuResult);
		return CUDA_ERROR_FILE_NOT_FOUND;
	}

	delete[] jitOptions;
	delete[] jitOptVals;
	delete[] jitLogBuffer;

	cuModuleGetFunction(&m_cuNV12ToARGBFunction, m_cuModule, "NV12ToARGB_drvapi");

	cresult = cuvidCtxLockCreate(&m_ctxLock, m_cuContext);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidCtxLockCreate error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}

	{
		CCtxAutoLock lck(m_ctxLock);
		cresult = cuStreamCreate(&m_ReadbackSID, 0);
		if (cresult != CUDA_SUCCESS)
		{
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuStreamCreate error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
			return CUDA_ERROR_NO_DEVICE;
		}
		cresult = cuStreamCreate(&m_KernelSID, 0);
		if (cresult != CUDA_SUCCESS)
		{
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuStreamCreate error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
			return CUDA_ERROR_NO_DEVICE;
		}
	}

	cresult = cuCtxPopCurrent(&cuContextCurr);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuCtxPopCurrent error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NO_DEVICE;
	}
	return CUDA_SUCCESS;
}

int NvidiaDecode::UninitCuda()
{
	CUresult cuResult = CUDA_SUCCESS;

	if (OUTTEX)
	{
// 		if (m_cuContext)
// 			cuCtxPushCurrent(m_cuContext);
// 		if (m_pRGBA)
// 		{
// 			cuResult = cuMemFree(m_pRGBA);
// 			if (cuResult != CUDA_SUCCESS)
// 				Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuMemFree error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
// 			m_pRGBA = NULL;
// 		}
// 		cuCtxPopCurrent(NULL);
	}

	if (m_pDecHandle)
	{
		cuResult = cuvidDestroyDecoder(m_pDecHandle);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidDestroyDecoder error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_pDecHandle = NULL;
	}
	if (m_cuda_parser)
	{
		cuResult = cuvidDestroyVideoParser(m_cuda_parser);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidDestroyVideoParser error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_cuda_parser = NULL;
	}
	if (m_ReadbackSID)
	{
		cuResult = cuStreamDestroy(m_ReadbackSID);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuStreamDestroy error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_ReadbackSID = NULL;
	}
	if (m_KernelSID)
	{
		cuResult = cuStreamDestroy(m_KernelSID);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuStreamDestroy error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_KernelSID = NULL;
	}
	if (m_out_buffer)
	{
		cuResult = cuMemFreeHost(m_out_buffer);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuMemFreeHost error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_out_buffer = NULL;
		m_out_buffer_len = 0;
	}
	if (m_ctxLock)
	{
		cuResult = cuvidCtxLockDestroy(m_ctxLock);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidCtxLockDestroy error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_ctxLock = NULL;
	}
	if (m_cuContext)
	{
		cuResult = cuCtxDestroy(m_cuContext);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuCtxDestroy error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
		m_cuContext = NULL;
	}

	return ERROR_NONE;
}

int NvidiaDecode::CreateDecoder(CUVIDEOFORMAT *pFormat)
{
	if (NULL == pFormat)
		return ERROR_INVALID_PARAM;

	if (NULL == m_pDecHandle)
	{
		memset(&m_cuda_decodeinfo, 0, sizeof(CUVIDDECODECREATEINFO));
		m_cuda_decodeinfo.ChromaFormat = cudaVideoChromaFormat_420;//pFormat->chroma_format;
		m_cuda_decodeinfo.CodecType = pFormat->codec;
		m_cuda_decodeinfo.ulWidth = pFormat->coded_width;
// 		m_cuda_decodeinfo.ulHeight = m_height;
// 		m_deviceHeight = m_height;
		m_cuda_decodeinfo.ulHeight = pFormat->coded_height;
		m_deviceHeight = pFormat->coded_height;
		m_cuda_decodeinfo.ulNumDecodeSurfaces = DECODESURFACECOUNT;
		m_cuda_decodeinfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
		m_cuda_decodeinfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
		// No scaling
		m_cuda_decodeinfo.ulTargetWidth = pFormat->coded_width;
// 		m_cuda_decodeinfo.ulTargetHeight = m_height;
		m_cuda_decodeinfo.ulTargetHeight = pFormat->coded_height;
		m_cuda_decodeinfo.ulNumOutputSurfaces = OUTPUTSURFACECOUNT;  // We won't simultaneously map more than 8 surfaces
		m_cuda_decodeinfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
		m_cuda_decodeinfo.vidLock = m_ctxLock;
		CUresult cResult = cuvidCreateDecoder(&m_pDecHandle, &m_cuda_decodeinfo);
		if (cResult != CUDA_SUCCESS)
		{
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidCreateDecoder error:0x%x, line:%d", __FILE__, __FUNCTION__, cResult, __LINE__);
			return CUDA_ERROR_NOT_INITIALIZED;
		}
		bCreateDecode = true;
	}

	return ERROR_NONE;
}

void NvidiaDecode::NvSendPacket(InputPacket *pPacket)
{
	if (NULL == m_lock)
		return;
	EnterCriticalSection(m_lock);
	if (!m_b_initialize)
	{
		LeaveCriticalSection(m_lock);
		return;
	}
	LeaveCriticalSection(m_lock);
	if (pPacket == NULL)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s :%s invalid video buffer, line: %d", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	//¼æÈÝTS
	// 	if (pbuffer->is_key == false && m_b_dec_init == false)
	// 	{
	// 		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s video decode has not init, line: %d", __FILE__, __FUNCTION__, __LINE__);
	// 		return;
	// 	}
	if (pPacket->pFrame == NULL || pPacket->frameLen <= 0 || pPacket->dts < 0)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s :%s invalid video buffer param, line: %d", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	int avg_frame_rate_num = pPacket->avg_frame_rate_num;
	int avg_frame_rate_den = pPacket->avg_frame_rate_den;
	int r_frame_rate_num = pPacket->r_frame_rate_num;
	int r_frame_rate_den = pPacket->r_frame_rate_den;
	time_base_num = pPacket->time_base_num;
	time_base_den = pPacket->time_base_den;

	float avg = (float)avg_frame_rate_num / (float)avg_frame_rate_den;
	float r = (float)r_frame_rate_num / (float)r_frame_rate_den;
	if (avg < r)
	{
		m_duration = (time_base_den * avg_frame_rate_den + m_precision) / (time_base_num * avg_frame_rate_num);
		m_precision = time_base_den * (avg_frame_rate_den + m_precision) % (time_base_num * avg_frame_rate_num);
	}
	else
	{
		m_duration = (time_base_den * r_frame_rate_den + m_precision) / (time_base_num * r_frame_rate_num);
		m_precision = time_base_den * (r_frame_rate_den + m_precision) % (time_base_num * r_frame_rate_num);
	}
	m_duration = m_duration * 1000 * time_base_num / time_base_den;

	EnterCriticalSection(m_lock);
	if (m_b_dec_init == false)
	{
		int ret = decode_head_and_init(pPacket);
		if (ret == ERROR_NONE)
		{
			m_b_dec_init = true;
		}
		else
		{
			LeaveCriticalSection(m_lock);
			return;
		}
	}
	LeaveCriticalSection(m_lock);
	if (NULL == m_cuda_parser)
		return;
	CUVIDSOURCEDATAPACKET _packet = {};
	_packet.payload_size = pPacket->frameLen;
	_packet.payload = pPacket->pFrame;
	_packet.flags = CUVID_PKT_TIMESTAMP;
	_packet.timestamp = pPacket->pts;
	CUresult oResult = cuvidParseVideoData(m_cuda_parser, &_packet);
	if (oResult != CUDA_SUCCESS)
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidParseVideoData failed ,result:0x%x ,line:%d", __FILE__, __FUNCTION__, __LINE__);

	if (bCreateDecode && OUTTEX)
	{
// 		cuCtxPushCurrent(m_cuContext);
// 
// 		D3D11_TEXTURE2D_DESC desc = {
// 			m_width, m_deviceHeight, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, { 1, 0 },
// 			D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, 0
// 		};
// 
// 		HRESULT hResult = m_pD3DDevice->CreateTexture2D(&desc, NULL, &m_pTexture);
// 		cuGraphicsD3D11RegisterResource(&m_CudaResource, m_pTexture, CU_GRAPHICS_REGISTER_FLAGS_NONE);
// 		cuGraphicsResourceSetMapFlags(m_CudaResource, CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD);
// 
// 		D3D11_TEXTURE2D_DESC td;
// 		memset(&td, 0, sizeof(td));
// 		td.Width = m_width;
// 		td.Height = m_deviceHeight;
// 		td.MipLevels = 1;
// 		td.ArraySize = 1;
// 		td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
// 		td.SampleDesc = { 1, 0 };
// 		td.Usage = D3D11_USAGE_DEFAULT;
// 		td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
// 		td.CPUAccessFlags = 0;
// 		td.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
// 
// 		hResult = m_pD3DDevice->CreateTexture2D(&td, NULL, &m_pTextureOutput);
// 		IDXGIResource* pDXGIResource = NULL;
// 		hResult = m_pTextureOutput->QueryInterface(__uuidof(IDXGIResource), (LPVOID*)&pDXGIResource);
// 		hResult = pDXGIResource->GetSharedHandle(&m_sharedHandle);
// 		pDXGIResource->Release();
// 
// 		cuMemAlloc(&m_pRGBA, m_width * m_deviceHeight * 4);
// 
// 		cuCtxPopCurrent(NULL);
// 		bCreateDecode = false;
	}
	return;
}

void NvidiaDecode::NvReceiveFrame(OutFrame **ppOutFrame)
{
	CUVIDPARSERDISPINFO oDisplayInfo;
	oDisplayInfo.picture_index = -1;
	if (m_lock)
	{
		bool bDeque = false;
		EnterCriticalSection(m_lock);
		if (nFramesInQueue_ > 0)
		{
			oDisplayInfo = aDisplayQueue_[nReadPosition_];
			nReadPosition_ = (nReadPosition_ + 1) % cnMaximumSize;
			nFramesInQueue_--;
			bDeque = true;
		}
		LeaveCriticalSection(m_lock);

		if (bDeque)
		{
			deliver_output(&oDisplayInfo, ppOutFrame);
			aIsFrameInUse_[oDisplayInfo.picture_index] = false;
		}
	}
}

int NvidiaDecode::decode_head_and_init(InputPacket *pBuffer)
{
	unsigned char *pbuffer = pBuffer->pSpspps;
	int nLenSpspps = pBuffer->nLenSpspps;
	if (pBuffer->bTS || memcmp(pbuffer, g_start_code_six, g_start_code_len_six) == 0)
	{
		pbuffer += 6;
		nLenSpspps -= 6;
	}

	int naltype = 0;
	if (memcmp(pbuffer, g_start_code, g_start_code_len) == 0)
		naltype = pbuffer[g_start_code_len] & 0x1f;
	else if (memcmp(pbuffer, g_start_code_thr, g_start_code_len_thr) == 0)
		naltype = pbuffer[g_start_code_len_thr] & 0x1f;
	const char *pps = last_strstr((const char*)pbuffer, g_start_code, MAX_SPS_PPS_LEN, g_start_code_len);
	if (pps == NULL)
	{
		pps = last_strstr((const char*)pbuffer, g_start_code_thr, MAX_SPS_PPS_LEN, g_start_code_len_thr);
	}
	if (naltype == H264_SPS && pps == NULL)
	{
		//pframe = sps
		m_spslen = nLenSpspps;
		memcpy(m_sps, pbuffer, m_spslen);
	}
	else if (naltype == H264_SPS && pps)
	{
		//pframe = sps + pps +...
		m_spslen = pps - (const char*)pbuffer;
		memcpy(m_sps, pbuffer, m_spslen);
		const char *iframe = last_strstr(pps, g_start_code, MAX_SPS_PPS_LEN, g_start_code_len);
		if (iframe == NULL)
		{
			iframe = last_strstr(pps, g_start_code_thr, MAX_SPS_PPS_LEN, g_start_code_len_thr);
		}
		if (memcmp(pps, g_start_code, g_start_code_len) == 0)
			naltype = pps[g_start_code_len] & 0x1f;
		else if (memcmp(pps, g_start_code_thr, g_start_code_len_thr) == 0)
			naltype = pps[g_start_code_len_thr] & 0x1f;
		if (iframe == NULL && naltype == H264_PPS)
		{
			//pframe = sps + pps
			m_ppslen = nLenSpspps - m_spslen;
			memcpy(m_pps, pps, m_ppslen);
		}
		else if (iframe && naltype == H264_PPS)
		{
			//pframe = sps + pps + iframe +...
			m_ppslen = iframe - pps;
			memcpy(m_pps, pps, m_ppslen);
		}
	}
	else if (naltype == H264_PPS)
	{
		//pframe = pps
		m_ppslen = nLenSpspps;
		memcpy(m_pps, pbuffer, m_ppslen);
	}

	if (m_spslen <= 0 || m_ppslen <= 0)
		return ERROR_MORE_VIDEO_DATA;

	CUVIDPARSERPARAMS oVideoParserParameters;
	memset(&oVideoParserParameters, 0, sizeof(CUVIDPARSERPARAMS));
	oVideoParserParameters.CodecType = cudaVideoCodec_H264;
	oVideoParserParameters.ulMaxNumDecodeSurfaces = DECODESURFACECOUNT;//8;
	oVideoParserParameters.ulErrorThreshold = 0;
	oVideoParserParameters.ulMaxDisplayDelay = 1;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
	oVideoParserParameters.pUserData = this;
	oVideoParserParameters.pfnSequenceCallback = HandleVideoSequence;
	oVideoParserParameters.pfnDecodePicture = HandlePictureDecode;
	oVideoParserParameters.pfnDisplayPicture = HandlePictureDisplay;

	memset(&m_extra_data_info, 0, sizeof(CUVIDEOFORMATEX));
	memcpy(m_extra_data_info.raw_seqhdr_data, m_sps, m_spslen);
	memcpy(m_extra_data_info.raw_seqhdr_data + m_spslen, m_pps, m_ppslen);
	m_extra_data_info.format.seqhdr_data_length = m_spslen + m_ppslen;
	oVideoParserParameters.pExtVideoInfo = &m_extra_data_info;
	CUresult cresult = cuvidCreateVideoParser(&m_cuda_parser, &oVideoParserParameters);
	if (cresult != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidCreateVideoParser error:0x%x, line:%d", __FILE__, __FUNCTION__, cresult, __LINE__);
		return CUDA_ERROR_NOT_INITIALIZED;
	}
	CUVIDSOURCEDATAPACKET seq_pkt = {};
	seq_pkt.payload = m_extra_data_info.raw_seqhdr_data;
	seq_pkt.payload_size = m_extra_data_info.format.seqhdr_data_length;
	if (seq_pkt.payload_size > 0) {
		cuvidParseVideoData(m_cuda_parser, &seq_pkt);
	}
	return ERROR_NONE;
}

int CUDAAPI NvidiaDecode::HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat)
{
	NvidiaDecode *pInst = reinterpret_cast<NvidiaDecode*>(pUserData);
	if (pInst)
		pInst->CreateDecoder(pFormat);
	return 1;
}

int CUDAAPI	NvidiaDecode::HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams)
{
	NvidiaDecode *pInst = reinterpret_cast<NvidiaDecode*>(pUserData);
	if (pInst && pInst->m_pDecHandle)
	{
		if (pPicParams->CurrPicIdx >= 0 && pPicParams->CurrPicIdx < cnMaximumSize)
		{
			while (pInst->aIsFrameInUse_[pPicParams->CurrPicIdx])
			{
				Sleep(1);
			}
		}
		CUresult cuResult = cuvidDecodePicture(pInst->m_pDecHandle, pPicParams);
		if (cuResult != CUDA_SUCCESS)
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s HandlePictureDecode error:0x%x, line:%d", __FILE__, __FUNCTION__, cuResult, __LINE__);
	}
	return 1;
}

int CUDAAPI	NvidiaDecode::HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams)
{
	NvidiaDecode *pInst = reinterpret_cast<NvidiaDecode*>(pUserData);
	if (pInst)
	{
		pInst->aIsFrameInUse_[pPicParams->picture_index] = true;
		do
		{
			bool _bPlacedFrame = false;
			EnterCriticalSection(pInst->m_lock);
			if (pInst->nFramesInQueue_ < (int)cnMaximumSize)
			{
				int pos = (pInst->nReadPosition_ + pInst->nFramesInQueue_) % cnMaximumSize;
				pInst->aDisplayQueue_[pos] = *pPicParams;
				pInst->nFramesInQueue_++;
				_bPlacedFrame = true;
			}
			LeaveCriticalSection(pInst->m_lock);
			if (_bPlacedFrame) // Done
				break;
// 			Sleep(25);   // Wait a bit
			Sleep(5);   
		} while (true);
	}

	return 1;
}

CUresult  NvidiaDecode::cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, int nBytesPerSample,
	CUdeviceptr d_dstARGB, size_t nDestPitch,
	uint32_t width, uint32_t height,
	CUfunction fpFunc, CUstream streamID)
{
	CUresult status = CUDA_SUCCESS;

	dim3 block(32, 16, 1);
	dim3 grid((width + (2 * block.x - 1)) / (2 * block.x), (height + (block.y - 1)) / block.y, 1);

	void *args[] = { &d_srcNV12, &nSourcePitch, &nBytesPerSample,
		&d_dstARGB, &nDestPitch, &width, &height };

	status = cuLaunchKernel(fpFunc, grid.x, grid.y, grid.z,
		block.x, block.y, block.z, 0, streamID, args, NULL);

	return status;
}

typedef enum
{
	ITU601 = 1,
	ITU709 = 2
} eColorSpace;
__constant__ float  constHueColorSpaceMat[9];
__constant__ float  constAlpha;

void setColorSpaceMatrix(eColorSpace CSC, float *hueCSC, float hue)
{
	float hueSin = sin(hue);
	float hueCos = cos(hue);
	if (CSC == ITU601)
	{
		hueCSC[0] = 1.1644f;
		hueCSC[1] = hueSin * 1.5960f;
		hueCSC[2] = hueCos * 1.5960f;
		hueCSC[3] = 1.1644f;
		hueCSC[4] = (hueCos * -0.3918f) - (hueSin * 0.8130f);
		hueCSC[5] = (hueSin *  0.3918f) - (hueCos * 0.8130f);
		hueCSC[6] = 1.1644f;
		hueCSC[7] = hueCos *  2.0172f;
		hueCSC[8] = hueSin * -2.0172f;
	}
	else if (CSC == ITU709)
	{
		hueCSC[0] = 1.0f;
		hueCSC[1] = hueSin * 1.57480f;
		hueCSC[2] = hueCos * 1.57480f;
		hueCSC[3] = 1.0;
		hueCSC[4] = (hueCos * -0.18732f) - (hueSin * 0.46812f);
		hueCSC[5] = (hueSin *  0.18732f) - (hueCos * 0.46812f);
		hueCSC[6] = 1.0f;
		hueCSC[7] = hueCos *  1.85560f;
		hueCSC[8] = hueSin * -1.85560f;
	}
}

CUresult  updateConstantMemory_drvapi(CUmodule module, float *hueCSC)
{
	CUdeviceptr  d_constHueCSC, d_constAlpha;
	size_t       d_cscBytes, d_alphaBytes;

	cuModuleGetGlobal(&d_constHueCSC, &d_cscBytes, module, "constHueColorSpaceMat");
	cuModuleGetGlobal(&d_constAlpha, &d_alphaBytes, module, "constAlpha");

	cuMemcpyHtoD(d_constHueCSC, reinterpret_cast<const void *>(hueCSC), d_cscBytes);
	cuCtxSynchronize();

	uint32_t cudaAlpha = ((uint32_t)0xff << 24);
	cuMemcpyHtoD(d_constAlpha, reinterpret_cast<const void *>(&cudaAlpha), d_alphaBytes);
	cuCtxSynchronize();

	return CUDA_SUCCESS;
}

void NvidiaDecode::cudaPostProcess(CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch, size_t nDeviceHeight, int nBytesPerSample,
	CUarray array, CUmodule cuModNV12toARGB, CUfunction fpCudaKernel, CUstream streamID)
{
	if (m_bUpdateCSC)
	{
		float hueColorSpaceMat[9];
		// 		setColorSpaceMatrix(ITU601, hueColorSpaceMat, 0.0f);
		setColorSpaceMatrix(ITU709, hueColorSpaceMat, 0.0f);
		updateConstantMemory_drvapi(cuModNV12toARGB, hueColorSpaceMat);
		m_bUpdateCSC = false;
	}

	cudaLaunchNV12toARGBDrv(*ppDecodedFrame, nDecodedPitch, nBytesPerSample,
		m_pRGBA, m_width * 4, m_width, nDeviceHeight, fpCudaKernel, streamID);

	CUDA_MEMCPY2D memcpy2D = { 0 };
	memcpy2D.srcMemoryType = CU_MEMORYTYPE_DEVICE;
	memcpy2D.srcDevice = m_pRGBA;
	memcpy2D.srcPitch = m_width * 4;
	memcpy2D.dstMemoryType = CU_MEMORYTYPE_ARRAY;
	memcpy2D.dstArray = array;
	memcpy2D.dstPitch = m_width * 4;
	memcpy2D.WidthInBytes = m_width * 4;
	memcpy2D.Height = nDeviceHeight;
	cuMemcpy2D(&memcpy2D);
}

bool NvidiaDecode::deliver_output(CUVIDPARSERDISPINFO *pPicParams, OutFrame **ppOutFrame)
{
	CCtxAutoLock lck(m_ctxLock);
	cuCtxPushCurrent(m_cuContext);

	OutFrame *pOutFrame = (OutFrame*)malloc(sizeof(OutFrame));
	if (!pOutFrame)
		return false;
	memset(pOutFrame, 0, sizeof(OutFrame));

	int64_t pts = pPicParams->timestamp * 1000 * time_base_num / time_base_den;

	CUdeviceptr devptr = 0;
	CUVIDPROCPARAMS proc_params;
	memset(&proc_params, 0, sizeof(CUVIDPROCPARAMS));
	proc_params.progressive_frame = pPicParams->progressive_frame;
	proc_params.top_field_first = pPicParams->top_field_first;
	proc_params.unpaired_field = (pPicParams->repeat_first_field < 0);
	proc_params.second_field = 0;
	unsigned int pitch = 0;
	CUresult cuStatus = cuvidMapVideoFrame(m_pDecHandle, pPicParams->picture_index, &devptr, &pitch, &proc_params);
// 	Log::writeMessage(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidMapVideoFrame on index:%d, pts:%d", __FILE__, __FUNCTION__, pPicParams->picture_index, pPicParams->timestamp);

	if (cuStatus != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidMapVideoFrame failed on index:%d, status:%d", __FILE__, __FUNCTION__, pPicParams->picture_index, cuStatus);
		cuvidUnmapVideoFrame(m_pDecHandle, devptr);
		cuCtxPopCurrent(NULL);
		return false;
	}

	int w = m_width;
	int h = m_height;
	int ch = m_cuda_decodeinfo.ulHeight;
	int size = pitch * ch * 3 / 2;
	if (size > m_out_buffer_len && m_out_buffer)
	{
		cuMemFreeHost(m_out_buffer);
		m_out_buffer = NULL;
		m_out_buffer_len = 0;
	}
	if (m_out_buffer == NULL)
	{
		cuStatus = cuMemAllocHost((void**)&m_out_buffer, size);
		if (cuStatus != CUDA_SUCCESS)
		{
			Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuMemAllocHost failed 0x%x", __FILE__, __FUNCTION__, cuStatus);
			cuvidUnmapVideoFrame(m_pDecHandle, devptr);
			return false;
		}
		m_out_buffer_len = size;
	}
	if (m_out_buffer == NULL)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuMemAllocHost failed, line:%d", __FILE__, __FUNCTION__, __LINE__);
		cuvidUnmapVideoFrame(m_pDecHandle, devptr);
		return false;
	}
	cuStatus = cuMemcpyDtoHAsync(m_out_buffer, devptr, size, m_ReadbackSID);
	if (cuStatus != CUDA_SUCCESS)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s  cuMemcpyDtoHAsync failed 0x%x", __FILE__, __FUNCTION__, cuStatus);
		cuvidUnmapVideoFrame(m_pDecHandle, devptr);
		return false;
	}

	if (OUTTEX)
	{
// 		cuGraphicsMapResources(1, &m_CudaResource, 0);
// 		cuGraphicsSubResourceGetMappedArray(&m_backBufferArray, m_CudaResource, 0, 0);
// 		cudaPostProcess(&devptr, pitch, m_deviceHeight, 1, m_backBufferArray, m_cuModule, m_cuNV12ToARGBFunction, m_KernelSID);
// 		cuGraphicsUnmapResources(1, &m_CudaResource, 0);
	}

	cuvidUnmapVideoFrame(m_pDecHandle, devptr);

	if (OUTTEX)
	{
		// 	HRESULT hr1 = D3DX11SaveTextureToFile(m_pD3DContext, m_pTexture, D3DX11_IFF_JPG, L"1.jpg");
// 		m_pD3DContext->CopyResource(m_pTextureOutput, m_pTexture);
// 		m_pD3DContext->Flush();
		// 	hr1 = D3DX11SaveTextureToFile(m_pD3DContext, m_pTextureOutput, D3DX11_IFF_JPG, L"2.jpg");
	}

	cuStatus = cuStreamSynchronize(m_ReadbackSID);
	if (cuStatus != CUDA_SUCCESS)
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s  cuStreamSynchronize failed 0x%x", __FILE__, __FUNCTION__, cuStatus);

	unsigned char *src_y = m_out_buffer;
	unsigned char* src_uv = m_out_buffer + pitch*ch;
	size = pitch * h;
	if (NULL == m_pU)
	{
		m_pU = (char*)malloc(size >> 2);
		memset(m_pU, 0, sizeof(char)* (size >> 2));
	}
	if (NULL == m_pV)
	{
		m_pV = (char*)malloc(size >> 2);
		memset(m_pV, 0, sizeof(char)* (size >> 2));
	}
	int i, j;
	for (j = 0, i = 0; j < size / 2; j += 2, i++)
	{
		m_pU[i] = src_uv[j];
		m_pV[i] = src_uv[j + 1];
	}
	pOutFrame->frame_data[0] = (char*)src_y;
	pOutFrame->frame_data[1] = (char*)m_pU;
	pOutFrame->frame_data[2] = (char*)m_pV;
	pOutFrame->frame_size[0] = pitch;
	pOutFrame->frame_size[1] = pitch * 0.5f;
	pOutFrame->frame_size[2] = pitch * 0.5f;
	pOutFrame->media_type = 1;
	pOutFrame->width = w;
	pOutFrame->height = h;
	pOutFrame->pts = pts;
	pOutFrame->duration = m_duration;
	if (OUTTEX)
	{
// 		pOutFrame->deviceHeight = m_deviceHeight;
// 		pOutFrame->hShare = m_sharedHandle;
	}
	*ppOutFrame = pOutFrame;

	cuCtxPopCurrent(NULL);

	return true;
}

int NvidiaDecode::FlushDecoder()
{
	CUVIDSOURCEDATAPACKET _packet;
	memset(&_packet, 0, sizeof(CUVIDSOURCEDATAPACKET));
	_packet.flags = CUVID_PKT_ENDOFSTREAM;

	CUresult oResult = cuvidParseVideoData(m_cuda_parser, &_packet);
	if (oResult != CUDA_SUCCESS)
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s cuvidParseVideoData failed ,result:0x%x ,line:%d", __FILE__, __FUNCTION__, __LINE__);

	int a = nFramesInQueue_;
	int b = 0;

	return 0;
}

void* NvidiaDecode::NvDecodeCreate(int nWidth, int nHeight)
{
	m_width = nWidth;
	m_height = nHeight;
	int nRet = Init();
	if (nRet)
		return NULL;
	return this;
}

int NvidiaDecode::NvDecodeDestroy()
{
	Release();
	return 0;
}

int NvidiaDecode::QueryHardDecodeSupport()
{
	int nRet = 0;
	static LPCWSTR __CudaLibName = L"nvcuda.dll";
	void *_pCudaDll = LoadLibrary(__CudaLibName);
	if (_pCudaDll == NULL)
	{
		Log::writeError(LOG_VIDEO_NVIDIA_DECODE, LOG_VIDEO_NVIDIA_PROCESS, "%s : %s there is no nvcuda.dll ,result:0x%x ,line:%d", __FILE__, __FUNCTION__, __LINE__);
	}
	else
	{
		tcuInit *_cuInit = NULL;
		tcuDeviceGetCount *cuDeviceGetCount = NULL;
		_cuInit = (tcuInit *)GetProcAddress((HMODULE)_pCudaDll, "cuInit");
		cuDeviceGetCount = (tcuDeviceGetCount *)GetProcAddress((HMODULE)_pCudaDll, "cuDeviceGetCount");
		if (_cuInit != NULL && cuDeviceGetCount != NULL)
		{
			_cuInit(0);
			int _count = 0;
			cuDeviceGetCount(&_count);
			if (_count >= 1)
				nRet = 1;
		}
		FreeLibrary((HMODULE)_pCudaDll);
	}
	return nRet;
}

