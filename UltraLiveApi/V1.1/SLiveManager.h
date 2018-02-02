#ifndef SLIVEMANAGER_H
#define SLIVEMANAGER_H
#include <memory>
#include <string>
#include "BMap.h"
#include "Instance.h"
#include "BlackMagic.h"


using namespace std;

enum
{
	ID_MICVOLUME = 5000
};

class MemInfo {
private:
	void* ptr;
	const char* file;
	unsigned int line;
	MemInfo* link;
	friend class MemStack;
};

class MemStack {
private:
	MemInfo* head;
public:
	MemStack();
	~MemStack();
	void Insert(void* ptr, const char* file, unsigned int line);
	void Delete(void* ptr);
	void Print();
public:
	bool bDelete;
private:
	static CRITICAL_SECTION Section;
};

class CSLiveManager
{
	friend struct SharedDevice;
	friend class  X264Encoder;
	friend class DelayedPublisher;
public:
	CSLiveManager();
	~CSLiveManager();

	static CSLiveManager *GetInstance();

	//接口调用
	int SLiveInit(const SLiveParam *Param);
	static void SLiveRelese();
	void ShutDown();
	int SLiveSetParam(const SLiveParam *Param);
	int SLiveCreateInstance(uint64_t *iIntanceID, uint64_t hwnd, bool bLiveIntance, bool bLittlePre);
	int SLiveDestroyInstance(uint64_t iIntanceID);
	int SLiveAddStream(uint64_t iIntanceID, const char* cParamJson, VideoArea *Area, uint64_t *StreamID1, uint64_t *StreamID2 = NULL);
	int SLiveDelStream(uint64_t iIntanceID, uint64_t iStreamID);
	int SLiveGetStreamInfo(uint64_t iIntanceID, uint64_t iStreamID, char **Info);
	int SLiveGetStreamStatus(uint64_t iIntanceID, uint64_t iStreamID, DBOperation *Status);
	int SLiveSetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT Pos);
	int SLiveGetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT* Pos);
	int SLiveOperaterStream(uint64_t iIntanceID, uint64_t iStreamID, DBOperation OperType);
	int SLiveUpdateStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cJsonParam);
	int SLiveUpdateStreamPosition(uint64_t iIntanceID, uint64_t iStreamID, VideoArea *Area, bool bScale);
	int SLiveMoveStream(uint64_t iIntanceID, uint64_t iStreamID, StreamMoveOperation Type);
	int SLiveStartRenderVStream(uint64_t iIntanceID, uint64_t iStreamID, uint64_t hwnd);
	int SLiveStopRenderVStream(uint64_t iIntanceID, uint64_t iStreamID);
	int SLiveStartRenderAStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cRenderAudioDevice);
	int SLiveSetAudioStreamDBCallBack(uint64_t iIntanceID, uint64_t iStreamID, AudioDBCallBack DBCallBack);
	int SLiveStopRenderAStream(uint64_t iIntanceID, uint64_t iStreamID);
	int SLiveStartResize(uint64_t iIntanceID,bool bDragResize);
	int SLiveStopResize(uint64_t iIntanceID);
	int SLiveSetRenderStream(uint64_t iIntanceID, uint64_t iStreamID, bool bRender);
	int SLiveStartLive(uint64_t iIntanceID, bool bRecordOnly = false);
	int SLiveStopLive(uint64_t iIntanceID);
	const char* SLiveGetLastError();
	int SLiveAllowMemory(void **Mem, size_t size);
	void SLiveFreeMemory(void *Mem);
	int SLiveGetVideoCaptureList(char** JsonVideoCaptureList);
	int SLiveGetAudioCaptureList(char** JsonAudioCaptureList);
	int SLiveGetAudioRenderList(char** JsonAudioRenderList);
	int SLiveGetProcessList(char** JsonProcessList);
	int SLiveGetImgInfo(const char* ImgPath, UINT *Width, UINT *Height, char **Format);
	int SLiveSelectStream(uint64_t iIntanceID, uint64_t iStreamID, bool bSelect);
	int SLiveSwitchInstance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, TransFormType Type, UINT TransTime);
	int SLiveAdd2Intance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, VideoArea *Area, bool bRender);
	int SLiveConfigStream(uint64_t iIntanceID, uint64_t iStreamID, const char *cJson);
	int SLiveClearIntances(uint64_t iIntanceID);
	int SLiveReNameStream(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName);
	int SLiveAdd2Agent(const char *StreamName, bool bAdd2PGM);
	int SLiveGetStreamSize(uint64_t iIntanceID, uint64_t StreamID, UINT *Width, UINT *Height);
	int SLiveDisplayDevices(char **DevicesList);
	int SLiveSetCropping(uint64_t iInstanceID, uint64_t iStreamID, float left, float top, float right, float bottom);
	int SLiveSetSoundAndLocalMinitorParam(SoundAndLocalMinitor *SoundParam);
	int SLiveGetMinitorNum(UINT *Num);
	int SLiveEnableProjector(UINT monitorID);
	int SLiveDisableProjector();
	int SLiveSetPlayPreAudio(uint64_t iInstansID, uint64_t iStreamID, bool *bRet);
	int SLiveSetSenceToBackUp();
	int SLiveGetPreConfig(char **ConfigList);
	int SLiveGetPluginsName(char **NameList);
	int SLiveCancelDelayPush();
	int SLiveSetLogLevel(int Level);
	int SLiveGetBlackMagicDevices(char **DevicesList);
	int SLiveGetBlackMagicDisplayMode(char **DisplayModeList);
	int SLiveBlackMagicOutputOn(bool bOn /*= true*/);
	int SLiveSetBlackMagicOut(__SDIOut * SDIOut);
	int SLiveHasIntancesCanRecord(bool *bRecord);

	int SLiveAddFilter(uint64_t iInstansID, uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID);
	int SLiveDeleteFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID);
	int SLiveUpdateFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID, const char *cJson);
	int SLiveSetTopest(uint64_t iInstansID, uint64_t iStreamID, bool bTopest);

	int ReNameStreamSec(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName);
	//内部使用
	void SLiveSetLastError(const char *Error);
	void WcharToChar(const WCHAR* wc, std::string &RetStr);
	bool FillOutListOfDevices(GUID matchGUID, Json::Value& deviceList);
	bool GetAudioRenderDevices(Json::Value &deviceList);
	bool GetProcessList(Json::Value& ProcessList);
	void ActuallyEnableProjector();
	void EnableProjector(UINT monitorID);
	void DisableProjector();
	void DrawPreviewProject(Texture* Prev, const Vect2 &renderSize, const Vect2 &renderOffset, const Vect2 &renderCtrlSize);


	//内部线程
	//线程函数
	static DWORD MainVideoCapture(LPVOID lparam);
	static DWORD MainAudioCapture(LPVOID lparam);
	static DWORD VideoEncoderThread(LPVOID lparam);
	static DWORD VideoEncoderThread_back(LPVOID lparam);
	static LRESULT CALLBACK ProjectorFrameProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static  DWORD CheckStatusThreadProc(LPVOID lParam);
	static  DWORD RenderLittlePreviewThreadProc(LPVOID lParam);

	//实现函数
	void MainVideoLoop();
	void MainAudioLoop();
	void VideoEncoderLoop();
	void VideoEncoderLoop_back();
	void CheckStatus();
	void RenderLittlePreview();

	bool ManagerInit(uint64_t hwnd);

	void BulidD3D();

	void SwitchInstanceCut(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD);

	void SwitchInstanceDisSolve(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD, UINT TransTime);

	void SwitchInstanceUpDownOrDiffuse(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD, TransFormType type, UINT TransTime);

	void ProcessSwitch(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD, TransFormType type);

	void ChangeLiveInstanceSameAsLocalInstance(IBaseVideo *Video);
	
	bool GetNameList(Value &data);
	bool ScanSourceElmentByClassName(const char *ClassName, List<Value>& List);
	void RemoveLiveInstanceAudio(IBaseVideo *BaseVideo, bool bMustDel, IBaseVideo *NewGobal);
	void AddLiveInstanceAudio(IBaseVideo *Video, IBaseVideo *Agent);
	void AddFilter2PGMOrPVM(IBaseVideo *Video);
	void AddFilter2PGMOrPVM(IBaseVideo *Video, const List<Filter> &FilterList);
	void RemoveFilterFromPGMOrPVM(IBaseVideo *Video,bool bRemovePGM);
	void RemoveFilterFromPGMOrPVM(IBaseVideo *Video, const List<Filter> &FilterList, bool bRemovePGM);
	D3DAPI *GetD3DRender() const;


	bool HaveSDIOut();
	void PushOrUpdateSIDId(const SDIID& id);
	bool FindAndRemoveId(int id);
	void ResetDevice(IBaseVideo *Video, shared_ptr<IBaseVideo>& ResetVideo, bool bNULL, IBaseVideo *PreVideo = NULL);
	bool IsHaveStream(IBaseVideo *Video);
	void RenderSDI(int Index);
	bool FindVideoInLocalIntance(IBaseAudio *Audio);
	bool FindVideoInLiveIntance(IBaseAudio *Audio);

public:
	static MemStack *mem_stack;
	CBMap<CInstanceProcess*> m_InstanceList = 1;

	CRITICAL_SECTION MapInstanceSec;
	SLiveParam BSParam;
	List<MonitorInfo>       monitors;
	UINT    baseCX, baseCY, baseCX_back, baseCY_back;

	CInstanceProcess *LiveInstance;
	ColorDescription colorDesc;
private:
	string ErrMsg;
	bool bInit;
	static CSLiveManager *m_Intances;
	
	vector<std::string> NameList;

	static D3DAPI *m_D3DRender;

	bool bFirstCreate;

	bool bRunning;
	int FPS;
	bool bfirstTimeStamp;
	bool bFristAudioEncode;
	bool bOutPicDel;
	bool bOutPicDel_Back;
	bool bStartLive;

	HANDLE HVideoCapture;
	HANDLE HAudioCapture;
	HANDLE HVideoEncoder, HVideoEncoder_back;
	HANDLE HLittlePreview;
	HANDLE hVideoEvent, hVideoEvent_back,hVideoComplete;

	Shader *mainVertexShader;
	Shader *mainPixelShader;
	Shader *yuvScalePixelShader, *transitionPixel, *circleTransitionPixel;
	Shader *solidVertexShader;
	Shader *solidPixelShader;
	UINT    scaleCX, scaleCY;
	UINT    outputCX, outputCY, outputCX_back, outputCY_back;
	

	Texture *copyTextures,*copyTextures_back;
	Texture *mainRenderTextures[2];
	Texture *PreRenderTexture;
	Texture *yuvRenderTextures, *yuvRenderTextures_back;
	Texture *transitionTexture;
	unique_ptr<Texture> transitionAddress;
	Texture *transNewTexture;
	Texture *copyRGBTexture;
	Texture *SDITexture;

	//timestamp
	QWORD CurrentVideoTime;
	//QWORD CurrentAudioTime;
	QWORD StartVideoTime;
	QWORD sleepTargetTime;

	bool bTransDisSolving;
	bool bTransUpDown;
	bool bTransDownUp;
	bool bTransLeftRight;
	bool bTransRightLeft;
	bool bTransDiffuse;
	bool bRadius;
	bool bClock;
	float TransFormTime;
	float TransEscapeTime;

	SamplerState *ss;

	x264_picture_t * volatile Outpic;
	x264_picture_t * volatile Outpic_back;

	bool  bUseBack;
	
	CInstanceProcess *LocalInstance;

	//调音台
	float   desktopVol, leftdesktopVol, rightdesktopVol,m_quotietyVolume;
	bool    m_bPlayPcmLocal, m_bPlayPcmLive;

	bool    bPleaseEnableProjector;
	bool    bPleaseDisableProjector;
	bool    m_bProject;
	bool    bEnableProjectorCursor;
	UINT    projectorX, projectorY;
	UINT    projectorWidth, projectorHeight;
	UINT    projectorMonitorID;
	HWND    hwndProjector;
	Texture *projectorTexture;

	int      m_CheckTime;
	int      m_FPSCount;
	int      m_EncodeVideoCount;
	int      m_EncodeAudioCount;
	HANDLE   m_CheckEvent;
	QWORD    m_RealTime;
	float    m_RealFrameTime;
	float    m_RealAudioFrameTime;
	HANDLE   HStatus;
	//SDIOut
	int      FirstSDIRender = -1000;
	std::list<SDIID> SIDIDs;
	HANDLE   SDIMutex;

	SDIOutInfo *__SDIOutInfo;
	bool bStartView;

	CDeinterlacer *Deinterlacer;
	CDeinterlacer *DeinterlacerLocal;
	DeinterlacerConfig DeinterConfig;
	bool bNeedAgentInPGM;
};

#endif