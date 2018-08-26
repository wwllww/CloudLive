#ifndef _NVIDIA_DECODE_H__
#define _NVIDIA_DECODE_H__

#include <windows.h>
#include <vector>
#include <process.h>

#include <stdint.h>
#include "inc/dynlink_cuda_cuda.h"
#include "inc/dynlink_cuda.h"
#include "inc/dynlink_cudaD3D11.h"
#include "inc/helper_string.h"
#include "inc/dynlink_cuviddec.h"
#include "inc/dynlink_nvcuvid.h"
#include "inc/dynlink_builtin_types.h"

#include "NvCodecDef.h"

const unsigned int cnMaximumSize = 20;
const int MAX_SPS_PPS_LEN = 256;
#define	DECODESURFACECOUNT 20
#define OUTPUTSURFACECOUNT 2

typedef enum NALTYPE
{
	H264_SPS = 7,
	H264_PPS = 8,
};

typedef enum ERRORCODE
{
	ERROR_NONE = 0,
	ERROR_INVALID_PARAM = -1,
	ERROR_MALLOC_FAIL = -2,
	ERROR_SESSION_INIT = -3,
	ERROR_START = -4,
	ERROR_STOP = -5,
	ERROR_INIT = -6,
	ERROR_UNINIT = -7,
	ERROR_NO_FUN = -8,
	ERROR_ENV = -9,
	ERROR_QUEUE = -10,
	ERROR_BUFFER = -11,
	ERROR_MORE_VIDEO_DATA = -12,
	ERROR_NO_PLUGIN = -13,
	ERROR_EGL = -14,

	WARNING_HAS_NOT_INITIALIZED = 1
};

typedef void* LPVOID;
typedef unsigned long long uint64_t;

#define OUTTEX 0
class NvidiaDecode
{
public:
	NvidiaDecode();
	~NvidiaDecode();

	void* NvDecodeCreate(int nWidth, int nHeight);
	int   NvDecodeDestroy();
	void  NvSendPacket(InputPacket *pPacket);
	void  NvReceiveFrame(OutFrame **ppOutFrame);
	int   QueryHardDecodeSupport();

	int			m_width = 0;
	int			m_height = 0;
	char*		m_pU = NULL;
	char*		m_pV = NULL;
	int			m_deviceHeight = 0;

	int        m_precision = 0;
	int		   time_base_num = 0;
	int        time_base_den = 0;
	int64_t    m_duration = 0;

	bool			m_b_initialize = false;
	bool			bCreateDecode = false;

private:
	int Init();
	int Release();
	int CreateDecoder(CUVIDEOFORMAT *pFormat);
	int InitCuda(uint32_t deviceID);
	int UninitCuda();
	int FlushDecoder();

	int  decode_head_and_init(InputPacket *pBuffer);
	bool deliver_output(CUVIDPARSERDISPINFO *pPicParams, OutFrame **ppOutFrame);

	bool InitD3D11();
	void cudaPostProcess(CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch, size_t nDeviceHeight, int nBytesPerSample,
		CUarray array, CUmodule cuModNV12toARGB, CUfunction fpCudaKernel, CUstream streamID);
	CUresult cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, int nBytesPerSample,
		CUdeviceptr d_dstARGB, size_t nDestPitch, uint32_t width, uint32_t height,
		CUfunction fpNV12toARGB, CUstream streamID);

	static int CUDAAPI  HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat);
	static int CUDAAPI	HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams);
	static int CUDAAPI	HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams);

private:
	CUvideodecoder			m_pDecHandle = NULL;
	CUcontext               m_cuContext = NULL;
	CUVIDDECODECREATEINFO   m_cuda_decodeinfo;
	CUvideoparser			m_cuda_parser = NULL;
	CUvideoctxlock			m_ctxLock = NULL;
	CUVIDEOFORMATEX			m_extra_data_info;
	CUVIDPARSERDISPINFO     aDisplayQueue_[cnMaximumSize];
	bool					aIsFrameInUse_[cnMaximumSize];
	volatile int			nReadPosition_ = 0;
	volatile int			nFramesInQueue_ = 0;
	CRITICAL_SECTION        *m_lock = NULL;

	ID3D11Device			*m_pD3DDevice = NULL;
	ID3D11DeviceContext		*m_pD3DContext = NULL;
	ID3D11Texture2D			*m_pTexture = NULL;
	ID3D11Texture2D			*m_pTextureOutput = NULL;
	HANDLE					m_sharedHandle = NULL;
	CUgraphicsResource		m_CudaResource = NULL;
	CUdeviceptr				m_pRGBA = 0;
	CUarray					m_backBufferArray = NULL;
	CUstream				m_ReadbackSID = NULL;
	CUstream				m_KernelSID = NULL;
	CUmodule                m_cuModule = NULL;
	CUfunction              m_cuNV12ToARGBFunction = NULL;
	bool					m_bUpdateCSC = true;

	unsigned char*			m_out_buffer = NULL;
	int						m_out_buffer_len = 0;
	bool				m_b_dec_init = false;
	char				m_sps[MAX_SPS_PPS_LEN];
	char				m_pps[MAX_SPS_PPS_LEN];
	int					m_spslen = 0;
	int					m_ppslen = 0;
};

#endif