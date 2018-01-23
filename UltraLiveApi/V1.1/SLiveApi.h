#ifndef SLIVEAPI_H
#define SLIVEAPI_H
typedef unsigned long long uint64_t;
typedef unsigned int        UINT, *LPUINT;
typedef unsigned long       DWORD;

#ifndef NULL
#define NULL 0
#endif

#ifdef ULTRALIVEAPI_EXPORTS
#define API_EXPORT  __declspec(dllexport)
#else
#define API_EXPORT  __declspec(dllimport)
#endif // BUTELAPI_EXPORTS

#ifndef MAX_PATH
#define MAX_PATH 256
#endif


typedef struct PushStauts
{
	int    BitRate;
	DWORD  Color;
	int    LostTotalNum;
	double LostPercent;
}PushStauts;

//音频DB值回调
typedef void(*AudioDBCallBack)(uint64_t StreamID, float leftDb,float rightDb);
//推流状态回调
typedef void(*PushStatusCallBack)(uint64_t InstanceID, int BitRate, int FPS, int iStatus,bool IsReject);
typedef void(*PushStatusCallBack2)(uint64_t InstanceID, int FPS, PushStauts *Status);
typedef void(*ConfigCallBack)(uint64_t InstanceID,uint64_t StreamID,const char *ConfigParam);
typedef void(*LiveAudioCallBack)(float LeftDb,float RightDb);
typedef void(*DelayPushTimeCallBack)(bool bShow,const char* ShowTime, bool bClose);
typedef void(*NickNameCallBack)(uint64_t InstanceID, uint64_t StreamID, const char *NickName);
typedef void(*TipsCallBack)(int TipsId, const char* TipsMsg);

enum StreamMoveOperation
{
	MoveUp,
	MoveDown,
	MoveTop,
	MoveBottom
};

enum TransFormType
{
	Cut,
	DisSolve,
	UpDown,
	Diffuse,
	Radius,
	DownUp,
	LeftRight,
	RightLeft,
	Clock
};


typedef struct SoundAndLocalMinitor
{
	bool  bPlayLocal;
	bool  bPlayLocalLive;
	float fLeft;//默认1.0f;
	float fRight;//默认1.0f;
	float fMix;//默认1.0f;
	float fQuotietyVolume; //默认3.0f;
}SoundAndLocalMinitor;

typedef struct LiveSettingParam
{
	bool      bChange;
	//视频
	int       Width; //视频输出宽
	int       Height;//视频输出高
	int       VideoBitRate;
	UINT      FPS;
	bool      bUseHardEncoder;
	bool      bUseCBR;
	int       Quality;

	//音频
	int       AudioEncoderType;
	int       AudioBitRate;
	int       AudioSampleRate;
	int       AudioChannel;
	//推流
	bool      bUsePush;
	bool      bUseBackPush;
	char      LivePushUrl[MAX_PATH];
	char      LiveBackPushUrl[MAX_PATH];
	int       AutoConnect;
	int       DelayTime;
	//录制
	bool      bRecoder;
	char      RecoderPath[MAX_PATH];
	int       FileType;
	int       RecordMin;

	//高级
	char      X264Preset[20];
	char      X264Profile[20];
	int       KeyFrame;
	int       BFrameCount;

	//次直播
	bool      bUseLiveSec;
	//视频
	int       WidthSec;
	int       HeightSec;
	int       VideoBitRateSec;
	UINT      FPSSec;
	bool      bUseHardEncoderSec;
	bool      bUseCBRSec;
	int       QualitySec;

	//音频
	int       AudioEncoderTypeSec;
	int       AudioBitRateSec;
	int       AudioSampleRateSec;
	int       AudioChannelSec;

	//推流
	bool      bUsePushSec;
	bool      bUseBackPushSec;
	char      LivePushUrlSec[MAX_PATH];
	char      LiveBackPushUrlSec[MAX_PATH];
	int       AutoConnectSec;
	int       DelayTimeSec;

	//录制
	bool      bRecoderSec;
	char      RecoderPathSec[MAX_PATH];
	int       FileTypeSec;
	int       RecordMinSec;

	//高级
	char      X264PresetSec[20];
	char      X264ProfileSec[20];
	int       KeyFrameSec;
	int       BFrameCountSec;

}LiveSettingParam;


typedef struct DeviceParam
{
	bool      bChange;
	UINT      AdpterID;
	char      MonitorDevice[MAX_PATH];
	char      ScrProDevice[MAX_PATH];

}DeviceParam;


typedef struct BlackMagicParam
{
	bool      bChange;
	bool      *bOutSDI;

}BlackMagicParam;

typedef struct AdvancedParam
{
	bool      bChange;
	bool      bUseMultiThread;
	int       PriorityID;
	int       BufferTime;
	int       DeinterlaceType;
}AdvancedParam;


typedef struct __SDIOut
{
	int  Id;
	bool bEnable;
	const char *SourceName;
	const char *AudioName;
	const char *Format;
	
}__SDIOut;

typedef struct SLiveParam
{
	LiveSettingParam      LiveSetting;
	DeviceParam			  DeviceSetting;
	AdvancedParam         Advanced;
	BlackMagicParam       BlackMagic;
	__SDIOut              *SDIOut;
	int                   SDICount;


	PushStatusCallBack       PushStatus;
	uint64_t                 MainHwnd;
	ConfigCallBack           ConfigCB;
	LiveAudioCallBack        LiveAudioCb;
	PushStatusCallBack2      PushStatus2;
	DelayPushTimeCallBack    DelayPushTimeCb;
	NickNameCallBack         NickNameCb;
	TipsCallBack             TipsCb;
}SLiveParam;
typedef struct VideoArea
{
	float top, left;
	float height, width;
	float CropLeft, CropTop;
	float CropRight, CropBottom;
}VideoArea;


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

#ifdef  __cplusplus
extern "C" {
#endif //  __cplusplus


	/************************************************************************/
	/* 函数功能：                  初始化SDK
	/* 输入参数：                  Param 参数
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 初始化成功
	< 0 : 初始化失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveInit(const SLiveParam *Param);

	/************************************************************************/
	/* 函数功能：                  释放SDK
	/* 输入参数：
	/* 输出参数：                  无

	/* 返 回 值：                  无

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/
	API_EXPORT void SLiveRelese();


	/************************************************************************/
	/* 函数功能：                  设置参数
	/* 输入参数：				   Param 参数
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 设置成功
	< 0 : 设置失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/
	API_EXPORT int SLiveSetParam(const SLiveParam *Param);

	/************************************************************************/
	/* 函数功能：                  创建一个空实例
	/* 输入参数：				   hwnd 渲染到当前窗口上（窗口实例），如果渲染到纹理填0（纹理实例）
	/* 输出参数：                  实例ID

	/* 返 回 值：                  0 : 成功
	< 0 : 创建失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/
	API_EXPORT int SLiveCreateInstance(uint64_t *iIntanceID,uint64_t hwnd,bool bLiveIntance = false,bool bLittlePre = false);

	/************************************************************************/
	/* 函数功能：                  销毁实例ID
	/* 输入参数：                  iIntanceID 要销毁的实例ID

	/* 输出参数：                  无

	/* 返 回 值：                  0 :    成功
	< 0 :  销毁失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int	SLiveDestroyInstance(uint64_t iIntanceID);

	/************************************************************************/
	/* 函数功能：                  实例中添加流（如果当前实例是窗口实例则自动播放（视频渲染到窗口，音频渲染到流，要音频本地输出，要调用SLiveStartRenderAStream））
	/* 输入参数：                  cParamJson   流的参数
	                               Area         视频（或点播）流必填，音频流填NULL								
	/* 输出参数：                   流1ID StreamID1,流2ID  StreamID2(如果存在，则流1为视频，流2为音频)

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveAddStream(uint64_t iIntanceID, const char* cParamJson, VideoArea *Area,uint64_t *StreamID1, uint64_t *StreamID2 = NULL);


	/************************************************************************/
	/* 函数功能：                  删除实例中的流
	/* 输入参数：                  iIntanceID   实例ID
	                               iStreamID  要删除的StreamID
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveDelStream(uint64_t iIntanceID, uint64_t iStreamID);

	/************************************************************************/
	/* 函数功能：                  获取实例中的流信息
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID  要获取信息流的StreamID
	/* 输出参数：                  Info      流信息

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetStreamInfo(uint64_t iIntanceID, uint64_t iStreamID,char **Info);

	/************************************************************************/
	/* 函数功能：                  获取实例中的流状态
	/* 输入参数：                  iIntanceID   实例ID
	                               iStreamID  要获取信息流的StreamID（必须为点播流）
	/* 输出参数：                  Status      流状态

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetStreamStatus(uint64_t iIntanceID, uint64_t iStreamID, DBOperation *Status);
	/************************************************************************/
	/* 函数功能：                  用于点播快进快退流操作
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID  要快进快退流的StreamID（必须为点播流）
								   Pos        快进快退的位置
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT Pos);

	/************************************************************************/
	/* 函数功能：                  用于点播获取点播流的播放位置
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID    StreamID（必须为点播流）
	                               
	/* 输出参数：                  Pos        当前播放的位置

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT* Pos);

	/************************************************************************/
	/* 函数功能：                  用于点播暂停、播放、重新播放操作
	/* 输入参数：                  iIntanceID   实例ID
									iStreamID  要操作的StreamID（必须为点播流）
									OperType   具体的操作
	/* 输出参数：                  无
	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveOperaterStream(uint64_t iIntanceID, uint64_t iStreamID, DBOperation OperType);

	/************************************************************************/
	/* 函数功能：                  实例中更新流本身的参数
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID    流ID
								   cJsonParam   更新参数
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因        SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveUpdateStream(uint64_t iIntanceID, uint64_t iStreamID,const char* cJsonParam);


	/************************************************************************/
	/* 函数功能：                  实例中更新流的渲染位置
	/* 输入参数：                  iIntanceID   实例ID
	                               iStreamID    流ID
	                               Area   更新参数
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 操作失败 原因        SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveUpdateStreamPosition(uint64_t iIntanceID, uint64_t iStreamID, VideoArea *Area, bool bScale = false);

	/************************************************************************/
	/* 函数功能：                  移动实例中的流，改变渲染前后位置
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID    要移动的流ID
								   Type         移动操作类型
	/* 输出参数：                  无

	/* 返 回 值：                  0 :   成功
	< 0 : 移动失败 原因          SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveMoveStream(uint64_t iIntanceID, uint64_t iStreamID, StreamMoveOperation Type);

	/************************************************************************/
	/* 函数功能：                  渲染视频流（要在当前实例为纹理实例时,才能调用）
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID    流ID
	                               hwnd         视频显示的窗口句柄
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStartRenderVStream(uint64_t iIntanceID, uint64_t iStreamID,uint64_t hwnd);

	/************************************************************************/
	/* 函数功能：                  停止渲染视频流（要在当前实例为纹理实例时,才能调用）
	/* 输入参数：                  iIntanceID   实例ID
	iStreamID    流ID
	hwnd         视频显示的窗口句柄
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStopRenderVStream(uint64_t iIntanceID, uint64_t iStreamID);

	/************************************************************************/
	/* 函数功能：                  音频本地输出
	/* 输入参数：                  iIntanceID   实例ID
									iStreamID    流ID
									cRenderAudioDevice         渲染设备的名字
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStartRenderAStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cRenderAudioDevice);

	/************************************************************************/
	/* 函数功能：                  设置音频DB值回调函数
	/* 输入参数：                  iIntanceID   实例ID
									iStreamID    流ID
									DBCallBack         回调函数
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetAudioStreamDBCallBack(uint64_t iIntanceID, uint64_t iStreamID, AudioDBCallBack DBCallBack);

	/************************************************************************/
	/* 函数功能：                  停止音频输出本地
	/* 输入参数：                  iIntanceID   实例ID
	iStreamID    流ID
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStopRenderAStream(uint64_t iIntanceID, uint64_t iStreamID);


	/************************************************************************/
	/* 函数功能：                  当改变窗口大小的时候调用，调整渲染大小
	/* 输入参数：                  bDragResize   是否是拖拽改变的大小，如果是，则停止拖拽时调用SLiveStopResize()
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStartResize(uint64_t iIntanceID,bool bDragResize);

	/************************************************************************/
	/* 函数功能：                  停止拖拽时调用
	/* 输入参数：                  无
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 预览实例失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/
	API_EXPORT int SLiveStopResize(uint64_t iIntanceID);

	

	/************************************************************************/
	/* 函数功能：                  在推流过程中是否需要加入该流
	/* 输入参数：                  iIntanceID   实例ID
	                               iStreamID    流ID
								   bRender      是否渲染到流
	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetRenderStream(uint64_t iIntanceID, uint64_t iStreamID, bool bRender);

	/************************************************************************/
	/* 函数功能：                  实例推流
	/* 输入参数：                  iIntanceID   要推流的实例ID
								   cPushURL     推流地址
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 实例推流成功
	< 0 : 实例推流失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStartLive(uint64_t iIntanceID, bool bRecordOnly = false);


	/************************************************************************/
	/* 函数功能：                  停止实例推流
	/* 输入参数：                  iIntanceID   要停止推流的实例ID
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 停止实例推流成功
	< 0 : 停止实例推流失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveStopLive(uint64_t iIntanceID);

	/************************************************************************/
	/* 函数功能：                  获取最后一次的错误信息
	/* 输入参数：                  无
	/* 输出参数：                  无

	/* 返 回 值：                  错误说明

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT const char* SLiveGetLastError();

	/************************************************************************/
	/* 函数功能：                  分配指定size大小的内存，由必须由SLiveFreeMemory释放
	/* 输入参数：                  Mem 要分配的内存 size分配内存的大小
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 分配成功
	< 0 : 分配失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveAllowMemory(void **Mem, size_t size);


	/************************************************************************/
	/* 函数功能：                  分配内存
	/* 输入参数：                  Mem 要释放的内存地址
	/* 输出参数：                  无

	/* 返 回 值：                  无

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT void SLiveFreeMemory(void *Mem);

	/************************************************************************/
	/* 函数功能：                  获取视频设备列表 必须由SLiveFreeMemory释放
	/* 输入参数：                  JsonVideoCaptureList --> 视频设备列表
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 获取视频设备列表失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetVideoCaptureList(char** JsonVideoCaptureList);

	/************************************************************************/
	/* 函数功能：                  获取音频捕捉设备列表 必须由SLiveFreeMemory释放
	/* 输入参数：                  JsonAudioCaptureList --> 视频设备列表
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 :  获取音频捕捉设备列表失败 原因 ButelBSGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetAudioCaptureList(char** JsonAudioCaptureList);

	/************************************************************************/
	/* 函数功能：                  获取音频渲染设备列表 必须由SLiveFreeMemory释放
	/* 输入参数：                  JsonAudioRenderList --> 视频设备列表
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 获取音频渲染设备列表失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetAudioRenderList(char** JsonAudioRenderList);

	/************************************************************************/
	/* 函数功能：                  获取进程列表 必须由SLiveFreeMemory释放
	/* 输入参数：                  JsonProcessList --> 进程列表
	/* 输出参数：                  无

	/* 返 回 值：                  0 : 成功
	< 0 : 获取进程列表失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetProcessList(char** JsonProcessList);

	/************************************************************************/
	/* 函数功能：                  获取图片宽高
	/* 输入参数：                  ImgPath --> 图片路径                      
	/* 输出参数：                  Width  图片宽
								   Height 图片高
								   Format 图片格式

	/* 返 回 值：                  0 : 成功
	< 0 : 获取进程列表失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetImgInfo(const char* ImgPath, UINT *Width, UINT *Height,char **Format);


	/************************************************************************/
	/* 函数功能：                  选中该流（必须为视频流）
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID    流ID
								   bSelect      是否选中
	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSelectStream(uint64_t iIntanceID, uint64_t iStreamID, bool bSelect);

	/************************************************************************/
	/* 函数功能：                  切换场景(把iIntanceID_S上的所有流复制到iIntanceID_D上去)
	/* 输入参数：                  iIntanceID_S   源实例ID
								   iIntanceID_D   目的实例ID
								   TransFormType  切换效果
								   TransTime      切换时间(只在TransFormType为DisSolve时有效，单位ms)
	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSwitchInstance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, TransFormType Type, UINT TransTime = 1000);

	/************************************************************************/
	/* 函数功能：                  把iIntanceID_S上的所有流添加到iIntanceID_D上去
	/* 输入参数：                  iIntanceID_S   源实例ID
								   iIntanceID_D   目的实例ID

	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveAdd2Intance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, VideoArea *Area, bool bRender = true);


	/************************************************************************/
	/* 函数功能：                  配置iIntanceID上的iStreamID流
	/* 输入参数：                  iIntanceID   实例ID
								   iStreamID   流ID

	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveConfigStream(uint64_t iIntanceID, uint64_t iStreamID,const char *cJson = NULL);

	/************************************************************************/
	/* 函数功能：                  清空iIntanceID中的音视频列表，但不删除iIntanceID
	/* 输入参数：                  iIntanceID   实例ID

	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveClearIntances(uint64_t iIntanceID);

	/************************************************************************/
	/* 函数功能：                  重命名Stream
	/* 输入参数：                  iIntanceID   实例ID
								   iStream      流ID
								   NewName      流名字
	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveReNameStream(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName);

	/************************************************************************/
	/* 函数功能：                  把StreamName添加到区域占位源
	/* 输入参数：                  StreamName   流名字
	/* 输出参数：                  无

	/* 返 回 值：                  0 :成功
	< 0 : 停止实例预览失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveAdd2Agent(const char *StreamName,bool bAdd2PGM = false);

	/************************************************************************/
	/* 函数功能：                  获取流的大小StreamID必须为视频流
	/* 输入参数：                  iIntanceID   流的实例
	StreamID     流的ID
	/* 输出参数：                   流Width,流Height

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetStreamSize(uint64_t iIntanceID, uint64_t StreamID, UINT *Width, UINT *Height);

	/************************************************************************/
	/* 函数功能：                  获取显卡列表，成功后DevicesList必须调用SLiveFreeMemory进行释放
	/* 输入参数：                  
	/* 输出参数：                  DevicesList 显卡列表

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveDisplayDevices(char **DevicesList);

	/************************************************************************/
	/* 函数功能：                  设置裁剪
	/* 输入参数：				   left 左裁剪 top上裁剪 right 右裁剪 bottom 下裁剪
	/* 输出参数：                 

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetCropping(uint64_t iInstanceID,uint64_t iStreamID,float left,float top,float right,float bottom);


	/************************************************************************/
	/* 函数功能：                  设置调音台和本地监听参数
	/* 输入参数：				   SoundParam 要设置的参数
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetSoundAndLocalMinitorParam(SoundAndLocalMinitor *SoundParam);

	/************************************************************************/
	/* 函数功能：                  获取显示器个数
	/* 输入参数：				  
	/* 输出参数：                  Num  获取显示器个数

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetMinitorNum(UINT *Num);

	/************************************************************************/
	/* 函数功能：                  使用投屏
	/* 输入参数：                  monitorID 显示器ID
	/* 输出参数：                  

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveEnableProjector(UINT monitorID);

	/************************************************************************/
	/* 函数功能：                  禁用投屏
	/* 输入参数：                 
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveDisableProjector();

	/************************************************************************/
	/* 函数功能：                  设置小预览声音开关，该实例必须为带音视频的预览的小实例
	/* 输入参数：                  iInstansID 实例ID,iStreamID流ID
	/* 输出参数：                  bRet ture显示开，false 显示关

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetPlayPreAudio(uint64_t iInstansID,uint64_t iStreamID,bool *bRet);


	/************************************************************************/
	/* 函数功能：                  当Shift + F2 切换到备用场景时调用
	/* 输入参数：                  
	/* 输出参数：                  

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetSenceToBackUp();

	/************************************************************************/
	/* 函数功能：                  获取每个素材的配置,成功后Config必须调用SLiveFreeMemory进行释放
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetPreConfig(char **ConfigList);

	/************************************************************************/
	/* 函数功能：                  获取注册插件的名字,成功后NameList必须调用SLiveFreeMemory进行释放
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetPluginsName(char **NameList);

	/************************************************************************/
	/* 函数功能：                  取消延时推流
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveCancelDelayPush();

	/************************************************************************/
	/* 函数功能：                  设置日志级别
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetLogLevel(int Level);


	/************************************************************************/
	/* 函数功能：                  获取BlackMagicDevices列表,成功后DevicesList必须调用SLiveFreeMemory进行释放
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetBlackMagicDevices(char **DevicesList);

	/************************************************************************/
	/* 函数功能：                  获取BlackMagicDisplayMode列表,成功后DisplayModeList必须调用SLiveFreeMemory进行释放
	/* 输入参数：
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveGetBlackMagicDisplayMode(char **DisplayModeList);

	/************************************************************************/
	/* 函数功能：                  设置BlackMagic输出开关
	/* 输入参数：                  bOn -->true 打开;
								       -->false 关闭;
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveBlackMagicOutputOn(bool bOn = true);


	/************************************************************************/
	/* 函数功能：                  设置BlackMagic
	/* 输入参数：                  SDIOut BlackMagic参数
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveSetBlackMagicOut(__SDIOut *SDIOut);


	/************************************************************************/
	/* 函数功能：                  是否有可以录制的实例
	/* 输入参数：                  bRecord参数
	/* 输出参数：				   bRecord true 有可以录制实例
	                                       false 没有可以录制的实例

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveHasIntancesCanRecord(bool *bRecord);


	/************************************************************************/
	/* 函数功能：                  向实例ID为iInstansID中的流iStreamID添加Filter
	/* 输入参数：                  iInstansID 实例ID,iStreamID 流ID,FilterName Filter名字
	/* 输出参数：				   iFilterID 返回创建的FilterID

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveAddFilter(uint64_t iInstansID, uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID);


	/************************************************************************/
	/* 函数功能：                  删除实例ID为iInstansID中的流iStreamID的Filter
	/* 输入参数：                  iInstansID 实例ID,iStreamID 流ID,iFilterID  要删除的iFilterID
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveDeleteFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID);

	/************************************************************************/
	/* 函数功能：                  更新Filter参数
	/* 输入参数：                  iInstansID 实例ID,iStreamID 流ID,iFilterID  要更新的iFilterID,cJson Filter参数
	/* 输出参数：

	/* 返 回 值：                  0 : 成功
	< 0 : 添加源失败 原因 SLiveGetLastError()

	/* 时    间：                  操作人                     操作内容
	2017年02月28日                                           修改定义接口
	/************************************************************************/

	API_EXPORT int SLiveUpdateFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID,const char *cJson);

#ifdef  __cplusplus
}
#endif //  __cplusplus

#endif