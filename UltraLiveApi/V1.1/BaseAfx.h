#if !defined(BASEAFX_H)
#define BASEAFX_H

#include "D3DRender.h"
#include <gdiplus.h>

#include "SLiveInterface.h"
#include "json/json.h"
#include "Error.h"
#include "LogDeliver.h"
#include "VolumeControl.h"
#include "ColorControl.h"
#include "VolumeMeter.h"
#include "Deinterlacer.h"
#include "D3DResize.h"


using namespace Json;

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif // !LOG_RTSPSERV

typedef  bool(STDCALL *CONFINGFUN)(Json::Value &element, bool bCreating);
typedef  void (*DataCallBack)(void *Context,CSampleData *);
class IBaseAudio;
class IBaseVideo;

struct MonitorInfo
{
	inline MonitorInfo() { zero(this, sizeof(MonitorInfo)); }

	inline MonitorInfo(HMONITOR hMonitor, RECT *lpRect)
	{
		this->hMonitor = hMonitor;
		mcpy(&this->rect, lpRect, sizeof(rect));
	}

	HMONITOR hMonitor;
	RECT rect;
	float rotationDegrees;
};

struct DeviceOutputData
{
	String strDevice;
	List<MonitorInfo> monitors;
	StringList monitorNameList;

	inline void ClearData()
	{
		strDevice.Clear();
		monitors.Clear();
		monitorNameList.Clear();
	}
};

struct DeviceOutputs
{
	List<DeviceOutputData> devices;

	inline ~DeviceOutputs()
	{
		ClearData();
	}

	inline void ClearData()
	{
		for (UINT i = 0; i < devices.Num(); i++)
			devices[i].ClearData();
		devices.Clear();
	}
};

class API_EXPORT CPObject
{
public:
	CPObject(){};
	virtual ~CPObject(){};

	virtual const char *GainClassName() = 0;
};

typedef struct CREATOBJ
{
	const char *Class_Name;
	const char *Desc;
	const char *Version;
	CREATEOBJ fp;
	static int TotalCount;
}CreateOBJ,*pCreateOBJ;

typedef struct CREATOBJFILTER
{
	const char *Class_Name;
	const char *Desc;
	bool IsShader;
	CREATEOBJ fp;
	static int TotalCount;
}CreateOBJFilter, *pCreateOBJFilter;


typedef struct __DataCallBack
{
	void *Context;
	DataCallBack CallBack;
	__DataCallBack()
	{
		CallBack = NULL;
		Context = NULL;
	}
}__DataCallBack;

typedef struct ShardVideo
{
	std::shared_ptr<IBaseVideo> VideoStream;
	std::shared_ptr<Value>  Config;

	ShardVideo(){}

	ShardVideo(const ShardVideo& Video)
	{
		if (this != &Video)
		{
			VideoStream = Video.VideoStream;
			Config = Video.Config;
		}
	}

	ShardVideo &operator = (const ShardVideo& Video)
	{
		if (this != &Video)
		{
			VideoStream = Video.VideoStream;
			Config = Video.Config;
		}
		return *this;
	}

	~ShardVideo()
	{
		if (VideoStream)
			VideoStream.reset();

		if (Config)
			Config.reset();
	}

}ShardVideo;

typedef struct API_EXPORT SharedDevice : public CPObject
{
	String DeviceName;
	String DeviceID;
	int    iWidth;
	int    iHeigth;
	int    iType;
	void   *Server;

	static std::vector<ShardVideo> VideoList;
	static HANDLE HLock;

	int RegisterSharedDevice(DataCallBack Cb);

	const char *GainClassName();

	void UnRegisterSharedDevice();

	void Clear();

}SharedDevice;

CPObject* CreatStreamObject(const char *Classname);
 int GetSourceObjectCount();
 const CreateOBJ* GetArrayObject();
 void GetPluginsName(Value &NameList);

 CONFINGFUN GetConfigFunc(const char *ClassName);

 CPObject* CreatStreamObjectFilter(const char *Classname);
 int GetSourceObjectCountFilter();
 const CreateOBJFilter* GetArrayObjectFilter();

 enum ObjectType
 {
	 Video = 0,
	 Aduio
 };


 enum TitleType
 {
	 TitleType_Setting,
	 TitleType_Min,
	 TitleType_Max,
	 TitleType_Close
 };

 typedef struct TitlePic
 {
	 RECT Rect;
	 void *Img[4];
	 TitleType Type;
	 bool IsPointInRect(const POINT & Point)
	 {
		 if (Point.x > Rect.left && Point.x < Rect.right && Point.y < Rect.bottom && Point.y > Rect.top)
		 {
			 return true;
		 }
		 return false;
	 }
 }TitlePicInfo;

 struct LocaleStringItem
 {
	 String      lookup;
	 String      strValue;
 };

 struct StringLookupNode
 {
	 String str;
	 List<StringLookupNode*> subNodes;
	 LocaleStringItem *leaf;

	 inline ~StringLookupNode()
	 {
		 for (unsigned int i = 0; i < subNodes.Num(); i++)
			 delete subNodes[i];
	 }

	 inline StringLookupNode* FindSubNodeByChar(TCHAR ch)
	 {
		 for (unsigned int i = 0; i < subNodes.Num(); i++)
		 {
			 StringLookupNode *node = subNodes[i];
			 if (node->str.IsValid() && node->str[0] == ch)
				 return subNodes[i];
		 }

		 return NULL;
	 }

	 inline UINT FindSubNodeID(CTSTR lpLookup)
	 {
		 for (unsigned int i = 0; i < subNodes.Num(); i++)
		 {
			 StringLookupNode *node = subNodes[i];
			 if (scmpi_n(node->str, lpLookup, node->str.Length()) == 0)
				 return i;
		 }

		 return INVALID;
	 }

	 inline StringLookupNode* FindSubNode(CTSTR lpLookup)
	 {
		 for (unsigned int i = 0; i < subNodes.Num(); i++)
		 {
			 StringLookupNode *node = subNodes[i];
			 if (scmpi_n(node->str, lpLookup, node->str.Length()) == 0)
				 return subNodes[i];
		 }

		 return NULL;
	 }
 };

 struct LocaleStringCache : public List<LocaleStringItem*>
 {
	 LocaleStringCache()
	 {
		 num = 0;
	 }
	 inline ~LocaleStringCache()
	 {
		 for (unsigned int i = 0; i < Num(); i++)
			 delete array[i];
	 }

	 inline void Clear()
	 {
		 for (unsigned int i = 0; i < Num(); i++)
			 delete array[i];
		 List<LocaleStringItem*>::Clear();
	 }

	 inline void Remove(UINT i)
	 {
		 if (i < Num())
		 {
			 delete array[i];
			 List<LocaleStringItem*>::Remove(i);
		 }
	 }

	 inline void SetSize(UINT newSize)
	 {
		 if (newSize < Num())
		 {
			 for (unsigned int i = newSize; i < Num(); i++)
				 delete array[i];
		 }

		 List<LocaleStringItem*>::SetSize(newSize);
	 }
 };

 class API_EXPORT LocaleStringLookup
 {
	 StringLookupNode *top;
	 LocaleStringCache cache;

	 void AddLookup(CTSTR lookupVal, LocaleStringItem *item, StringLookupNode *node = NULL);
	 void RemoveLookup(CTSTR lookupVal, StringLookupNode *node = NULL);

	 StringLookupNode* FindNode(CTSTR lookupVal, StringLookupNode *node = NULL) const;

 public:
	 LocaleStringLookup();
	 ~LocaleStringLookup();

	 BOOL LoadStringFile(CTSTR lpFile, bool bClear = false);

	 BOOL HasLookup(CTSTR lookupVal) const;

	 void AddLookupString(CTSTR lookupVal, CTSTR lpVal);
	 void RemoveLookupString(CTSTR lookupVal);

	 CTSTR LookupString(CTSTR lookupVal);

	 const LocaleStringCache& GetCache() const;
 };


#ifndef AGENTLOCALPATH
#define AGENTLOCALPATH L"./Plugins/AgentPlugin/Local/zh.txt"
#endif

#ifndef AUDIOLOCALPATH
#define AUDIOLOCALPATH L"./Plugins/AuidoPlugin/locale/zh.txt"
#endif

#ifndef DSHOWLOCALPATH
#define DSHOWLOCALPATH L"./Plugins/DShowPlugin/local/zh.txt"
#endif

#ifndef PIPECAPTURELOCALPATH
#define PIPECAPTURELOCALPATH L"./Plugins/PipeVideoPlugin/locale/zh.txt"
#endif

#ifndef VIDEOLIVELOCALPATH
#define VIDEOLIVELOCALPATH L"./Plugins/VideoSourcePlugin/Local/zh.txt"
#endif

#ifndef VIDEOSOURCELOCALPATH
#define VIDEOSOURCELOCALPATH L"./Plugins/VideoSourcePlugin/Local/zh.txt"
#endif

#ifndef WINDOWCAPTURELOCALPATH
#define WINDOWCAPTURELOCALPATH L"./Plugins/WindowCapturePlugin/Local/zh.txt"
#endif

API_EXPORT QWORD GetQPCNS();
API_EXPORT QWORD GetQPC100NS();
API_EXPORT QWORD GetQPCMS();
API_EXPORT bool Sleep2NS(QWORD qwNSTime);
API_EXPORT std::wstring Asic2WChar(const std::string& str);
API_EXPORT std::string WcharToAnsi(const std::wstring& strSrc);
API_EXPORT int GetNTDLLVersion();
API_EXPORT void DrawFrame(HWND hwnd, int Type, bool IsOnlyColse);
API_EXPORT std::vector<TitlePicInfo>& GetListTitle();
API_EXPORT String GetEditText(HWND hwndEdit);
API_EXPORT int AddConfigFun2Map(const char *ClassName, CONFINGFUN Fp);
API_EXPORT HWND GetMainWindow();
API_EXPORT void PackPlanar(LPBYTE convertBuffer, LPBYTE lpPlanar, UINT renderCX, UINT renderCY, UINT pitch, UINT startY, UINT endY, UINT linePitch, UINT lineShift);
API_EXPORT int BLiveMessageBox(HWND hwnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT flags);
API_EXPORT String GetLBText(HWND hwndList, UINT id = LB_ERR);
API_EXPORT String GetLVText(HWND hwndList, UINT id);
API_EXPORT String GetCBText(HWND hwndCombo, UINT id = CB_ERR);
API_EXPORT INT_PTR BLiveDialogBox(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam = 0);
API_EXPORT void GetBaseSize(UINT &Width, UINT &Height);
API_EXPORT int GetMaxFPS();
API_EXPORT bool CurrentDeviceExists(Value &data);
API_EXPORT bool GetNameList(Value &data);
API_EXPORT bool ScanSourceElmentByClassName(const char *ClassName, List<Value>& List);
API_EXPORT void ReNameStreamSec(__int64 IntanceID,__int64 StreamID, const char *NewName);
API_EXPORT BOOL CALLBACK MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, List<MonitorInfo> &monitors);
API_EXPORT UINT NumMonitors();
API_EXPORT const MonitorInfo& GetMonitor(UINT id);
API_EXPORT LPBYTE GetCursorData(HICON hIcon, ICONINFO &ii, UINT &width, UINT &height);
API_EXPORT void GetDisplayDevices(DeviceOutputs &deviceList);
API_EXPORT String GetDirectorMonitorDevices();
API_EXPORT String GetSecMonitorDevices();
API_EXPORT void LocalizeWindow(HWND hwnd, LocaleStringLookup *lookup);
API_EXPORT void RemoveLiveInstanceAudio(IBaseVideo *BaseVideo,bool bMustDel = false,IBaseVideo *NewGobal = NULL);
API_EXPORT void AddLiveInstanceAudio(IBaseVideo *Video,IBaseVideo *Agent);
API_EXPORT UINT GetOutputAudioSampleRate();
API_EXPORT void* GetNickNameCallBack();
API_EXPORT void EnterLiveVideoSection();
API_EXPORT void LeaveLiveVideoSection();
API_EXPORT void ChangeLiveInstanceSameAsLocalInstance(IBaseVideo *Video);
API_EXPORT D3DAPI *GetD3DRender();
API_EXPORT bool IsSupportRecord(const TCHAR *DisplayName);
#endif
