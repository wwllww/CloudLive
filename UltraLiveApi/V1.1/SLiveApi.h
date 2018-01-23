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

//��ƵDBֵ�ص�
typedef void(*AudioDBCallBack)(uint64_t StreamID, float leftDb,float rightDb);
//����״̬�ص�
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
	float fLeft;//Ĭ��1.0f;
	float fRight;//Ĭ��1.0f;
	float fMix;//Ĭ��1.0f;
	float fQuotietyVolume; //Ĭ��3.0f;
}SoundAndLocalMinitor;

typedef struct LiveSettingParam
{
	bool      bChange;
	//��Ƶ
	int       Width; //��Ƶ�����
	int       Height;//��Ƶ�����
	int       VideoBitRate;
	UINT      FPS;
	bool      bUseHardEncoder;
	bool      bUseCBR;
	int       Quality;

	//��Ƶ
	int       AudioEncoderType;
	int       AudioBitRate;
	int       AudioSampleRate;
	int       AudioChannel;
	//����
	bool      bUsePush;
	bool      bUseBackPush;
	char      LivePushUrl[MAX_PATH];
	char      LiveBackPushUrl[MAX_PATH];
	int       AutoConnect;
	int       DelayTime;
	//¼��
	bool      bRecoder;
	char      RecoderPath[MAX_PATH];
	int       FileType;
	int       RecordMin;

	//�߼�
	char      X264Preset[20];
	char      X264Profile[20];
	int       KeyFrame;
	int       BFrameCount;

	//��ֱ��
	bool      bUseLiveSec;
	//��Ƶ
	int       WidthSec;
	int       HeightSec;
	int       VideoBitRateSec;
	UINT      FPSSec;
	bool      bUseHardEncoderSec;
	bool      bUseCBRSec;
	int       QualitySec;

	//��Ƶ
	int       AudioEncoderTypeSec;
	int       AudioBitRateSec;
	int       AudioSampleRateSec;
	int       AudioChannelSec;

	//����
	bool      bUsePushSec;
	bool      bUseBackPushSec;
	char      LivePushUrlSec[MAX_PATH];
	char      LiveBackPushUrlSec[MAX_PATH];
	int       AutoConnectSec;
	int       DelayTimeSec;

	//¼��
	bool      bRecoderSec;
	char      RecoderPathSec[MAX_PATH];
	int       FileTypeSec;
	int       RecordMinSec;

	//�߼�
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
	/* �������ܣ�                  ��ʼ��SDK
	/* ���������                  Param ����
	/* ���������                  ��

	/* �� �� ֵ��                  0 : ��ʼ���ɹ�
	< 0 : ��ʼ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveInit(const SLiveParam *Param);

	/************************************************************************/
	/* �������ܣ�                  �ͷ�SDK
	/* ���������
	/* ���������                  ��

	/* �� �� ֵ��                  ��

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/
	API_EXPORT void SLiveRelese();


	/************************************************************************/
	/* �������ܣ�                  ���ò���
	/* ���������				   Param ����
	/* ���������                  ��

	/* �� �� ֵ��                  0 : ���óɹ�
	< 0 : ����ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/
	API_EXPORT int SLiveSetParam(const SLiveParam *Param);

	/************************************************************************/
	/* �������ܣ�                  ����һ����ʵ��
	/* ���������				   hwnd ��Ⱦ����ǰ�����ϣ�����ʵ�����������Ⱦ��������0������ʵ����
	/* ���������                  ʵ��ID

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/
	API_EXPORT int SLiveCreateInstance(uint64_t *iIntanceID,uint64_t hwnd,bool bLiveIntance = false,bool bLittlePre = false);

	/************************************************************************/
	/* �������ܣ�                  ����ʵ��ID
	/* ���������                  iIntanceID Ҫ���ٵ�ʵ��ID

	/* ���������                  ��

	/* �� �� ֵ��                  0 :    �ɹ�
	< 0 :  ����ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int	SLiveDestroyInstance(uint64_t iIntanceID);

	/************************************************************************/
	/* �������ܣ�                  ʵ����������������ǰʵ���Ǵ���ʵ�����Զ����ţ���Ƶ��Ⱦ�����ڣ���Ƶ��Ⱦ������Ҫ��Ƶ���������Ҫ����SLiveStartRenderAStream����
	/* ���������                  cParamJson   ���Ĳ���
	                               Area         ��Ƶ����㲥���������Ƶ����NULL								
	/* ���������                   ��1ID StreamID1,��2ID  StreamID2(������ڣ�����1Ϊ��Ƶ����2Ϊ��Ƶ)

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveAddStream(uint64_t iIntanceID, const char* cParamJson, VideoArea *Area,uint64_t *StreamID1, uint64_t *StreamID2 = NULL);


	/************************************************************************/
	/* �������ܣ�                  ɾ��ʵ���е���
	/* ���������                  iIntanceID   ʵ��ID
	                               iStreamID  Ҫɾ����StreamID
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveDelStream(uint64_t iIntanceID, uint64_t iStreamID);

	/************************************************************************/
	/* �������ܣ�                  ��ȡʵ���е�����Ϣ
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID  Ҫ��ȡ��Ϣ����StreamID
	/* ���������                  Info      ����Ϣ

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetStreamInfo(uint64_t iIntanceID, uint64_t iStreamID,char **Info);

	/************************************************************************/
	/* �������ܣ�                  ��ȡʵ���е���״̬
	/* ���������                  iIntanceID   ʵ��ID
	                               iStreamID  Ҫ��ȡ��Ϣ����StreamID������Ϊ�㲥����
	/* ���������                  Status      ��״̬

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetStreamStatus(uint64_t iIntanceID, uint64_t iStreamID, DBOperation *Status);
	/************************************************************************/
	/* �������ܣ�                  ���ڵ㲥�������������
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID  Ҫ�����������StreamID������Ϊ�㲥����
								   Pos        ������˵�λ��
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT Pos);

	/************************************************************************/
	/* �������ܣ�                  ���ڵ㲥��ȡ�㲥���Ĳ���λ��
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID    StreamID������Ϊ�㲥����
	                               
	/* ���������                  Pos        ��ǰ���ŵ�λ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT* Pos);

	/************************************************************************/
	/* �������ܣ�                  ���ڵ㲥��ͣ�����š����²��Ų���
	/* ���������                  iIntanceID   ʵ��ID
									iStreamID  Ҫ������StreamID������Ϊ�㲥����
									OperType   ����Ĳ���
	/* ���������                  ��
	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveOperaterStream(uint64_t iIntanceID, uint64_t iStreamID, DBOperation OperType);

	/************************************************************************/
	/* �������ܣ�                  ʵ���и���������Ĳ���
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID    ��ID
								   cJsonParam   ���²���
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��        SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveUpdateStream(uint64_t iIntanceID, uint64_t iStreamID,const char* cJsonParam);


	/************************************************************************/
	/* �������ܣ�                  ʵ���и���������Ⱦλ��
	/* ���������                  iIntanceID   ʵ��ID
	                               iStreamID    ��ID
	                               Area   ���²���
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ����ʧ�� ԭ��        SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveUpdateStreamPosition(uint64_t iIntanceID, uint64_t iStreamID, VideoArea *Area, bool bScale = false);

	/************************************************************************/
	/* �������ܣ�                  �ƶ�ʵ���е������ı���Ⱦǰ��λ��
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID    Ҫ�ƶ�����ID
								   Type         �ƶ���������
	/* ���������                  ��

	/* �� �� ֵ��                  0 :   �ɹ�
	< 0 : �ƶ�ʧ�� ԭ��          SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveMoveStream(uint64_t iIntanceID, uint64_t iStreamID, StreamMoveOperation Type);

	/************************************************************************/
	/* �������ܣ�                  ��Ⱦ��Ƶ����Ҫ�ڵ�ǰʵ��Ϊ����ʵ��ʱ,���ܵ��ã�
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID    ��ID
	                               hwnd         ��Ƶ��ʾ�Ĵ��ھ��
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStartRenderVStream(uint64_t iIntanceID, uint64_t iStreamID,uint64_t hwnd);

	/************************************************************************/
	/* �������ܣ�                  ֹͣ��Ⱦ��Ƶ����Ҫ�ڵ�ǰʵ��Ϊ����ʵ��ʱ,���ܵ��ã�
	/* ���������                  iIntanceID   ʵ��ID
	iStreamID    ��ID
	hwnd         ��Ƶ��ʾ�Ĵ��ھ��
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStopRenderVStream(uint64_t iIntanceID, uint64_t iStreamID);

	/************************************************************************/
	/* �������ܣ�                  ��Ƶ�������
	/* ���������                  iIntanceID   ʵ��ID
									iStreamID    ��ID
									cRenderAudioDevice         ��Ⱦ�豸������
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStartRenderAStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cRenderAudioDevice);

	/************************************************************************/
	/* �������ܣ�                  ������ƵDBֵ�ص�����
	/* ���������                  iIntanceID   ʵ��ID
									iStreamID    ��ID
									DBCallBack         �ص�����
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetAudioStreamDBCallBack(uint64_t iIntanceID, uint64_t iStreamID, AudioDBCallBack DBCallBack);

	/************************************************************************/
	/* �������ܣ�                  ֹͣ��Ƶ�������
	/* ���������                  iIntanceID   ʵ��ID
	iStreamID    ��ID
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStopRenderAStream(uint64_t iIntanceID, uint64_t iStreamID);


	/************************************************************************/
	/* �������ܣ�                  ���ı䴰�ڴ�С��ʱ����ã�������Ⱦ��С
	/* ���������                  bDragResize   �Ƿ�����ק�ı�Ĵ�С������ǣ���ֹͣ��קʱ����SLiveStopResize()
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStartResize(uint64_t iIntanceID,bool bDragResize);

	/************************************************************************/
	/* �������ܣ�                  ֹͣ��קʱ����
	/* ���������                  ��
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : Ԥ��ʵ��ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/
	API_EXPORT int SLiveStopResize(uint64_t iIntanceID);

	

	/************************************************************************/
	/* �������ܣ�                  �������������Ƿ���Ҫ�������
	/* ���������                  iIntanceID   ʵ��ID
	                               iStreamID    ��ID
								   bRender      �Ƿ���Ⱦ����
	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetRenderStream(uint64_t iIntanceID, uint64_t iStreamID, bool bRender);

	/************************************************************************/
	/* �������ܣ�                  ʵ������
	/* ���������                  iIntanceID   Ҫ������ʵ��ID
								   cPushURL     ������ַ
	/* ���������                  ��

	/* �� �� ֵ��                  0 : ʵ�������ɹ�
	< 0 : ʵ������ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStartLive(uint64_t iIntanceID, bool bRecordOnly = false);


	/************************************************************************/
	/* �������ܣ�                  ֹͣʵ������
	/* ���������                  iIntanceID   Ҫֹͣ������ʵ��ID
	/* ���������                  ��

	/* �� �� ֵ��                  0 : ֹͣʵ�������ɹ�
	< 0 : ֹͣʵ������ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveStopLive(uint64_t iIntanceID);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ���һ�εĴ�����Ϣ
	/* ���������                  ��
	/* ���������                  ��

	/* �� �� ֵ��                  ����˵��

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT const char* SLiveGetLastError();

	/************************************************************************/
	/* �������ܣ�                  ����ָ��size��С���ڴ棬�ɱ�����SLiveFreeMemory�ͷ�
	/* ���������                  Mem Ҫ������ڴ� size�����ڴ�Ĵ�С
	/* ���������                  ��

	/* �� �� ֵ��                  0 : ����ɹ�
	< 0 : ����ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveAllowMemory(void **Mem, size_t size);


	/************************************************************************/
	/* �������ܣ�                  �����ڴ�
	/* ���������                  Mem Ҫ�ͷŵ��ڴ��ַ
	/* ���������                  ��

	/* �� �� ֵ��                  ��

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT void SLiveFreeMemory(void *Mem);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ��Ƶ�豸�б� ������SLiveFreeMemory�ͷ�
	/* ���������                  JsonVideoCaptureList --> ��Ƶ�豸�б�
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ��ȡ��Ƶ�豸�б�ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetVideoCaptureList(char** JsonVideoCaptureList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ��Ƶ��׽�豸�б� ������SLiveFreeMemory�ͷ�
	/* ���������                  JsonAudioCaptureList --> ��Ƶ�豸�б�
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 :  ��ȡ��Ƶ��׽�豸�б�ʧ�� ԭ�� ButelBSGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetAudioCaptureList(char** JsonAudioCaptureList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ��Ƶ��Ⱦ�豸�б� ������SLiveFreeMemory�ͷ�
	/* ���������                  JsonAudioRenderList --> ��Ƶ�豸�б�
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ��ȡ��Ƶ��Ⱦ�豸�б�ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetAudioRenderList(char** JsonAudioRenderList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ�����б� ������SLiveFreeMemory�ͷ�
	/* ���������                  JsonProcessList --> �����б�
	/* ���������                  ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ��ȡ�����б�ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetProcessList(char** JsonProcessList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡͼƬ���
	/* ���������                  ImgPath --> ͼƬ·��                      
	/* ���������                  Width  ͼƬ��
								   Height ͼƬ��
								   Format ͼƬ��ʽ

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ��ȡ�����б�ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetImgInfo(const char* ImgPath, UINT *Width, UINT *Height,char **Format);


	/************************************************************************/
	/* �������ܣ�                  ѡ�и���������Ϊ��Ƶ����
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID    ��ID
								   bSelect      �Ƿ�ѡ��
	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSelectStream(uint64_t iIntanceID, uint64_t iStreamID, bool bSelect);

	/************************************************************************/
	/* �������ܣ�                  �л�����(��iIntanceID_S�ϵ����������Ƶ�iIntanceID_D��ȥ)
	/* ���������                  iIntanceID_S   Դʵ��ID
								   iIntanceID_D   Ŀ��ʵ��ID
								   TransFormType  �л�Ч��
								   TransTime      �л�ʱ��(ֻ��TransFormTypeΪDisSolveʱ��Ч����λms)
	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSwitchInstance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, TransFormType Type, UINT TransTime = 1000);

	/************************************************************************/
	/* �������ܣ�                  ��iIntanceID_S�ϵ���������ӵ�iIntanceID_D��ȥ
	/* ���������                  iIntanceID_S   Դʵ��ID
								   iIntanceID_D   Ŀ��ʵ��ID

	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveAdd2Intance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, VideoArea *Area, bool bRender = true);


	/************************************************************************/
	/* �������ܣ�                  ����iIntanceID�ϵ�iStreamID��
	/* ���������                  iIntanceID   ʵ��ID
								   iStreamID   ��ID

	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveConfigStream(uint64_t iIntanceID, uint64_t iStreamID,const char *cJson = NULL);

	/************************************************************************/
	/* �������ܣ�                  ���iIntanceID�е�����Ƶ�б�����ɾ��iIntanceID
	/* ���������                  iIntanceID   ʵ��ID

	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveClearIntances(uint64_t iIntanceID);

	/************************************************************************/
	/* �������ܣ�                  ������Stream
	/* ���������                  iIntanceID   ʵ��ID
								   iStream      ��ID
								   NewName      ������
	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveReNameStream(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName);

	/************************************************************************/
	/* �������ܣ�                  ��StreamName��ӵ�����ռλԴ
	/* ���������                  StreamName   ������
	/* ���������                  ��

	/* �� �� ֵ��                  0 :�ɹ�
	< 0 : ֹͣʵ��Ԥ��ʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveAdd2Agent(const char *StreamName,bool bAdd2PGM = false);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ���Ĵ�СStreamID����Ϊ��Ƶ��
	/* ���������                  iIntanceID   ����ʵ��
	StreamID     ����ID
	/* ���������                   ��Width,��Height

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetStreamSize(uint64_t iIntanceID, uint64_t StreamID, UINT *Width, UINT *Height);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ�Կ��б��ɹ���DevicesList�������SLiveFreeMemory�����ͷ�
	/* ���������                  
	/* ���������                  DevicesList �Կ��б�

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveDisplayDevices(char **DevicesList);

	/************************************************************************/
	/* �������ܣ�                  ���òü�
	/* ���������				   left ��ü� top�ϲü� right �Ҳü� bottom �²ü�
	/* ���������                 

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetCropping(uint64_t iInstanceID,uint64_t iStreamID,float left,float top,float right,float bottom);


	/************************************************************************/
	/* �������ܣ�                  ���õ���̨�ͱ��ؼ�������
	/* ���������				   SoundParam Ҫ���õĲ���
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetSoundAndLocalMinitorParam(SoundAndLocalMinitor *SoundParam);

	/************************************************************************/
	/* �������ܣ�                  ��ȡ��ʾ������
	/* ���������				  
	/* ���������                  Num  ��ȡ��ʾ������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetMinitorNum(UINT *Num);

	/************************************************************************/
	/* �������ܣ�                  ʹ��Ͷ��
	/* ���������                  monitorID ��ʾ��ID
	/* ���������                  

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveEnableProjector(UINT monitorID);

	/************************************************************************/
	/* �������ܣ�                  ����Ͷ��
	/* ���������                 
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveDisableProjector();

	/************************************************************************/
	/* �������ܣ�                  ����СԤ���������أ���ʵ������Ϊ������Ƶ��Ԥ����Сʵ��
	/* ���������                  iInstansID ʵ��ID,iStreamID��ID
	/* ���������                  bRet ture��ʾ����false ��ʾ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetPlayPreAudio(uint64_t iInstansID,uint64_t iStreamID,bool *bRet);


	/************************************************************************/
	/* �������ܣ�                  ��Shift + F2 �л������ó���ʱ����
	/* ���������                  
	/* ���������                  

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetSenceToBackUp();

	/************************************************************************/
	/* �������ܣ�                  ��ȡÿ���زĵ�����,�ɹ���Config�������SLiveFreeMemory�����ͷ�
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetPreConfig(char **ConfigList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡע����������,�ɹ���NameList�������SLiveFreeMemory�����ͷ�
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetPluginsName(char **NameList);

	/************************************************************************/
	/* �������ܣ�                  ȡ����ʱ����
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveCancelDelayPush();

	/************************************************************************/
	/* �������ܣ�                  ������־����
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetLogLevel(int Level);


	/************************************************************************/
	/* �������ܣ�                  ��ȡBlackMagicDevices�б�,�ɹ���DevicesList�������SLiveFreeMemory�����ͷ�
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetBlackMagicDevices(char **DevicesList);

	/************************************************************************/
	/* �������ܣ�                  ��ȡBlackMagicDisplayMode�б�,�ɹ���DisplayModeList�������SLiveFreeMemory�����ͷ�
	/* ���������
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveGetBlackMagicDisplayMode(char **DisplayModeList);

	/************************************************************************/
	/* �������ܣ�                  ����BlackMagic�������
	/* ���������                  bOn -->true ��;
								       -->false �ر�;
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveBlackMagicOutputOn(bool bOn = true);


	/************************************************************************/
	/* �������ܣ�                  ����BlackMagic
	/* ���������                  SDIOut BlackMagic����
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveSetBlackMagicOut(__SDIOut *SDIOut);


	/************************************************************************/
	/* �������ܣ�                  �Ƿ��п���¼�Ƶ�ʵ��
	/* ���������                  bRecord����
	/* ���������				   bRecord true �п���¼��ʵ��
	                                       false û�п���¼�Ƶ�ʵ��

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveHasIntancesCanRecord(bool *bRecord);


	/************************************************************************/
	/* �������ܣ�                  ��ʵ��IDΪiInstansID�е���iStreamID���Filter
	/* ���������                  iInstansID ʵ��ID,iStreamID ��ID,FilterName Filter����
	/* ���������				   iFilterID ���ش�����FilterID

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveAddFilter(uint64_t iInstansID, uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID);


	/************************************************************************/
	/* �������ܣ�                  ɾ��ʵ��IDΪiInstansID�е���iStreamID��Filter
	/* ���������                  iInstansID ʵ��ID,iStreamID ��ID,iFilterID  Ҫɾ����iFilterID
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveDeleteFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID);

	/************************************************************************/
	/* �������ܣ�                  ����Filter����
	/* ���������                  iInstansID ʵ��ID,iStreamID ��ID,iFilterID  Ҫ���µ�iFilterID,cJson Filter����
	/* ���������

	/* �� �� ֵ��                  0 : �ɹ�
	< 0 : ���Դʧ�� ԭ�� SLiveGetLastError()

	/* ʱ    �䣺                  ������                     ��������
	2017��02��28��                                           �޸Ķ���ӿ�
	/************************************************************************/

	API_EXPORT int SLiveUpdateFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID,const char *cJson);

#ifdef  __cplusplus
}
#endif //  __cplusplus

#endif