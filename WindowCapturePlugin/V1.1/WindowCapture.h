#ifndef WINDOWCAPTURE_H
#define WINDOWCAPTURE_H
#include "BaseAfx.h"
#include <dwmapi.h>
#include "BaseVideo.h"
#include "resource.h"

#define NUM_CAPTURE_TEXTURES 2

class DesktopImageSource : public IBaseVideo
{
	Texture *renderTextures[NUM_CAPTURE_TEXTURES];
	Texture *lastRendered;

	UINT     captureType;
	String   strWindow, strWindowClass;
	BOOL     bClientCapture, bCaptureMouse, bCaptureLayered, bRegionCapture;
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
	int cx, cy;

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
	bool     bHasPreprocess;

	OutputDuplicator *duplicator;
	D3DAPI  *D3DRender;

public:
	DesktopImageSource();

	~DesktopImageSource();

	bool Init(Value &JsonParam);

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

	virtual void SetHasPreProcess(bool bHasPre);;
	virtual bool GetHasPreProcess() const;

	virtual void SetD3DRender(D3DAPI *D3DRender);
};

class WindowCaptureSource : public DesktopImageSource
{
	DYNIC_DECLARE()
};

class MonitorCaptureSource : public DesktopImageSource
{
	DYNIC_DECLARE()
};


#endif