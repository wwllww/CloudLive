#include <locale.h> 
#include "BaseAfx.h"
#include <dwmapi.h>
#include <TlHelp32.h>  
#include "BaseVideo.h"
#include "resource.h"

#define CAPTURE_TEXTURES_NUM 2

class ProcTopWindowSource;

class ProcTopWindowRealSource
{
	Texture *renderTextures[CAPTURE_TEXTURES_NUM];
	Texture *lastRendered;

	UINT     captureType;
	String   strWindow, strWindowClass;
	BOOL     bClientCapture, bCaptureMouse, bCaptureLayered;
	HWND     hwndFoundWindow;

	Shader   *colorKeyShader, *alphaIgnoreShader;

	int      width, height;
	RECT     captureRect;
	UINT     frameTime;
	int      curCaptureTexture;
	Value    data;

	UINT     warningID;

	bool     bUseColorKey, bUsePointFiltering;
	DWORD    keyColor;
	UINT     keySimilarity, keyBlend;

	UINT     opacity;
	int      gamma;

	float    rotateDegrees;

	//-------------------------
	// stuff for compatibility mode
	bool     bCompatibilityMode;
	HDC      hdcCompatible;
	HBITMAP  hbmpCompatible, hbmpOld;
	BYTE     *captureBits;

	//-------------------------
	// win 8 capture stuff
	Texture  *cursorTexture;
	int      xHotspot, yHotspot;
	UINT     monitor;
	UINT     deviceOutputID;
	float    retryAcquire;
	bool     bWindows8MonitorCapture;
	MonitorInfo monitorData;
	bool     bMouseCaptured;
	POINT    cursorPos;
	HCURSOR  hCurrentCursor;
	bool     bInInit;

	OutputDuplicator *duplicator;

protected:
	friend class ProcTopWindowSource;

public:
	ProcTopWindowRealSource(Value &data);

	~ProcTopWindowRealSource();

	void BeginScene();

	void EndScene();

	//----------------------------------------------------------------------------------
	// TODO  - have win8 monitor capture behave even when using it as a global source

	void GlobalSourceEnterScene();

	void GlobalSourceLeaveScene();

	//----------------------------------------------------------------------------------

	void Tick(float fSeconds);

	void PreprocessWindows8MonitorCapture();

	HDC GetCurrentHDC();

	void ReleaseCurrentHDC(HDC hDC);

	void EndPreprocess();

	void Preprocess();

	void Render(const Vect2 &pos, const Vect2 &size, Texture *tex, bool bScaleFull, bool bIsLiveC);

	Vect2 GetSize() const;


	void GetWindowList(StringList &classList);


	bool ExistWindow(String& winName);

	void UpdateSettings(Value &data);

	void SetInt(CTSTR lpName, int iVal);
};


class ProcTopWindowSource : public IBaseVideo
{
	DYNIC_DECLARE()
	ProcTopWindowRealSource* source;
	HANDLE hMutex;
	Value data;
	UINT frameTime;
	String WindowName;
	String WindowClass;
	bool bStop;
	bool bHasProprocess;

public:
	ProcTopWindowSource();

	~ProcTopWindowSource();

	bool Init(Value &JsonParam);

	void UpdateSettings(Value &JsonParam);

	void BeginScene();

	void EndScene();

	//----------------------------------------------------------------------------------
	// TODO  - have win8 monitor capture behave even when using it as a global source

	void GlobalSourceEnterScene();

	void GlobalSourceLeaveScene();

	//----------------------------------------------------------------------------------

	void Tick(float fSeconds);

	void Preprocess();

	void Render(const Vect2 &pos, const Vect2 &size, Texture *tex, bool bScaleFull, bool bIsLiveC);

	Vect2 GetSize() const;

	void GetTargetWindow(String winClass, String strWindow);

	void GetProcTopWindowInfoByPID(DWORD pid);

	void CheckProcTopWindowChange(bool rebegin = false);

	void SetInt(CTSTR lpName, int iVal);
	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
};


