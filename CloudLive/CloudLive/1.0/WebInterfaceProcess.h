#ifndef WEBINTERFACEPROCESS
#define WEBINTERFACEPROCESS
#include "BaseAfx.h"
#include "ExcProcess.h"
#include "CHttpNetWork.h"


typedef struct __InfoSource
{
	std::string SourceName;
	float x;
	float y;
	float width;
	float height;
	int  layer;
	int  Transparent;

}TagInfoSource;

typedef struct __Scenes
{
	int index;
	std::vector<TagInfoSource> SouceList;

}TagScenes;

typedef struct __InfoID
{
	uint64_t VideoId;
	uint64_t AudioId;
	int   InfoType;
	__InfoID()
	{
		VideoId = AudioId = 0;
		InfoType = -1;
	}
}TagInfoID;

typedef struct __ImgSource
{
	std::string ImgID;
	bool     bRender;
	std::string ImgUrl;
	float   x, y, width, height;
	int transparent;
	std::string RealName;
}TagImgSource;

typedef struct __TextSource
{
	std::string TextID;
	bool     bRender;
	std::string text;
	float   x, y;
	int    textSize;
	UINT   textColor;
	std::string textType;
	int    enableRoll;
	int    rollSpeed;
	int    rollType;
	float  RealWidth; //百分比
	float  RealHeight;
}TagTextSource;

typedef struct __Instance
{
	std::string InstanceName;
	uint64_t    InstanceID;
	std::vector<__InfoID> SouceList;
	TagImgSource ImgParam;
	TagTextSource TextParam;
	VideoArea    PreviewArea;

	__Instance()
	{
		ZeroMemory(&ImgParam, sizeof TagImgSource);
		ZeroMemory(&TextParam, sizeof TagTextSource);
		ZeroMemory(&PreviewArea, sizeof VideoArea);
	}

}TagInstance;



class WebInterFaceProcess : public ExcProcess
{
	DYNIC_DECLARE(WebInterFaceProcess)
protected:
	//1-5
	void CloudInitSwitcher(CRequse &Req, CRespond &Res);
	void CloudCloseSwitcher(CRequse &Req, CRespond &Res);
	void CloudGetSenceConfig(CRequse &Req, CRespond &Res);
	void CloudSetSenceParam(CRequse &Req, CRespond &Res);
	void CloudChangeSence(CRequse &Req, CRespond &Res);

	//6-10
	void CloudSwitchPVWtoPGM(CRequse &Req, CRespond &Res);
	void CloudSetEmergencySence(CRequse &Req, CRespond &Res);
	void CloudLiveSetInputParam(CRequse &Req, CRespond &Res);
	void CloudChangeInput(CRequse &Req, CRespond &Res);
	void CloudSetLiveEncodeParam(CRequse &Req, CRespond &Res);

	//11-15
	void CloudSetLiveStreamParam(CRequse &Req, CRespond &Res);
	void CloudStartLive(CRequse &Req, CRespond &Res);
	void CloudStopLive(CRequse &Req, CRespond &Res);
	void CloudSetImage(CRequse &Req, CRespond &Res);
	void CloudDeleteImage(CRequse &Req, CRespond &Res);

	//16-20
	void CloudStartImage(CRequse &Req, CRespond &Res);
	void CloudStopImage(CRequse &Req, CRespond &Res);
	void CloudAddText(CRequse &Req, CRespond &Res);
	void CloudDeleteText(CRequse &Req, CRespond &Res);
	void CloudStartText(CRequse &Req, CRespond &Res);

	//21-25
	void CloudStopText(CRequse &Req, CRespond &Res);
	void CloudSetVolum(CRequse &Req, CRespond &Res);
	void CloudStartAudioMonitor(CRequse &Req, CRespond &Res);
	void CloudStopAudioMonitor(CRequse &Req, CRespond &Res);
	void CloudSetAudioMix(CRequse &Req, CRespond &Res);

	//26-30
	void CloudEmergencySwitch(CRequse &Req, CRespond &Res);
	void CloudSetAudioFollow(CRequse &Req, CRespond &Res);
	void CloudSetLiveDelayTime(CRequse &Req, CRespond &Res);


	//自己内部调用函数
	void CheckCloudIdAndToken(const char* Param,Json::Value &JsonParam,bool bGet = false);
	void AddText(TagInstance &Text,Json::Value &JParam);
	void AddImage(TagInstance &Img, Json::Value &JParam, bool bUpdate = false);

	static void DownLoadComplete(void *Context, const std::string& name, bool bSucess);

protected:
	WebInterFaceProcess();
	~WebInterFaceProcess();

private:
	bool bHasInit;
	SLiveParam LiveParam;
	std::string CloudId;
	std::string Token;
	std::map<int, TagScenes> ScenesMap;
	Json::Value SencesConfig;
	std::map<std::string,TagInstance> InstanceMap;
	TransFormType EffectType[9];
    
	//备用场景
	int BackupIndex;
	std::string InputName;

	//是否正在直播
	bool bLiving;

	bool  bPlayPGM;
	bool  bPlayPVW;
	float PGMVolum;
};

#endif

