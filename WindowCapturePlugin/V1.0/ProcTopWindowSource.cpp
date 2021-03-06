#include "ProcTopWindowSource.h"
#include <sstream>

WNDPROC RefreshBtnProc1 = NULL;
WNDPROC DefBtnProc1 = NULL;
WNDPROC YesBtnProc1 = NULL;
WNDPROC NoBtnProc1 = NULL;
WNDPROC SelectBtnProc1 = NULL;
static HWND Hwnd = NULL;
extern HINSTANCE hinstMain;
extern LocaleStringLookup *pluginLocale;
extern int SetSliderText(HWND hwndParent, int controlSlider, int controlText);

IMPLEMENT_DYNIC(ProcTopWindowSource,"获取进程顶层窗口","V1.0.0.1")

ProcTopWindowSource::ProcTopWindowSource()
{
	bStop = false;
	hMutex = OSCreateMutex();
	source = NULL;
	bHasProprocess = false;

}

ProcTopWindowSource::~ProcTopWindowSource()
{
	OSEnterMutex(hMutex);
	if (source)
	{
		delete source;
		source = NULL;
	}
	OSLeaveMutex(hMutex);

	if (hMutex)
	{
		OSCloseMutex(hMutex);
		hMutex = NULL;
	}
}

bool ProcTopWindowSource::Init(Value &JsonParam)
{
	UpdateSettings(JsonParam);
	return true;

}

void ProcTopWindowSource::UpdateSettings(Value &JsonParam)
{
	this->data = JsonParam;
	CheckProcTopWindowChange();
}

void ProcTopWindowSource::BeginScene()
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->BeginScene();
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::EndScene()
{
	OSEnterMutex(hMutex);
	if (source)
	{
		bStop = true;
		Log(TEXT("1111"));
		source->EndScene();
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::GlobalSourceEnterScene()
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->GlobalSourceEnterScene();
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::GlobalSourceLeaveScene()
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->GlobalSourceLeaveScene();
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::Tick(float fSeconds)
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->Tick(fSeconds);
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::Preprocess()
{
	CheckProcTopWindowChange(true);

	OSEnterMutex(hMutex);
	if (source)
	{
		source->Preprocess();
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::Render(const Vect2 &pos, const Vect2 &size, Texture *tex, bool bScaleFull, bool bIsLiveC)
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->Render(pos, size, tex, bScaleFull, bIsLiveC);
	}
	OSLeaveMutex(hMutex);
}

Vect2 ProcTopWindowSource::GetSize() const
{
	Vect2 size(0, 0);
	OSEnterMutex(hMutex);
	if (source)
	{
		size = source->GetSize();
	}
	OSLeaveMutex(hMutex);
	return size;
}

void ProcTopWindowSource::GetTargetWindow(String winClass, String strWindow)
{
	HWND hwndTarget = FindWindow(winClass, strWindow);
	if (!hwndTarget)
	{
		hwndTarget = FindWindow(winClass, NULL);
	}

	if (!hwndTarget)
		return;

	//------------------------------------------

	BOOL bInnerWindow = data["innerWindow"].asInt() == 0;

	RECT rc;
	if (bInnerWindow)
		GetClientRect(hwndTarget, &rc);
	else
	{
		GetWindowRect(hwndTarget, &rc);

		rc.bottom -= rc.top;
		rc.right -= rc.left;
		rc.left = rc.top = 0;
	}

	//------------------------------------------

	rc.bottom -= rc.top;
	rc.right -= rc.left;

	if (rc.right < 16)
		rc.right = 16;
	if (rc.bottom < 16)
		rc.bottom = 16;

	data["captureX"] = rc.left;
	data["captureY"] = rc.top;
	data["captureCX"] = rc.right;
	data["captureCY"] = rc.bottom;
}

void ProcTopWindowSource::GetProcTopWindowInfoByPID(DWORD pid)
{
	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	do
	{
		if (!IsWindow(hwndCurrent))
		{
			return;
		}
		if (IsWindowVisible(hwndCurrent) && !IsIconic(hwndCurrent))
		{
			DWORD processID;
			GetWindowThreadProcessId(hwndCurrent, &processID);
			if (processID == GetCurrentProcessId())
				continue;

			RECT clientRect;
			GetClientRect(hwndCurrent, &clientRect);

			String strWindowName;
			strWindowName.SetLength(GetWindowTextLength(hwndCurrent));
			GetWindowText(hwndCurrent, strWindowName, strWindowName.Length() + 1);

			DWORD exStyles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_EXSTYLE);
			DWORD styles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_STYLE);

			if (strWindowName.IsValid() && sstri(strWindowName, L"battlefield") != nullptr)
				exStyles &= ~WS_EX_TOOLWINDOW;

			if ((exStyles & WS_EX_TOOLWINDOW) == 0 && (styles & WS_CHILD) == 0 &&
				clientRect.bottom != 0 && clientRect.right != 0 /*&& hwndParent == NULL*/)
			{
				if (processID == pid)
				{
					//-----------------------------------------------------
					String strClassName;
					strClassName.SetLength(256);
					GetClassName(hwndCurrent, strClassName.Array(), 255);
					strClassName.SetLength(slen(strClassName));
					data["windowClass"] = WcharToAnsi(strClassName.Array()).c_str();
					//----------------------------------------------------
					TCHAR fileName[MAX_PATH + 1];
					scpy(fileName, TEXT("unknown"));

					HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
					if (hProcess)
					{
						DWORD dwSize = MAX_PATH;
						QueryFullProcessImageName(hProcess, 0, fileName, &dwSize);
						CloseHandle(hProcess);
					}

					String strFileName = fileName;
					strFileName.FindReplace(TEXT("\\"), TEXT("/"));

					String strText;
					strText << TEXT("[") << GetPathFileName(strFileName) << TEXT("]: ") << strWindowName;

					if (strWindowName.IsEmpty())
						strWindowName = GetPathFileName(strFileName);

					data["window"] = WcharToAnsi(strWindowName.Array()).c_str();

					//---------------------------------------------------------------
					GetTargetWindow(strClassName, strWindowName);

					break;
				}
			}
		}
	} while ((hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT)) && !bStop);
}

void ProcTopWindowSource::CheckProcTopWindowChange(bool rebegin /*= false*/)
{
	OSEnterMutex(hMutex);

	DWORD pid = data["ProcID"].asInt(); //获取程序ID
	GetProcTopWindowInfoByPID(pid);    //填写程序信息

	String strNewWindow = L"";
	if(!data["window"].isNull())
		strNewWindow = Asic2WChar(data["window"].asString().c_str()).c_str();
	String strNewWindowClass = L"";

	if (!data["windowClass"].isNull())
		strNewWindowClass = Asic2WChar(data["windowClass"].asString().c_str()).c_str();

	if ((strNewWindow != WindowName || strNewWindowClass != WindowClass) && (source != NULL))
	{
		delete source;
		source = NULL;

		WindowName = strNewWindow;
		WindowClass = strNewWindowClass;
	}

	if (source == NULL)
	{
		source = new ProcTopWindowRealSource(data);
	}
	else
	{
		source->UpdateSettings(data);
	}

	if (source && rebegin)
	{
		source->BeginScene();
	}

	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::SetInt(CTSTR lpName, int iVal)
{
	OSEnterMutex(hMutex);
	if (source)
	{
		source->SetInt(lpName, iVal);
	}
	OSLeaveMutex(hMutex);
}

void ProcTopWindowSource::SetHasPreProcess(bool bHasPre)
{
	bHasProprocess = bHasPre;
}

bool ProcTopWindowSource::GetHasPreProcess() const
{
	return bHasProprocess;
}




ProcTopWindowRealSource::ProcTopWindowRealSource(Value& data)
{
	duplicator = NULL;

	bInInit = true;

	UpdateSettings(data);

	bInInit = false;

	curCaptureTexture = 0;
	this->frameTime = frameTime;

	colorKeyShader = CreatePixelShaderFromFile(TEXT("shaders\\ColorKey_RGB.pShader"));
	alphaIgnoreShader = CreatePixelShaderFromFile(TEXT("shaders\\AlphaIgnore.pShader"));

	if (captureType == 0)
		Log(TEXT("Using Monitor Capture"));
	else if (captureType == 1)
		Log(TEXT("Using Window Capture"));
}

ProcTopWindowRealSource::~ProcTopWindowRealSource()
{
	for (int i = 0; i < CAPTURE_TEXTURES_NUM; i++)
		delete renderTextures[i];

	delete duplicator;
	delete cursorTexture;
	delete alphaIgnoreShader;
	delete colorKeyShader;

	if (bCompatibilityMode)
	{
		SelectObject(hdcCompatible, hbmpOld);
		DeleteDC(hdcCompatible);
		DeleteObject(hbmpCompatible);
	}
}

void ProcTopWindowRealSource::BeginScene()
{
	if (bWindows8MonitorCapture && !duplicator)
		duplicator = CreateOutputDuplicator(deviceOutputID);
}

void ProcTopWindowRealSource::EndScene()
{
	if (bWindows8MonitorCapture && duplicator)
	{
		delete duplicator;
		duplicator = NULL;
	}
}

void ProcTopWindowRealSource::GlobalSourceEnterScene()
{

}

void ProcTopWindowRealSource::GlobalSourceLeaveScene()
{

}

void ProcTopWindowRealSource::Tick(float fSeconds)
{
	if (bWindows8MonitorCapture && !duplicator)
	{
		retryAcquire += fSeconds;
		if (retryAcquire > 1.0f)
		{
			retryAcquire = 0.0f;

			lastRendered = NULL;
			duplicator = CreateOutputDuplicator(deviceOutputID);
		}
	}
}

void ProcTopWindowRealSource::PreprocessWindows8MonitorCapture()
{
	//----------------------------------------------------------
	// capture monitor

	if (duplicator)
	{
		switch (duplicator->AcquireNextFrame(0))
		{
		case DuplicatorInfo_Lost:
		{
									delete duplicator;
									lastRendered = NULL;
									duplicator = CreateOutputDuplicator(deviceOutputID);
									return;
		}

		case DuplicatorInfo_Error:
			delete duplicator;
			duplicator = NULL;
			lastRendered = NULL;
			return;

		case DuplicatorInfo_Timeout:
			return;
		}

		lastRendered = duplicator->GetCopyTexture();
	}

	//----------------------------------------------------------
	// capture mouse

	bMouseCaptured = false;
	if (bCaptureMouse)
	{
		CURSORINFO ci;
		zero(&ci, sizeof(ci));
		ci.cbSize = sizeof(ci);

		if (GetCursorInfo(&ci))
		{
			mcpy(&cursorPos, &ci.ptScreenPos, sizeof(cursorPos));

			if (ci.flags & CURSOR_SHOWING)
			{
				if (ci.hCursor == hCurrentCursor)
					bMouseCaptured = true;
				else
				{
					HICON hIcon = CopyIcon(ci.hCursor);
					hCurrentCursor = ci.hCursor;

					delete cursorTexture;
					cursorTexture = NULL;

					if (hIcon)
					{
						ICONINFO ii;
						if (GetIconInfo(hIcon, &ii))
						{
							xHotspot = int(ii.xHotspot);
							yHotspot = int(ii.yHotspot);

							UINT width, height;
							LPBYTE lpData = GetCursorData(hIcon, ii, width, height);
							if (lpData && width && height)
							{
								cursorTexture = CreateTexture(width, height, GS_BGRA, lpData, FALSE,TRUE);
								if (cursorTexture)
									bMouseCaptured = true;

								Free(lpData);
							}

							DeleteObject(ii.hbmColor);
							DeleteObject(ii.hbmMask);
						}

						DestroyIcon(hIcon);
					}
				}
			}
		}
	}
}

HDC ProcTopWindowRealSource::GetCurrentHDC()
{
	HDC hDC = NULL;

	Texture *captureTexture = renderTextures[curCaptureTexture];

	if (bCompatibilityMode)
	{
		hDC = hdcCompatible;
		//zero(captureBits, width*height*4);
	}
	else if (captureTexture)
		GetTextureDC(captureTexture, hDC);

	return hDC;
}

void ProcTopWindowRealSource::ReleaseCurrentHDC(HDC hDC)
{
	Texture *captureTexture = renderTextures[curCaptureTexture];

	if (!bCompatibilityMode)
		ReleaseTextureDC(captureTexture);
}

void ProcTopWindowRealSource::EndPreprocess()
{
	if (bCompatibilityMode)
	{
		SetImage(renderTextures[0], captureBits, GS_IMAGEFORMAT_BGRA, width * 4);
		lastRendered = renderTextures[0];
	}
	else
	{
		lastRendered = renderTextures[curCaptureTexture];

		if (++curCaptureTexture == CAPTURE_TEXTURES_NUM)
			curCaptureTexture = 0;
	}
}

void ProcTopWindowRealSource::Preprocess()
{
	if (bWindows8MonitorCapture)
	{
		PreprocessWindows8MonitorCapture();
		return;
	}

	HDC hDC;
	if (hDC = GetCurrentHDC())
	{
		//----------------------------------------------------------
		// capture screen

		CURSORINFO ci;
		zero(&ci, sizeof(ci));
		ci.cbSize = sizeof(ci);
		BOOL bMouseCaptured;

		bMouseCaptured = bCaptureMouse && GetCursorInfo(&ci);

		bool bWindowNotFound = false;
		HWND hwndCapture = NULL;
		if (captureType == 1)
		{
			if (hwndFoundWindow && IsWindow(hwndFoundWindow))
			{
				TCHAR lpClassName[256];
				BOOL bSuccess = GetClassName(hwndFoundWindow, lpClassName, 255);

				if (bSuccess && scmpi(lpClassName, strWindowClass) == 0)
					hwndCapture = hwndFoundWindow;
				else
				{
					hwndCapture = FindWindow(strWindowClass, strWindow);
					if (!hwndCapture)
						hwndCapture = FindWindow(strWindowClass, NULL);
				}
			}
			else
			{
				hwndCapture = FindWindow(strWindowClass, strWindow);
				if (!hwndCapture)
					hwndCapture = FindWindow(strWindowClass, NULL);
			}

			//note to self - hwndFoundWindow is used to make sure it doesn't pick up another window unintentionally if the title changes
			hwndFoundWindow = hwndCapture;

			if (!hwndCapture)
				bWindowNotFound = true;
			if (hwndCapture && (IsIconic(hwndCapture) || !IsWindowVisible(hwndCapture)))
			{
				ReleaseCurrentHDC(hDC);
				//bWindowMinimized = true;

				if (!warningID)
				{
					String strWarning;

					if (bWindowNotFound)
					{
						strWarning << TEXT("进程窗口消失，请确认是否最小化或者进程已关闭！");
					}
					else
					{
						strWarning << L"Sources.SoftwareCaptureSource.WindowMinimized";
					}

				}
				return;
			}
			else if (!bWindowNotFound)
			{
			}
		}

		HDC hCaptureDC;
		if (hwndCapture && captureType == 1 && !bClientCapture)
			hCaptureDC = GetWindowDC(hwndCapture);
		else
			hCaptureDC = GetDC(hwndCapture);

		GetClientRect(hwndCapture, &captureRect);
		unsigned int newWidth = captureRect.right - captureRect.left;
		unsigned int newHeight = captureRect.bottom - captureRect.top;
		captureRect.left = 0;
		captureRect.top = 0;
		if (newWidth != width || newHeight != height)
		{
			if (newWidth == 0)
				newWidth = 1;
			if (newHeight == 0)
				newHeight = 1;

			for (int i = 0; i < CAPTURE_TEXTURES_NUM; i++)
			{
				delete renderTextures[i];
				renderTextures[i] = NULL;
			}
			if (bCompatibilityMode)
				renderTextures[0] = CreateTexture(newWidth, newHeight, GS_BGRA, NULL, FALSE, FALSE);
			else
			{
				for (UINT i = 0; i < CAPTURE_TEXTURES_NUM; i++)
					renderTextures[i] = CreateGDITexture(newWidth, newHeight);
			}

			width = newWidth;
			height = newHeight;
			lastRendered = NULL;

			Texture *captureTexture = renderTextures[curCaptureTexture];
			if (bCompatibilityMode)
				hDC = hdcCompatible;
			else if (captureTexture)
				GetTextureDC(captureTexture,hDC);
		}
		if (bWindowNotFound)
		{
			RECT rc = { 0, 0, width, height };
			FillRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
		}
		else
		{
			RECT rc = { 0, 0, width, height };
			FillRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

			//CAPTUREBLT causes mouse flicker, so make capturing layered optional
			if (!BitBlt(hDC, 0, 0, width, height, hCaptureDC, captureRect.left, captureRect.top, bCaptureLayered ? SRCCOPY | CAPTUREBLT : SRCCOPY))
			{
				RUNONCE AppWarning(TEXT("Capture BitBlt failed (%d)..  just so you know"), GetLastError());
			}
		}

		ReleaseDC(hwndCapture, hCaptureDC);

		//----------------------------------------------------------
		// capture mouse

		if (bMouseCaptured && (captureType == 0 || (captureType == 1 && hwndFoundWindow == GetForegroundWindow())))
		{
			if (ci.flags & CURSOR_SHOWING)
			{
				HICON hIcon = CopyIcon(ci.hCursor);

				if (hIcon)
				{
					ICONINFO ii;
					if (GetIconInfo(hIcon, &ii))
					{
						POINT capturePos = { captureRect.left, captureRect.top };

						if (captureType == 1)
						{
							if (bClientCapture)
								ClientToScreen(hwndCapture, &capturePos);
							else
							{
								RECT windowRect;
								GetWindowRect(hwndCapture, &windowRect);
								capturePos.x += windowRect.left;
								capturePos.y += windowRect.top;
							}
						}

						int x = ci.ptScreenPos.x - int(ii.xHotspot) - capturePos.x;
						int y = ci.ptScreenPos.y - int(ii.yHotspot) - capturePos.y;

						DrawIcon(hDC, x, y, hIcon);

						DeleteObject(ii.hbmColor);
						DeleteObject(ii.hbmMask);
					}

					DestroyIcon(hIcon);
				}
			}
		}

		ReleaseCurrentHDC(hDC);
	}
	else
	{
		RUNONCE AppWarning(TEXT("Failed to get DC from capture surface"));
	}

	EndPreprocess();
}

void ProcTopWindowRealSource::Render(const Vect2 &pos, const Vect2 &size, Texture *tex, bool bScaleFull, bool bIsLiveC)
{
	SamplerState *sampler = NULL;
	/*if(bWindows8MonitorCapture)
	{
	RenderWindows8MonitorCapture(pos, size);
	return;
	}*/

	Vect2 ulCoord = Vect2(0.0f, 0.0f),
		lrCoord = Vect2(1.0f, 1.0f);

	if (captureType == 1 && !hwndFoundWindow) {
		// Don't render a giant black rectangle if the window isn't found.
		return;
	}

	if (bWindows8MonitorCapture)
	{
		LONG monitorWidth = monitorData.rect.right - monitorData.rect.left;
		LONG monitorHeight = monitorData.rect.bottom - monitorData.rect.top;
		RECT captureArea = { captureRect.left - monitorData.rect.left,
			captureRect.top - monitorData.rect.top,
			captureRect.right - monitorData.rect.left,
			captureRect.bottom - monitorData.rect.top };

		ulCoord.x = float(double(captureArea.left) / double(monitorWidth));
		ulCoord.y = float(double(captureArea.top) / double(monitorHeight));

		lrCoord.x = float(double(captureArea.right) / double(monitorWidth));
		lrCoord.y = float(double(captureArea.bottom) / double(monitorHeight));
	}

	if (lastRendered)
	{
		float fGamma = float(-(gamma - 100) + 100) * 0.01f;

		Shader *lastPixelShader = GetCurrentPixelShader();

		float fOpacity = float(opacity)*0.01f;
		DWORD opacity255 = DWORD(fOpacity*255.0f);

		

		if (bScaleFull)
		{

			if (tex)
			{
				if (bCompatibilityMode)
					DrawSpriteEx(tex, (opacity255 << 24) | 0xFFFFFF,
					pos.x, pos.y + size.y, pos.x + size.x, pos.y,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y);
				else
					DrawSpriteExRotate(tex, (opacity255 << 24) | 0xFFFFFF,
					pos.x, pos.y, pos.x + size.x, pos.y + size.y, 0.0f,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y, rotateDegrees);

				return;
			}
			else
			{
				if (bUseColorKey)
				{
					LoadPixelShader(colorKeyShader);
					HANDLE hGamma = colorKeyShader->GetParameterByName(TEXT("gamma"));
					if (hGamma)
						colorKeyShader->SetFloat(hGamma, fGamma);

					float fSimilarity = float(keySimilarity)*0.01f;
					float fBlend = float(keyBlend)*0.01f;

					colorKeyShader->SetColor(colorKeyShader->GetParameter(2), keyColor);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(3), fSimilarity);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(4), fBlend);

				}
				else
				{
					LoadPixelShader(alphaIgnoreShader);
					HANDLE hGamma = alphaIgnoreShader->GetParameterByName(TEXT("gamma"));
					if (hGamma)
						alphaIgnoreShader->SetFloat(hGamma, fGamma);
				}

				if (bUsePointFiltering) {
					SamplerInfo samplerinfo;
					samplerinfo.filter = GS_FILTER_POINT;
					sampler = CreateSamplerState(samplerinfo);
					LoadSamplerState(sampler, 0);
				}

				if (bCompatibilityMode)
					DrawSpriteEx(lastRendered, (opacity255 << 24) | 0xFFFFFF,
					pos.x, pos.y + size.y, pos.x + size.x, pos.y,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y);
				else
					DrawSpriteExRotate(lastRendered, (opacity255 << 24) | 0xFFFFFF,
					pos.x, pos.y, pos.x + size.x, pos.y + size.y, 0.0f,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y, rotateDegrees);
			}
			
		}
		else
		{
			//等比例缩放
			float aspect = (float)size.x / size.y;

			float x, x2;
			float y, y2;

			if (aspect > 1.0f)
			{
				y = pos.y;
				y2 = y + size.y;

				float NewWidth = width * size.y / height;

				x = pos.x + (size.x - NewWidth) / 2;
				x2 = x + NewWidth;

				if (NewWidth > size.x)
				{
					x = pos.x;
					x2 = pos.x + size.x;

					float NewHeight = height * size.x / width;

					y = pos.y + (size.y - NewHeight) / 2;
					y2 = y + NewHeight;
				}
			}
			else
			{
				x = pos.x;
				x2 = pos.x + size.x;

				float NewHeight = height * size.x / width;

				y = pos.y + (size.y - NewHeight) / 2;
				y2 = y + NewHeight;

				if (NewHeight > size.y)
				{
					y = pos.y;
					y2 = y + size.y;

					float NewWidth = width * size.y / height;

					x = pos.x + (size.x - NewWidth) / 2;
					x2 = x + NewWidth;
				}
			}


			if (!tex)
			{
				if (bUseColorKey)
				{
					LoadPixelShader(colorKeyShader);
					HANDLE hGamma = colorKeyShader->GetParameterByName(TEXT("gamma"));
					if (hGamma)
						colorKeyShader->SetFloat(hGamma, fGamma);

					float fSimilarity = float(keySimilarity)*0.01f;
					float fBlend = float(keyBlend)*0.01f;

					colorKeyShader->SetColor(colorKeyShader->GetParameter(2), keyColor);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(3), fSimilarity);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(4), fBlend);

				}
				else
				{
					LoadPixelShader(alphaIgnoreShader);
					HANDLE hGamma = alphaIgnoreShader->GetParameterByName(TEXT("gamma"));
					if (hGamma)
						alphaIgnoreShader->SetFloat(hGamma, fGamma);
				}

				if (bUsePointFiltering) {
					SamplerInfo samplerinfo;
					samplerinfo.filter = GS_FILTER_POINT;
					sampler = CreateSamplerState(samplerinfo);
					LoadSamplerState(sampler, 0);
				}


				if (bCompatibilityMode)
					DrawSpriteEx(lastRendered, (opacity255 << 24) | 0xFFFFFF,
					x, y2, x2, y,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y);
				else
					DrawSpriteExRotate(lastRendered, (opacity255 << 24) | 0xFFFFFF,
					x, y, x2, y2, 0.0f,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y, rotateDegrees);
			}
			else
			{
				if (bCompatibilityMode)
					DrawSpriteEx(tex, (opacity255 << 24) | 0xFFFFFF,
					x, y2, x2, y,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y);
				else
					DrawSpriteExRotate(tex, (opacity255 << 24) | 0xFFFFFF,
					x, y, x2, y2, 0.0f,
					ulCoord.x, ulCoord.y,
					lrCoord.x, lrCoord.y, rotateDegrees);
				return;
			}
		}

		if (bUsePointFiltering) delete(sampler);

		LoadPixelShader(lastPixelShader);

		//draw mouse
		if (bWindows8MonitorCapture && cursorTexture && bMouseCaptured && bCaptureMouse)
		{
			POINT newPos = { cursorPos.x - xHotspot, cursorPos.y - yHotspot };

			if (newPos.x >= captureRect.left && newPos.y >= captureRect.top &&
				newPos.x < captureRect.right && newPos.y < captureRect.bottom)
			{
				DWORD Width, Height;
				GetTextureWH(cursorTexture, Width, Height);
				Vect2 newCursorPos = Vect2(float(newPos.x - captureRect.left), float(newPos.y - captureRect.top));
				Vect2 newCursorSize = Vect2(float(Width), float(Height));

				Vect2 sizeMultiplier = size / GetSize();
				newCursorPos *= sizeMultiplier;
				newCursorPos += pos;
				newCursorSize *= sizeMultiplier;

				SetCropping(0.0f, 0.0f, 0.0f, 0.0f);
				DrawSprite(cursorTexture, 0xFFFFFFFF, newCursorPos.x, newCursorPos.y, newCursorPos.x + newCursorSize.x, newCursorPos.y + newCursorSize.y);
			}
		}
	}
}

Vect2 ProcTopWindowRealSource::GetSize() const
{
	return Vect2(float(width), float(height));
}

void ProcTopWindowRealSource::GetWindowList(StringList &classList)
{
	classList.Clear();

	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	do
	{
		if (IsWindowVisible(hwndCurrent) && !IsIconic(hwndCurrent))
		{
			DWORD processID;
			GetWindowThreadProcessId(hwndCurrent, &processID);
			if (processID == GetCurrentProcessId())
				continue;

			RECT clientRect;
			GetClientRect(hwndCurrent, &clientRect);

			String strWindowName;
			strWindowName.SetLength(GetWindowTextLength(hwndCurrent));
			GetWindowText(hwndCurrent, strWindowName, strWindowName.Length() + 1);

			DWORD exStyles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_EXSTYLE);
			DWORD styles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_STYLE);

			if (strWindowName.IsValid() && sstri(strWindowName, L"battlefield") != nullptr)
				exStyles &= ~WS_EX_TOOLWINDOW;

			if ((exStyles & WS_EX_TOOLWINDOW) == 0 && (styles & WS_CHILD) == 0 &&
				clientRect.bottom != 0 && clientRect.right != 0 /*&& hwndParent == NULL*/)
			{
				//-------

				DWORD processID;
				GetWindowThreadProcessId(hwndCurrent, &processID);
				if (processID == GetCurrentProcessId())
					continue;

				TCHAR fileName[MAX_PATH + 1];
				scpy(fileName, TEXT("unknown"));

				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
				if (hProcess)
				{
					DWORD dwSize = MAX_PATH;
					QueryFullProcessImageName(hProcess, 0, fileName, &dwSize);
					CloseHandle(hProcess);
				}

				//-------

				String strFileName = fileName;
				strFileName.FindReplace(TEXT("\\"), TEXT("/"));

				String strText;
				strText << TEXT("[") << GetPathFileName(strFileName) << TEXT("]: ") << strWindowName;

				if (strWindowName.IsEmpty())
					strWindowName = GetPathFileName(strFileName);

				classList << strWindowName;
			}
		}
	} while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));
}

bool ProcTopWindowRealSource::ExistWindow(String& winName)
{
	StringList winLst;
	GetWindowList(winLst);

	bool find = false;
	int num = winLst.Num();
	for (int i = 0; i < num; ++i)
	{
		if (winName == winLst[i])
		{
			find = true;
			break;
		}
	}

	return find;
}

void ProcTopWindowRealSource::UpdateSettings(Value &data)
{
	this->data = data;
	UINT newCaptureType = data["captureType"].asInt();
	String strNewWindow = L"";
	if (!data["window"].isNull())
		strNewWindow = Asic2WChar(data["window"].asString().c_str()).c_str();
	//-----------------------------------
	if (strNewWindow && !ExistWindow(strNewWindow))
	{
	}
	//-----------------------------------

	String strNewWindowClass = L"";

	if (!data["windowClass"].isNull())
		strNewWindowClass = Asic2WChar(data["windowClass"].asString().c_str()).c_str();

	BOOL bNewClientCapture = data["innerWindow"].asInt() == 0;

	bCaptureMouse = data["captureMouse"].asInt() == 1;
	bCaptureLayered = data["captureLayered"].asInt();

	bool bNewUsePointFiltering = data["usePointFiltering"].asInt() != 0;

	int x = data["captureX"].asInt();
	int y = data["captureY"].asInt();
	int cx = 640;
	int cy = 480;

	if (!data["captureCX"].isNull())
		cx = data["captureCX"].asInt();

	if (!data["captureCY"].isNull())
		cy = data["captureCY"].asInt();

	bool bNewCompatibleMode = data["compatibilityMode"].asInt() != 0;
	if (bNewCompatibleMode && (OSGetVersion() >= 8 && newCaptureType == 0))
		bNewCompatibleMode = false;


	bool UseQuality = data["UseQuality"].asInt() == 1;
	gamma = 100;

	if (!data["gamma"].isNull())
	{
		gamma = data["gamma"].asInt();
	}
	if (gamma < 50)        gamma = 50;
	else if (gamma > 175)  gamma = 175;

	if (!UseQuality)
	{
		gamma = 100;
	}

	UINT newMonitor = data["monitor"].asInt();
	if (newMonitor > NumMonitors())
		newMonitor = 0;

	if (captureRect.left != x || captureRect.right != (x + cx) || captureRect.top != cy || captureRect.bottom != (y + cy) ||
		newCaptureType != captureType || !strNewWindowClass.CompareI(strWindowClass) || !strNewWindow.CompareI(strWindow) ||
		bNewClientCapture != bClientCapture || (OSGetVersion() >= 8 && newMonitor != monitor) ||
		bNewCompatibleMode != bCompatibilityMode)
	{
		for (int i = 0; i < CAPTURE_TEXTURES_NUM; i++)
		{
			delete renderTextures[i];
			renderTextures[i] = NULL;
		}

		if (duplicator)
		{
			delete duplicator;
			duplicator = NULL;
		}

		if (cursorTexture)
		{
			delete cursorTexture;
			cursorTexture = NULL;
		}

		if (bCompatibilityMode)
		{
			SelectObject(hdcCompatible, hbmpOld);
			DeleteDC(hdcCompatible);
			DeleteObject(hbmpCompatible);

			hdcCompatible = NULL;
			hbmpCompatible = NULL;
			captureBits = NULL;
		}

		hCurrentCursor = NULL;

		captureType = newCaptureType;
		strWindow = strNewWindow;
		strWindowClass = strNewWindowClass;
		bClientCapture = bNewClientCapture;

		bCompatibilityMode = bNewCompatibleMode;
		bUsePointFiltering = bNewUsePointFiltering;

		captureRect.left = x;
		captureRect.top = y;
		captureRect.right = x + cx;
		captureRect.bottom = y + cy;

		monitor = newMonitor;
		const MonitorInfo &monitorInfo = GetMonitor(monitor);
		mcpy(&monitorData, &monitorInfo, sizeof(monitorInfo));

		rotateDegrees = 0.0f;

		if (captureType == 0 && OSGetVersion() >= 8)
		{
			LONG monitorWidth = monitorInfo.rect.right - monitorInfo.rect.left;
			LONG monitorHeight = monitorInfo.rect.bottom - monitorInfo.rect.top;

			DeviceOutputs outputs;
			GetDisplayDevices(outputs);

			bWindows8MonitorCapture = false;

			if (outputs.devices.Num())
			{
				const MonitorInfo &info = GetMonitor(monitor);
				for (UINT j = 0; j < outputs.devices[0].monitors.Num(); j++)
				{
					if (outputs.devices[0].monitors[j].hMonitor == info.hMonitor)
					{
						deviceOutputID = j;
						rotateDegrees = outputs.devices[0].monitors[j].rotationDegrees;
						bWindows8MonitorCapture = true;
					}
				}
			}
		}

		width = cx;
		height = cy;

		if (bCompatibilityMode)
		{
			hdcCompatible = CreateCompatibleDC(NULL);

			BITMAPINFO bi;
			zero(&bi, sizeof(bi));

			BITMAPINFOHEADER &bih = bi.bmiHeader;
			bih.biSize = sizeof(bih);
			bih.biBitCount = 32;
			bih.biWidth = width;
			bih.biHeight = height;
			bih.biPlanes = 1;

			hbmpCompatible = CreateDIBSection(hdcCompatible, &bi, DIB_RGB_COLORS, (void**)&captureBits, NULL, 0);
			hbmpOld = (HBITMAP)SelectObject(hdcCompatible, hbmpCompatible);
		}

		if (bWindows8MonitorCapture && !bInInit)
			duplicator = CreateOutputDuplicator(deviceOutputID);
		else if (bCompatibilityMode)
			renderTextures[0] = CreateTexture(width, height, GS_BGRA, NULL, FALSE, FALSE);
		else
		{
			for (UINT i = 0; i < CAPTURE_TEXTURES_NUM; i++)
				renderTextures[i] = CreateGDITexture(width, height);
		}

		lastRendered = NULL;
	}

	bool bNewUseColorKey = data["useColorKey"].asInt() != 0;

	keyColor = 0xFFFFFFFF;
	if (!data["keyColor"].isNull())
		keyColor = data["keyColor"].asUInt();
	keySimilarity = 10;
	if (!data["keySimilarity"].isNull())
		keySimilarity = data["keySimilarity"].asUInt();
	UINT  blend = data["keyBlend"].asInt();

	bUsePointFiltering = data["usePointFiltering"].asInt() != 0;

	bUseColorKey = bNewUseColorKey;

	opacity = 100;

	if (!data["opacity"].isNull())
		opacity = data["opacity"].asUInt();

	if (!UseQuality)
	{
		opacity = 100;
	}
}

void ProcTopWindowRealSource::SetInt(CTSTR lpName, int iVal)
{
	if (scmpi(lpName, TEXT("useColorKey")) == 0)
	{
		bool bNewVal = iVal != 0;
		bUseColorKey = bNewVal;
	}
	else if (scmpi(lpName, TEXT("keyColor")) == 0)
	{
		keyColor = (DWORD)iVal;
	}
	else if (scmpi(lpName, TEXT("keySimilarity")) == 0)
	{
		keySimilarity = iVal;
	}
	else if (scmpi(lpName, TEXT("keyBlend")) == 0)
	{
		keyBlend = iVal;
	}
	else if (scmpi(lpName, TEXT("opacity")) == 0)
	{
		opacity = (UINT)iVal;
	}
	else if (scmpi(lpName, TEXT("gamma")) == 0)
	{
		gamma = iVal;
		if (gamma < 50)        gamma = 50;
		else if (gamma > 175)  gamma = 175;
	}
	else if (scmpi(lpName, TEXT("captureMouse")) == 0)
	{
		bCaptureMouse = iVal;
	}
}


bool GetProcessNameByID(String& name, DWORD pid)
{
	bool result = false;
	PROCESSENTRY32 pe32;
	// 在使用这个结构之前，先设置它的大小
	pe32.dwSize = sizeof(pe32);

	// 给系统内的所有进程拍一个快照
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// 遍历进程快照，轮流显示每个进程的信息
	BOOL bMore = ::Process32First(hProcessSnap, &pe32);
	while (bMore)
	{
		DWORD p = pe32.th32ProcessID;
		String tmp(pe32.szExeFile);

		if (p == pid)
		{
			result = true;
			name = tmp + String(L"[ PID: ") + UIntString(pid) + String(L" ]");
			break;
		}
		bMore = ::Process32Next(hProcessSnap, &pe32);
	}

	// 不要忘记清除掉snapshot对象
	::CloseHandle(hProcessSnap);
	return result;
}






void RefreshPorcIDList(HWND hwndCombobox, List<DWORD>& ProcIDList)
{
	SendMessage(hwndCombobox, CB_RESETCONTENT, 0, 0);
	ProcIDList.Clear();

	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	do
	{
		if (IsWindowVisible(hwndCurrent) && !IsIconic(hwndCurrent))
		{
			RECT clientRect;
			GetClientRect(hwndCurrent, &clientRect);

			String strWindowName;
			strWindowName.SetLength(GetWindowTextLength(hwndCurrent));
			GetWindowText(hwndCurrent, strWindowName, strWindowName.Length() + 1);

			DWORD exStyles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_EXSTYLE);
			DWORD styles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_STYLE);

			if (strWindowName.IsValid() && sstri(strWindowName, L"battlefield") != nullptr)
				exStyles &= ~WS_EX_TOOLWINDOW;

			if ((exStyles & WS_EX_TOOLWINDOW) == 0 && (styles & WS_CHILD) == 0 &&
				clientRect.bottom != 0 && clientRect.right != 0 /*&& hwndParent == NULL*/)
			{
				//-------

				DWORD processID;
				GetWindowThreadProcessId(hwndCurrent, &processID);
				if (processID == GetCurrentProcessId() || ProcIDList.HasValue(processID))
					continue;

				//------------------------------------------
				//一下为调试信息，正式代码可以不包含此段
				TCHAR fileName[MAX_PATH + 1];
				scpy(fileName, TEXT("unknown"));

				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
				if (hProcess)
				{
					DWORD dwSize = MAX_PATH;
					QueryFullProcessImageName(hProcess, 0, fileName, &dwSize);
					CloseHandle(hProcess);
				}

				String strFileName = fileName;
				strFileName.FindReplace(TEXT("\\"), TEXT("/"));

				String strText;
				strText << TEXT("[") << GetPathFileName(strFileName) << TEXT("]: ") << strWindowName;

				if (strWindowName.IsEmpty()) strWindowName = GetPathFileName(strFileName);

				//-------------------------------------------

				String ProcName;
				if (GetProcessNameByID(ProcName, processID))
				{
					int id = (int)SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM)ProcName.Array());
					SendMessage(hwndCombobox, CB_SETITEMDATA, id, (LPARAM)hwndCurrent);

					ProcIDList << processID;
				}
			}
		}
	} while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));
}

struct ConfigProcTopWindowSourceInfo
{
	CTSTR lpName;
	Value &data;
	int dialogID;
	List<DWORD> ProcIDs;
	int prevCX, prevCY;
	bool sizeSet;

	inline ConfigProcTopWindowSourceInfo(Value &Data) :data(Data)
	{
	}
};

void SetDesktopCaptureLocalType(HWND hwnd, UINT type)
{
	SendMessage(GetDlgItem(hwnd, IDC_MONITORCAPTURE), BM_SETCHECK, type == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_SETCHECK, type == 1 ? BST_CHECKED : BST_UNCHECKED, 0);

	EnableWindow(GetDlgItem(hwnd, IDC_MONITOR), type == 0);

	EnableWindow(GetDlgItem(hwnd, IDC_WINDOW), type == 1);
	EnableWindow(GetDlgItem(hwnd, IDC_REFRESH), type == 1);
	EnableWindow(GetDlgItem(hwnd, IDC_OUTERWINDOW), type == 1);
	EnableWindow(GetDlgItem(hwnd, IDC_INNERWINDOW), type == 1);
}

struct RegionWindowDataLocal
{
	HWND hwndConfigDialog, hwndCaptureWindow;
	POINT startPos;
	RECT captureRect;
	bool bMovingWindow;
	bool bInnerWindowRegion;

	inline RegionWindowDataLocal()
	{
		bMovingWindow = false;
		bInnerWindowRegion = false;
	}
};

RegionWindowDataLocal regionWindowData;

#define PROCCAPTUREREGIONCLASS TEXT("CaptureRegionThingyLocal")

LRESULT WINAPI RegionWindowProcLocal(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_MOUSEMOVE)
	{
		RECT client;
		GetClientRect(hwnd, &client);

		POINT pos;
		pos.x = (long)(short)LOWORD(lParam);
		pos.y = (long)(short)HIWORD(lParam);

		if (regionWindowData.bMovingWindow)
		{
			RECT rc;
			GetWindowRect(hwnd, &rc);

			POINT posOffset = { pos.x - regionWindowData.startPos.x, pos.y - regionWindowData.startPos.y };
			POINT newPos = { rc.left + posOffset.x, rc.top + posOffset.y };
			SIZE windowSize = { rc.right - rc.left, rc.bottom - rc.top };

			if (newPos.x < regionWindowData.captureRect.left)
				newPos.x = regionWindowData.captureRect.left;
			if (newPos.y < regionWindowData.captureRect.top)
				newPos.y = regionWindowData.captureRect.top;

			POINT newBottomRight = { rc.right + posOffset.x, rc.bottom + posOffset.y };

			if (newBottomRight.x > regionWindowData.captureRect.right)
				newPos.x -= (newBottomRight.x - regionWindowData.captureRect.right);
			if (newBottomRight.y > regionWindowData.captureRect.bottom)
				newPos.y -= (newBottomRight.y - regionWindowData.captureRect.bottom);

			SetWindowPos(hwnd, NULL, newPos.x, newPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
		else
		{
			BOOL bLeft = (pos.x < 6);
			BOOL bTop = (pos.y < 6);
			BOOL bRight = (!bLeft && (pos.x > client.right - 6));
			BOOL bBottom = (!bTop && (pos.y > client.bottom - 6));

			if (bLeft)
			{
				if (bTop)
					SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else if (bBottom)
					SetCursor(LoadCursor(NULL, IDC_SIZENESW));
				else
					SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			}
			else if (bRight)
			{
				if (bTop)
					SetCursor(LoadCursor(NULL, IDC_SIZENESW));
				else if (bBottom)
					SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else
					SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			}
			else if (bTop || bBottom)
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
		}

		return 0;
	}
	else if (message == WM_LBUTTONDOWN)
	{
		RECT client;
		GetClientRect(hwnd, &client);

		POINT pos;
		pos.x = (long)LOWORD(lParam);
		pos.y = (long)HIWORD(lParam);

		BOOL bLeft = (pos.x < 6);
		BOOL bTop = (pos.y < 6);
		BOOL bRight = (!bLeft && (pos.x > client.right - 6));
		BOOL bBottom = (!bTop && (pos.y > client.bottom - 6));

		ClientToScreen(hwnd, &pos);

		POINTS newPos;
		newPos.x = (SHORT)pos.x;
		newPos.y = (SHORT)pos.y;

		SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);

		if (bLeft)
		{
			if (bTop)
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOPLEFT, (LPARAM)&newPos);
			else if (bBottom)
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOMLEFT, (LPARAM)&newPos);
			else
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTLEFT, (LPARAM)&newPos);
		}
		else if (bRight)
		{
			if (bTop)
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOPRIGHT, (LPARAM)&newPos);
			else if (bBottom)
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, (LPARAM)&newPos);
			else
				SendMessage(hwnd, WM_NCLBUTTONDOWN, HTRIGHT, (LPARAM)&newPos);
		}
		else if (bTop)
			SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOP, (LPARAM)&newPos);
		else if (bBottom)
			SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOM, (LPARAM)&newPos);
		else
		{
			regionWindowData.bMovingWindow = true; //can't use HTCAPTION -- if holding down long enough, other windows minimize

			pos.x = (long)(short)LOWORD(lParam);
			pos.y = (long)(short)HIWORD(lParam);
			mcpy(&regionWindowData.startPos, &pos, sizeof(pos));
			SetCapture(hwnd);
		}

		return 0;
	}
	else if (message == WM_LBUTTONUP)
	{
		if (regionWindowData.bMovingWindow)
		{
			regionWindowData.bMovingWindow = false;
			ReleaseCapture();
		}
	}
	else if (message == WM_PAINT)
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		if (hDC)
		{
			RECT clientRect;
			GetClientRect(hwnd, &clientRect);

			//-----------------------------------------

			HPEN oldPen = (HPEN)SelectObject(hDC, GetStockObject(BLACK_PEN));

			MoveToEx(hDC, 0, 0, NULL);
			LineTo(hDC, clientRect.right - 1, 0);
			LineTo(hDC, clientRect.right - 1, clientRect.bottom - 1);
			LineTo(hDC, 0, clientRect.bottom - 1);
			LineTo(hDC, 0, 0);

			MoveToEx(hDC, 5, 5, NULL);
			LineTo(hDC, clientRect.right - 6, 5);
			LineTo(hDC, clientRect.right - 6, clientRect.bottom - 6);
			LineTo(hDC, 5, clientRect.bottom - 6);
			LineTo(hDC, 5, 5);

			SelectObject(hDC, oldPen);

			//-----------------------------------------

			CTSTR lpStr = L"Sources.SoftwareCaptureSource.RegionWindowText";

			HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			HFONT hfontOld = (HFONT)SelectObject(hDC, hFont);

			SIZE textExtent;
			GetTextExtentPoint32(hDC, lpStr, slen(lpStr), &textExtent);

			SetBkMode(hDC, TRANSPARENT);
			SetTextAlign(hDC, TA_CENTER);
			TextOut(hDC, clientRect.right / 2, (clientRect.bottom - textExtent.cy) / 2, lpStr, slen(lpStr));

			//-----------------------------------------

			SelectObject(hDC, hfontOld);

			EndPaint(hwnd, &ps);
		}
	}
	else if (message == WM_KEYDOWN)
	{
		if (wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == 'Q')
			DestroyWindow(hwnd);
	}
	else if (message == WM_SIZING)
	{
		RECT &rc = *(RECT*)lParam;

		bool bTop = wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_TOP;
		bool bLeft = wParam == WMSZ_TOPLEFT || wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT;

		if (bLeft)
		{
			if (rc.right - rc.left < 16)
				rc.left = rc.right - 16;
		}
		else
		{
			if (rc.right - rc.left < 16)
				rc.right = rc.left + 16;
		}

		if (bTop)
		{
			if (rc.bottom - rc.top < 16)
				rc.top = rc.bottom - 16;
		}
		else
		{
			if (rc.bottom - rc.top < 16)
				rc.bottom = rc.top + 16;
		}

		if (regionWindowData.hwndCaptureWindow)
		{
			if (rc.left < regionWindowData.captureRect.left)
				rc.left = regionWindowData.captureRect.left;
			if (rc.top < regionWindowData.captureRect.top)
				rc.top = regionWindowData.captureRect.top;

			if (rc.right > regionWindowData.captureRect.right)
				rc.right = regionWindowData.captureRect.right;
			if (rc.bottom > regionWindowData.captureRect.bottom)
				rc.bottom = regionWindowData.captureRect.bottom;
		}

		return TRUE;
	}
	else if (message == WM_SIZE || message == WM_MOVE)
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);

		if (rc.right - rc.left < 16)
			rc.right = rc.left + 16;
		if (rc.bottom - rc.top < 16)
			rc.bottom = rc.top + 16;

		SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_SIZEX), IntString(rc.right - rc.left).Array());
		SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_SIZEY), IntString(rc.bottom - rc.top).Array());

		if (regionWindowData.hwndCaptureWindow)
		{
			rc.left -= regionWindowData.captureRect.left;
			rc.top -= regionWindowData.captureRect.top;
		}

		SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_POSX), IntString(rc.left).Array());
		SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_POSY), IntString(rc.top).Array());

		if (message == WM_SIZE)
			RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT | RDW_ERASE | RDW_INVALIDATE);
	}
	else if (message == WM_ACTIVATE)
	{
		if (wParam == WA_INACTIVE)
			DestroyWindow(hwnd);
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

struct ColorSelectionData
{
	HDC hdcDesktop;
	HDC hdcDestination;
	HBITMAP hBitmap;
	bool bValid;

	inline ColorSelectionData() : hdcDesktop(NULL), hdcDestination(NULL), hBitmap(NULL), bValid(false) {}
	inline ~ColorSelectionData() { Clear(); }

	inline bool Init()
	{
		hdcDesktop = GetDC(NULL);
		if (!hdcDesktop)
			return false;

		hdcDestination = CreateCompatibleDC(hdcDesktop);
		if (!hdcDestination)
			return false;

		hBitmap = CreateCompatibleBitmap(hdcDesktop, 1, 1);
		if (!hBitmap)
			return false;

		SelectObject(hdcDestination, hBitmap);
		bValid = true;

		return true;
	}

	inline void Clear()
	{
		if (hdcDesktop)
		{
			ReleaseDC(NULL, hdcDesktop);
			hdcDesktop = NULL;
		}

		if (hdcDestination)
		{
			DeleteDC(hdcDestination);
			hdcDestination = NULL;
		}

		if (hBitmap)
		{
			DeleteObject(hBitmap);
			hBitmap = NULL;
		}

		bValid = false;
	}

	inline DWORD GetColor()
	{
		POINT p;
		if (GetCursorPos(&p))
		{
			BITMAPINFO data;
			zero(&data, sizeof(data));

			data.bmiHeader.biSize = sizeof(data.bmiHeader);
			data.bmiHeader.biWidth = 1;
			data.bmiHeader.biHeight = 1;
			data.bmiHeader.biPlanes = 1;
			data.bmiHeader.biBitCount = 24;
			data.bmiHeader.biCompression = BI_RGB;
			data.bmiHeader.biSizeImage = 4;

			if (BitBlt(hdcDestination, 0, 0, 1, 1, hdcDesktop, p.x, p.y, SRCCOPY | CAPTUREBLT))
			{
				DWORD buffer;
				if (GetDIBits(hdcDestination, hBitmap, 0, 1, &buffer, &data, DIB_RGB_COLORS))
					return 0xFF000000 | buffer;
			}
			else
			{
				int err = GetLastError();
				nop();
			}
		}

		return 0xFF000000;
	}
};


INT_PTR CALLBACK ProcTopButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (WM_PAINT == message)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		HBRUSH HBrush = CreateSolidBrush(RGB(102, 102, 102));

		FillRect(hdc, &Rect, HBrush);
		DeleteObject(HBrush);

		if (IsWindowEnabled(hwnd))
		{
			SetTextColor(hdc, RGB(255, 255, 255));
		}
		else
		{
			SetTextColor(hdc, RGB(153, 153, 153));
		}
		SetBkColor(hdc, RGB(102, 102, 102));
		SetBkMode(hdc, TRANSPARENT);

		SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

		TCHAR Title[MAX_PATH] = { 0 };

		GetWindowText(hwnd, Title, sizeof Title);

		RECT rtClient;
		GetClientRect(hwnd, &rtClient);

		if (wcslen(Title) > 0)
		{
			DrawText(hdc, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		EndPaint(hwnd, &ps);
	}


	WNDPROC WndProcTem = NULL;
	if (hwnd == GetDlgItem(Hwnd, IDOK))
	{
		WndProcTem = YesBtnProc1;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDC_REFRESH))
	{
		WndProcTem = RefreshBtnProc1;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = NoBtnProc1;
	}
	else if (Hwnd == GetDlgItem(Hwnd, IDC_SELECTREGION))
	{
		WndProcTem = SelectBtnProc1;
	}
	else
	{
		WndProcTem = DefBtnProc1;
	}
	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigProcTopWindowSourceProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool bSelectingColor = false;
	static bool bMouseDown = false;
	static ColorSelectionData colorData;
	HWND hwndTemp;

	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;
	switch (message)
	{
	case WM_NCACTIVATE:
	{
						  BOOL bChecked = SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED;
						  if (bChecked)
						  {
							  if (wParam == 0)
								  return FALSE;
						  }
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwnd, -1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwnd, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwnd, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT Rect;
					 GetClientRect(hwnd, &Rect);

					 HBRUSH HBrush = CreateSolidBrush(RGB(153, 153, 153));

					 FillRect(hdc, &Rect, HBrush);
					 DeleteObject(HBrush);

					 EndPaint(hwnd, &ps);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwnd;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwnd, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwnd, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwnd, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwnd, -1, true);
	}
		break;

	case WM_CTLCOLORSTATIC:
	{
							  HDC hdc = (HDC)wParam;
							  SetTextColor(hdc, RGB(255, 255, 255));
							  SetBkColor(hdc, RGB(153, 153, 153));

							  if (HBrush)
							  {
								  DeleteObject(HBrush);
							  }
							  HBrush = CreateSolidBrush(RGB(153, 153, 153));

							  return (LRESULT)HBrush;
	}
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	{
							   HDC hdc = (HDC)wParam;
							   SetTextColor(hdc, RGB(255, 255, 255));
							   SetBkColor(hdc, RGB(121, 121, 121));

							   if (HBrush)
							   {
								   DeleteObject(HBrush);
							   }
							   HBrush = CreateSolidBrush(RGB(121, 121, 121));
							   return (LRESULT)HBrush;
	}

	case WM_INITDIALOG:
	{

						  RefreshBtnProc1 = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_REFRESH), GWLP_WNDPROC);
						  DefBtnProc1 = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_REDEFAULT), GWLP_WNDPROC);
						  YesBtnProc1 = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  NoBtnProc1 = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
						  SelectBtnProc1 = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_SELECTREGION), GWLP_WNDPROC);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_REFRESH), GWLP_WNDPROC, (LONG_PTR)ProcTopButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_REDEFAULT), GWLP_WNDPROC, (LONG_PTR)ProcTopButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)ProcTopButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)ProcTopButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_SELECTREGION), GWLP_WNDPROC, (LONG_PTR)ProcTopButtonProc);

						  SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
						  LocalizeWindow(hwnd, pluginLocale);


						  RECT QuailtyRect;
						  GetWindowRect(GetDlgItem(hwnd, IDC_QUALITY), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_QUALITY), 17,112, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_QUALITY_STATIC), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_QUALITY_STATIC), 17 + 16, 112, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_REGION_CAPTURE), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_REGION_CAPTURE), 17, 1000, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_STATIC_GAMMA), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_STATIC_GAMMA), 15, 133, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_GAMMAVAL), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_GAMMAVAL), 15 + 470, 133, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_GAMMA), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_GAMMA), 147, 120, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_STATIC_OPACITY), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_STATIC_OPACITY), 15, 177, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_OPACIYT_VALUE), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_OPACIYT_VALUE), 15 + 470, 177, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), 147, 165, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDC_REDEFAULT), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDC_REDEFAULT), 418, 165 + 55, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);


						  GetWindowRect(GetDlgItem(hwnd, IDOK), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDOK), 400, 165 + 55 + 40, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &QuailtyRect);
						  MoveWindow(GetDlgItem(hwnd, IDCANCEL), 490, 165 + 55 + 40, QuailtyRect.right - QuailtyRect.left, QuailtyRect.bottom - QuailtyRect.top, TRUE);

						  GetWindowRect(GetMainWindow(), &QuailtyRect);

						  MoveWindow(hwnd, (QuailtyRect.right - 585) >> 1, (QuailtyRect.bottom - 330) >> 1, 585, 330, TRUE);

						  //--------------------------------------------

						  HWND hwndToolTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
							  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							  hwnd, NULL, hinstMain, NULL);

						  TOOLINFO ti;
						  zero(&ti, sizeof(ti));
						  ti.cbSize = sizeof(ti);
						  ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
						  ti.hwnd = hwnd;

// 						  if (LocaleIsRTL())
// 							  ti.uFlags |= TTF_RTLREADING;

						  SendMessage(hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 500);
						  SendMessage(hwndToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 8000);

						  //-----------------------------------------------------

						  ConfigProcTopWindowSourceInfo *info = (ConfigProcTopWindowSourceInfo*)lParam;
						  Value &data = info->data;

						  UINT Width, Height;
						  GetBaseSize(Width, Height);

						  info->prevCX = Width;
						  info->prevCY = Height;

						  if (info->dialogID == IDD_CONFIGUREMONITORCAPTURE || info->dialogID == IDD_CONFIGUREDESKTOPSOURCE) {
							  hwndTemp = GetDlgItem(hwnd, IDC_MONITOR);
							  for (UINT i = 0; i< NumMonitors(); i++)
								  SendMessage(hwndTemp, CB_ADDSTRING, 0, (LPARAM)UIntString(i + 1).Array());
							  SendMessage(hwndTemp, CB_SETCURSEL, 0, 0);

							  if (OSGetVersion() > 7) {
								  EnableWindow(GetDlgItem(hwnd, IDC_CAPTURELAYERED), FALSE);
								  EnableWindow(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), (info->dialogID != IDD_CONFIGUREMONITORCAPTURE));
							  }
						  }

						  UINT captureType = data["captureType"].asInt();

						  if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE)
							  SetDesktopCaptureLocalType(hwnd, captureType);

						  //-----------------------------------------------------

						  if (info->dialogID == IDD_CONFIGUREMONITORCAPTURE || info->dialogID == IDD_CONFIGUREDESKTOPSOURCE) {
							  hwndTemp = GetDlgItem(hwnd, IDC_MONITOR);
							  UINT monitor = data["monitor"].asInt();
							  SendMessage(hwndTemp, CB_SETCURSEL, monitor, 0);
						  }

						  //-----------------------------------------------------

						  if (info->dialogID == IDD_CONFIGUREMONITORCAPTURE) {
							  String strWarnings;
							  if (OSGetVersion() < 8) {
								  strWarnings << pluginLocale->LookupString(L"Sources.SoftwareCaptureSource.Warning");

								  BOOL bComposition;
								  DwmIsCompositionEnabled(&bComposition);
								  if (bComposition)
									  strWarnings << TEXT("\r\n") << pluginLocale->LookupString(L"Sources.SoftwareCaptureSource.WarningAero");
							  }

							  SetWindowText(GetDlgItem(hwnd, IDC_WARNING), strWarnings);
						  }

						  //-----------------------------------------------------

						  bool bFoundWindow = false;
						  if (info->dialogID == IDD_CONFIGUREWINDOWCAPTURE || info->dialogID == IDD_CONFIGUREDESKTOPSOURCE) {
							  HWND hwndWindowList = GetDlgItem(hwnd, IDC_WINDOW);

							  String lpProcName = L"";
							  if (!data["ProcName"].isNull())
								  lpProcName = Asic2WChar(data["ProcName"].asString().c_str()).c_str();
							  //DWORD ProcID = data->GetInt(TEXT("ProcID"));
							  BOOL bInnerWindow = data["innerWindow"].asInt() == 0;

							  RefreshPorcIDList(hwndWindowList, info->ProcIDs);

							  UINT ProcIndex = 0;
							  if (!lpProcName.IsEmpty())
								  ProcIndex = (UINT)SendMessage(hwndWindowList, CB_FINDSTRINGEXACT, -1, (LPARAM)lpProcName.Array());

							  bFoundWindow = (ProcIndex != CB_ERR);
							  if (!bFoundWindow)
								  ProcIndex = 0;

							  SendMessage(hwndWindowList, CB_SETCURSEL, ProcIndex, 0);

							  if (bInnerWindow)
								  SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_SETCHECK, BST_CHECKED, 0);
							  else
								  SendMessage(GetDlgItem(hwnd, IDC_OUTERWINDOW), BM_SETCHECK, BST_CHECKED, 0);
						  }

						  //-----------------------------------------------------
						  bool bMouseCapture = true;
						  if (!data["captureMouse"].isNull())
							  bMouseCapture = data["captureMouse"].asInt() == 1;

						  SendMessage(GetDlgItem(hwnd, IDC_CAPTUREMOUSE), BM_SETCHECK, (bMouseCapture) ? BST_CHECKED : BST_UNCHECKED, 0);

						  bool bCaptureLayered = data["captureLayered"].asInt() != 0;
						  SendMessage(GetDlgItem(hwnd, IDC_CAPTURELAYERED), BM_SETCHECK, (bCaptureLayered) ? BST_CHECKED : BST_UNCHECKED, 0);

						  ti.lpszText = (LPWSTR)L"Sources.SoftwareCaptureSource.CaptureLayeredTip";
						  ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_CAPTURELAYERED);
						  SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

						  bool bCompatibilityMode = data["compatibilityMode"].asInt() != 0;
						  SendMessage(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), BM_SETCHECK, (bCompatibilityMode) ? BST_CHECKED : BST_UNCHECKED, 0);

						  //-----------------------------------------------------

						  bool bRegion = true; //data->GetInt(TEXT("regionCapture")) != FALSE;
						  if (captureType == 1 && !bFoundWindow)
							  bRegion = false;

						  SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_SETCHECK, (bRegion) ? BST_CHECKED : BST_UNCHECKED, 0);
						  ShowWindow(GetDlgItem(hwnd, IDC_POSX), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_POSY), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_SIZEX), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_SIZEY), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_SELECTREGION), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_STATIC1), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_HIDE);

						  int posX = 0, posY = 0, sizeX = 640, sizeY = 480;

						  {
							  posX = data["captureX"].asInt();
							  posY = data["captureY"].asInt();
							  sizeX = 640;
							  if (!data["captureCX"].isNull())
								  sizeX = data["captureCX"].asInt();

							  sizeY = 480;

							  if (!data["captureCY"].isNull())
								  sizeY = data["captureCY"].asInt();

							  if (sizeX < 16)
								  sizeX = 16;
							  if (sizeY < 16)
								  sizeY = 16;

							  SetWindowText(GetDlgItem(hwnd, IDC_POSX), IntString(posX).Array());
							  SetWindowText(GetDlgItem(hwnd, IDC_POSY), IntString(posY).Array());
							  SetWindowText(GetDlgItem(hwnd, IDC_SIZEX), IntString(sizeX).Array());
							  SetWindowText(GetDlgItem(hwnd, IDC_SIZEY), IntString(sizeY).Array());
						  }

						  ti.lpszText = (LPWSTR)L"Sources.SoftwareCaptureSource.SelectRegionTooltip";
						  ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_SELECTREGION);
						  SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

						  //------------------------------------------

						  BOOL  bUseColorKey = data["useColorKey"].asInt();
						  DWORD keyColor = 0xFFFFFFFF;
						  if (!data["keyColor"].isNull())
							  keyColor = data["keyColor"].asUInt();
						  UINT  similarity = 10;
						  if(!data["keySimilarity"].isNull())
							  similarity = data["keySimilarity"].asUInt();
						  UINT  blend = data["keyBlend"].asInt();

						  BOOL  bUsePointFiltering = data["usePointFiltering"].asInt();
						  SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_SETCHECK, bUsePointFiltering ? BST_CHECKED : BST_UNCHECKED, 0);

						  SendMessage(GetDlgItem(hwnd, IDC_USECOLORKEY), BM_SETCHECK, bUseColorKey ? BST_CHECKED : BST_UNCHECKED, 0);
						  CCSetColor(GetDlgItem(hwnd, IDC_COLOR), keyColor);

						  SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETRANGE32, 0, 100);
						  SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETPOS32, 0, similarity);

						  SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETRANGE32, 0, 100);
						  SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETPOS32, 0, blend);

						  EnableWindow(GetDlgItem(hwnd, IDC_COLOR), bUseColorKey);
						  EnableWindow(GetDlgItem(hwnd, IDC_SELECT), bUseColorKey);
						  EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT), bUseColorKey);
						  EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD), bUseColorKey);
						  EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT), bUseColorKey);
						  EnableWindow(GetDlgItem(hwnd, IDC_BLEND), bUseColorKey);

						  //------------------------------------------

						  int IsQuality = data["UseQuality"].asInt();

						  if (IsQuality)
						  {
							  SendMessage(GetDlgItem(hwnd, IDC_QUALITY), BM_SETCHECK, BST_CHECKED, 0);
						  }
						  else
						  {
							  SendMessage(GetDlgItem(hwnd, IDC_QUALITY), BM_SETCHECK, BST_UNCHECKED, 0);
						  }

						  EnableWindow(GetDlgItem(hwnd, IDC_GAMMA), IsQuality);
						  EnableWindow(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), IsQuality);
						  EnableWindow(GetDlgItem(hwnd, IDC_REDEFAULT), IsQuality);

						  //-------------------------------------------------

						  UINT opacity = 100;
						  if(!data["opacity"].isNull())
							  opacity = data["opacity"].asInt();

						  SendMessage(GetDlgItem(hwnd, IDC_OPACITY2), UDM_SETRANGE32, 0, 100);
						  SendMessage(GetDlgItem(hwnd, IDC_OPACITY2), UDM_SETPOS32, 0, opacity);

						  HWND hwndTemp = GetDlgItem(hwnd, IDC_OPACITY_SLIDER);
						  SendMessage(hwndTemp, TBM_CLEARTICS, FALSE, 0);
						  SendMessage(hwndTemp, TBM_SETRANGE, FALSE, MAKELPARAM(0, 100));
						  SendMessage(hwndTemp, TBM_SETTIC, 0, 100);
						  SendMessage(hwndTemp, TBM_SETPOS, TRUE, opacity);

						  SetWindowText(GetDlgItem(hwnd, IDC_OPACIYT_VALUE), FormattedString(TEXT("%d"), opacity));

						  //------------------------------------------

						  int gammaVal = 100;
						  if(!data["gamma"].isNull())
							  gammaVal = data["gamma"].asInt();

						  hwndTemp = GetDlgItem(hwnd, IDC_GAMMA);
						  SendMessage(hwndTemp, TBM_CLEARTICS, FALSE, 0);
						  SendMessage(hwndTemp, TBM_SETRANGE, FALSE, MAKELPARAM(50, 175));
						  SendMessage(hwndTemp, TBM_SETTIC, 0, 100);
						  SendMessage(hwndTemp, TBM_SETPOS, TRUE, gammaVal);

						  SetSliderText(hwnd, IDC_GAMMA, IDC_GAMMAVAL);
						  //--------------------------------------------

						  if (info->dialogID == IDD_CONFIGUREWINDOWCAPTURE)
							  PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);

						  //------------------------------
						  ShowWindow(GetDlgItem(hwnd, IDC_SELECTREGION), SW_HIDE);
						  ShowWindow(GetDlgItem(hwnd, IDC_REGIONCAPTURE), SW_HIDE);

						  //-----------------------------
						  InvalidateRect(hwnd, NULL, FALSE);
						  SendMessage(hwnd, WM_NCPAINT, 0, 0);
						  return TRUE;
	}

	case WM_DESTROY:
		if (colorData.bValid)
		{
			CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
			colorData.Clear();
		}
		if (HBrush)
			DeleteObject(HBrush);
		break;

	case WM_LBUTTONDOWN:
		if (bSelectingColor)
		{
			bMouseDown = true;
			CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
			ConfigProcTopWindowSourceProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
		}
		break;

	case WM_MOUSEMOVE:
		if (bSelectingColor && bMouseDown)
		{
			CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
			ConfigProcTopWindowSourceProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
		}
		break;

	case WM_LBUTTONUP:
		if (bSelectingColor)
		{
			colorData.Clear();
			ReleaseCapture();
			bMouseDown = false;
			bSelectingColor = false;

			ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
			std::stringstream SourceId;
			uint64_t VideoId = 0;
			if (!configData->data["SourceID"].isNull())
			{
				SourceId << configData->data["SourceID"].asString().c_str();
				SourceId >> VideoId;
			}

			IBaseVideo *source = (IBaseVideo*)VideoId;
			if (source)
				source->SetInt(TEXT("useColorKey"), true);
		}
		break;

	case WM_CAPTURECHANGED:
		if (bSelectingColor)
		{
			if (colorData.bValid)
			{
				CCSetColor(GetDlgItem(hwnd, IDC_COLOR), colorData.GetColor());
				ConfigProcTopWindowSourceProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_COLOR));
				colorData.Clear();
			}

			ReleaseCapture();
			bMouseDown = false;
			bSelectingColor = false;

			ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
			std::stringstream SourceId;
			uint64_t VideoId = 0;
			if (!configData->data["SourceID"].isNull())
			{
				SourceId << configData->data["SourceID"].asString().c_str();
				SourceId >> VideoId;
			}

			IBaseVideo *source = (IBaseVideo*)VideoId;
			if (source)
				source->SetInt(TEXT("useColorKey"), true);
		}
		break;

	case WM_HSCROLL:
	{
					   if (GetDlgCtrlID((HWND)lParam) == IDC_GAMMA)
					   {
						   int gamma = SetSliderText(hwnd, IDC_GAMMA, IDC_GAMMAVAL);

						   ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
						   std::stringstream SourceId;
						   uint64_t VideoId = 0;
						   if (!configData->data["SourceID"].isNull())
						   {
							   SourceId << configData->data["SourceID"].asString().c_str();
							   SourceId >> VideoId;
						   }

						   IBaseVideo *source = (IBaseVideo*)VideoId;
						   if (source)
							   source->SetInt(TEXT("gamma"), gamma);
					   }
					   else if (GetDlgCtrlID((HWND)lParam) == IDC_OPACITY_SLIDER)
					   {
						   int sliderVal = (int)SendMessage(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), TBM_GETPOS, 0, 0);
						   SetWindowText(GetDlgItem(hwnd, IDC_OPACIYT_VALUE), FormattedString(TEXT("%d"), sliderVal));
						   ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
						   std::stringstream SourceId;
						   uint64_t VideoId = 0;
						   if (!configData->data["SourceID"].isNull())
						   {
							   SourceId << configData->data["SourceID"].asString().c_str();
							   SourceId >> VideoId;
						   }

						   IBaseVideo *source = (IBaseVideo*)VideoId;
						   if (source)
							   source->SetInt(TEXT("opacity"), sliderVal);
					   }
	}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MONITORCAPTURE:
			SetDesktopCaptureLocalType(hwnd, 0);
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MONITOR, 0), 0);

			if (OSGetVersion() >= 8)
			{
				EnableWindow(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), FALSE);
				SendMessage(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), BM_SETCHECK, BST_UNCHECKED, 0);
			}
			break;

		case IDC_WINDOWCAPTURE:
			SetDesktopCaptureLocalType(hwnd, 1);
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);

			if (OSGetVersion() >= 8)
				EnableWindow(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), TRUE);
			break;

		case IDC_REGIONCAPTURE:
		{
								  ConfigProcTopWindowSourceInfo *info = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

								  UINT captureType = 0;

								  if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE)
									  captureType = (SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
								  else if (info->dialogID == IDD_CONFIGUREMONITORCAPTURE)
									  captureType = 0;
								  else
									  captureType = 1;

								  BOOL bRegion = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

								  EnableWindow(GetDlgItem(hwnd, IDC_POSX), bRegion);
								  EnableWindow(GetDlgItem(hwnd, IDC_POSY), bRegion);
								  EnableWindow(GetDlgItem(hwnd, IDC_SIZEX), bRegion);
								  EnableWindow(GetDlgItem(hwnd, IDC_SIZEY), bRegion);
								  EnableWindow(GetDlgItem(hwnd, IDC_SELECTREGION), bRegion);

								  if (!bRegion)
								  {
									  if (captureType == 0)
										  PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MONITOR, 0), 0);
									  else
										  PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);
								  }
								  break;
		}

		case IDC_SELECTREGION:
		{
			break;
		}
		case IDC_CAPTUREMOUSE:
		{
								 ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
								 std::stringstream SourceId;
								 uint64_t VideoId = 0;
								 if (!configData->data["SourceID"].isNull())
								 {
									 SourceId << configData->data["SourceID"].asString().c_str();
									 SourceId >> VideoId;
								 }

								 IBaseVideo *source = (IBaseVideo*)VideoId;
								 BOOL bCaptureMouse = SendMessage(HWND(lParam), BM_GETCHECK, 0, 0) == BST_CHECKED;
								 if (source)
								 {
									 source->SetInt(TEXT("captureMouse"), bCaptureMouse);
								 }
		}
			break;
		case IDC_QUALITY:
		{
							BOOL bChecked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;
							EnableWindow(GetDlgItem(hwnd, IDC_GAMMA), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_REDEFAULT), bChecked);

							ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *source = (IBaseVideo*)VideoId;
							if (!bChecked && source)
							{
								source->SetInt(TEXT("gamma"), 100);
								source->SetInt(TEXT("opacity"), 100);
							}
							else if (source)
							{
								int opacity = (int)SendMessage(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), TBM_GETPOS, 0, 0);
								int gamma = (int)SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);

								source->SetInt(TEXT("opacity"), opacity);
								source->SetInt(TEXT("gamma"), gamma);
							}
		}
			break;
		case IDC_SETSTREAMSIZE:
			if (MessageBoxW(hwnd, L"Sources.SoftwareCaptureSource.ResizeWarning", L"Sources.SoftwareCaptureSource.ResizeWarningTitle", MB_YESNO | MB_ICONWARNING) == IDYES)
			{
				ConfigProcTopWindowSourceInfo *info = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

				UINT sizeX = (UINT)GetEditText(GetDlgItem(hwnd, IDC_SIZEX)).ToInt();
				UINT sizeY = (UINT)GetEditText(GetDlgItem(hwnd, IDC_SIZEY)).ToInt();

				if (sizeX < 128)
					sizeX = 128;
				else if (sizeX > 4096)
					sizeX = 4096;

				if (sizeY < 128)
					sizeY = 128;
				else if (sizeY > 4096)
					sizeY = 4096;

// 				AppConfig->SetInt(TEXT("Video"), TEXT("BaseWidth"), sizeX);
// 				AppConfig->SetInt(TEXT("Video"), TEXT("BaseHeight"), sizeY);
// 
// 				if (!App->IsRunning())
// 					App->ResizeWindow(false);

				info->sizeSet = true;
			}
			break;

		case IDC_MONITOR:
		{
							if (!lParam || HIWORD(wParam) == CBN_SELCHANGE) {
								UINT id = (UINT)SendMessage(GetDlgItem(hwnd, IDC_MONITOR), CB_GETCURSEL, 0, 0);
								const MonitorInfo &monitor = GetMonitor(id);

								UINT x, y, cx, cy;
								x = monitor.rect.left;
								y = monitor.rect.top;
								cx = monitor.rect.right - monitor.rect.left;
								cy = monitor.rect.bottom - monitor.rect.top;

								if (cx < 16)
									cx = 16;
								if (cy < 16)
									cy = 16;

								SetWindowText(GetDlgItem(hwnd, IDC_POSX), IntString(x).Array());
								SetWindowText(GetDlgItem(hwnd, IDC_POSY), IntString(y).Array());
								SetWindowText(GetDlgItem(hwnd, IDC_SIZEX), IntString(cx).Array());
								SetWindowText(GetDlgItem(hwnd, IDC_SIZEY), IntString(cy).Array());
							}
							break;
		}

		case IDC_WINDOW:
		case IDC_OUTERWINDOW:
		case IDC_INNERWINDOW:
			//SelectTargetWindow(hwnd, HIWORD(wParam) == CBN_SELCHANGE);
			break;

		case IDC_REFRESH:
		{
							ConfigProcTopWindowSourceInfo *info = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
							Value &data = info->data;

							String lpProcName = L"";

							if (!data["ProcName"].isNull())
								lpProcName = Asic2WChar(data["ProcName"].asString().c_str()).c_str();

							HWND hwndWindowList = GetDlgItem(hwnd, IDC_WINDOW);
							RefreshPorcIDList(hwndWindowList, info->ProcIDs);

							UINT ProcIndex = 0;
							if (!lpProcName.IsEmpty())
								ProcIndex = (UINT)SendMessage(hwndWindowList, CB_FINDSTRINGEXACT, -1, (LPARAM)lpProcName.Array());

							if (ProcIndex != CB_ERR)
								SendMessage(hwndWindowList, CB_SETCURSEL, ProcIndex, 0);
							else
							{
								//SendMessage(hwndWindowList, CB_INSERTSTRING, 0, (LPARAM)lpProcName);
								SendMessage(hwndWindowList, CB_SETCURSEL, 0, 0);
							}
							break;
		}

		case IDC_USECOLORKEY:
		{
								HWND hwndUseColorKey = (HWND)lParam;
								BOOL bUseColorKey = SendMessage(hwndUseColorKey, BM_GETCHECK, 0, 0) == BST_CHECKED;

								ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
								
								std::stringstream SourceId;
								uint64_t VideoId = 0;
								if (!configData->data["SourceID"].isNull())
								{
									SourceId << configData->data["SourceID"].asString().c_str();
									SourceId >> VideoId;
								}

								IBaseVideo *source = (IBaseVideo*)VideoId;
								if (source)
									source->SetInt(TEXT("useColorKey"), bUseColorKey);

								EnableWindow(GetDlgItem(hwnd, IDC_COLOR), bUseColorKey);
								EnableWindow(GetDlgItem(hwnd, IDC_SELECT), bUseColorKey);
								EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT), bUseColorKey);
								EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD), bUseColorKey);
								EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT), bUseColorKey);
								EnableWindow(GetDlgItem(hwnd, IDC_BLEND), bUseColorKey);
								break;
		}

		case IDC_COLOR:
		{
						  ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
						  std::stringstream SourceId;
						  uint64_t VideoId = 0;
						  if (!configData->data["SourceID"].isNull())
						  {
							  SourceId << configData->data["SourceID"].asString().c_str();
							  SourceId >> VideoId;
						  }

						  IBaseVideo *source = (IBaseVideo*)VideoId;

						  if (source)
						  {
							  DWORD color = CCGetColor((HWND)lParam);
							  source->SetInt(TEXT("keyColor"), color);
						  }
						  break;
		}

		case IDC_SELECT:
		{
						   if (!bSelectingColor)
						   {
							   if (colorData.Init())
							   {
								   bMouseDown = false;
								   bSelectingColor = true;
								   SetCapture(hwnd);

								   ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
								   std::stringstream SourceId;
								   uint64_t VideoId = 0;
								   if (!configData->data["SourceID"].isNull())
								   {
									   SourceId << configData->data["SourceID"].asString().c_str();
									   SourceId >> VideoId;
								   }

								   IBaseVideo *source = (IBaseVideo*)VideoId;
								   if (source)
									   source->SetInt(TEXT("useColorKey"), false);
							   }
							   else
								   colorData.Clear();
						   }
						   break;
		}

		case IDC_REDEFAULT:
		{
							  SendMessage(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), TBM_SETPOS, TRUE, 100);
							  SetWindowText(GetDlgItem(hwnd, IDC_OPACIYT_VALUE), FormattedString(TEXT("%d"), 100));
							  SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_SETPOS, TRUE, 100);
							  SetSliderText(hwnd, IDC_GAMMA, IDC_GAMMAVAL);
							  ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
							  std::stringstream SourceId;
							  uint64_t VideoId = 0;
							  if (!configData->data["SourceID"].isNull())
							  {
								  SourceId << configData->data["SourceID"].asString().c_str();
								  SourceId >> VideoId;
							  }

							  IBaseVideo *source = (IBaseVideo*)VideoId;
							  if (source)
							  {
								  source->SetInt(TEXT("gamma"), 100);
								  source->SetInt(TEXT("opacity"), 100);
							  }
		}

	   break;

		case IDC_OPACITY_EDIT:
		case IDC_BASETHRESHOLD_EDIT:
		case IDC_BLEND_EDIT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
				if (configData)
				{
					std::stringstream SourceId;
					uint64_t VideoId = 0;
					if (!configData->data["SourceID"].isNull())
					{
						SourceId << configData->data["SourceID"].asString().c_str();
						SourceId >> VideoId;
					}

					IBaseVideo *source = (IBaseVideo*)VideoId;

					if (source)
					{
						HWND hwndVal = NULL;
						switch (LOWORD(wParam))
						{
						case IDC_BASETHRESHOLD_EDIT:    hwndVal = GetDlgItem(hwnd, IDC_BASETHRESHOLD); break;
						case IDC_BLEND_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_BLEND); break;
						case IDC_OPACITY_EDIT:          hwndVal = GetDlgItem(hwnd, IDC_OPACITY2); break;
						}

						int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
						switch (LOWORD(wParam))
						{
						case IDC_BASETHRESHOLD_EDIT:    source->SetInt(TEXT("keySimilarity"), val); break;
						case IDC_BLEND_EDIT:            source->SetInt(TEXT("keyBlend"), val); break;
						case IDC_OPACITY_EDIT:          
						{
												 BOOL bChecked = SendMessage(GetDlgItem(hwnd, IDC_QUALITY), BM_GETCHECK, 0, 0) == BST_CHECKED;
												 if (bChecked)
													 source->SetInt(TEXT("opacity"), val);
												 else
												 {
													 source->SetInt(TEXT("opacity"), 100);
												 }
												 break;
						}
						}
					}
				}
			}
			break;

		case IDOK:
		{
					 ConfigProcTopWindowSourceInfo *info = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

					 UINT captureType = 0;

					 if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE) {
						 if (SendMessage(GetDlgItem(hwnd, IDC_MONITORCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED)
							 captureType = 0;
						 else if (SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED)
							 captureType = 1;
					 }
					 else if (info->dialogID == IDD_CONFIGUREMONITORCAPTURE) {
						 captureType = 0;
					 }
					 else { //IDD_CONFIGUREWINDOWCAPTURE
						 captureType = 1;
					 }

					 BOOL bRegion = false;// (SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED);

					 UINT monitorID = 0;

					 if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE || info->dialogID == IDD_CONFIGUREMONITORCAPTURE)
						 monitorID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_MONITOR), CB_GETCURSEL, 0, 0);
					 if (monitorID == CB_ERR) monitorID = 0;

					 UINT ProcIndex = 0;
					 if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE || info->dialogID == IDD_CONFIGUREWINDOWCAPTURE)
						 ProcIndex = (UINT)SendMessage(GetDlgItem(hwnd, IDC_WINDOW), CB_GETCURSEL, 0, 0);
					 if (ProcIndex == CB_ERR) ProcIndex = 0;

					 if (captureType == 1 && ProcIndex >= info->ProcIDs.Num())
						 break;

					 BOOL bInnerWindow = FALSE;
					 String strProcName;

					 if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE || info->dialogID == IDD_CONFIGUREWINDOWCAPTURE) {
						 strProcName = GetCBText(GetDlgItem(hwnd, IDC_WINDOW), ProcIndex);
						 bInnerWindow = SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 }

					 UINT posX, posY, sizeX, sizeY;
					 posX = GetEditText(GetDlgItem(hwnd, IDC_POSX)).ToInt();
					 posY = GetEditText(GetDlgItem(hwnd, IDC_POSY)).ToInt();
					 sizeX = GetEditText(GetDlgItem(hwnd, IDC_SIZEX)).ToInt();
					 sizeY = GetEditText(GetDlgItem(hwnd, IDC_SIZEY)).ToInt();

					 if (sizeX < 16)
						 sizeX = 16;
					 if (sizeY < 16)
						 sizeY = 16;

					 BOOL bCaptureLayered = FALSE;
					 BOOL bCaptureMouse = SendMessage(GetDlgItem(hwnd, IDC_CAPTUREMOUSE), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 //if (info->dialogID == IDD_CONFIGUREDESKTOPSOURCE || info->dialogID == IDD_CONFIGUREMONITORCAPTURE)
					 bCaptureLayered = SendMessage(GetDlgItem(hwnd, IDC_CAPTURELAYERED), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bCompatibilityMode = SendMessage(GetDlgItem(hwnd, IDC_COMPATIBILITYMODE), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bUsePointFiltering = SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_GETCHECK, 0, 0) == BST_CHECKED;

					 //---------------------------------

					 Value &data = info->data;

					 data["captureType"] = captureType;

					 data["monitor"] = monitorID;

					 if (captureType == 1)
					 {
						 data["ProcName"] = WcharToAnsi(strProcName.Array()).c_str();
						 data["ProcID"] = (UINT)info->ProcIDs[ProcIndex];
					 }

					 data["innerWindow"] = bInnerWindow;

					 data["regionCapture"] = bRegion;

					 data["captureMouse"] = bCaptureMouse ? 1 : 0;

					 data["captureLayered"] = bCaptureLayered;

					 data["compatibilityMode"] = bCompatibilityMode;

					 data["usePointFiltering"] = bUsePointFiltering;

					 data["captureX"] = posX;
					 data["captureY"] = posY;
					 data["captureCX"] = sizeX;
					 data["captureCY"] = sizeY;

					 //---------------------------------

					 BOOL  bUseColorKey = SendMessage(GetDlgItem(hwnd, IDC_USECOLORKEY), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 DWORD keyColor = CCGetColor(GetDlgItem(hwnd, IDC_COLOR));
					 UINT  keySimilarity = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_GETPOS32, 0, 0);
					 UINT  keyBlend = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_GETPOS32, 0, 0);

					 data["useColorKey"] =  bUseColorKey;
					 data["keyColor"] = (UINT)keyColor;
					 data["keySimilarity"] = keySimilarity;
					 data["keyBlend"] = keyBlend;

					 //---------------------------------

					 BOOL bChecked = SendMessage(GetDlgItem(hwnd, IDC_QUALITY), BM_GETCHECK, 0, 0) == BST_CHECKED;

					 data["UseQuality"] = bChecked ? 1 : 0;

					 //---------------------------------

					// UINT opacity = (UINT)SendMessage(GetDlgItem(hwnd, IDC_OPACITY2), UDM_GETPOS32, 0, 0);
					 int opacity = (int)SendMessage(GetDlgItem(hwnd, IDC_OPACITY_SLIDER), TBM_GETPOS, 0, 0);
					 data["opacity"] = opacity;

					 //---------------------------------

					 int gamma = (int)SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);
					 data["gamma"] = gamma;

					 if (captureType == 0 && OSGetVersion() < 8)
					 {
						 BOOL bComposition;
						 DwmIsCompositionEnabled(&bComposition);

						 if (bComposition)
							 BLiveMessageBox(hwnd, pluginLocale->LookupString(L"Sources.SoftwareCaptureSource.WarningAero"), pluginLocale->LookupString(TEXT("Warning")), MB_ICONEXCLAMATION);
					 }
		}

		case IDCANCEL:
			if (LOWORD(wParam) == IDCANCEL)
			{
				ConfigProcTopWindowSourceInfo *configData = (ConfigProcTopWindowSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configData->data["SourceID"].isNull())
				{
					SourceId << configData->data["SourceID"].asString().c_str();
					SourceId >> VideoId;
				}

				IBaseVideo *source = (IBaseVideo*)VideoId;
				if (source)
				{
					Value &data = configData->data;
					source->SetInt(TEXT("useColorKey"), data["useColorKey"].asInt());

					UINT Color = 0xFFFFFFFF;

					if (!data["keyColor"].isNull())
					{
						Color = data["keyColor"].asUInt();
					}
					source->SetInt(TEXT("keyColor"), Color);
					int Tem = 10;

					if (!data["keySimilarity"].isNull())
						Tem = data["keySimilarity"].asInt();
					source->SetInt(TEXT("keySimilarity"), Tem);

					source->SetInt(TEXT("keyBlend"), data["keyBlend"].asInt());

					Tem = 100;
					if (!data["opacity"].isNull())
						Tem = data["opacity"].asInt();
					source->SetInt(TEXT("opacity"), Tem);

					Tem = 100;

					if (!data["gamma"].asInt())
						Tem = data["gamma"].asInt();
					source->SetInt(TEXT("gamma"), Tem);
				}
			}

			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
	}

	return FALSE;
}

bool bCaptureRegionClass = false;

static bool STDCALL ConfigProcTopWindowSourceProc2(Value &element, bool bInitialize, int dialogID)
{
	if (!bCaptureRegionClass)
	{
		WNDCLASS wc;
		zero(&wc, sizeof(wc));
		wc.hInstance = hinstMain;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = PROCCAPTUREREGIONCLASS;
		wc.lpfnWndProc = (WNDPROC)RegionWindowProcLocal;
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		RegisterClass(&wc);
		InitColorControl(hinstMain);
		bCaptureRegionClass = true;

		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(WINDOWCAPTURELOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(WINDOWCAPTURELOCALPATH).c_str());
		}
	}

	ConfigProcTopWindowSourceInfo info(element);
	info.dialogID = dialogID;

	if (BLiveDialogBox(hinstMain, MAKEINTRESOURCE(dialogID), GetMainWindow(), ConfigProcTopWindowSourceProc, (LPARAM)&info) == IDOK)
	{
		return true;
	}
	else {
// 		if (info.sizeSet) {
// 			AppConfig->SetInt(TEXT("Video"), TEXT("BaseWidth"), info.prevCX);
// 			AppConfig->SetInt(TEXT("Video"), TEXT("BaseHeight"), info.prevCY);
// 
// 			if (!App->IsRunning())
// 				App->ResizeWindow(false);
// 		}
	}

	return false;
}

bool STDCALL ConfigureProcTopWindowSource(Value &element, bool bInitialize)
{
	return ConfigProcTopWindowSourceProc2(element, bInitialize, IDD_CONFIGUREWINDOWCAPTURE);
}

REGINST_CONFIGFUNC(ProcTopWindowSource, ConfigureProcTopWindowSource)


