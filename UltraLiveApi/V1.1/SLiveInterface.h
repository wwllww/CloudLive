#ifndef SLIVEINTERFACE_H
#define SLIVEINTERFACE_H
#ifdef __cplusplus
#include <dshow.h>
#include "json/json.h"


#ifndef ULTRALIVEAPI_EXPORTS
enum DBOperation
{
	None = -1,
	Pause,
	Play,
	ReStart,
	PlayNext,
	PlayPrev,
	Stop
};

typedef unsigned long long  uint64_t;
typedef void(*AudioDBCallBack)(uint64_t StreamID, float leftDb,float rightDb);
typedef void(*NickNameCallBack)(uint64_t InstanceID, uint64_t StreamID, const char *NickName);

#endif // !ULTRALIVEAPI_EXPORTS

//色彩空间路径
#define SHADER_PATH TEXT("shaders/")

enum TColorType
{
	ColorType_RGB = 0,

	//planar 4:2:0
	ColorType_I420,
	ColorType_YV12,

	//packed 4:2:2
	ColorType_YVYU,
	ColorType_YUY2,
	ColorType_UYVY,
	ColorType_HDYC,

	//RGB24
	ColorType_RGB24,
	ColorType_RGBA32REVERSE,
};

enum DeviceColorType
{
	DeviceOutputType_RGB,

	//planar 4:2:0
	DeviceOutputType_I420,
	DeviceOutputType_YV12,

	//packed 4:2:2
	DeviceOutputType_YVYU,
	DeviceOutputType_YUY2,
	DeviceOutputType_UYVY,
	DeviceOutputType_HDYC,
	//RGB24
	DeviceOutputType_RGB24,
};

enum DeinterlacingType {
	DEINTERLACING_NONE,
	DEINTERLACING_DISCARD,
	DEINTERLACING_RETRO,
	DEINTERLACING_BLEND,
	DEINTERLACING_BLEND2x,
	DEINTERLACING_LINEAR,
	DEINTERLACING_LINEAR2x,
	DEINTERLACING_YADIF,
	DEINTERLACING_YADIF2x,
	DEINTERLACING__DEBUG,
	DEINTERLACING_TYPE_LAST
};

enum DeinterlacingFieldOrder {
	FIELD_ORDER_NONE,
	FIELD_ORDER_TFF = 1,
	FIELD_ORDER_BFF,
};

enum DeinterlacingProcessor {
	DEINTERLACING_PROCESSOR_CPU = 1,
	DEINTERLACING_PROCESSOR_GPU,
};

struct DeinterlacerConfig {
	int    type, fieldOrder, processor;
	bool   doublesFramerate;
};

__declspec(selectany) CTSTR deinterlacerLocalizations[DEINTERLACING_TYPE_LAST] = {
	TEXT("DeinterlacingType.None"),
	TEXT("DeinterlacingType.Discard"),
	TEXT("DeinterlacingType.Retro"),
	TEXT("DeinterlacingType.Blend"),
	TEXT("DeinterlacingType.Blend2x"),
	TEXT("DeinterlacingType.Linear"),
	TEXT("DeinterlacingType.Linear2x"),
	TEXT("DeinterlacingType.Yadif"),
	TEXT("DeinterlacingType.Yadif2x"),
	TEXT("DeinterlacingType.Debug")
};

__declspec(selectany) DeinterlacerConfig deinterlacerConfigs[DEINTERLACING_TYPE_LAST] = {
	{ DEINTERLACING_NONE, FIELD_ORDER_NONE, DEINTERLACING_PROCESSOR_CPU },
	{ DEINTERLACING_DISCARD, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_CPU },
	{ DEINTERLACING_RETRO, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_CPU | DEINTERLACING_PROCESSOR_GPU, true },
	{ DEINTERLACING_BLEND, FIELD_ORDER_NONE, DEINTERLACING_PROCESSOR_GPU },
	{ DEINTERLACING_BLEND2x, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU, true },
	{ DEINTERLACING_LINEAR, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU },
	{ DEINTERLACING_LINEAR2x, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU, true },
	{ DEINTERLACING_YADIF, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU },
	{ DEINTERLACING_YADIF2x, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU, true },
	{ DEINTERLACING__DEBUG, FIELD_ORDER_TFF | FIELD_ORDER_BFF, DEINTERLACING_PROCESSOR_GPU },
};

//音视频单帧包
struct CSampleData {
	//IMediaSample *sample;
	LPBYTE lpData;
	long dataLength;
	int cx, cy;
	bool bAudio;
	INT64 timestamp;
	volatile long refs;
	void* pAudioFormat;
	int colorType;
	IMediaSample *SampleData;
	bool bDisableAudio;
	bool bFieldSignal;
	Json::Value *UserData;
	uint64_t CheckNum;

	inline CSampleData() { lpData = NULL; UserData = NULL; refs = 1; bDisableAudio = false; SampleData = NULL; bFieldSignal = false; }
	inline CSampleData(IMediaSample *Sample) :SampleData(Sample)
	{
		lpData = NULL;
		refs = 1;
		SampleData->AddRef();
		bDisableAudio = false;
	}
	inline virtual ~CSampleData() 
	{
		if (SampleData)
		{
			SampleData->Release();
		}
		else if (lpData)
		{
			Free(lpData);
		}
	} //sample->Release();}

	inline void AddRef() { ++refs; if(SampleData)SampleData->AddRef(); }
	inline void Release()
	{
		if (!InterlockedDecrement(&refs))
		{
			delete this;
		}
		else if (SampleData)
		{
			SampleData->Release();
		}
	}
};

// 录制相关函数
typedef struct recordParaTag
{
	// 录制名字，带后缀格式
	wchar_t cName[1024];

	// 音频参数
	unsigned uChannelNums;

	// 采样率
	unsigned uSampleRateHz;

	// 采样格式
	// /* Input Formats */
	//#define FAAC_INPUT_NULL    0
	///#define FAAC_INPUT_16BIT   1
	//#define FAAC_INPUT_24BIT   2
	//#define FAAC_INPUT_32BIT   3
	//#define FAAC_INPUT_FLOAT   4
	unsigned uAForamt;

	// 视频格式
	TColorType uVideoFormat;

	// 视频宽度
	unsigned uWidth;

	// 视频高度
	unsigned uHeight;

	// 帧间隔
	unsigned fps;

	// 音视频编码格式	
	unsigned uVBiterate;

	// 视频编码格式	
	unsigned uABiterate;
}RecordPara;

class CPObject;
class IBaseVideo;
typedef  CPObject* (*CREATEOBJ)();

#ifdef ULTRALIVEAPI_EXPORTS
#define API_EXPORT  __declspec(dllexport)
#else
#define API_EXPORT  __declspec(dllimport)
#endif // BUTELAPI_EXPORTS

API_EXPORT int AddCreateOBject2Array(const char * ClassName, CREATEOBJ Fp, const char *Desc, const char *Version);
API_EXPORT int AddCreateOBject2ArrayFilter(const char * ClassName, CREATEOBJ Fp, const char *Desc, bool bIsShader);

#define DYNIC_DECLARE()\
public:\
	static CPObject *CreateOBject(); \
	virtual const char *GainClassName();

#define IMPLEMENT_DYNIC(CLASS_NAME,DESC,VER)\
	CPObject *CLASS_NAME::CreateOBject()\
{\
	return new CLASS_NAME; \
}\
	const char *CLASS_NAME::GainClassName()\
{\
	return #CLASS_NAME; \
}\
	static int  j##CLASS_NAME = AddCreateOBject2Array(#CLASS_NAME, CLASS_NAME::CreateOBject, DESC, VER);


#define IMPLEMENT_DYNIC_FILTER(CLASS_NAME,DESC,ISHADER)\
	CPObject *CLASS_NAME::CreateOBject()\
{\
	return new CLASS_NAME; \
}\
	const char *CLASS_NAME::GainClassName()\
{\
	return #CLASS_NAME; \
}\
	static int  j##CLASS_NAME = AddCreateOBject2ArrayFilter(#CLASS_NAME, CLASS_NAME::CreateOBject, DESC, ISHADER);

#endif

#define REGINST_CONFIGFUNC(CLASS_NAME,FUNC)\
	static int  K##CLASS_NAME = AddConfigFun2Map(#CLASS_NAME, FUNC);



#endif // !SLIVEINTERFACE_H
