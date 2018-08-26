#include "WebInterfaceProcess.h"
#include <string>
#include "MD5.h"
#include<iphlpapi.h>
#include <shlwapi.h>

#pragma warning(disable:4244)

#define POSTCONFIGFILE "/bcs/internal/updatesencefile"

IMPLEMENT_DYNIC(WebInterFaceProcess)
BEGIN_REGIST_CLASS(WebInterFaceProcess, 27)
//1-5
F1_REGIST_CLASS("initswitcher", WebInterFaceProcess, WebInterFaceProcess::CloudInitSwitcher)
F1_REGIST_CLASS("closeswitcher", WebInterFaceProcess, WebInterFaceProcess::CloudCloseSwitcher)
F1_REGIST_CLASS("getsenceconfig", WebInterFaceProcess, WebInterFaceProcess::CloudGetSenceConfig)
F1_REGIST_CLASS("setsenceparam", WebInterFaceProcess, WebInterFaceProcess::CloudSetSenceParam)
F1_REGIST_CLASS("changesence", WebInterFaceProcess, WebInterFaceProcess::CloudChangeSence)

//6-10
F1_REGIST_CLASS("switchpvwtopgm", WebInterFaceProcess, WebInterFaceProcess::CloudSwitchPVWtoPGM)
F1_REGIST_CLASS("setemergencysence", WebInterFaceProcess, WebInterFaceProcess::CloudSetEmergencySence)
F1_REGIST_CLASS("setinputparam", WebInterFaceProcess, WebInterFaceProcess::CloudLiveSetInputParam)
F1_REGIST_CLASS("changeinput", WebInterFaceProcess, WebInterFaceProcess::CloudChangeInput)
F1_REGIST_CLASS("setliveencodeparam", WebInterFaceProcess, WebInterFaceProcess::CloudSetLiveEncodeParam)

//11-15
F1_REGIST_CLASS("setlivestreamparam", WebInterFaceProcess, WebInterFaceProcess::CloudSetLiveStreamParam)
F1_REGIST_CLASS("startlive", WebInterFaceProcess, WebInterFaceProcess::CloudStartLive)
F1_REGIST_CLASS("stoplive", WebInterFaceProcess, WebInterFaceProcess::CloudStopLive)
F1_REGIST_CLASS("setimage", WebInterFaceProcess, WebInterFaceProcess::CloudSetImage)
F1_REGIST_CLASS("deleteimage", WebInterFaceProcess, WebInterFaceProcess::CloudDeleteImage)

//16-20
F1_REGIST_CLASS("startimage", WebInterFaceProcess, WebInterFaceProcess::CloudStartImage)
F1_REGIST_CLASS("stopimage", WebInterFaceProcess, WebInterFaceProcess::CloudStopImage)
F1_REGIST_CLASS("addtext", WebInterFaceProcess, WebInterFaceProcess::CloudAddText)
F1_REGIST_CLASS("deletetext", WebInterFaceProcess, WebInterFaceProcess::CloudDeleteText)
F1_REGIST_CLASS("starttext", WebInterFaceProcess, WebInterFaceProcess::CloudStartText)

//21-25
F1_REGIST_CLASS("stoptext", WebInterFaceProcess, WebInterFaceProcess::CloudStopText)
F1_REGIST_CLASS("setvolum", WebInterFaceProcess, WebInterFaceProcess::CloudSetVolum)
F1_REGIST_CLASS("startaudiomonitor", WebInterFaceProcess, WebInterFaceProcess::CloudStartAudioMonitor)
F1_REGIST_CLASS("stopaudiomonitor", WebInterFaceProcess, WebInterFaceProcess::CloudStopAudioMonitor)
F1_REGIST_CLASS("setaudiomix", WebInterFaceProcess, WebInterFaceProcess::CloudSetAudioMix)

//26-30
F1_REGIST_CLASS("emergencyswitch", WebInterFaceProcess, WebInterFaceProcess::CloudEmergencySwitch)
F1_REGIST_CLASS("setaudiofollow", WebInterFaceProcess, WebInterFaceProcess::CloudSetAudioFollow)

END_REGIST_CLASS(WebInterFaceProcess)

inline int getFileExtension(char *filename, char **extension)
{
	int string_length = (int)strlen(filename);

	while (filename[string_length--] != '.') {
		if (string_length == 0)
			break;
	}
	if (string_length > 0) string_length += 2;

	if (string_length == 0)
		*extension = NULL;
	else
		*extension = &filename[string_length];

	return string_length;
}

WebInterFaceProcess::WebInterFaceProcess() :bHasInit(false)
{
	ZeroMemory(&LiveParam, sizeof SLiveParam);
	EffectType[0] = Cut;
	EffectType[1] = DisSolve;
	EffectType[2] = UpDown;
	EffectType[3] = DownUp;
	EffectType[4] = LeftRight;
	EffectType[5] = RightLeft;
	EffectType[6] = Diffuse;
	EffectType[7] = Radius;
	EffectType[8] = Clock;
	BackupIndex = -1;
	bLiving = false;
	bPlayPGM = true;
	bPlayPVW = false;
	PGMVolum = 1.0f;
}
WebInterFaceProcess::~WebInterFaceProcess()
{
	if (bHasInit)
		SLiveRelese();
}

const char* LiveStream = "{ \"ClassName\":\"VideoLiveSource\", \"data\" : {"
"\"CurrentIndex\": 0,"
"\"LastbufferTime\" : 0,"
"\"Name\" : \"%s\","
"\"ScanInterlace\" : %d,"
"\"WarnTime\" : 0,"
"\"bufferTime\" : %d,"
"\"playlist\" : ["
"\"%s\""
"],"
"\"videoBrightness\" : 0,"
"\"videoContrast\" : 0,"
"\"videoGamma\" : 0,"
"\"volume\" : 100,"
"\"initbuffertime\":%d"
"}"
"}";

const char* VideoStream = "{ \"ClassName\":\"VideoSource\", \"data\" : {"
"\"CurrentIndex\": 0,"
"\"LastbufferTime\" : 0,"
"\"Name\" : \"%s\","
"\"ScanInterlace\" : %d,"
"\"isApplyingVideoFilter\" : 1,"
"\"isAudioOutputToStream\" : 1,"
"\"isFileLoop\" : 0,"
"\"isHardwareDecode\" : 0,"
"\"isHaveSelect\" : 0,"
"\"isListLoop\" : 1,"
"\"isPlaylistLooping\" : 1,"
"\"isSenceChangeStartPlay\" : 0,"
"\"bufferTime\" : %d,"
"\"playlist\" : ["
"\"%s\""
"],"
"\"videoBrightness\" : 100,"
"\"videoContrast\" : 100,"
"\"videoGamma\" : 100,"
"\"volume\" : 100,"
"\"initbuffertime\":%d"
"}"
"}";

const char* ImgStream = "{ \"ClassName\":\"BitmapImageSource\", \"data\" : {"
"\"color\" : 4294967295,"
"\"keyBlend\" : 0,"
"\"keyColor\" : 4294967295,"
"\"keySimilarity\" : 10,"
"\"monitor\" : 0,"
"\"opacity\" : %d,"
"\"path\" : \"%s\""
"}"
"}";


const char *TextStream = "{ \"ClassName\":\"TextOutputSource\", \"data\" : {"
"\"SubTitleRepeatCount\" : -1,"
"\"align\" : 0,"
"\"backgroundColor\" : 4278190080,"
"\"backgroundOpacity\" : 100,"
"\"bold\" : 0,"
"\"color\" : %u,"
"\"extentHeight\" : %d,"
"\"extentWidth\" : %d,"
"\"file\" : null,"
"\"font\" : \"%s\","
"\"fontSize\" : %d,"
"\"italic\" : 0,"
"\"mode\" : 0,"
"\"outlineColor\" : 4278190080,"
"\"outlineOpacity\" : 0,"
"\"outlineSize\" : 2,"
"\"pointFiltering\" : 0,"
"\"scrollMode\" : false,"
"\"scrollSpeed\" : %d,"
"\"subTitleRepeat\" : false,"
"\"subTitleUpDown\" : %s," //false
"\"text\" : \"%s\","
"\"textOpacity\" : 0,"
"\"underline\" : false,"
"\"useOutline\" : 0,"
"\"useSubTitle\" : %s,"  //true
"\"useTextExtents\" : %d,"
 "\"vertical\": 0,"
"\"wrap\" : false"
"}"
"}";

void WebInterFaceProcess::CloudInitSwitcher(CRequse &Req, CRespond &Res)
{
	if (bHasInit)
	{
		BUTEL_THORWERROR("Only Call Once initswitcher");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	const char *Param = Req.GetParam();

	const char *pJsonStart = strstr(Param, "\r\n\r\n");

	Json::Reader jReader;
	Json::Value  jParamValue;

	if (!jReader.parse(pJsonStart + 4, jParamValue))
	{
		BUTEL_THORWERROR("Json Parse Failed! %s",pJsonStart + 4);
	}


	if (jParamValue["cloudswitcherid"].isNull())
	{
		BUTEL_THORWERROR("cloudswitcherid is empty %s", pJsonStart + 4);
	}

	CloudId = jParamValue["cloudswitcherid"].asString();

	if (CloudId.empty())
	{
		BUTEL_THORWERROR("cloudswitcherid is empty %s",pJsonStart + 4);
	}


	if (jParamValue["token"].isNull())
	{
		BUTEL_THORWERROR("token is empty %s", pJsonStart + 4);
	}

	Token = jParamValue["token"].asString();

	if (Token.empty())
	{
		BUTEL_THORWERROR("token is empty %s", pJsonStart + 4);
	}
	
	bool IsObject = jParamValue["sencefile"].isObject();

	if (!IsObject)
	{
		//加载默认场景
		SencesConfig = CHttpNetWork::GetInstance()->GetDefaultSences();
	}
	else
	{
		SencesConfig = jParamValue["sencefile"];

		if (SencesConfig["scenes"].isNull())
		{
			Log::writeError(LOG_RTSPSERV,1," %s sencefile 中没有 scenes字段,加载默认场景文件",__FUNCTION__);

			SencesConfig = CHttpNetWork::GetInstance()->GetDefaultSences();
		}
	}

// 	if (jParamValue["sencefile"].isNull())
// 	{
// 		//加载默认场景
// 		SencesConfig = CHttpNetWork::GetInstance()->GetDefaultSences();
// 	}
// 	else
// 	{
// 		std::string cSenceFile = jParamValue["sencefile"].asCString();
// 
// 		if (cSenceFile.empty())
// 		{
// 			//加载默认场景
// 			SencesConfig = CHttpNetWork::GetInstance()->GetDefaultSences();
// 		}
// 		else
// 		{
// 
// 			if (!jReader.parse(cSenceFile.c_str(), SencesConfig))
// 			{
// 				BUTEL_THORWERROR("场景文件解析失败 %s", cSenceFile.c_str());
// 			}
// 
// 			if (SencesConfig["scenes"].isNull())
// 			{
// 				Log::writeError(LOG_RTSPSERV, 1, " %s sencefile 中没有 scenes字段,加载默认场景文件", __FUNCTION__);
// 
// 				SencesConfig = CHttpNetWork::GetInstance()->GetDefaultSences();
// 			}
// 		}
// 	}

	if (jParamValue["layout"].isNull())
	{
		BUTEL_THORWERROR("layout字段为空 %s", pJsonStart + 4);
	}


// 	std::string &strLayOut = jParamValue["layout"].asString();
// 
// 	if (strLayOut.empty())
// 	{
// 		BUTEL_THORWERROR("LayOut 内容为空");
// 	}
// 
// 	std::string JsonLayOut = "{";
// 	JsonLayOut += strLayOut;
// 	JsonLayOut += "}";
// 
// 	Json::Value vLayout;
// 	if (!jReader.parse(JsonLayOut.c_str(), vLayout))
// 	{
// 		BUTEL_THORWERROR("layout解析失败 %s", JsonLayOut.c_str());
// 	}

	Json::Value &AudioParam = SencesConfig["encodeparam"]["audioparam"];
	Json::Value &VideoParam = SencesConfig["encodeparam"]["videoparam"];
	Json::Value &LiveStreamParam = SencesConfig["livestreamparam"];

	LiveParam.LiveSetting.AudioBitRate = AudioParam["bitrate"].asInt();

	if (LiveParam.LiveSetting.AudioBitRate == 0)
		LiveParam.LiveSetting.AudioBitRate = 96;

	LiveParam.LiveSetting.AudioBitRateSec = LiveParam.LiveSetting.AudioBitRate;
	LiveParam.LiveSetting.AudioChannel = AudioParam["channelcount"].asInt();

	if (LiveParam.LiveSetting.AudioChannel == 0)
		LiveParam.LiveSetting.AudioChannel = 2;

	LiveParam.LiveSetting.AudioChannelSec = LiveParam.LiveSetting.AudioChannel;
	LiveParam.LiveSetting.AudioEncoderType = AudioParam["codec"].asInt();

	if (LiveParam.LiveSetting.AudioEncoderType == 0)
		LiveParam.LiveSetting.AudioEncoderType = 1;

	LiveParam.LiveSetting.AudioEncoderTypeSec = LiveParam.LiveSetting.AudioEncoderType;
	LiveParam.LiveSetting.AudioSampleRate = AudioParam["samplerate"].asInt();

	if (LiveParam.LiveSetting.AudioSampleRate == 0)
		LiveParam.LiveSetting.AudioSampleRate = 48000;

	LiveParam.LiveSetting.AudioSampleRateSec = LiveParam.LiveSetting.AudioSampleRate;
	LiveParam.LiveSetting.AutoConnect = 3;
	LiveParam.LiveSetting.AutoConnectSec = LiveParam.LiveSetting.AutoConnect;
	LiveParam.LiveSetting.BFrameCount = VideoParam["bframenumber"].asInt();

	if (LiveParam.LiveSetting.BFrameCount == 0)
		LiveParam.LiveSetting.BFrameCount = 0;

	LiveParam.LiveSetting.BFrameCountSec = LiveParam.LiveSetting.BFrameCount;
	LiveParam.LiveSetting.bRecoder = false;
	LiveParam.LiveSetting.bRecoderSec = LiveParam.LiveSetting.bRecoder;
	LiveParam.LiveSetting.bUseLiveSec = false;
	LiveParam.LiveSetting.bUsePush = true;
	LiveParam.LiveSetting.bUseBackPush = false;
	LiveParam.LiveSetting.bUseBackPushSec = false;
	LiveParam.LiveSetting.bUseCBR = true;
	LiveParam.LiveSetting.bUseCBRSec = true;

	int EnodeType = SLiveQueryHardEncodeSupport();

	if (EnodeType == 1 || EnodeType == 3)
		LiveParam.LiveSetting.bUseHardEncoder = true;
	else
	{
		LiveParam.LiveSetting.bUseHardEncoder = false;
	}

	LiveParam.HardEncoderType = EnodeType;

	if (LiveStreamParam.size() > 0)
		LiveParam.LiveSetting.DelayTime = LiveStreamParam[UINT(0)]["livedelaytime"].asInt();
	else
	{
		LiveParam.LiveSetting.DelayTime = 0;
	}
	LiveParam.LiveSetting.FPS = VideoParam["framerate"].asInt();

	if (LiveParam.LiveSetting.FPS == 0)
		LiveParam.LiveSetting.FPS = 25;

	LiveParam.LiveSetting.FPSSec = LiveParam.LiveSetting.FPS;
	LiveParam.LiveSetting.Width = VideoParam["width"].asInt();

	if (LiveParam.LiveSetting.Width == 0)
		LiveParam.LiveSetting.Width = 1280;

	LiveParam.LiveSetting.Height = VideoParam["height"].asInt();

	if (LiveParam.LiveSetting.Height == 0)
		LiveParam.LiveSetting.Height = 720;

	LiveParam.LiveSetting.WidthSec = LiveParam.LiveSetting.Width;
	LiveParam.LiveSetting.HeightSec = LiveParam.LiveSetting.Height;
	LiveParam.LiveSetting.VideoBitRate = VideoParam["bitrate"].asInt();

	if (LiveParam.LiveSetting.VideoBitRate == 0)
		LiveParam.LiveSetting.VideoBitRate = 1200;

	LiveParam.LiveSetting.VideoBitRateSec = LiveParam.LiveSetting.VideoBitRate;
	LiveParam.LiveSetting.KeyFrame = VideoParam["gopsize"].asInt();

	if (LiveParam.LiveSetting.KeyFrame == 0)
		LiveParam.LiveSetting.KeyFrame = 1;

	LiveParam.LiveSetting.KeyFrameSec = LiveParam.LiveSetting.KeyFrame;
	LiveParam.LiveSetting.FileType = 1;
	LiveParam.LiveSetting.FileTypeSec = LiveParam.LiveSetting.FileType;
	LiveParam.LiveSetting.bUseHardEncoderSec = false;

	LiveParam.PreviewWidth = jParamValue["videowidth"].asInt();

	if (LiveParam.PreviewWidth == 0)
		LiveParam.PreviewWidth = 1280;

	LiveParam.PreviewHeight = jParamValue["videoheight"].asInt();

	if (LiveParam.PreviewHeight == 0)
		LiveParam.PreviewHeight = 720;
	
	if (LiveStreamParam.size() > 0)
		strcpy_s(LiveParam.LiveSetting.LivePushUrl, LiveStreamParam[UINT(0)]["streamurl"].asCString());
	else
	{
		LiveParam.LiveSetting.LivePushUrl[0] = 0;
	}
	strcpy_s(LiveParam.LiveSetting.X264Preset, "veryfast");
	strcpy_s(LiveParam.LiveSetting.X264Profile, "high");
	strcpy_s(LiveParam.LiveSetting.X264PresetSec, "veryfast");
	strcpy_s(LiveParam.LiveSetting.X264ProfileSec, "high");

	strcpy_s(LiveParam.DeviceSetting.MonitorDevice, "停用");
	strcpy_s(LiveParam.DeviceSetting.ScrProDevice, "停用");

	LiveParam.DeviceSetting.AdpterID = 0;
	//选择显卡
	char *DisplayDevice = NULL;
	if (0 == SLiveDisplayDevices(&DisplayDevice))
	{
		Json::Reader ReaderDvice;
		Json::Value ValueDevice;

		if (ReaderDvice.parse(DisplayDevice, ValueDevice))
		{
			Json::Value &ArryList = ValueDevice["NameList"];

			for (int i = 0; i < ArryList.size(); ++i)
			{
				if (strstr(ArryList[i].asCString(), "NVIDIA"))
				{
					LiveParam.DeviceSetting.AdpterID = i;
					break;
				}
			}
		}
		SLiveFreeMemory(DisplayDevice);
	}


	LiveParam.SDICount = 0;

	LiveParam.Advanced.BufferTime = 100;
	LiveParam.Advanced.DeinterlaceType = 1;
	LiveParam.MediaPort = CHttpNetWork::GetInstance()->GetMediaPort();

	//设置参数
	int ret = 0;
	FunCall((ret = SLiveInit(&LiveParam)));

	if (ret == 0)
	{
		//说明SLiveInit成功，根据配置文件开始创建信源和场景

		Json::Value &Layout = jParamValue["layout"];
		Json::Value &InfoSource = SencesConfig["infosource"];

		for (UINT i = 0; i < Layout.size(); ++i)
		{
			Json::Value &OneValue = Layout[i];
			TagInstance OneInstance;
			OneInstance.InstanceName = OneValue["windowname"].asCString();

			OneInstance.PreviewArea.left = OneValue["windowpositionx"].asDouble() * LiveParam.PreviewWidth / 100;
			OneInstance.PreviewArea.top = OneValue["windowpositiony"].asDouble() * LiveParam.PreviewHeight / 100;
			OneInstance.PreviewArea.width = OneValue["windowwidth"].asDouble() * LiveParam.PreviewWidth / 100;
			OneInstance.PreviewArea.height = OneValue["windowheight"].asDouble() * LiveParam.PreviewHeight / 100;

			if (OneInstance.InstanceName.compare("PGM") == 0)
			{
				//创建直播实例
				FunCall(SLiveCreateInstance(&OneInstance.InstanceID, &OneInstance.PreviewArea, true, false));

			}
			else if (OneInstance.InstanceName.compare("PVW") == 0)
			{
				//创建预览实例
				FunCall(SLiveCreateInstance(&OneInstance.InstanceID, &OneInstance.PreviewArea));
			}
			else
			{
				FunCall(SLiveCreateInstance(&OneInstance.InstanceID, &OneInstance.PreviewArea, false, true));

				//查找小预览中应该添加的信源
				if (InfoSource.size() > 0)
				{
					for (UINT j = 0; j < InfoSource.size(); ++j)
					{
						Json::Value &OneInfo = InfoSource[j];

						if (OneInstance.InstanceName.compare(OneInfo["inputname"].asCString()) == 0)
						{
							int InputType = OneInfo["inputtype"].asInt();
							char TemBuf[2048] = { 0 };
							TagInfoID InfoID;
							VideoArea Area = { 0 };
							Area.width = LiveParam.LiveSetting.Width;
							Area.height = LiveParam.LiveSetting.Height;
							InfoID.InfoType = InputType;

							if (InputType == 2)
							{
								//直播
								sprintf_s(TemBuf, sizeof TemBuf, LiveStream, OneInfo["inputname"].asCString(),
																			 OneInfo["isinterlace"].asInt(),
																			 OneInfo["buffertime"].asInt() / 1000,
																			 OneInfo["inputurl"].asCString(),
																			 OneInfo["initbuffertime"].asInt());

								


							}
							else if (InputType == 1)
							{
								//点播
								sprintf_s(TemBuf, sizeof TemBuf, VideoStream, OneInfo["inputname"].asCString(),
																			  OneInfo["isinterlace"].asInt(),
																			  OneInfo["buffertime"].asInt() / 1000,
																			  OneInfo["inputurl"].asCString(),
																			  OneInfo["initbuffertime"].asInt());
							}

							FunCall(SLiveAddStream(OneInstance.InstanceID, TemBuf, &Area, &InfoID.VideoId, &InfoID.AudioId));
							
							OneInstance.SouceList.push_back(InfoID);

							break;
						}

					}
				}

			}

			InstanceMap.insert(std::make_pair(OneInstance.InstanceName, OneInstance));

		}


		//建立场景

		Json::Value &Scenes = SencesConfig["scenes"];

		if (Scenes.size() > 0)
		{
			for (UINT k = 0; k < Scenes.size(); ++k)
			{
				TagScenes _Scenes;

				Json::Value &OneScene = Scenes[k];

				Json::Value &Sources = OneScene["sources"];

				if (Sources.size() > 0)
				{
					for (UINT m = 0; m < Sources.size(); ++m)
					{
						Json::Value &OneSource = Sources[m];

						TagInfoSource InfSource;
						InfSource.SourceName = OneSource["name"].asCString();
						InfSource.x = OneSource["positionx"].asDouble();
						InfSource.y = OneSource["positiony"].asDouble();
						InfSource.width = OneSource["width"].asDouble();
						InfSource.height = OneSource["height"].asDouble();
						InfSource.layer = OneSource["layer"].asInt();
						InfSource.Transparent = OneSource["transparence"].asInt();

						_Scenes.SouceList.push_back(InfSource);
					}
				}

				ScenesMap.insert(std::make_pair(OneScene["senceindex"].asInt(), _Scenes));
			}
		}

		//创建图片源

		Json::Value &ImgSource = SencesConfig["imagesource"];

		if (ImgSource.size() > 0)
		{
			VideoArea Area = { 0 };
			Area.width = LiveParam.LiveSetting.Width;
			Area.height = LiveParam.LiveSetting.Height;

			for (UINT i = 0; i < ImgSource.size(); ++i)
			{
				Json::Value &OneImg = ImgSource[i];

				TagInstance Img;
				AddImage(Img, OneImg);

				if (Img.ImgParam.bRender)
				{
					//所有场景中都添加该源

					std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

					for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
					{
						bool bFind = false;
						for (int i = 0; i < Itbegin->second.SouceList.size(); ++i)
						{
							TagInfoSource& Source = Itbegin->second.SouceList[i];
							if (!Source.SourceName.compare(Img.InstanceName))
							{
								bFind = true;
								break;
							}
						}

						if (!bFind)
						{
							TagInfoSource InfoSource;
							InfoSource.x = Img.ImgParam.x;
							InfoSource.y = Img.ImgParam.y;
							InfoSource.SourceName = Img.ImgParam.ImgID;
							InfoSource.Transparent = Img.ImgParam.transparent;
							InfoSource.width = Img.ImgParam.width;
							InfoSource.height = Img.ImgParam.height;

							Itbegin->second.SouceList.push_back(InfoSource);
						}
					}

					//添加到PVW中去

					VideoArea Area = { 0 };

					Area.left = Img.ImgParam.x * LiveParam.LiveSetting.Width / 100;
					Area.top = Img.ImgParam.y * LiveParam.LiveSetting.Height / 100;
					Area.width = Img.ImgParam.width * LiveParam.LiveSetting.Width / 100;
					Area.height = Img.ImgParam.height * LiveParam.LiveSetting.Height / 100;

					FunCall(SLiveAdd2Intance(Img.InstanceID, InstanceMap["PVW"].InstanceID, &Area, true));
				}

			}
		}

		//创建文件源

		Json::Value &TextSource = SencesConfig["textsource"];

		if (TextSource.size() > 0)
		{
			VideoArea Area = { 0 };
			Area.width = LiveParam.LiveSetting.Width;
			Area.height = LiveParam.LiveSetting.Height;
			for (UINT i = 0; i < TextSource.size(); ++i)
			{
				Json::Value &OneText = TextSource[i];

				char *OutText = CHttpNetWork::GetInstance()->UTF8ToGB(OneText["text"].asCString());

				if (OutText)
				{
					OneText["text"] = OutText;
					delete[] OutText;
				}

				TagInstance Text;
				AddText(Text, OneText);

				if (Text.TextParam.bRender)
				{
					//在PVW和所有的场景中加入该源


					std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

					for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
					{
						bool bFind = false;
						for (int i = 0; i < Itbegin->second.SouceList.size(); ++i)
						{
							TagInfoSource& Source = Itbegin->second.SouceList[i];
							if (!Source.SourceName.compare(Text.InstanceName))
							{
								bFind = true;
								break;
							}
						}

						if (!bFind)
						{
							TagInfoSource InfoSource;
							InfoSource.x = Text.TextParam.x;
							InfoSource.y = Text.TextParam.y;
							InfoSource.SourceName = Text.TextParam.TextID;
							InfoSource.width = Text.TextParam.RealWidth;
							InfoSource.height = Text.TextParam.RealHeight;

							Itbegin->second.SouceList.push_back(InfoSource);
						}
					}

					//添加到PVW中去

					VideoArea Area = { 0 };

					Area.left = Text.TextParam.x * LiveParam.LiveSetting.Width / 100;
					Area.top = Text.TextParam.y * LiveParam.LiveSetting.Height / 100;
					Area.width = Text.TextParam.RealWidth * LiveParam.LiveSetting.Width / 100;
					Area.height = Text.TextParam.RealHeight * LiveParam.LiveSetting.Height / 100;

					FunCall(SLiveAdd2Intance(Text.InstanceID, InstanceMap["PVW"].InstanceID, &Area, true));


				}
			}
		}

		
	}
	

	Json::Value jVaule;
	jVaule["state"]["rc"] = ret;

	if (ret != 0)
	{
		jVaule["state"]["msg"] = SLiveGetLastError();
	}
	else
	{
		jVaule["state"]["msg"] = "成功";
		jVaule["cloudswitcherid"] = CloudId.c_str();

		std::string LocalIp;

		PIP_ADAPTER_INFO pAdapterInfo;
		PIP_ADAPTER_INFO pAdapter = NULL;
		DWORD dwRetVal = 0;
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
		{
			pAdapter = pAdapterInfo;
			while (pAdapter)
			{
				//pAdapter->Description中包含"PCI"为：物理网卡
				//pAdapter->Type是71为：无线网卡
				bool PhysicsAdress = StrStrIA(pAdapter->Description, "PCI") && pAdapter->Type == 6;
				bool WireLessAdress = StrStrIA(pAdapter->Description, "WireLess") && pAdapter->Type == 71;
				if (PhysicsAdress || WireLessAdress)
				{
					std::string localIpTemp = pAdapter->IpAddressList.IpAddress.String;
					Log::writeMessage(LOG_RTSPSERV, 1, "GetAdaptersInfo LocalIp:%s PhysicsAdress:%d,WireLessAdress:%d", LocalIp.c_str(), PhysicsAdress, WireLessAdress);
					if (PhysicsAdress)
					{
						LocalIp = localIpTemp;
						break;
					}
				}
				pAdapter = pAdapter->Next;
			}

			Log::writeMessage(LOG_RTSPSERV, 1, "get LocalIp:[%s]", LocalIp.c_str());

			if (LocalIp.empty())
			{
				char HostName[100] = { 0 };
				gethostname(HostName, sizeof HostName);

				hostent *Htent = gethostbyname(HostName);
				if (Htent)
				{
					LocalIp = inet_ntoa(*(struct in_addr*)(Htent->h_addr_list[0]));
				}

				Log::writeMessage(LOG_RTSPSERV, 1, "LocalIp为空 取默认IP %s", LocalIp.c_str());
			}
		}
		else
		{
			char HostName[100] = { 0 };
			gethostname(HostName, sizeof HostName);

			hostent *Htent = gethostbyname(HostName);
			if (Htent)
			{
				LocalIp = inet_ntoa(*(struct in_addr*)(Htent->h_addr_list[0]));
			}
			Log::writeMessage(LOG_RTSPSERV, 1, "GetAdaptersInfo failed 取默认IP %s", LocalIp.c_str());
		}

		char PlayUrl[256] = { 0 };

		sprintf_s(PlayUrl, sizeof PlayUrl, "http://%s:%d/1.flv", LocalIp.c_str(), LiveParam.MediaPort);

		jVaule["result"]["playurl"] = PlayUrl;

		Log::writeMessage(LOG_RTSPSERV, 1, "%s PlayURL %s", __FUNCTION__, PlayUrl);
	}
	
	Res.SetRespond(jVaule);

	if (ret == 0)
	{
		Res.SetResType(RES_OK);
		bHasInit = true;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end ret = %d!", __FUNCTION__,ret);
}

void WebInterFaceProcess::CloudCloseSwitcher(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);


	const char *Param = Req.GetParam();

	const char *pJsonStart = strstr(Param, "\r\n\r\n");

	Json::Reader jReader;
	Json::Value  jParamValue;

	if (!jReader.parse(pJsonStart + 4, jParamValue))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
		BUTEL_THORWERROR("Json解析失败 %s", pJsonStart + 4);
	}

	std::string &CloudCheckId = jParamValue["cloudswitcherid"].asString();

	if (CloudCheckId.empty())
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
		BUTEL_THORWERROR("cloudswitcherid 为空");
	}

	if (CloudId.compare(CloudCheckId))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
		BUTEL_THORWERROR("cloudswitcherid 不正确 %s", CloudCheckId.c_str());
	}

	jParamValue.clear();

	Res.SetResType(RES_OK);

	jParamValue["state"]["rc"] = 0;
	jParamValue["state"]["msg"] = "成功";
	Res.SetRespond(jParamValue);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudGetSenceConfig(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam, true);


	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	JValueParam["result"]["config"] = SencesConfig;
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetSenceParam(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	int SenceIndex = JValueParam["senceindex"].asInt();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin! SenceIndex = %d", __FUNCTION__, SenceIndex);

	std::map<int, TagScenes>::iterator it = ScenesMap.find(SenceIndex);

	if (it != ScenesMap.end())
	{
		Json::Value &Scenes = SencesConfig["scenes"];

		for (UINT k = 0; k < Scenes.size(); ++k)
		{
			TagScenes _Scenes;

			Json::Value &OneScene = Scenes[k];

			if (OneScene["senceindex"].asInt() == SenceIndex)
			{
				OneScene["sources"] = JValueParam["inputparam"];
				break;
			}
		}


		it->second.SouceList.clear();

		Json::Value &Sources = JValueParam["inputparam"];

		if (Sources.size() > 0)
		{
			for (UINT i = 0; i < Sources.size(); ++i)
			{
				Json::Value &OneSource = Sources[i];

				TagInfoSource InfSource;
				InfSource.SourceName = OneSource["name"].asCString();
				InfSource.x = OneSource["positionx"].asDouble();
				InfSource.y = OneSource["positiony"].asDouble();
				InfSource.width = OneSource["width"].asDouble();
				InfSource.height = OneSource["height"].asDouble();
				InfSource.layer = OneSource["layer"].asInt();
				InfSource.Transparent = OneSource["transparence"].asInt();

				it->second.SouceList.push_back(InfSource);

				//如果要设置全局的一个透明度在这里设置
// 				std::map<std::string, TagInstance>::iterator itInstance = InstanceMap.find(InfSource.SourceName);
// 
// 				if (itInstance != InstanceMap.end())
// 				{
// 					//在这里设置透明度信息
// 					if (itInstance->second.SouceList.size() > 0)
// 						FunCall(SLiveSetOpacity(itInstance->second.InstanceID, itInstance->second.SouceList[0].VideoId, 255 * (1.0f - (float)InfSource.Transparent / 100)));
// 				}
			}
		}

		
	}
	else
	{
		BUTEL_THORWERROR("该场景索引 %d 不存在",SenceIndex);
	}


	//更改了配置文件要向管理中心汇报

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	Res.SetResType(RES_OK);
	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);

}

void WebInterFaceProcess::CloudChangeSence(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(),JValueParam);

	int SenceIndex = JValueParam["senceindex"].asInt();
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin! SenceIndex = %d", __FUNCTION__, SenceIndex);

	std::map<int, TagScenes>::iterator it = ScenesMap.find(SenceIndex);

	if (it != ScenesMap.end())
	{
		TagInstance &PVW = InstanceMap["PVW"];
		FunCall(SLiveClearIntances(PVW.InstanceID,false));

		for (int i = 0; i < it->second.SouceList.size(); i++)
		{
			VideoArea Area = { 0 };
			TagInfoSource &Source = it->second.SouceList[i];

			Area.left = Source.x * LiveParam.LiveSetting.Width / 100;
			Area.top = Source.y * LiveParam.LiveSetting.Height / 100;
			Area.width = Source.width * LiveParam.LiveSetting.Width / 100;
			Area.height = Source.height * LiveParam.LiveSetting.Height / 100;
			
			

			TagInstance &Instance = InstanceMap[Source.SourceName];
			FunCall(SLiveAdd2Intance(Instance.InstanceID, PVW.InstanceID, &Area, true));

			//在这里设置每个场景独有的透明度信息
			if (Instance.SouceList.size() > 0)
				FunCall(SLiveSetOpacity(Instance.InstanceID, Instance.SouceList[0].VideoId, 255 * (1.0f - (float)Source.Transparent / 100)));
		}
	}
	else
	{
		BUTEL_THORWERROR("该场景索引 %d 不存在", SenceIndex);
	}

	Res.SetResType(RES_OK);
	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSwitchPVWtoPGM(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);


	int EffectIndex = JValueParam["effecttype"].asInt();
	int Duration = JValueParam["duration"].asInt();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin Effect = %d,Duration = %d!", __FUNCTION__, EffectIndex, Duration);


	if (EffectIndex >= sizeof EffectType / sizeof EffectType[0])
	{
		BUTEL_THORWERROR("effecttype 过大 = %d",EffectIndex);
	}

	TagInstance &PVW = InstanceMap["PVW"];
	TagInstance &PGM = InstanceMap["PGM"];

	FunCall(SLiveSwitchInstance(PVW.InstanceID, PGM.InstanceID, EffectType[EffectIndex], Duration));

	JValueParam.clear();
	Res.SetResType(RES_OK);
	
	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetEmergencySence(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);


	int SenceIndex = JValueParam["senceindex"].asInt();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin SenceIndex = %d!", __FUNCTION__, SenceIndex);

	if (SenceIndex != 0)
	{
		std::map<int, TagScenes>::iterator it = ScenesMap.find(SenceIndex);

		if (it == ScenesMap.end())
		{
			BUTEL_THORWERROR("该场景索引 %d 不存在", SenceIndex);
		}

		BackupIndex = SenceIndex;
	}

	if (SenceIndex == 0)
	{
		InputName = JValueParam["inputname"].asCString();

		if (InputName.empty())
		{
			BUTEL_THORWERROR("inputname 为空");
		}

		Log::writeMessage(LOG_RTSPSERV, 1, "%s InputName = %s", __FUNCTION__, InputName.c_str());
	}

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudLiveSetInputParam(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	const char* cInputName = JValueParam["inputname"].asCString();
	int Inputtpye = JValueParam["inputtype"].asInt();

	BUTEL_IFNULLRETURNERROR(cInputName);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin cInputName =%s,Inputtype = %d!", __FUNCTION__,cInputName,Inputtpye);

	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(cInputName);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("该信源 %s 不存在",cInputName);
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "%s it->second.SouceList.size = %d!", __FUNCTION__, it->second.SouceList.size());

	char TemBuf[1024] = { 0 };

	if (Inputtpye == 2)
	{
		//直播
		sprintf_s(TemBuf, sizeof TemBuf, LiveStream, JValueParam["inputname"].asCString(),
			JValueParam["isinterlace"].asInt(),
			JValueParam["buffertime"].asInt() / 1000,
			JValueParam["inputurl"].asCString(),
			JValueParam["initbuffertime"].asInt());
	}
	else if (Inputtpye == 1)
	{
		//点播
		sprintf_s(TemBuf, sizeof TemBuf, VideoStream, JValueParam["inputname"].asCString(),
			JValueParam["isinterlace"].asInt(),
			JValueParam["buffertime"].asInt() / 1000,
			JValueParam["inputurl"].asCString(),
			JValueParam["initbuffertime"].asInt());
	}

	TagInfoID InfoID;
	VideoArea Area = { 0 };
	Area.width = LiveParam.LiveSetting.Width;
	Area.height = LiveParam.LiveSetting.Height;
	InfoID.InfoType = Inputtpye;

	if (it->second.SouceList.size() > 0)
	{
		TagInfoID &Info = it->second.SouceList[0];

		if (Info.InfoType == Inputtpye)
		{
			FunCall(SLiveUpdateStream(it->second.InstanceID, Info.VideoId, TemBuf));
		}
		else
		{
			//删除重新创建
			Log::writeMessage(LOG_RTSPSERV, 1, "%s 源类型和设置的类型不一致，需要删除重建，源类型 %d,设置类型 %d", __FUNCTION__,Info.InfoType,Inputtpye);


			FunCall(SLiveDestroyInstance(it->second.InstanceID));

			TagInstance Intance;

			Intance.PreviewArea = it->second.PreviewArea;

			InstanceMap.erase(it);

			Intance.InstanceName = cInputName;

			FunCall(SLiveCreateInstance(&Intance.InstanceID, &Intance.PreviewArea, false, true));

			FunCall(SLiveAddStream(Intance.InstanceID, TemBuf, &Area, &InfoID.VideoId, &InfoID.AudioId));
			Intance.SouceList.push_back(InfoID);


			InstanceMap.insert(std::make_pair(cInputName, Intance));

		}

	}
	else
	{
		FunCall(SLiveAddStream(it->second.InstanceID, TemBuf, &Area, &InfoID.VideoId, &InfoID.AudioId));

		it->second.SouceList.push_back(InfoID);
	}

	//修改场景文件

	Json::Value &InfoSource = SencesConfig["infosource"];

	for (UINT i = 0; i < InfoSource.size(); ++i)
	{
		Json::Value& OneSource = InfoSource[i];

		if (!OneSource["inputname"].asString().compare(cInputName))
		{
			OneSource["isinterlace"] = JValueParam["isinterlace"].asInt();
			OneSource["buffertime"] = JValueParam["buffertime"].asInt();
			OneSource["inputurl"] = JValueParam["inputurl"].asCString();
			OneSource["initbuffertime"] = JValueParam["initbuffertime"].asInt();
			OneSource["inputtype"] = JValueParam["inputtype"];
			break;
		}
	}


	//更改了配置文件要向管理中心汇报

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);


	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudChangeInput(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	const char* cInputName = JValueParam["inputname"].asCString();

	BUTEL_IFNULLRETURNERROR(cInputName);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin cInputName = %s!", __FUNCTION__, cInputName);


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(cInputName);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("该信源 %s 不存在", cInputName);
	}

	TagInstance &PVW = InstanceMap["PVW"];

	SLiveClearIntances(PVW.InstanceID,false);

	VideoArea Area = { 0 };
	Area.width = LiveParam.LiveSetting.Width;
	Area.height = LiveParam.LiveSetting.Height;

	SLiveAdd2Intance(it->second.InstanceID, PVW.InstanceID, &Area, true);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetLiveEncodeParam(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	if (bLiving)
	{
		BUTEL_THORWERROR("直播中不允许更改参数");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	Json::Value &AudioParam = JValueParam["audioparam"];
	Json::Value &VideoParam = JValueParam["videoparam"];

	LiveParam.LiveSetting.bChange = true;
	LiveParam.LiveSetting.AudioBitRate = AudioParam["bitrate"].asInt();
	LiveParam.LiveSetting.AudioChannel = AudioParam["channelcount"].asInt();
	LiveParam.LiveSetting.AudioEncoderType = AudioParam["channelcount"].asInt();
	LiveParam.LiveSetting.AudioSampleRate = AudioParam["samplerate"].asInt();
	LiveParam.LiveSetting.BFrameCount = VideoParam["bframenumber"].asInt();
	LiveParam.LiveSetting.FPS = VideoParam["framerate"].asInt();
	LiveParam.LiveSetting.Width = VideoParam["width"].asInt();
	LiveParam.LiveSetting.Height = VideoParam["height"].asInt();
	LiveParam.LiveSetting.VideoBitRate = VideoParam["bitrate"].asInt();
	LiveParam.LiveSetting.KeyFrame = VideoParam["gopsize"].asInt();

	FunCall(SLiveSetParam(&LiveParam));


	//修改场景文件

	Json::Value &Encodeparam = SencesConfig["encodeparam"];

	Encodeparam["videoparam"] = VideoParam;
	Encodeparam["audioparam"] = AudioParam;

	//更改了配置文件要向管理中心汇报

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetLiveStreamParam(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}


	if (bLiving)
	{
		BUTEL_THORWERROR("直播中不允许更改参数");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	LiveParam.LiveSetting.bChange = true;
	LiveParam.LiveSetting.DelayTime = JValueParam["livedelaytime"].asInt();
	strcpy_s(LiveParam.LiveSetting.LivePushUrl, JValueParam["streamurl"].asCString());

	FunCall(SLiveSetParam(&LiveParam));

	//修改场景文件

	Json::Value &LiveStreamParam = SencesConfig["livestreamparam"];

	LiveStreamParam[Json::UInt(0)]["streamurl"] = LiveParam.LiveSetting.LivePushUrl;
	LiveStreamParam[Json::UInt(0)]["livedelaytime"] = LiveParam.LiveSetting.DelayTime;

	//更改了配置文件要向管理中心汇报
	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStartLive(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	int Ret = 0;
	FunCall((Ret = SLiveStartLive(InstanceMap["PGM"].InstanceID)));

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = Ret;
	if (Ret == -1)
	{
		JValueParam["state"]["msg"] = SLiveGetLastError();
	}
	else
	{
		JValueParam["state"]["msg"] = "成功";
		bLiving = true;
	}
	
	Res.SetRespond(JValueParam);


	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStopLive(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);


	int Ret = 0;
	FunCall((Ret = SLiveStopLive(InstanceMap["PGM"].InstanceID)));

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = Ret;
	if (Ret == -1)
	{
		JValueParam["state"]["msg"] = SLiveGetLastError();
	}
	else
	{
		JValueParam["state"]["msg"] = "成功";
		bLiving = false;
	}

	JValueParam.clear();

	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetImage(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	std::string &ImgId = JValueParam["imageid"].asString();

	if (ImgId.empty())
	{
		//新创建
		TagInstance Img;
		AddImage(Img, JValueParam);

		//更新配置文件
		Json::Value &ImgInfo = SencesConfig["imagesource"];

		Json::Value AddVaule;

		AddVaule["imageurl"] = JValueParam["imageurl"].asString();
		AddVaule["positionx"] = JValueParam["positionx"].asDouble();
		AddVaule["positiony"] = JValueParam["positiony"].asDouble();
		AddVaule["width"] = JValueParam["width"].asDouble();
		AddVaule["height"] = JValueParam["height"].asDouble();
		AddVaule["transparence"] = JValueParam["transparence"].asInt();
		AddVaule["imageid"] = Img.InstanceName.c_str();
		AddVaule["name"] = JValueParam["imagename"].asString();
		AddVaule["brender"] = false;

		ImgInfo.append(AddVaule);

		ImgId = Img.InstanceName;
	}
	else
	{
		//设置

		std::map<std::string, TagInstance>::iterator it = InstanceMap.find(ImgId);

		if (it == InstanceMap.end())
		{
			BUTEL_THORWERROR("图片Id %s 不存在",ImgId.c_str());
		}

// 		it->second.ImgParam.ImgUrl = JValueParam["imageurl"].asCString();
// 		it->second.ImgParam.x = JValueParam["positionx"].asDouble();
// 		it->second.ImgParam.y = JValueParam["positiony"].asDouble();
// 		it->second.ImgParam.width = JValueParam["width"].asDouble();
// 		it->second.ImgParam.height = JValueParam["height"].asDouble();
// 		it->second.ImgParam.transparent = JValueParam["transparence"].asInt();
// 
// 		char TemBuf[1024] = { 0 };
// 
// 		sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - it->second.ImgParam.transparent, it->second.ImgParam.ImgUrl.c_str());
// 
// 		FunCall(SLiveUpdateStream(it->second.InstanceID, it->second.SouceList[0].VideoId, TemBuf));

		JValueParam["brender"] = it->second.ImgParam.bRender;

		AddImage(it->second, JValueParam, true);

		//更新配置文件

		Json::Value &ImgInfo = SencesConfig["imagesource"];

		for (UINT i = 0; i < ImgInfo.size(); ++i)
		{
			Json::Value &OneImg = ImgInfo[i];

			if (!OneImg["imageid"].asString().compare(ImgId))
			{
				OneImg["imageurl"] = JValueParam["imageurl"].asCString();
				OneImg["positionx"] = JValueParam["positionx"].asDouble();
				OneImg["positiony"] = JValueParam["positiony"].asDouble();
				OneImg["width"] = JValueParam["width"].asDouble();
				OneImg["height"] = JValueParam["height"].asDouble();
				OneImg["transparence"] = JValueParam["transparence"].asInt();
				OneImg["name"] = JValueParam["imagename"].asString();
				break;
			}
		}
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	JValueParam["result"]["imageid"] = ImgId.c_str();
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudDeleteImage(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	std::string &ImgId = JValueParam["imageid"].asString();

	if (ImgId.empty())
	{
		BUTEL_THORWERROR("imageid 为空");
	}


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(ImgId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("图片Id %s 不存在", ImgId.c_str());
	}

	if (it->second.ImgParam.bRender)
	{
		//删除所有场景中的该源

		std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

		for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
		{
			std::vector<TagInfoSource>::iterator itSourceBegin = Itbegin->second.SouceList.begin();

			for (itSourceBegin; itSourceBegin != Itbegin->second.SouceList.end(); itSourceBegin++)
			{
				if (!itSourceBegin->SourceName.compare(ImgId))
				{
					Itbegin->second.SouceList.erase(itSourceBegin);
					break;
				}
			}
		}
	}

	//删除PVW中的该图片
	FunCall(SLiveDelStream(InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId));
	FunCall(SLiveDestroyInstance(it->second.InstanceID));

	InstanceMap.erase(it);

	Json::Value &ImageInfo = SencesConfig["imagesource"];

	int size = ImageInfo.size();

	for (UINT i = 0; i < size; ++i)
	{
		Json::Value &OneImg = ImageInfo[i];

		if (!OneImg["imageid"].asString().compare(ImgId))
		{
			//这种删除的方式不一定正确
			if (i != size - 1)
			{
				OneImg.swap(ImageInfo[size - 1]);
			}

			break;
		}
	}
	if (size - 1 != 0)
		ImageInfo.resize(size - 1);
	else
	{
		ImageInfo.clear();
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStartImage(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);


	std::string &ImgId = JValueParam["imageid"].asString();

	if (ImgId.empty())
	{
		BUTEL_THORWERROR("imageid 为空");
	}


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(ImgId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("图片Id %s 不存在", ImgId.c_str());
	}


	it->second.ImgParam.bRender = true;
	
	//所有场景中都添加该源

	std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

	for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
	{
		bool bFind = false;
		for (int i = 0; i < Itbegin->second.SouceList.size(); ++i)
		{
			TagInfoSource& Source = Itbegin->second.SouceList[i];
			if (!Source.SourceName.compare(it->second.ImgParam.ImgID))
			{
				bFind = true;
				break;
			}
		}

		if (!bFind)
		{
			TagInfoSource InfoSource;
			InfoSource.x = it->second.ImgParam.x;
			InfoSource.y = it->second.ImgParam.y;
			InfoSource.SourceName = it->second.ImgParam.ImgID;
			InfoSource.Transparent = it->second.ImgParam.transparent;
			InfoSource.width = it->second.ImgParam.width;
			InfoSource.height = it->second.ImgParam.height;

			Itbegin->second.SouceList.push_back(InfoSource);
		}
	}
	
	//添加到PVW中去

	VideoArea Area = { 0 };

	Area.left = it->second.ImgParam.x * LiveParam.LiveSetting.Width / 100;
	Area.top = it->second.ImgParam.y * LiveParam.LiveSetting.Height / 100;
	Area.width = it->second.ImgParam.width * LiveParam.LiveSetting.Width / 100;
	Area.height = it->second.ImgParam.height * LiveParam.LiveSetting.Height / 100;

	FunCall(SLiveAdd2Intance(it->second.InstanceID, InstanceMap["PVW"].InstanceID, &Area, true));


	Json::Value &ImageInfo = SencesConfig["imagesource"];

	for (UINT i = 0; i < ImageInfo.size(); ++i)
	{
		Json::Value &OneImg = ImageInfo[i];

		if (!OneImg["imageid"].asString().compare(ImgId))
		{
			OneImg["brender"] = true;
			break;
		}
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStopImage(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	const char *ImgId = JValueParam["imageid"].asCString();

	if (!ImgId)
	{
		BUTEL_THORWERROR("imageid 为空");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin ImgId = %s!", __FUNCTION__,ImgId);

	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(ImgId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("图片Id %s 不存在", ImgId);
	}

	it->second.ImgParam.bRender = false;

	//去掉所有场景及PVW中该图片

	std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

	for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
	{
		std::vector<TagInfoSource>::iterator itSource = Itbegin->second.SouceList.begin();
		for (itSource; itSource != Itbegin->second.SouceList.end(); itSource ++)
		{
			if (!itSource->SourceName.compare(ImgId))
			{
				Itbegin->second.SouceList.erase(itSource);
				break;
			}
		}
	}

	//删除PVW中的该图片
	FunCall(SLiveDelStream(InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId));

	Json::Value &ImageInfo = SencesConfig["imagesource"];

	for (UINT i = 0; i < ImageInfo.size(); ++i)
	{
		Json::Value &OneImg = ImageInfo[i];

		if (!OneImg["imageid"].asString().compare(ImgId))
		{
			OneImg["brender"] = false;
			break;
		}
	}

	//更新配置文件后上报管理中心
	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudAddText(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	std::string &TextId = JValueParam["textid"].asString();

	char *OutText = CHttpNetWork::GetInstance()->UTF8ToGB(JValueParam["text"].asCString());

	if (OutText)
	{
		JValueParam["text"] = OutText;
		delete [] OutText;
	}

	if (TextId.empty())
	{
		//新创建

		TagInstance Text;
		
		AddText(Text, JValueParam);

		//更新配置文件

		Json::Value &ImgInfo = SencesConfig["imagesource"];

		Json::Value AddVaule;

		AddVaule["brender"] = false;
		AddVaule["text"] = JValueParam["text"].asCString();
		AddVaule["positionx"] = JValueParam["positionx"].asDouble();
		AddVaule["positiony"] = JValueParam["positiony"].asDouble();
		AddVaule["textsize"] = JValueParam["textsize"].asInt();
		if (JValueParam["textcolor"].isNull())
		{
			UINT Color = 16777215;
			AddVaule["textcolor"] = Color;
		}
		else
		{
			AddVaule["textcolor"] = JValueParam["textcolor"].asUInt();
		}
		
		AddVaule["texttype"] = JValueParam["texttype"].asCString();
		AddVaule["enableroll"] = JValueParam["enableroll"].asBool();
		AddVaule["rollspeed"] = JValueParam["rollspeed"].asInt();
		AddVaule["rolltype"] = JValueParam["rolltype"].asInt();
		AddVaule["name"] = JValueParam["textname"].asCString();

		ImgInfo.append(AddVaule);

		TextId = Text.InstanceName;
	}
	else
	{
		//设置

		std::map<std::string, TagInstance>::iterator it = InstanceMap.find(TextId);

		if (it == InstanceMap.end())
		{
			BUTEL_THORWERROR("文字Id %s 不存在", TextId.c_str());
		}

		it->second.TextParam.text = JValueParam["text"].asCString();
		it->second.TextParam.x = JValueParam["positionx"].asDouble();
		it->second.TextParam.y = JValueParam["positiony"].asDouble();
		it->second.TextParam.textSize = JValueParam["textsize"].asInt();

		if (JValueParam["textcolor"].isNull())
		{
			UINT Color = 16777215;
			it->second.TextParam.textColor = Color;
		}
		else
		{
			it->second.TextParam.textColor = JValueParam["textcolor"].asUInt();
		}

		it->second.TextParam.textType = JValueParam["texttype"].asCString();
		it->second.TextParam.enableRoll = JValueParam["enableroll"].asBool();
		it->second.TextParam.rollSpeed = JValueParam["rollspeed"].asInt();
		it->second.TextParam.rollType = JValueParam["rolltype"].asInt();

		char TemBuf[4096] = { 0 };

		sprintf_s(TemBuf, sizeof TemBuf, TextStream, 0xFF << 24 | it->second.TextParam.textColor, it->second.TextParam.textSize + 2, LiveParam.LiveSetting.Width,
			it->second.TextParam.textType.c_str(), it->second.TextParam.textSize, it->second.TextParam.rollSpeed, it->second.TextParam.rollType == 1 ? "false" : "true", it->second.TextParam.text.c_str(), it->second.TextParam.enableRoll == 1 ? "true" : "false",
			it->second.TextParam.enableRoll);

		FunCall(SLiveUpdateStream(it->second.InstanceID, it->second.SouceList[0].VideoId, TemBuf));

		UINT Width = 0, Height = it->second.TextParam.textSize;

		FunCall(SLiveGetStreamSize(it->second.InstanceID, it->second.SouceList[0].VideoId, &Width, &Height));

		it->second.TextParam.RealWidth = (float)Width / LiveParam.LiveSetting.Width * 100;
		it->second.TextParam.RealHeight = (float)Height / LiveParam.LiveSetting.Height * 100;

		VideoArea Area = { 0 };

		Area.left = it->second.TextParam.x * LiveParam.LiveSetting.Width / 100;
		Area.top = it->second.TextParam.y * LiveParam.LiveSetting.Height / 100;
		Area.width = it->second.TextParam.RealWidth * LiveParam.LiveSetting.Width / 100;
		Area.height = it->second.TextParam.RealHeight * LiveParam.LiveSetting.Height / 100;

		if (it->second.TextParam.bRender)
		{
			FunCall(SLiveUpdateStreamPosition(InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId, &Area));
			//FunCall(SLiveUpdateStreamPosition(InstanceMap["PGM"].InstanceID, it->second.SouceList[0].VideoId, &Area));
		}

		//更新配置文件

		Json::Value &TextInfo = SencesConfig["textsource"];

		for (UINT i = 0; i < TextInfo.size(); ++i)
		{
			Json::Value &OneText = TextInfo[i];

			if (!OneText["textid"].asString().compare(TextId))
			{
				OneText["text"] = it->second.TextParam.text;
				OneText["positionx"] = it->second.TextParam.x;
				OneText["positiony"] = it->second.TextParam.y;
				OneText["textsize"] = it->second.TextParam.textSize;
				OneText["textcolor"] = it->second.TextParam.textColor;
				OneText["texttype"] = it->second.TextParam.textType;
				OneText["enableroll"] = it->second.TextParam.enableRoll;
				OneText["rollspeed"] = it->second.TextParam.rollSpeed;
				OneText["rolltype"] = it->second.TextParam.rollType;
				OneText["name"] = JValueParam["textname"].asCString();
				break;
			}
		}
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	JValueParam["result"]["textid"] = TextId.c_str();
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudDeleteText(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	std::string &TextId = JValueParam["textid"].asString();

	if (TextId.empty())
	{
		BUTEL_THORWERROR("textid 为空");
	}


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(TextId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("文字Id %s 不存在", TextId.c_str());
	}

	if (it->second.TextParam.bRender)
	{
		//删除所有场景中的该源

		std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

		for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
		{
			std::vector<TagInfoSource>::iterator itSourceBegin = Itbegin->second.SouceList.begin();

			for (itSourceBegin; itSourceBegin != Itbegin->second.SouceList.end(); itSourceBegin++)
			{
				if (!itSourceBegin->SourceName.compare(TextId))
				{
					Itbegin->second.SouceList.erase(itSourceBegin);
					break;
				}
			}
		}
	}

	//删除PVW中的该图片
	FunCall(SLiveDelStream(InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId));
	FunCall(SLiveDestroyInstance(it->second.InstanceID));

	InstanceMap.erase(it);

	Json::Value &TextInfo = SencesConfig["textsource"];

	int size = TextInfo.size();

	for (UINT i = 0; i < size; ++i)
	{
		Json::Value &OneText = TextInfo[i];

		if (!OneText["textid"].asString().compare(TextId))
		{
			//这种删除的方式不一定正确
			if (i != size - 1)
			{
				OneText.swap(TextInfo[size - 1]);
			}

			break;
		}
	}
	if (size - 1 != 0)
		TextInfo.resize(size - 1);
	else
	{
		TextInfo.clear();
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStartText(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	std::string &TextId = JValueParam["textid"].asString();

	if (TextId.empty())
	{
		BUTEL_THORWERROR("textid 为空");
	}


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(TextId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("文字Id %s 不存在", TextId.c_str());
	}


	it->second.TextParam.bRender = true;

	//所有场景中都添加该源

	std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

	for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
	{
		bool bFind = false;
		for (int i = 0; i < Itbegin->second.SouceList.size(); ++i)
		{
			TagInfoSource& Source = Itbegin->second.SouceList[i];
			if (!Source.SourceName.compare(it->second.TextParam.TextID))
			{
				bFind = true;
				break;
			}
		}

		if (!bFind)
		{
			TagInfoSource InfoSource;
			InfoSource.x = it->second.TextParam.x;
			InfoSource.y = it->second.TextParam.y;
			InfoSource.SourceName = it->second.TextParam.TextID;
			InfoSource.Transparent = 0;
			InfoSource.width = it->second.TextParam.RealWidth;
			InfoSource.height = it->second.TextParam.RealHeight;

			Itbegin->second.SouceList.push_back(InfoSource);
		}
	}

	//添加到PVW中去

	VideoArea Area = { 0 };

	Area.left = it->second.TextParam.x * LiveParam.LiveSetting.Width / 100;
	Area.top = it->second.TextParam.y * LiveParam.LiveSetting.Height / 100;
	Area.width = it->second.TextParam.RealWidth * LiveParam.LiveSetting.Width / 100;
	Area.height = it->second.TextParam.RealHeight * LiveParam.LiveSetting.Height / 100;

	FunCall(SLiveAdd2Intance(it->second.InstanceID, InstanceMap["PVW"].InstanceID, &Area, true));


	Json::Value &TextInfo = SencesConfig["textsource"];

	for (UINT i = 0; i < TextInfo.size(); ++i)
	{
		Json::Value &OneText = TextInfo[i];

		if (!OneText["textid"].asString().compare(TextId))
		{
			OneText["brender"] = true;
			break;
		}
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStopText(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	const char *TextId = JValueParam["textid"].asCString();

	if (!TextId)
	{
		BUTEL_THORWERROR("textid 为空");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin ImgId = %s!", __FUNCTION__, TextId);

	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(TextId);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("文字Id %s 不存在", TextId);
	}

	it->second.TextParam.bRender = false;

	//去掉所有场景及PVW中该图片

	std::map<int, TagScenes>::iterator Itbegin = ScenesMap.begin();

	for (Itbegin; Itbegin != ScenesMap.end(); Itbegin++)
	{
		std::vector<TagInfoSource>::iterator itSource = Itbegin->second.SouceList.begin();
		for (itSource; itSource != Itbegin->second.SouceList.end(); itSource++)
		{
			if (!itSource->SourceName.compare(TextId))
			{
				Itbegin->second.SouceList.erase(itSource);
				break;
			}
		}
	}

	//删除PVW中的该图片
	FunCall(SLiveDelStream(InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId));

	Json::Value &TextInfo = SencesConfig["textid"];

	for (UINT i = 0; i < TextInfo.size(); ++i)
	{
		Json::Value &OneText = TextInfo[i];

		if (!OneText["textid"].asString().compare(TextId))
		{
			OneText["brender"] = false;
			break;
		}
	}

	//更新配置文件后上报管理中心

	Json::Value vSencesFile;
	vSencesFile["cloudswitcherid"] = CloudId.c_str();
	vSencesFile["sencefile"] = SencesConfig;

	CHttpNetWork::GetInstance()->SendCommandToManager(vSencesFile, POSTCONFIGFILE);

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetVolum(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	std::string &Name = JValueParam["name"].asString();

	if (Name.empty())
	{
		BUTEL_THORWERROR("name 为空");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin Name = %s!", __FUNCTION__,Name.c_str());


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(Name);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("信源名字 %s 不存在",Name.c_str());
	}
	
	int iVolum = JValueParam["volum"].asInt();

	if (!Name.compare("PGM"))
	{
		SoundAndLocalMinitor Mintor;
		Mintor.bPlayLocal = bPlayPVW;
		Mintor.bPlayLocalLive = bPlayPGM;
		Mintor.fLeft = 1.0f;
		Mintor.fRight = 1.0f;
		Mintor.fQuotietyVolume = 3.0f;
		Mintor.fMix = (float)iVolum / 40;

		PGMVolum = Mintor.fMix;

		FunCall(SLiveSetSoundAndLocalMinitorParam(&Mintor));
	}
	else
	{
		Json::Value jVolum;
		jVolum["Volume"] = (float)iVolum / 40;

		FunCall(SLiveUpdateStream(it->second.InstanceID, it->second.SouceList[0].AudioId, jVolum.toStyledString().c_str()));
	}



	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStartAudioMonitor(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	std::string &Name = JValueParam["name"].asString();

	if (Name.empty())
	{
		BUTEL_THORWERROR("name 为空");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin Name = %s!", __FUNCTION__, Name.c_str());


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(Name);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("信源名字 %s 不存在", Name.c_str());
	}

	if (!Name.compare("PGM"))
	{
		bPlayPGM = true;

		SoundAndLocalMinitor Mintor;
		Mintor.bPlayLocal = bPlayPVW;
		Mintor.bPlayLocalLive = bPlayPGM;
		Mintor.fLeft = 1.0f;
		Mintor.fRight = 1.0f;
		Mintor.fQuotietyVolume = 3.0f;
		Mintor.fMix = PGMVolum;

		FunCall(SLiveSetSoundAndLocalMinitorParam(&Mintor));

		FunCall(SLiveSetAudioNeed(bPlayPVW,bPlayPGM));

	}
	else if (!Name.compare("PVW"))
	{
		bPlayPVW = true;

		SoundAndLocalMinitor Mintor;
		Mintor.bPlayLocal = bPlayPVW;
		Mintor.bPlayLocalLive = bPlayPGM;
		Mintor.fLeft = 1.0f;
		Mintor.fRight = 1.0f;
		Mintor.fQuotietyVolume = 3.0f;
		Mintor.fMix = PGMVolum;

		FunCall(SLiveSetSoundAndLocalMinitorParam(&Mintor));

		FunCall(SLiveSetAudioNeed(bPlayPVW, bPlayPGM));
	}
	else
	{
		bool bOpen = false;
		FunCall(SLiveSetPlayPreAudio(it->second.InstanceID, it->second.SouceList[0].VideoId, &bOpen));

		if (!bOpen)
		{
			//如果没有打开再调用一次
			FunCall(SLiveSetPlayPreAudio(it->second.InstanceID, it->second.SouceList[0].VideoId, &bOpen));
		}
	}


	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudStopAudioMonitor(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	std::string &Name = JValueParam["name"].asString();

	if (Name.empty())
	{
		BUTEL_THORWERROR("name 为空");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin Name = %s!", __FUNCTION__, Name.c_str());


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(Name);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("信源名字 %s 不存在", Name.c_str());
	}

	if (!Name.compare("PGM"))
	{
		bPlayPGM = false;

		SoundAndLocalMinitor Mintor;
		Mintor.bPlayLocal = bPlayPVW;
		Mintor.bPlayLocalLive = bPlayPGM;
		Mintor.fLeft = 1.0f;
		Mintor.fRight = 1.0f;
		Mintor.fQuotietyVolume = 3.0f;
		Mintor.fMix = PGMVolum;

		FunCall(SLiveSetSoundAndLocalMinitorParam(&Mintor));

		FunCall(SLiveSetAudioNeed(bPlayPVW, bPlayPGM));

	}
	else if (!Name.compare("PVW"))
	{
		bPlayPVW = false;

		SoundAndLocalMinitor Mintor;
		Mintor.bPlayLocal = bPlayPVW;
		Mintor.bPlayLocalLive = bPlayPGM;
		Mintor.fLeft = 1.0f;
		Mintor.fRight = 1.0f;
		Mintor.fQuotietyVolume = 3.0f;
		Mintor.fMix = PGMVolum;

		FunCall(SLiveSetSoundAndLocalMinitorParam(&Mintor));

		FunCall(SLiveSetAudioNeed(bPlayPVW, bPlayPGM));
	}
	else
	{
		bool bOpen = false;
		FunCall(SLiveSetPlayPreAudio(it->second.InstanceID, it->second.SouceList[0].VideoId, &bOpen));

		if (bOpen)
		{
			//如果是打开的状态再关闭一次
			FunCall(SLiveSetPlayPreAudio(it->second.InstanceID, it->second.SouceList[0].VideoId, &bOpen));
		}
	}


	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetAudioMix(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	std::string &Name = JValueParam["name"].asString();


	if (Name.empty())
	{
		BUTEL_THORWERROR("name 为空");
	}

	int Mix = JValueParam["mix"].asInt();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin Name = %s!", __FUNCTION__, Name.c_str());


	std::map<std::string, TagInstance>::iterator it = InstanceMap.find(Name);

	if (it == InstanceMap.end())
	{
		BUTEL_THORWERROR("信源名字 %s 不存在", Name.c_str());
	}

	if (it->second.SouceList.size() > 0)
	{
		FunCall(SLiveSetAudioMixAndFollow(it->second.InstanceID, it->second.SouceList[0].AudioId, Mix, 0,true));
	}
	else
	{
		BUTEL_THORWERROR("%s Name = %s it->second.SouceList.size = 0!", __FUNCTION__, Name.c_str());
	}

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CheckCloudIdAndToken(const char* Param,Json::Value &JsonParam, bool bGet)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);
	if (bGet)
	{
		const char *pCloudId = strstr(Param, "cloudswitcherid");

		if (!pCloudId)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("没有找到cloudswitcherid参数");
		}
		pCloudId += strlen("cloudswitcherid");

		char TemBuf[256] = { 0 };

		int index = 0;
		while (*pCloudId != '&' && *pCloudId != ' '&& *pCloudId != '\0')
		{
			if (*pCloudId != '=')
			{
				TemBuf[index++] = *pCloudId;
			}
			pCloudId++;
		}

		if (CloudId.compare(TemBuf))
		{
			BUTEL_THORWERROR("cloudswitcherid 不正确 %s", TemBuf);
		}

		const char *pToken = strstr(Param, "token");

		if (!pToken)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("没有找到token参数");
		}

		pToken += strlen("token");

		ZeroMemory(TemBuf, sizeof TemBuf);

		index = 0;
		while (*pToken != '&' && *pToken != ' ' && *pToken != '\0')
		{
			if (*pToken != '=')
			{
				TemBuf[index++] = *pToken;
			}
			pToken++;
		}

		if (Token.compare(TemBuf))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("Token 校验不正确 %s", TemBuf);
		}

	}
	else
	{
		const char *pJsonStart = strstr(Param, "\r\n\r\n");

		Json::Reader jReader;

		if (!jReader.parse(pJsonStart + 4, JsonParam))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("Json解析失败 %s", pJsonStart + 4);
		}

		std::string &CloudCheckId = JsonParam["cloudswitcherid"].asString();

		if (CloudCheckId.empty())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("cloudswitcherid 为空");
		}

		if (CloudId.compare(CloudCheckId))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("cloudswitcherid 不正确 %s", CloudCheckId.c_str());
		}

		std::string &TokenCheckId = JsonParam["token"].asString();

		if (TokenCheckId.empty())
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("token 为空");
		}

		if (Token.compare(TokenCheckId))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Error!", __FUNCTION__);
			BUTEL_THORWERROR("token 不正确 %s", TokenCheckId.c_str());
		}
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::AddText(TagInstance &Text, Json::Value &JParam)
{
	VideoArea Area = { 0 };
	Area.width = LiveParam.LiveSetting.Width;
	Area.height = LiveParam.LiveSetting.Height;

	Text.TextParam.bRender = JParam["brender"].asBool();
	Text.TextParam.text = JParam["text"].asCString();
	Text.TextParam.x = JParam["positionx"].asDouble();
	Text.TextParam.y = JParam["positiony"].asDouble();
	Text.TextParam.textSize = JParam["textsize"].asInt();
	if (JParam["textcolor"].isNull())
	{
		UINT Color = 16777215;//4278190080;
		Text.TextParam.textColor = Color;
	}
	else
	{
		Text.TextParam.textColor = JParam["textcolor"].asUInt();
	}

	Text.TextParam.textType = JParam["texttype"].asCString();
	if (Text.TextParam.textType.empty())
	{
		Text.TextParam.textType = "Arial";
	}
	Text.TextParam.enableRoll = JParam["enableroll"].asBool();
	Text.TextParam.rollSpeed = JParam["rollspeed"].asInt();
	Text.TextParam.rollType = JParam["rolltype"].asInt();

	char TemBuf[4096] = { 0 };

	sprintf_s(TemBuf, sizeof TemBuf, TextStream, 0xFF << 24 | Text.TextParam.textColor, Text.TextParam.textSize + 2, LiveParam.LiveSetting.Width,
		Text.TextParam.textType.c_str(), Text.TextParam.textSize, Text.TextParam.rollSpeed, Text.TextParam.rollType == 1 ? "false" : "true", Text.TextParam.text.c_str(), Text.TextParam.enableRoll == 1 ? "true" : "false",
		Text.TextParam.enableRoll);

	FunCall(SLiveCreateInstance(&Text.InstanceID, NULL,false,true));

	char TextIdBuf[20] = { 0 };
	sprintf_s(TextIdBuf, sizeof TextIdBuf, "%llu", Text.InstanceID);
	Text.TextParam.TextID = TextIdBuf;
	Text.InstanceName = TextIdBuf;

	__InfoID TextInfoId;

	FunCall(SLiveAddStream(Text.InstanceID, TemBuf, &Area, &TextInfoId.VideoId, NULL));

	FunCall(SLiveSetTopest(Text.InstanceID, TextInfoId.VideoId, true));

	//FunCall(SLiveUpdateStream(Text.InstanceID, TextInfoId.VideoId, TemBuf));

	UINT Width = 0, Height = Text.TextParam.textSize;

	FunCall(SLiveGetStreamSize(Text.InstanceID, TextInfoId.VideoId, &Width, &Height));

	Text.TextParam.RealWidth = (float)Width / LiveParam.LiveSetting.Width * 100;
	Text.TextParam.RealHeight = (float)Height / LiveParam.LiveSetting.Height * 100;

	Text.SouceList.push_back(TextInfoId);

	InstanceMap.insert(std::make_pair(Text.InstanceName, Text));
}

void WebInterFaceProcess::AddImage(TagInstance &Img, Json::Value &JParam,bool bUpdate)
{
	VideoArea Area = { 0 };
	Area.width = LiveParam.LiveSetting.Width;
	Area.height = LiveParam.LiveSetting.Height;

	Img.ImgParam.bRender = JParam["brender"].asBool();
	Img.ImgParam.ImgUrl = JParam["imageurl"].asCString();
	Img.ImgParam.x = JParam["positionx"].asDouble();
	Img.ImgParam.y = JParam["positiony"].asDouble();
	Img.ImgParam.width = JParam["width"].asDouble();
	Img.ImgParam.height = JParam["height"].asDouble();
	Img.ImgParam.transparent = JParam["transparence"].asInt();

	if (!bUpdate)
	{
		FunCall(SLiveCreateInstance(&Img.InstanceID, NULL, false, true));

		char imgId[20] = { 0 };
		sprintf_s(imgId, sizeof imgId, "%llu", Img.InstanceID);
		Img.ImgParam.ImgID = imgId;
		Img.InstanceName = imgId;
	}

	char TemBuf[4096] = { 0 };

	if (!strncmp(Img.ImgParam.ImgUrl.c_str(), "http://", 7) || !strncmp(Img.ImgParam.ImgUrl.c_str(), "https://", 8))
	{
		//如果是网络图片
		MD5 Md5(Img.ImgParam.ImgUrl.c_str(), Img.ImgParam.ImgUrl.length());

		std::string strMd5 = Md5.tostring();
		char* Ext = NULL;
		getFileExtension((char*)Img.ImgParam.ImgUrl.c_str(), &Ext);

		if (!Ext)
		{
			BUTEL_THORWERROR("%s 没有扩展名", Img.ImgParam.ImgUrl.c_str());
		}

		//检查缓存路径下没有该图片
		std::string &FilePath = CHttpNetWork::GetInstance()->GetLocalCachePath();

		FilePath += strMd5;
		FilePath += --Ext;

		Img.ImgParam.RealName = FilePath;

		DWORD attrib = GetFileAttributesA(FilePath.c_str());
		bool bExists = attrib != INVALID_FILE_ATTRIBUTES;

		if (bExists)
		{
			sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - Img.ImgParam.transparent, FilePath.c_str());

			if (bUpdate)
			{
				FunCall(SLiveUpdateStream(Img.InstanceID, Img.SouceList[0].VideoId, TemBuf));

				VideoArea Area = { 0 };

				Area.left = Img.ImgParam.x * LiveParam.LiveSetting.Width / 100;
				Area.top = Img.ImgParam.y * LiveParam.LiveSetting.Height / 100;
				Area.width = Img.ImgParam.width * LiveParam.LiveSetting.Width / 100;
				Area.height = Img.ImgParam.height * LiveParam.LiveSetting.Height / 100;

				if (Img.ImgParam.bRender)
				{
					FunCall(SLiveUpdateStreamPosition(InstanceMap["PVW"].InstanceID, Img.SouceList[0].VideoId, &Area));
				}

				return;
			}

			__InfoID ImgInfoId;
			//这个是异步的不能马上SLiveGetStreamSize
			FunCall(SLiveAddStream(Img.InstanceID, TemBuf, &Area, &ImgInfoId.VideoId, NULL));

			FunCall(SLiveSetTopest(Img.InstanceID, ImgInfoId.VideoId, true));

			Img.SouceList.push_back(ImgInfoId);

			InstanceMap.insert(std::make_pair(Img.InstanceName, Img));

		}
		else
		{
			//加载默认图片
			if (!bUpdate)
			{
				sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - Img.ImgParam.transparent, "./img/DefaultImage.png");

				__InfoID ImgInfoId;
				//这个是异步的不能马上SLiveGetStreamSize
				FunCall(SLiveAddStream(Img.InstanceID, TemBuf, &Area, &ImgInfoId.VideoId, NULL));

				FunCall(SLiveSetTopest(Img.InstanceID, ImgInfoId.VideoId, true));

				Img.SouceList.push_back(ImgInfoId);

				InstanceMap.insert(std::make_pair(Img.InstanceName, Img));
			}

			//不存在图片就要去下载

			CHttpNetWork::GetInstance()->DoAsnycDownLoadFile(Img.ImgParam.ImgUrl, FilePath, DownLoadComplete, this, Img.InstanceName);
		}
		
		
	}
	else
	{
		sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - Img.ImgParam.transparent, Img.ImgParam.ImgUrl.c_str());

		if (bUpdate)
		{
			FunCall(SLiveUpdateStream(Img.InstanceID, Img.SouceList[0].VideoId, TemBuf));

			VideoArea Area = { 0 };

			Area.left = Img.ImgParam.x * LiveParam.LiveSetting.Width / 100;
			Area.top = Img.ImgParam.y * LiveParam.LiveSetting.Height / 100;
			Area.width = Img.ImgParam.width * LiveParam.LiveSetting.Width / 100;
			Area.height = Img.ImgParam.height * LiveParam.LiveSetting.Height / 100;

			if (Img.ImgParam.bRender)
			{
				FunCall(SLiveUpdateStreamPosition(InstanceMap["PVW"].InstanceID, Img.SouceList[0].VideoId, &Area));
			}

			return;
		}

		__InfoID ImgInfoId;
		//这个是异步的不能马上SLiveGetStreamSize
		FunCall(SLiveAddStream(Img.InstanceID, TemBuf, &Area, &ImgInfoId.VideoId, NULL));

		FunCall(SLiveSetTopest(Img.InstanceID, ImgInfoId.VideoId, true));

		Img.SouceList.push_back(ImgInfoId);

		InstanceMap.insert(std::make_pair(Img.InstanceName, Img));
	}

	
}

void WebInterFaceProcess::DownLoadComplete(void *Context, const std::string& name, bool bSucess)
{
	WebInterFaceProcess *_this = (WebInterFaceProcess*)Context;

	std::map<std::string, TagInstance>::iterator it = _this->InstanceMap.find(name);

	if (it == _this->InstanceMap.end())
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s 图片Id %s 不存在",__FUNCTION__, name.c_str());
		return;
	}

	char TemBuf[4096] = { 0 };

	if (bSucess)
	{
		sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - it->second.ImgParam.transparent, it->second.ImgParam.RealName.c_str());
	}
	else
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s 下载图片失败 url = %s,加载默认图片", __FUNCTION__,it->second.ImgParam.ImgUrl.c_str());

		sprintf_s(TemBuf, sizeof TemBuf, ImgStream, 100 - it->second.ImgParam.transparent, "./img/DefaultImage.png");
	}

	FunCall(SLiveUpdateStream(it->second.InstanceID, it->second.SouceList[0].VideoId, TemBuf));           

	VideoArea Area = { 0 };

	Area.left = it->second.ImgParam.x * _this->LiveParam.LiveSetting.Width / 100;
	Area.top = it->second.ImgParam.y * _this->LiveParam.LiveSetting.Height / 100;
	Area.width = it->second.ImgParam.width * _this->LiveParam.LiveSetting.Width / 100;
	Area.height = it->second.ImgParam.height * _this->LiveParam.LiveSetting.Height / 100;

	if (it->second.ImgParam.bRender)
	{
		FunCall(SLiveUpdateStreamPosition(_this->InstanceMap["PVW"].InstanceID, it->second.SouceList[0].VideoId, &Area));
	}
}

void WebInterFaceProcess::CloudEmergencySwitch(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);


	if (-1 == BackupIndex && InputName.empty())
	{
		BUTEL_THORWERROR("还未设置备用场景");
	}

	if (BackupIndex > 0)
	{
		std::map<int, TagScenes>::iterator it = ScenesMap.find(BackupIndex);

		if (it != ScenesMap.end())
		{
			TagInstance &PVW = InstanceMap["PVW"];
			FunCall(SLiveClearIntances(PVW.InstanceID, false));
			FunCall(SLiveSetSenceToBackUp());

			for (int i = 0; i < it->second.SouceList.size(); i++)
			{
				VideoArea Area = { 0 };
				TagInfoSource &Source = it->second.SouceList[i];

				Area.left = Source.x * LiveParam.LiveSetting.Width / 100;
				Area.top = Source.y * LiveParam.LiveSetting.Height / 100;
				Area.width = Source.width * LiveParam.LiveSetting.Width / 100;
				Area.height = Source.height * LiveParam.LiveSetting.Height / 100;



				TagInstance &Instance = InstanceMap[Source.SourceName];
				FunCall(SLiveAdd2Intance(Instance.InstanceID, PVW.InstanceID, &Area, true));

			}

			FunCall(SLiveSwitchInstance(PVW.InstanceID, InstanceMap["PGM"].InstanceID, Cut));
		}
		else
		{
			BUTEL_THORWERROR("该场景索引 %d 不存在", BackupIndex);
		}
	}
	else
	{
		std::map<std::string, TagInstance>::iterator it = InstanceMap.find(InputName);

		if (it == InstanceMap.end())
		{
			BUTEL_THORWERROR("该信源 %s 不存在", InputName.c_str());
		}

		TagInstance &PVW = InstanceMap["PVW"];
		FunCall(SLiveClearIntances(PVW.InstanceID, false));
		FunCall(SLiveSetSenceToBackUp());

		VideoArea Area = { 0 };

		Area.width = LiveParam.LiveSetting.Width;
		Area.height = LiveParam.LiveSetting.Height;

		FunCall(SLiveAdd2Intance(it->second.InstanceID, PVW.InstanceID, &Area, true));
		FunCall(SLiveSwitchInstance(PVW.InstanceID, InstanceMap["PGM"].InstanceID, Cut));
	}

	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}

void WebInterFaceProcess::CloudSetAudioFollow(CRequse &Req, CRespond &Res)
{
	if (!bHasInit)
	{
		BUTEL_THORWERROR("请先调用initswitcher进行初始化");
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s CheckCloudIdAndToken", __FUNCTION__);
	Json::Value JValueParam;
	CheckCloudIdAndToken(Req.GetParam(), JValueParam);

	int AutoFollow = JValueParam["autofollow"].asInt();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin! AutoFollow = %d", __FUNCTION__, AutoFollow);

	std::map<std::string, TagInstance>::iterator itBegin = InstanceMap.begin();
	std::map<std::string, TagInstance>::iterator itEnd = InstanceMap.end();

	for (; itBegin != itEnd; itBegin++)
	{
		if (itBegin->first.compare("PVW") && itBegin->first.compare("PGM"))
		{
			if (itBegin->second.SouceList.size() > 0)
			{
				FunCall(SLiveSetAudioMixAndFollow(itBegin->second.InstanceID, itBegin->second.SouceList[0].AudioId, 0, AutoFollow, false));
			}
		}
	}


	JValueParam.clear();

	Res.SetResType(RES_OK);

	JValueParam["state"]["rc"] = 0;
	JValueParam["state"]["msg"] = "成功";
	Res.SetRespond(JValueParam);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
}


