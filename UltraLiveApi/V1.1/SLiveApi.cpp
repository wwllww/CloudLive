// SLiveApi.cpp : 定义 DLL 应用程序的导出函数。
//

#include "SLiveApi.h"
#include "SLiveManager.h"


int SLiveInit(const SLiveParam *Param)
{
	return CSLiveManager::GetInstance()->SLiveInit(Param);
}

void SLiveRelese()
{
	CSLiveManager::SLiveRelese();
}

void SLiveInitException()
{
	CSLiveManager::InitException();
}


int SLiveSetParam(const SLiveParam *Param)
{
	return CSLiveManager::GetInstance()->SLiveSetParam(Param);
}

int SLiveCreateInstance(uint64_t *iIntanceID, VideoArea *PreArea, bool bLiveIntance, bool bLittlePre)
{
	return CSLiveManager::GetInstance()->SLiveCreateInstance(iIntanceID, PreArea, bLiveIntance, bLittlePre);
}

int SLiveDestroyInstance(uint64_t iIntanceID)
{
	return CSLiveManager::GetInstance()->SLiveDestroyInstance(iIntanceID);
}

int SLiveAddStream(uint64_t iIntanceID, const char* cParamJson, VideoArea *Area, uint64_t *StreamID1, uint64_t *StreamID2 /*= NULL*/)
{
	return CSLiveManager::GetInstance()->SLiveAddStream(iIntanceID, cParamJson, Area, StreamID1, StreamID2);
}

int SLiveDelStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	return CSLiveManager::GetInstance()->SLiveDelStream(iIntanceID, iStreamID);
}


int SLiveGetStreamInfo(uint64_t iIntanceID, uint64_t iStreamID, char **Info)
{
	return CSLiveManager::GetInstance()->SLiveGetStreamInfo(iIntanceID, iStreamID,Info);
}

int SLiveGetStreamStatus(uint64_t iIntanceID, uint64_t iStreamID, DBOperation *Status)
{
	return CSLiveManager::GetInstance()->SLiveGetStreamStatus(iIntanceID, iStreamID, Status);
}

int SLiveSetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT Pos)
{
	return CSLiveManager::GetInstance()->SLiveSetStreamPlayPos(iIntanceID, iStreamID, Pos);
}

int SLiveGetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT* Pos)
{
	return CSLiveManager::GetInstance()->SLiveGetStreamPlayPos(iIntanceID, iStreamID, Pos);
}

int SLiveOperaterStream(uint64_t iIntanceID, uint64_t iStreamID, DBOperation OperType)
{
	return CSLiveManager::GetInstance()->SLiveOperaterStream(iIntanceID, iStreamID, OperType);
}

int SLiveUpdateStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cJsonParam)
{
	return CSLiveManager::GetInstance()->SLiveUpdateStream(iIntanceID, iStreamID, cJsonParam);
}

int SLiveUpdateStreamPosition(uint64_t iIntanceID, uint64_t iStreamID, VideoArea *Area, bool bScale)
{
	return CSLiveManager::GetInstance()->SLiveUpdateStreamPosition(iIntanceID, iStreamID, Area, bScale);
}

int SLiveMoveStream(uint64_t iIntanceID, uint64_t iStreamID, StreamMoveOperation Type)
{
	return CSLiveManager::GetInstance()->SLiveMoveStream(iIntanceID, iStreamID, Type);
}


int SLiveStartRenderVStream(uint64_t iIntanceID, uint64_t iStreamID, uint64_t hwnd)
{
	return CSLiveManager::GetInstance()->SLiveStartRenderVStream(iIntanceID, iStreamID, hwnd);
}

int SLiveStopRenderVStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	return CSLiveManager::GetInstance()->SLiveStopRenderVStream(iIntanceID, iStreamID);
}


int SLiveStartRenderAStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cRenderAudioDevice)
{
	return CSLiveManager::GetInstance()->SLiveStartRenderAStream(iIntanceID, iStreamID, cRenderAudioDevice);
}

int SLiveSetAudioStreamDBCallBack(uint64_t iIntanceID, uint64_t iStreamID, AudioDBCallBack DBCallBack)
{
	return CSLiveManager::GetInstance()->SLiveSetAudioStreamDBCallBack(iIntanceID, iStreamID, DBCallBack);
}

int SLiveStopRenderAStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	return CSLiveManager::GetInstance()->SLiveStopRenderAStream(iIntanceID, iStreamID);
}

int SLiveStartResize(uint64_t iIntanceID,bool bDragResize)
{
	return CSLiveManager::GetInstance()->SLiveStartResize(iIntanceID,bDragResize);
}

int SLiveStopResize(uint64_t iIntanceID)
{
	return CSLiveManager::GetInstance()->SLiveStopResize(iIntanceID);
}

int SLiveSetRenderStream(uint64_t iIntanceID, uint64_t iStreamID, bool bRender)
{
	return CSLiveManager::GetInstance()->SLiveSetRenderStream(iIntanceID, iStreamID, bRender);
}

int SLiveStartLive(uint64_t iIntanceID, bool bRecordOnly /*= false*/)
{
	return CSLiveManager::GetInstance()->SLiveStartLive(iIntanceID,bRecordOnly);
}

int SLiveStopLive(uint64_t iIntanceID)
{
	return CSLiveManager::GetInstance()->SLiveStopLive(iIntanceID);
}

const char* SLiveGetLastError()
{
	return CSLiveManager::GetInstance()->SLiveGetLastError();
}

int SLiveAllowMemory(void **Mem, size_t size)
{
	return CSLiveManager::GetInstance()->SLiveAllowMemory(Mem, size);
}

void SLiveFreeMemory(void *Mem)
{
	CSLiveManager::GetInstance()->SLiveFreeMemory(Mem);
}

int SLiveGetVideoCaptureList(char** JsonVideoCaptureList)
{
	return CSLiveManager::GetInstance()->SLiveGetVideoCaptureList(JsonVideoCaptureList);
}

int SLiveGetAudioCaptureList(char** JsonAudioCaptureList)
{
	return CSLiveManager::GetInstance()->SLiveGetAudioCaptureList(JsonAudioCaptureList);
}

int SLiveGetAudioRenderList(char** JsonAudioRenderList)
{
	return CSLiveManager::GetInstance()->SLiveGetAudioRenderList(JsonAudioRenderList);
}

int SLiveGetProcessList(char** JsonProcessList)
{
	return CSLiveManager::GetInstance()->SLiveGetProcessList(JsonProcessList);
}

int SLiveGetImgInfo(const char* ImgPath, UINT *Width, UINT *Height, char **Format)
{
	return CSLiveManager::GetInstance()->SLiveGetImgInfo(ImgPath, Width, Height, Format);
}

int SLiveSelectStream(uint64_t iIntanceID, uint64_t iStreamID, bool bSelect)
{
	return CSLiveManager::GetInstance()->SLiveSelectStream(iIntanceID, iStreamID, bSelect);
}

int SLiveSwitchInstance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, TransFormType Type, UINT TransTime)
{
	return CSLiveManager::GetInstance()->SLiveSwitchInstance(iIntanceID_S, iIntanceID_D, Type, TransTime);
}

int SLiveAdd2Intance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, VideoArea *Area, bool bRender)
{
	return CSLiveManager::GetInstance()->SLiveAdd2Intance(iIntanceID_S, iIntanceID_D, Area, bRender);
}

int SLiveConfigStream(uint64_t iIntanceID, uint64_t iStreamID, const char *cJson)
{
	return CSLiveManager::GetInstance()->SLiveConfigStream(iIntanceID, iStreamID, cJson);
}

int SLiveClearIntances(uint64_t iIntanceID, bool bRemoveTop)
{
	return CSLiveManager::GetInstance()->SLiveClearIntances(iIntanceID, bRemoveTop);
}

int SLiveReNameStream(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName)
{
	return CSLiveManager::GetInstance()->SLiveReNameStream(iIntanceID, iStreamID, NewName);
}
int SLiveAdd2Agent(const char *StreamName, bool bAdd2PGM)
{
	return CSLiveManager::GetInstance()->SLiveAdd2Agent(StreamName, bAdd2PGM);
}

int SLiveGetStreamSize(uint64_t iIntanceID, uint64_t StreamID, UINT *Width, UINT *Height)
{
	return CSLiveManager::GetInstance()->SLiveGetStreamSize(iIntanceID,StreamID,Width,Height);
}

int SLiveDisplayDevices(char **DevicesList)
{
	return CSLiveManager::GetInstance()->SLiveDisplayDevices(DevicesList);
}

int SLiveSetCropping(uint64_t iInstanceID, uint64_t iStreamID, float left, float top, float right, float bottom)
{
	return CSLiveManager::GetInstance()->SLiveSetCropping(iInstanceID, iStreamID, left, top, right, bottom);
}

int SLiveSetSoundAndLocalMinitorParam(SoundAndLocalMinitor *SoundParam)
{
	return CSLiveManager::GetInstance()->SLiveSetSoundAndLocalMinitorParam(SoundParam);
}

int SLiveGetMinitorNum(UINT *Num)
{
	return CSLiveManager::GetInstance()->SLiveGetMinitorNum(Num);
}

int SLiveEnableProjector(UINT monitorID)
{
	return CSLiveManager::GetInstance()->SLiveEnableProjector(monitorID);
}

int SLiveDisableProjector()
{
	return CSLiveManager::GetInstance()->SLiveDisableProjector();
}

int SLiveSetPlayPreAudio(uint64_t iInstansID, uint64_t iStreamID, bool *bRet)
{
	return CSLiveManager::GetInstance()->SLiveSetPlayPreAudio(iInstansID, iStreamID,bRet);
}

int SLiveSetSenceToBackUp()
{
	return CSLiveManager::GetInstance()->SLiveSetSenceToBackUp();
}

int SLiveGetPreConfig(char **ConfigList)
{
	return CSLiveManager::GetInstance()->SLiveGetPreConfig(ConfigList);
}

int SLiveGetPluginsName(char **NameList)
{
	return CSLiveManager::GetInstance()->SLiveGetPluginsName(NameList);
}

int SLiveCancelDelayPush()
{
	return CSLiveManager::GetInstance()->SLiveCancelDelayPush();
}

int SLiveSetLogLevel(int Level)
{
	return CSLiveManager::GetInstance()->SLiveSetLogLevel(Level);
}

int SLiveGetBlackMagicDevices(char **DevicesList)
{
	return CSLiveManager::GetInstance()->SLiveGetBlackMagicDevices(DevicesList);
}

int SLiveGetBlackMagicDisplayMode(char **DisplayModeList)
{
	return CSLiveManager::GetInstance()->SLiveGetBlackMagicDisplayMode(DisplayModeList);
}

int SLiveBlackMagicOutputOn(bool bOn /*= true*/)
{
	return CSLiveManager::GetInstance()->SLiveBlackMagicOutputOn(bOn);
}

int SLiveSetBlackMagicOut(__SDIOut * SDIOut)
{
	return CSLiveManager::GetInstance()->SLiveSetBlackMagicOut(SDIOut);
}

int SLiveHasIntancesCanRecord(bool *bRecord)
{
	return CSLiveManager::GetInstance()->SLiveHasIntancesCanRecord(bRecord);
}

int SLiveAddFilter(uint64_t iInstansID, uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID)
{
	return CSLiveManager::GetInstance()->SLiveAddFilter(iInstansID, iStreamID, FilterName, iFilterID);
}

int SLiveDeleteFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID)
{
	return CSLiveManager::GetInstance()->SLiveDeleteFilter(iInstansID, iStreamID,iFilterID);
}

int SLiveUpdateFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID, const char *cJson)
{
	return CSLiveManager::GetInstance()->SLiveUpdateFilter(iInstansID, iStreamID, iFilterID, cJson);
}

int SLiveSetTopest(uint64_t iInstansID, uint64_t iStreamID, bool bTopest)
{
	return CSLiveManager::GetInstance()->SLiveSetTopest(iInstansID, iStreamID, bTopest);
}

int SLiveQueryHardEncodeSupport()
{
	return CSLiveManager::GetInstance()->SLiveQueryHardEncodeSupport();
}

int SLiveSetAudioNeed(bool bNeedPVMAudio, bool bNeedPGMAudio)
{
	return CSLiveManager::GetInstance()->SLiveSetAudioNeed(bNeedPVMAudio, bNeedPGMAudio);
}

int SLiveSetAudioMixAndFollow(uint64_t iInstansID, uint64_t iStreamID, int Mix, int Follow, bool bUseMix)
{
	return CSLiveManager::GetInstance()->SLiveSetAudioMixAndFollow(iInstansID, iStreamID, Mix, Follow, bUseMix);
}

int SLiveSetOpacity(uint64_t iInstansID, uint64_t iStreamID, DWORD Opacity)
{
	return CSLiveManager::GetInstance()->SLiveSetOpacity(iInstansID, iStreamID, Opacity);
}

