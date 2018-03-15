/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.LoadPlugin

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/

#include "AudioPlugin.h"
#include <sstream>
#include "CGdiPlusImage.h"
LocaleStringLookup *pluginLocale = NULL;
#define PluginStr(param)  pluginLocale->LookupString(TEXT2(param))
HINSTANCE hinstMain = NULL;

WNDPROC AudioOKProc = NULL;
WNDPROC AudioCancelProc = NULL;
WNDPROC denoiseeditproc = NULL;
static HWND Hwnd = NULL;

#define AUDIO_CLASSNAME TEXT("AudioCapture")

#define  UPTATEEDIT (WM_USER + 43327)
#define  UPTATESLIDER (WM_USER + 43328)
#define  HIGHLIGHT (WM_USER + 43329)

struct ConfigDialogData
{
    CTSTR lpName;
    Value &data;
    StringList audioNameList;
	StringList audioIDList;

	ConfigDialogData(Value &e) :data(e){}

	bool bGlobalSource;
    bool bCreating;
	bool bButtonDownFlag = false;
	bool bDenoiseCheckFlag = false;
	bool bDisable = false;
	bool m_bMouseTrack = FALSE;

	int nPos = 65;
	int nDB = -30;
	int nCurrentSelect = 0;
	HBRUSH HBrushGreen = NULL;
	HBRUSH HBrushBlack = NULL;
	HBRUSH HBrushBackGround = NULL;
	CGdiPlusImage* m_pSilderImage = NULL;
	CGdiPlusImage* m_pDenoiseCheckedImage = NULL;
	CGdiPlusImage* m_pDenoiseUncheckedImage = NULL;
	CGdiPlusImage* m_pDenoiseUpposImage = NULL;
	CGdiPlusImage* m_pDenoiseDownposImage = NULL;
	CGdiPlusImage* m_pDenoiseOkImage = NULL;
	CGdiPlusImage* m_pDenoiseOkHoverImage = NULL;
	CGdiPlusImage* m_pDenoiseCancelImage = NULL;
	CGdiPlusImage* m_pDenoiseCancelHoverImage = NULL;
};

//#include <Windows.h>  
//#pragma comment(lib, "winmm.lib")

ConfigDialogData *pGlobalConfigData = NULL;

LRESULT CALLBACK DenoiseSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT meterGray;
					 meterGray.left = 0;
					 meterGray.right = 130;
					 meterGray.bottom = 0;
					 meterGray.top = 40;
					 FillRect(hdc, &meterGray, configData->HBrushBackGround);

					 meterGray.left = 0;
					 meterGray.right = configData->nPos;
					 meterGray.bottom = 18;
					 meterGray.top = 22;
					 FillRect(hdc, &meterGray, configData->HBrushGreen);

					 meterGray.left = configData->nPos;
					 meterGray.right = 130;
					 meterGray.bottom = 18;
					 meterGray.top = 22;
					 FillRect(hdc, &meterGray, configData->HBrushBlack);

					 Graphics graphics(hdc);
					 INT nImageWidth = configData->m_pSilderImage->GetWidth();
					 INT nImageHeight = configData->m_pSilderImage->GetHeight();

					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)configData->nPos - 20;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 graphics.DrawImage(configData->m_pSilderImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 EndPaint(hwnd, &ps);
					 break;
	}

	case WM_LBUTTONDOWN:
	{
						   ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						   configData->bButtonDownFlag = true;
						   SetCapture(hwnd);
						   break;
	}
	case WM_LBUTTONUP:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 configData->bButtonDownFlag = false;
						 ReleaseCapture();
						 break;
	}
	case WM_MOUSEMOVE:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 if (!configData->bButtonDownFlag)
						 {
							 break;
						 }
						 int yPos = HIWORD(lParam);
						 int xPos = LOWORD(lParam);
						 
						  if (xPos > 125 && xPos < 5000)
						  {
						  xPos = 125;
						  }
						  else if (xPos < 5 || xPos > 4999)
						  {
						  xPos = 5;
						  }


						 HDC hDC = GetDC(hwnd);

						 HDC hdcTemp = CreateCompatibleDC(hDC);
						 HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, 130, 40);
						 HBITMAP oldhbmpTemp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);


						 configData->nPos = xPos;
						 RECT meterGray;

						 meterGray.left = 0;
						 meterGray.right = 130;
						 meterGray.bottom = 0;
						 meterGray.top = 40;
						 FillRect(hdcTemp, &meterGray, configData->HBrushBackGround);

						 meterGray.left = 0;
						 meterGray.right = xPos;
						 meterGray.bottom = 18;
						 meterGray.top = 22;
						 FillRect(hdcTemp, &meterGray, configData->HBrushGreen);

						 meterGray.left = xPos;
						 meterGray.right = 130;
						 meterGray.bottom = 18;
						 meterGray.top = 22;
						 FillRect(hdcTemp, &meterGray, configData->HBrushBlack);

						 Graphics graphics(hdcTemp);

						 INT nImageWidth = configData->m_pSilderImage->GetWidth();
						 INT nImageHeight = configData->m_pSilderImage->GetHeight();

						 //构造位置
						 RectF rcDrawRect;
						 //绘画图像
						 rcDrawRect.X = (REAL)xPos-20;
						 rcDrawRect.Y = (REAL)0;
						 rcDrawRect.Width = (REAL)nImageWidth;
						 rcDrawRect.Height = (REAL)nImageHeight;
						 graphics.DrawImage(configData->m_pSilderImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);

						 BitBlt(hDC, 0, 0, 130, 40, hdcTemp, 0, 0, SRCCOPY);

						 SelectObject(hdcTemp, oldhbmpTemp);
						 DeleteObject(hbmpTemp);
						 DeleteObject(hdcTemp);

						 ReleaseDC(hwnd, hDC);
						 configData->nPos = xPos;
						 if (configData->nPos <= 5)
						 {
							 configData->nDB = -60;
						 }
						 else if (configData->nPos >= 125)
						 {
							 configData->nDB = 0;
						 }
						 else
						 {
							 float nTmp = configData->nPos * 0.5;
							 configData->nDB = (int)nTmp - 65;
							 if (configData->nDB < -60)
							 {
								 configData->nDB = -60;
							 }
						 }

						 std::stringstream SourceId;
						 uint64_t VideoId = 0;
						 if (!configData->data["SourceID"].isNull())
						 {
							 SourceId << configData->data["SourceID"].asString().c_str();
							 SourceId >> VideoId;
						 }

						 IBaseVideo *Video = (IBaseVideo*)VideoId;

						 if (Video && configData->bDenoiseCheckFlag)
							 Video->SetFloat(TEXT("Denoise"), configData->nDB);

						 SendMessage(GetDlgItem(GetParent(hwnd), IDC_DENOISEEDIT), UPTATEEDIT, 0, 0);
						 break;
	}
	
	case UPTATESLIDER:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
		
						 HDC hDC = GetDC(hwnd);

						 HDC hdcTemp = CreateCompatibleDC(hDC);
						 HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, 130, 40);
						 HBITMAP oldhbmpTemp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);

						 RECT meterGray;

						 meterGray.left = 0;
						 meterGray.right = 130;
						 meterGray.bottom = 0;
						 meterGray.top = 40;
						 FillRect(hdcTemp, &meterGray, configData->HBrushBackGround);

						 meterGray.left = 0;
						 meterGray.right = configData->nPos;
						 meterGray.bottom = 18;
						 meterGray.top = 22;
						 FillRect(hdcTemp, &meterGray, configData->HBrushGreen);

						 meterGray.left = configData->nPos;
						 meterGray.right = 130;
						 meterGray.bottom = 18;
						 meterGray.top = 22;
						 FillRect(hdcTemp, &meterGray, configData->HBrushBlack);

						 Graphics graphics(hdcTemp);

						 INT nImageWidth = configData->m_pSilderImage->GetWidth();
						 INT nImageHeight = configData->m_pSilderImage->GetHeight();

						 //构造位置
						 RectF rcDrawRect;
						 //绘画图像
						 rcDrawRect.X = (REAL)configData->nPos -20;
						 rcDrawRect.Y = (REAL)0;
						 rcDrawRect.Width = (REAL)nImageWidth;
						 rcDrawRect.Height = (REAL)nImageHeight;
						 graphics.DrawImage(configData->m_pSilderImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);

						 BitBlt(hDC, 0, 0, 130, 40, hdcTemp, 0, 0, SRCCOPY);

						 SelectObject(hdcTemp, oldhbmpTemp);
						 DeleteObject(hbmpTemp);
						 DeleteObject(hdcTemp);

						 ReleaseDC(hwnd, hDC);
						
						 break;
	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DenoiseCheckProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT meterGray;
					  meterGray.left = 0;
					  meterGray.right = 40;
					  meterGray.bottom = 0;
					  meterGray.top = 40;

					  FillRect(hdc, &meterGray, configData->HBrushBackGround);

					 Graphics graphics(hdc);
					 INT nImageWidth = 0;
					 INT nImageHeight = 0;
					 if (configData->bDenoiseCheckFlag)
					 {
						 nImageWidth = configData->m_pDenoiseCheckedImage->GetWidth();
						 nImageHeight = configData->m_pDenoiseCheckedImage->GetHeight();
					 }
					 else
					 {
						 nImageWidth = configData->m_pDenoiseUncheckedImage->GetWidth();
						 nImageHeight = configData->m_pDenoiseUncheckedImage->GetHeight();
					 }


					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 if (configData->bDenoiseCheckFlag)
						 graphics.DrawImage(configData->m_pDenoiseCheckedImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 else
					 {
						 //graphics.DrawImage(configData->m_pDenoiseUncheckedImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						 HPEN hPen;
						 HPEN hPenOld;
						 hPen = CreatePen(PS_SOLID, 1, (COLORREF)0x5f5f61);
						 hPenOld = (HPEN)SelectObject(hdc, hPen);
						 MoveToEx(hdc, 12, 12, NULL);
						 LineTo(hdc, 28, 12);
						 MoveToEx(hdc, 28, 12, NULL);
						 LineTo(hdc, 28, 28);
						 MoveToEx(hdc, 12, 28, NULL);
						 LineTo(hdc, 28, 28);
						 MoveToEx(hdc, 12, 12, NULL);
						 LineTo(hdc, 12, 28);
						 SelectObject(hdc, hPenOld);
						 DeleteObject(hPen);
					 }
					 EndPaint(hwnd, &ps);
					 break;
	}

	case WM_LBUTTONDOWN:
	{
						   ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						  
						   break;
	}
	case WM_LBUTTONUP:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 configData->bDenoiseCheckFlag = !configData->bDenoiseCheckFlag;
						 HDC hDC = GetDC(hwnd);

						 HDC hdcTemp = CreateCompatibleDC(hDC);
						 HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, 40, 40);
						 HBITMAP oldhbmpTemp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);

						 RECT meterGray;

						 meterGray.left = 0;
						 meterGray.right = 40;
						 meterGray.bottom = 0;
						 meterGray.top = 40;
						 FillRect(hdcTemp, &meterGray, configData->HBrushBackGround);


						 Graphics graphics(hdcTemp);

						 INT nImageWidth = 0;
						 INT nImageHeight = 0;
						 if (configData->bDenoiseCheckFlag)
						 {
							 nImageWidth = configData->m_pDenoiseCheckedImage->GetWidth();
							 nImageHeight = configData->m_pDenoiseCheckedImage->GetHeight();
						 }
						 else
						 {
							 nImageWidth = configData->m_pDenoiseUncheckedImage->GetWidth();
							 nImageHeight = configData->m_pDenoiseUncheckedImage->GetHeight();
						 }

						 RectF rcDrawRect;
						 rcDrawRect.X = (REAL)0;
						 rcDrawRect.Y = (REAL)0;
						 rcDrawRect.Width = (REAL)nImageWidth;
						 rcDrawRect.Height = (REAL)nImageHeight;
						 if (configData->bDenoiseCheckFlag)
							 graphics.DrawImage(configData->m_pDenoiseCheckedImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);

						 BitBlt(hDC, 0, 0, 40, 40, hdcTemp, 0, 0, SRCCOPY);

						 SelectObject(hdcTemp, oldhbmpTemp);
						 DeleteObject(hbmpTemp);
						 DeleteObject(hdcTemp);
						 if (!configData->bDenoiseCheckFlag)
						 {
							 //graphics.DrawImage(configData->m_pDenoiseUncheckedImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							 HPEN hPen;
							 HPEN hPenOld;
							 hPen = CreatePen(PS_SOLID, 1, (COLORREF)0x5f5f61);
							 hPenOld = (HPEN)SelectObject(hDC, hPen);
							 MoveToEx(hDC, 12, 12, NULL);
							 LineTo(hDC, 28, 12);
							 MoveToEx(hDC, 28, 12, NULL);
							 LineTo(hDC, 28, 28);
							 MoveToEx(hDC, 12, 28, NULL);
							 LineTo(hDC, 28, 28);
							 MoveToEx(hDC, 12, 12, NULL);
							 LineTo(hDC, 12, 28);
							 SelectObject(hDC, hPenOld);
							 DeleteObject(hPen);
						 }
						 ReleaseDC(hwnd, hDC);
						 break;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DenoiseUpposProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);
					 Graphics graphics(hdc);
					 INT nImageWidth = 0;
					 INT nImageHeight = 0;
					
					 nImageWidth = configData->m_pDenoiseUpposImage->GetWidth();
					 nImageHeight = configData->m_pDenoiseUpposImage->GetHeight();
					
					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 graphics.DrawImage(configData->m_pDenoiseUpposImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 EndPaint(hwnd, &ps);
					 break;
	}

	case WM_LBUTTONDOWN:
	{
						   break;
	}
	case WM_LBUTTONUP:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 configData->nDB++;
						 if (configData->nDB > 0)
						 {
							 configData->nDB = 0;
						 }
						 if (configData->nDB < -60)
						 {
							 configData->nDB = -60;
						 }
						 configData->nPos = configData->nDB * 2 + 125;
						 SendMessage(GetDlgItem(GetParent(hwnd), ID_DENOISESLIDER), UPTATESLIDER, 0, 0);
						 SendMessage(GetDlgItem(GetParent(hwnd), IDC_DENOISEEDIT), UPTATEEDIT, 0, 0);
						 break;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DenoiseDownposProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);
					 Graphics graphics(hdc);
					 INT nImageWidth = 0;
					 INT nImageHeight = 0;

					 nImageWidth = configData->m_pDenoiseDownposImage->GetWidth();
					 nImageHeight = configData->m_pDenoiseDownposImage->GetHeight();

					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 graphics.DrawImage(configData->m_pDenoiseDownposImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 EndPaint(hwnd, &ps);
					 break;
	}

	case WM_LBUTTONDOWN:
	{
						   break;
	}
	case WM_LBUTTONUP:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 configData->nDB--;
						 if (configData->nDB > 0)
						 {
							 configData->nDB = 0;
						 }
						 if (configData->nDB < -60)
						 {
							 configData->nDB = -60;
						 }
						 configData->nPos = configData->nDB * 2 + 125;
						 SendMessage(GetDlgItem(GetParent(hwnd), ID_DENOISESLIDER), UPTATESLIDER, 0, 0);
						 SendMessage(GetDlgItem(GetParent(hwnd), IDC_DENOISEEDIT), UPTATEEDIT, 0, 0);
						 break;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DenoiseEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
		case UPTATEEDIT:
		{
			ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
			
			TCHAR NumberVal[10] = { 0 };
			HDC hDC = GetDC(hwnd);
			//SetBkColor(hDC, RGB(153, 153, 153));
			//SetTextColor(hDC, RGB(255, 255, 255));
			wsprintf(NumberVal, L"%d", configData->nDB);
			SetWindowText(hwnd, NumberVal);
			ReleaseDC(hwnd, hDC);
			break;
		}
		

	default:
		return CallWindowProc(denoiseeditproc, hwnd, message, wParam, lParam);
	}
	return 0;
}

void InitDenoiseSlider() {

	WNDCLASS wnd;

	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = sizeof(LPVOID);
	wnd.hbrBackground = NULL;
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hIcon = NULL;
	wnd.hInstance = hinstMain;
	wnd.lpfnWndProc = DenoiseSliderProc;
	wnd.lpszClassName = DENOISE_SLIDER_CLASS;
	wnd.lpszMenuName = NULL;
	wnd.style = CS_PARENTDC | CS_VREDRAW | CS_HREDRAW;

	if (!RegisterClass(&wnd))
		CrashError(TEXT("Could not register LeftVolumeShow class"));
}

INT_PTR CALLBACK WindowButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);
					 Graphics graphics(hdc);
					 INT nImageWidth = configData->m_pDenoiseOkHoverImage->GetWidth();
					 INT nImageHeight = configData->m_pDenoiseOkHoverImage->GetHeight();

					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
					 {
						 graphics.DrawImage(configData->m_pDenoiseOkImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 }
					 else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
					 {
						 graphics.DrawImage(configData->m_pDenoiseCancelImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 }


					 SetTextColor(hdc, RGB(255, 255, 255));

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
					 break;
	}
	case WM_MOUSEHOVER:
	{
						  ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						  if (!configData->m_bMouseTrack)
						  {
							  HDC hDC = GetDC(hwnd);
							  Graphics graphics(hDC);
							  INT nImageWidth = configData->m_pDenoiseOkHoverImage->GetWidth();
							  INT nImageHeight = configData->m_pDenoiseOkHoverImage->GetHeight();

							  //构造位置
							  RectF rcDrawRect;
							  rcDrawRect.X = (REAL)0;
							  rcDrawRect.Y = (REAL)0;
							  rcDrawRect.Width = (REAL)nImageWidth;
							  rcDrawRect.Height = (REAL)nImageHeight;

							  //绘画图像
							  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
							  {
								  graphics.DrawImage(configData->m_pDenoiseOkHoverImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							  }
							  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
							  {
								  graphics.DrawImage(configData->m_pDenoiseCancelHoverImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							  }

							  SetTextColor(hDC, RGB(255, 255, 255));

							  SetBkColor(hDC, RGB(102, 102, 102));
							  SetBkMode(hDC, TRANSPARENT);

							  SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

							  TCHAR Title[MAX_PATH] = { 0 };

							  GetWindowText(hwnd, Title, sizeof Title);

							  RECT rtClient;
							  GetClientRect(hwnd, &rtClient);

							  if (wcslen(Title) > 0)
							  {
								  DrawText(hDC, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
							  }

							  ReleaseDC(hwnd, hDC);
						  }
						  break;
	}

	case WM_MOUSEMOVE:
	{
						 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						 TRACKMOUSEEVENT csTME;
						 csTME.cbSize = sizeof (csTME);
						 csTME.dwFlags = TME_LEAVE | TME_HOVER;
						 csTME.hwndTrack = hwnd;// 指定要 追踪 的窗口 
						 csTME.dwHoverTime = 10;  // 鼠标在按钮上停留超过 10ms ，才认为状态为 HOVER
						 ::_TrackMouseEvent(&csTME); // 开启 Windows 的 WM_MOUSELEAVE ， WM_MOUSEHOVER 事件支持

						 configData->m_bMouseTrack = FALSE;   // 若已经 追踪 ，则停止 追踪 
						 break;
	}
	case WM_MOUSELEAVE:
	{
						  ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						  configData->m_bMouseTrack = TRUE;
						  //改变状态
						  HDC hDC = GetDC(hwnd);
						  RECT meterGray;
						  HBRUSH  HBrushBackGround = CreateSolidBrush(RGB(42, 42, 44));
						  meterGray.left = 0;
						  meterGray.right = 100;
						  meterGray.bottom = 0;
						  meterGray.top = 36;
						  FillRect(hDC, &meterGray, HBrushBackGround);
						  DeleteObject(HBrushBackGround);

						  Graphics graphics(hDC);
						  INT nImageWidth = configData->m_pDenoiseOkHoverImage->GetWidth();
						  INT nImageHeight = configData->m_pDenoiseOkHoverImage->GetHeight();

						  //构造位置
						  RectF rcDrawRect;
						  rcDrawRect.X = (REAL)0;
						  rcDrawRect.Y = (REAL)0;
						  rcDrawRect.Width = (REAL)nImageWidth;
						  rcDrawRect.Height = (REAL)nImageHeight;

						  //绘画图像
						  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
						  {
							  graphics.DrawImage(configData->m_pDenoiseOkImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						  }
						  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
						  {
							  graphics.DrawImage(configData->m_pDenoiseCancelImage->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						  }
						  SetTextColor(hDC, RGB(255, 255, 255));

						  SetBkColor(hDC, RGB(102, 102, 102));
						  SetBkMode(hDC, TRANSPARENT);

						  SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

						  TCHAR Title[MAX_PATH] = { 0 };

						  GetWindowText(hwnd, Title, sizeof Title);

						  RECT rtClient;
						  GetClientRect(hwnd, &rtClient);

						  if (wcslen(Title) > 0)
						  {
							  DrawText(hDC, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
						  }

						  ReleaseDC(hwnd, hDC);
						  break;
	}

	default:
		return CallWindowProc(AudioCancelProc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

//===================================================================
const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 } };

bool GetGUIDFromString(CTSTR lpGUID, GUID &targetGUID)
{
	String strGUID = lpGUID;
	if (strGUID.Length() != 38)
		return false;

	strGUID = strGUID.Mid(1, strGUID.Length() - 1);

	StringList GUIDData;
	strGUID.GetTokenList(GUIDData, '-', FALSE);

	if (GUIDData.Num() != 5)
		return false;

	if (GUIDData[0].Length() != 8 ||
		GUIDData[1].Length() != 4 ||
		GUIDData[2].Length() != 4 ||
		GUIDData[3].Length() != 4 ||
		GUIDData[4].Length() != 12)
	{
		return false;
	}
	targetGUID.Data1 = (UINT)tstring_base_to_uint(GUIDData[0], NULL, 16);
	targetGUID.Data2 = (WORD)tstring_base_to_uint(GUIDData[1], NULL, 16);
	targetGUID.Data3 = (WORD)tstring_base_to_uint(GUIDData[2], NULL, 16);
	targetGUID.Data4[0] = (BYTE)tstring_base_to_uint(GUIDData[3].Left(2), NULL, 16);
	targetGUID.Data4[1] = (BYTE)tstring_base_to_uint(GUIDData[3].Right(2), NULL, 16);
	targetGUID.Data4[2] = (BYTE)tstring_base_to_uint(GUIDData[4].Left(2), NULL, 16);
	targetGUID.Data4[3] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(2, 4), NULL, 16);
	targetGUID.Data4[4] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(4, 6), NULL, 16);
	targetGUID.Data4[5] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(6, 8), NULL, 16);
	targetGUID.Data4[6] = (BYTE)tstring_base_to_uint(GUIDData[4].Mid(8, 10), NULL, 16);
	targetGUID.Data4[7] = (BYTE)tstring_base_to_uint(GUIDData[4].Right(2), NULL, 16);

	return true;
}


IBaseFilter* GetExceptionDevice(CTSTR lpGUID)
{
	GUID targetGUID;
	if (!GetGUIDFromString(lpGUID, targetGUID))
		return NULL;

	IBaseFilter *filter;
	if (SUCCEEDED(CoCreateInstance(targetGUID, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filter)))
		return filter;

	return NULL;
}


IBaseFilter* GetDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName, WSTR lpType2, CTSTR lpName2)
{
	//---------------------------------
	// exception devices
	if (scmpi(lpType2, L"DevicePath") == 0 && lpName2 && *lpName2 == '{')
		return GetExceptionDevice(lpName2);

	//---------------------------------

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return NULL;
	}

	err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return NULL;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices, so NO ENUM FO U
		return NULL;

	//---------------------------------

	IBaseFilter *bestFilter = NULL;

	IMoniker *deviceInfo;
	DWORD count;
	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT valueThingy;
			VARIANT valueThingy2;
			VariantInit(&valueThingy);
			VariantInit(&valueThingy2);

			if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
			{
				if (lpType2 && lpName2)
				{
					if (FAILED(propertyData->Read(lpType2, &valueThingy2, NULL)))
						nop();
				}

				SafeRelease(propertyData);

				String strVal1 = (CWSTR)valueThingy.bstrVal;

				if (strVal1 == lpName)
				{
					IBaseFilter *filter;
					err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
					if (FAILED(err))
					{
						AppWarning(TEXT("GetDeviceByValue: deviceInfo->BindToObject failed, result = %08lX"), err);
						continue;
					}

					if (!bestFilter)
					{
						bestFilter = filter;

						if (!lpType2 || !lpName2)
						{
							SafeRelease(deviceInfo);
							SafeRelease(videoDeviceEnum);

							return bestFilter;
						}
					}
					else if (lpType2 && lpName2)
					{
						String strVal2 = (CWSTR)valueThingy2.bstrVal;
						if (strVal2 == lpName2)
						{
							bestFilter->Release();

							bestFilter = filter;

							SafeRelease(deviceInfo);
							SafeRelease(videoDeviceEnum);

							return bestFilter;
						}
					}
					else
						filter->Release();
				}
			}
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return bestFilter;
}

bool PinHasMajorType(IPin *pin, const GUID *majorType)
{
	HRESULT hRes;

	IAMStreamConfig *config;
	if (SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
	{
		int count, size;
		if (SUCCEEDED(config->GetNumberOfCapabilities(&count, &size)))
		{
			BYTE *capsData = (BYTE*)Allocate_Bak(size);

			int priority = -1;
			for (int i = 0; i<count; i++)
			{
				AM_MEDIA_TYPE *pMT;
				if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
				{
					BOOL bDesiredMediaType = (pMT->majortype == *majorType);

					FreeMediaType(*pMT);
					CoTaskMemFree(pMT);

					if (bDesiredMediaType) {
						Free(capsData);
						SafeRelease(config);

						return true;
					}
				}
			}

			Free(capsData);
		}

		SafeRelease(config);
	}

	AM_MEDIA_TYPE *pinMediaType;

	IEnumMediaTypes *mediaTypesEnum;
	if (FAILED(pin->EnumMediaTypes(&mediaTypesEnum)))
		return false;

	ULONG curVal = 0;
	hRes = mediaTypesEnum->Next(1, &pinMediaType, &curVal);

	mediaTypesEnum->Release();

	if (hRes != S_OK)
		return false;

	BOOL bDesiredMediaType = (pinMediaType->majortype == *majorType);
	DeleteMediaType(pinMediaType);

	if (!bDesiredMediaType)
		return false;

	return true;
}

IPin* GetOutputPin(IBaseFilter *filter, const GUID *majorType)
{
	IPin *foundPin = NULL;
	IEnumPins *pins;

	if (!filter) return NULL;
	if (FAILED(filter->EnumPins(&pins))) return NULL;

	IPin *curPin;
	ULONG num;
	while (pins->Next(1, &curPin, &num) == S_OK)
	{
		if (majorType)
		{
			if (!PinHasMajorType(curPin, majorType))
			{
				SafeRelease(curPin);
				continue;
			}
		}

		//------------------------------

		PIN_DIRECTION pinDir;
		if (SUCCEEDED(curPin->QueryDirection(&pinDir)))
		{
			if (pinDir == PINDIR_OUTPUT)
			{
				IKsPropertySet *propertySet;
				if (SUCCEEDED(curPin->QueryInterface(IID_IKsPropertySet, (void**)&propertySet)))
				{
					GUID pinCategory;
					DWORD retSize;

					PIN_INFO chi;
					curPin->QueryPinInfo(&chi);

					if (chi.pFilter)
						chi.pFilter->Release();

					if (SUCCEEDED(propertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &retSize)))
					{
						if (pinCategory == PIN_CATEGORY_CAPTURE || pinCategory == PIN_CATEGORY_ROXIOCAPTURE)
						{
							SafeRelease(propertySet);
							SafeRelease(pins);

							return curPin;
						}
					}

					SafeRelease(propertySet);
				}
			}
		}

		SafeRelease(curPin);
	}

	SafeRelease(pins);

	return foundPin;
}


#define DEV_EXCEPTION_COUNT 1
CTSTR lpExceptionNames[DEV_EXCEPTION_COUNT] = { TEXT("Elgato Game Capture HD") };
CTSTR lpExceptionGUIDs[DEV_EXCEPTION_COUNT] = { TEXT("{39F50F4C-99E1-464a-B6F9-D605B4FB5918}") };

bool FillOutListOfDevices(HWND hwndCombo, GUID matchGUID, StringList *deviceList, StringList *deviceIDList)
{
	deviceIDList->Clear();
	deviceList->Clear();
	//if (hwndCombo != NULL) SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

	//------------------------------------------

	for (int i = 0; i<DEV_EXCEPTION_COUNT; i++)
	{
		IBaseFilter *exceptionFilter = GetExceptionDevice(lpExceptionGUIDs[i]);
		if (exceptionFilter)
		{
			deviceList->Add(lpExceptionNames[i]);
			deviceIDList->Add(lpExceptionGUIDs[i]);

			if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)lpExceptionNames[i]);

			exceptionFilter->Release();
		}
	}

	//------------------------------------------

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("FillOutListDevices: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return false;
	}

	err = deviceEnum->CreateClassEnumerator(matchGUID, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("FillOutListDevices: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return false;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices
		return false;

	//------------------------------------------

	IMoniker *deviceInfo;
	DWORD count;

// 	bool bUseConfig = true;
// 
// 	String strConfigPath = BLiveGetPluginDataPath() + "\\dshowDevices.xconfig";
// 
// 	XConfig dshowDevicesConfig;
// 	XElement *devices = NULL;
// 
// 	if (!dshowDevicesConfig.Open(strConfigPath.Array()))
// 		bUseConfig = false;
// 
// 	if (bUseConfig) {
// 		devices = dshowDevicesConfig.GetElement(TEXT("dshowDevices"));
// 		if (!devices)
// 			devices = dshowDevicesConfig.CreateElement(TEXT("dshowDevices"));
// 	}

	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT friendlyNameValue, devicePathValue;
			friendlyNameValue.vt = VT_BSTR;
			friendlyNameValue.bstrVal = NULL;
			devicePathValue.vt = VT_BSTR;
			devicePathValue.bstrVal = NULL;

			err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
			propertyData->Read(L"DevicePath", &devicePathValue, NULL);

			if (SUCCEEDED(err))
			{
				if (sstri(friendlyNameValue.bstrVal, L"Blackmagic") || sstri(friendlyNameValue.bstrVal, L"Decklink"))
				{
					if (propertyData)
						propertyData->Release();

					if (deviceInfo)
						deviceInfo->Release();

					continue;
				}

				IBaseFilter *filter;
				err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
				if (SUCCEEDED(err))
				{
					String strDeviceName = (CWSTR)friendlyNameValue.bstrVal;
					deviceList->Add(strDeviceName);

					UINT count2 = 0;
					UINT id = INVALID;
					while ((id = deviceList->FindNextValueIndexI(strDeviceName, id)) != INVALID) count2++;

					if (count2 > 1)
						strDeviceName << TEXT(" (") << UIntString(count2) << TEXT(")");

					String strDeviceID = (CWSTR)devicePathValue.bstrVal;
// 					if (bUseConfig) {
// 						XElement *chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
// 						if (!chkDevice) {
// 							if (strDeviceID.Length() != 0) {
// 								devices->CreateElement((CWSTR)devicePathValue.bstrVal);
// 								chkDevice = devices->GetElement((CWSTR)devicePathValue.bstrVal);
// 								chkDevice->AddString(TEXT("deviceName"), strDeviceName.Array());
// 							}
// 							if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
// 						}
// 						else {
// 							if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)chkDevice->GetString(TEXT("deviceName")));
// 						}
// 					}
// 					else 
					{
						if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)strDeviceName.Array());
					}
					deviceIDList->Add(strDeviceID);

					SafeRelease(filter);
				}
			}

			SafeRelease(propertyData);
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return true;
}


bool  CheckDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName)
{
	//---------------------------------

	bool  bReturn = false;

	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX"), err);
		return bReturn;
	}

	err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		AppWarning(TEXT("GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
		deviceEnum->Release();
		return bReturn;
	}

	SafeRelease(deviceEnum);

	if (err == S_FALSE) //no devices, so NO ENUM FO U
		return bReturn;

	IMoniker *deviceInfo;
	DWORD count;

	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT valueThingy;
			VARIANT valueThingy2;
			VariantInit(&valueThingy);
			VariantInit(&valueThingy2);
			/*valueThingy.vt  = VT_BSTR;
			valueThingy.pbstrVal = NULL;

			valueThingy2.vt = VT_BSTR;
			valueThingy2.bstrVal = NULL;*/

			if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
			{
				SafeRelease(propertyData);

				String strVal1 = (CWSTR)valueThingy.bstrVal;

				if (strVal1 == lpName)
				{
					bReturn = true;
					break;
				}
			}
		}

		SafeRelease(deviceInfo);
	}

	SafeRelease(videoDeviceEnum);

	return bReturn;
}

//===================================================================
WNDPROC channelListProc = NULL;
INT_PTR CALLBACK ChannelListProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_MOUSEMOVE:
		case WM_MOUSEHOVER:
		{
							  ConfigDialogData *configData = pGlobalConfigData;
							  POINT Pt;
							  Pt.x = (int)(short)LOWORD(lParam);
							  Pt.y = (int)(short)HIWORD(lParam);

							  HDC hdc = GetDC(hwnd);
							  RECT Rect;
							  GetClientRect(hwnd, &Rect);
							  RECT *rc = new RECT[configData->audioNameList.Num()];
							  int index = -1;
							  for (int i = 0; i < configData->audioNameList.Num(); i++)
							  {
								  rc[i].left = Rect.left;
								  rc[i].right = Rect.right;
								  rc[i].top = Rect.top + i * 36;
								  rc[i].bottom = rc[i].top + 36;
							  }
							  for (int i = 0; i < configData->audioNameList.Num(); i++)
							  {
								  if (PtInRect(&rc[i], Pt))
								  {
									  index = i;
									  break;
								  }
							  }
							  if (index >= 0)
							  {
								  HBRUSH HBrush = CreateSolidBrush(RGB(22, 121, 145));
								  FillRect(hdc, &rc[index], HBrush);
								  DeleteObject(HBrush);
								  SetBkMode(hdc, TRANSPARENT);

								  HFONT hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
									  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
								  HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);

								  rc[index].left += 10;
								  String strAudioDevice;
								  strAudioDevice = configData->audioNameList[index];
								  DrawText(hdc, strAudioDevice.Array(), strAudioDevice.Length(), &rc[index], DT_LEFT | DT_VCENTER | DT_SINGLELINE);

								  SelectObject(hdc, hfontOld);
								  DeleteObject(hFont);
								  hFont = NULL;
							  }

							  delete[] rc;
							  rc = NULL;
							  ReleaseDC(hwnd, hdc);
		}
		break;
		case HIGHLIGHT:
		{
						  ConfigDialogData *configData = pGlobalConfigData;
						  POINT Pt;
						  Pt.x = (int)(short)LOWORD(lParam);
						  Pt.y = (int)(short)HIWORD(lParam);

						  HDC hdc = GetDC(hwnd);
						  RECT Rect;
						  GetClientRect(hwnd, &Rect);
						  RECT *rc = new RECT[configData->audioNameList.Num()];
						  int index = -1;
						  for (int i = 0; i < configData->audioNameList.Num(); i++)
						  {
							  rc[i].left = Rect.left;
							  rc[i].right = Rect.right;
							  rc[i].top = Rect.top + i * 36;
							  rc[i].bottom = rc[i].top + 36;
						  }
						  for (int i = 0; i < configData->audioNameList.Num(); i++)
						  {
							  if (PtInRect(&rc[i], Pt))
							  {
								  index = i;
								  break;
							  }
						  }
						  index = pGlobalConfigData->nCurrentSelect;
						  if (index >= 0)
						  {
							  HBRUSH HBrush = CreateSolidBrush(RGB(22, 121, 145));
							  FillRect(hdc, &rc[index], HBrush);
							  DeleteObject(HBrush);
							  SetBkMode(hdc, TRANSPARENT);

							  HFONT hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
								  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
							  HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);

							  rc[index].left += 10;
							  String strAudioDevice;		 
							  strAudioDevice = configData->audioNameList[index];
							  DrawText(hdc, strAudioDevice.Array(), strAudioDevice.Length(), &rc[index], DT_LEFT | DT_VCENTER | DT_SINGLELINE);

							  SelectObject(hdc, hfontOld);
							  DeleteObject(hFont);
							  hFont = NULL;
						  }

						  delete[] rc;
						  rc = NULL;
						  ReleaseDC(hwnd, hdc);
						  break;
		}
	}
	return CallWindowProc(channelListProc, hwnd, message, wParam, lParam);
}

WNDPROC channelComboProc = NULL;
INT_PTR CALLBACK ChannelComboProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	COMBOBOXINFO comboInfo;
	comboInfo.cbSize = sizeof(COMBOBOXINFO);
	GetComboBoxInfo(hwnd, &comboInfo);
	switch (message)
	{
	case WM_PAINT:
	{
					 ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
					 String strAudioDevice = L"";
					 if (!configData->data["audioDeviceName"].isNull())
					 {
						 strAudioDevice = Asic2WChar(configData->data["audioDeviceName"].asString().c_str()).c_str();
					 }
					
					 if (configData->audioNameList.Num() > 0)
					 {
						 strAudioDevice = configData->audioNameList[configData->nCurrentSelect];
					 }
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT Rect;
					 GetClientRect(hwnd, &Rect);
					 HBRUSH HBrush = CreateSolidBrush(RGB(31, 31, 32));
					 FillRect(hdc, &Rect, HBrush);
					 DeleteObject(HBrush);
					 SetTextColor(hdc, RGB(255, 255, 255));
					 SetBkMode(hdc, TRANSPARENT);

					 HFONT hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
					 HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);

					 Rect.left += 10;
					 
					 DrawText(hdc, strAudioDevice.Array(), strAudioDevice.Length(), &Rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
					 SelectObject(hdc, hfontOld);
					 DeleteObject(hFont);
					 hFont = NULL;

					 String strDirectory;
					 UINT dirSize = GetCurrentDirectory(0, 0);
					 strDirectory.SetLength(dirSize);
					 GetCurrentDirectory(dirSize, strDirectory.Array());
					 strDirectory = strDirectory + L"\\img\\combobox_arrow_hover.png";
					 Image image(strDirectory.Array()/*L"Z:\\releaseM_x64\\img\\combobox_arrow_hover.png"*/);
					 //绘图
					 if (image.GetLastStatus() != Status::Ok){
						 return 0;
					 }
					 Graphics graphics(hdc);
					 graphics.DrawImage(&image, comboInfo.rcButton.left - 10, comboInfo.rcButton.top, 26, 36);

					 EndPaint(hwnd, &ps);
	}
		break;
	case WM_LBUTTONDOWN:
	{
						SendMessage(comboInfo.hwndList, CB_SHOWDROPDOWN, TRUE, 0);
						if (channelListProc == NULL)
						{
							channelListProc = (WNDPROC)SetWindowLongPtr(comboInfo.hwndList, GWLP_WNDPROC, (LONG_PTR)ChannelListProc);
						}
	}
		break;
	case WM_DRAWITEM:
	{
						ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
						PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;
						HDC  hDC = (HDC)pdis->hDC;
						RECT rect = (RECT)pdis->rcItem;
						UINT itemID = (UINT)pdis->itemID;
						HBRUSH HBrush = CreateSolidBrush(RGB(31, 31, 32));
						FillRect(hDC, &rect, HBrush);
						DeleteObject(HBrush);
						SetTextColor(hDC, RGB(255, 255, 255));
						SetBkMode(hDC, TRANSPARENT);

						rect.left += 10;
						rect.right += 10;

						HFONT hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
							OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
						HFONT hfontOld = (HFONT)SelectObject(hDC, hFont);
						String strAudioDevice = L"";
						if (itemID < 100)
						{
							strAudioDevice = configData->audioNameList[itemID];
						}
						
						DrawText(hDC, strAudioDevice.Array(), strAudioDevice.Length(), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
						
						SelectObject(hDC, hfontOld);
						DeleteObject(hFont);
						hFont = NULL;

						UpdateWindow(hwnd);
	}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			ConfigDialogData *configData = (ConfigDialogData *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
			configData->nCurrentSelect = (UINT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
			SendMessage(hwnd, CB_SETCURSEL, configData->nCurrentSelect, 0);
		}
		break;
	}
	return CallWindowProc(channelComboProc, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------------------------------
INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;
    switch(message) 
	{
		case WM_NCACTIVATE:
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

						 HBRUSH HBrush = CreateSolidBrush(RGB(42, 42, 44));

						 FillRect(hdc, &Rect, HBrush);
						 DeleteObject(HBrush);
						 SetBkMode(hdc, TRANSPARENT);
						 HFONT       m_hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
							 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
						 HFONT oldFont20 = (HFONT)SelectObject(hdc, m_hFont);
						 String itemReset = TEXT("噪声抑制");
						 String DenoiseNumber = TEXT("抑制程度");
						 String DenoiseUnit = TEXT("(DB)");
						 String AudioList = TEXT("音频输入设备");
						 COLORREF color = SetTextColor(hdc, RGB(255, 255, 255)); //设置文字颜色为蓝色
						 TextOut(hdc, 50, 105, itemReset, itemReset.Length());
						 TextOut(hdc, 50, 140, DenoiseNumber, DenoiseNumber.Length());
						 TextOut(hdc, 340, 140, DenoiseUnit, DenoiseUnit.Length());
						 TextOut(hdc, 50, 10, AudioList, AudioList.Length());
						 SetTextColor(hdc, color); //还原为之前的颜色
						 SelectObject(hdc, oldFont20);
						 DeleteObject(m_hFont);
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
								  SetBkColor(hdc, RGB(42, 42, 44));

								  if (HBrush)
								  {
									  DeleteObject(HBrush);
								  }
								  HBrush = CreateSolidBrush(RGB(42, 42, 44));

								  return (LRESULT)HBrush;
		}
		case	WM_CTLCOLOREDIT:
		{
								   HDC hdc = (HDC)wParam;
								   SetTextColor(hdc, RGB(255, 255, 255));
								   SetBkMode(hdc, TRANSPARENT);
								   HBRUSH HBrush = CreateSolidBrush(RGB(31, 31, 33));

								   return (LRESULT)HBrush;
		}
		case WM_CTLCOLORLISTBOX:
		{
								   HDC hdc = (HDC)wParam;
								   SetTextColor(hdc, RGB(255, 255, 255));
								   SetBkColor(hdc, RGB(42, 42, 44));

								   if (HBrush)
								   {
									   DeleteObject(HBrush);
								   }
								   HBrush = CreateSolidBrush(RGB(42, 42, 44));
								   return (LRESULT)HBrush;
		}
			break;
		case WM_DESTROY:
			if (HBrush)
				DeleteObject(HBrush);
			break;
        case WM_INITDIALOG:
            {
							  channelListProc = NULL;
							  SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
							  ConfigDialogData *configData = (ConfigDialogData*)lParam;
							  
							  configData->bDenoiseCheckFlag = configData->data["audioDenoiseCheck"].asInt();
							  configData->bDenoiseCheckFlag = configData->data["audioDenoiseCheck"].asInt();
							  configData->nDB = configData->data["audioDenoise"].asInt();
							  configData->nPos = configData->data["audioDenoisePos"].asInt();

							  configData->HBrushGreen = CreateSolidBrush(RGB(38, 161, 192));
							  configData->HBrushBlack = CreateSolidBrush(RGB(31, 31, 32));
							  configData->HBrushBackGround = CreateSolidBrush(RGB(42, 42, 44));
							  configData->m_pSilderImage = new CGdiPlusImage;
							  configData->m_pSilderImage->LoadFromFile(L".\\img\\huakuai.png");
							  configData->m_pDenoiseCheckedImage = new CGdiPlusImage;
							  configData->m_pDenoiseCheckedImage->LoadFromFile(L".\\img\\checkbox_select.png");
							  configData->m_pDenoiseUncheckedImage = new CGdiPlusImage;
							  configData->m_pDenoiseUncheckedImage->LoadFromFile(L".\\img\\checkbox_normal.png.png");
							  configData->m_pDenoiseUpposImage = new CGdiPlusImage;
							  configData->m_pDenoiseUpposImage->LoadFromFile(L".\\img\\lj_arrow_up_press.png");
							  configData->m_pDenoiseDownposImage = new CGdiPlusImage;
							  configData->m_pDenoiseDownposImage->LoadFromFile(L".\\img\\lj_arrow_down_press.png");
							  configData->m_pDenoiseOkImage = new CGdiPlusImage;
							  configData->m_pDenoiseOkImage->LoadFromFile(L".\\img\\btn_blue_nor.png");
							  configData->m_pDenoiseCancelImage = new CGdiPlusImage;
							  configData->m_pDenoiseCancelImage->LoadFromFile(L".\\img\\btn_gray_nor.png");
							  configData->m_pDenoiseOkHoverImage = new CGdiPlusImage;
							  configData->m_pDenoiseOkHoverImage->LoadFromFile(L".\\img\\btn_blue_hover.png");
							  configData->m_pDenoiseCancelHoverImage = new CGdiPlusImage;
							  configData->m_pDenoiseCancelHoverImage->LoadFromFile(L".\\img\\btn_gray_hover.png");
							  
							  if (!pGlobalConfigData)
							  {
								  InitDenoiseSlider();
							  }
							  pGlobalConfigData = configData;
							  
							  CreateWindow(DENOISE_SLIDER_CLASS, NULL, WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
								  135, 133, 130, 40, hwnd, (HMENU)ID_DENOISESLIDER, hinstMain, NULL);

							  CreateWindow(L"Button", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
								  123, 96, 40, 40, hwnd, (HMENU)ID_CHECKDENOISE, hinstMain, NULL);

							  CreateWindow(L"Button", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
								  320, 140, 18, 14, hwnd, (HMENU)ID_UPPOSDENOISE, hinstMain, NULL);

							  CreateWindow(L"Button", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
								  320, 154, 18, 14, hwnd, (HMENU)ID_DOWNPOSDENOISE, hinstMain, NULL);
							  SetWindowPos(GetDlgItem(hwnd, IDC_DENOISEEDIT), NULL, 280, 140, 40, 28, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

							  SetWindowPos(GetDlgItem(hwnd, IDC_AUDIOLIST), NULL, 50, 46, 300, 360, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

							  SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, 264, 204, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

							  SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, 150, 204, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

							  SendMessage(GetDlgItem(hwnd, IDC_AUDIOLIST), CB_SETITEMHEIGHT, -1, 36);
							 /* AudioCancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
							  AudioCancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
*/
							  AudioCancelProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);
							  AudioCancelProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);

							  SetWindowLongPtr(GetDlgItem(hwnd, ID_CHECKDENOISE), GWLP_WNDPROC, (LONG_PTR)DenoiseCheckProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, ID_UPPOSDENOISE), GWLP_WNDPROC, (LONG_PTR)DenoiseUpposProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, ID_DOWNPOSDENOISE), GWLP_WNDPROC, (LONG_PTR)DenoiseDownposProc);

							  denoiseeditproc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_DENOISEEDIT), GWLP_WNDPROC);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDC_DENOISEEDIT), GWLP_WNDPROC, (LONG_PTR)DenoiseEditProc);
							  
							  HFONT hFont = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
								  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
							  ::SendMessage(GetDlgItem(hwnd, IDC_DENOISEEDIT), WM_SETFONT, (WPARAM)hFont, TRUE);
							  TEXTMETRIC tm;
							  GetTextMetrics(GetDC(GetDlgItem(hwnd, IDC_DENOISEEDIT)), &tm);
							  int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
							  Rect rect;
							  GetClientRect(GetDlgItem(hwnd, IDC_DENOISEEDIT), (LPRECT)&rect);
							  OffsetRect((LPRECT)&rect, 0, (rect.Height - nFontHeight) / 2 - 3);
							  SendMessage(GetDlgItem(hwnd, IDC_DENOISEEDIT), EM_SETRECT, 0, (LPARAM)&rect);

							  SendMessage(GetDlgItem(hwnd, IDC_DENOISEEDIT), UPTATEEDIT, 0, 0);
							  //::SendMessage(GetDlgItem(hwnd, IDC_DENOISEEDIT), WM_SETFONT, (WPARAM)hFont, TRUE);
							  /*  TEXTMETRIC tm;
								GetTextMetrics(GetDC(GetDlgItem(hwnd, IDC_DENOISEEDIT)), &tm);
								int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
								Rect rect;
								GetClientRect(GetDlgItem(hwnd, IDC_DENOISEEDIT), (LPRECT)&rect);
								OffsetRect((LPRECT)&rect, 0, (rect.Height - nFontHeight) / 2 - 3);
								SendMessage(GetDlgItem(hwnd, IDC_DENOISEEDIT), EM_SETRECT, 0, (LPARAM)&rect);*/
               

				//=======================================================
							  
								HWND hwndAudioList      = GetDlgItem(hwnd, IDC_AUDIOLIST);

								UINT deviceID = 0;
								CTSTR pDisabled = L"禁用";
								//SendMessage(hwndAudioList, CB_INSERTSTRING, 0, (LPARAM)pDisabled);

								String strAudioDevice = L"";
								if (!configData->data["audioDeviceName"].isNull())
								{
								strAudioDevice = Asic2WChar(configData->data["audioDeviceName"].asString().c_str()).c_str();
								}

								FillOutListOfDevices(hwndAudioList, CLSID_AudioInputDeviceCategory, &configData->audioNameList, &configData->audioIDList);
								
								for (int i = 0; i < configData->audioNameList.Num(); ++i)
								{
								if (configData->audioNameList[i] == strAudioDevice)
								{
								deviceID = i;
								break;
								}
								}

								SendMessage(hwndAudioList, CB_SETCURSEL, deviceID, 0);
								configData->nCurrentSelect = deviceID;
								for (int i = 0; i < 10; i++)
									SendMessage(hwndAudioList, CB_SETITEMHEIGHT, i, 36);
								channelComboProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_AUDIOLIST), GWLP_WNDPROC, (LONG_PTR)ChannelComboProc);
								
				bool enable =  1;
				EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET), enable);
				EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT), enable);
				EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), enable);
				//SendMessage(GetDlgItem(hwnd, IDC_DENOISE), TBM_SETRANGE, (WPARAM)(int)(1), MAKELPARAM(-60, 0));
				//SendMessage(GetDlgItem(hwnd, IDC_DENOISE), TBM_SETPOS, (WPARAM)(int)(1), (LPARAM)configData->data["audioDenoise"].asInt());
				EnableWindow(GetDlgItem(hwnd, ID_DENOISESLIDER), enable);
				//SendMessage(GetDlgItem(hwnd, ID_CHECKDENOISE), BM_SETCHECK, configData->data["audioDenoiseCheck"].asInt(), 0);
				EnableWindow(GetDlgItem(hwnd, ID_CHECKDENOISE), enable);
				EnableWindow(GetDlgItem(hwnd, IDC_DENOISEEDIT), enable);
				EnableWindow(GetDlgItem(hwnd, ID_UPPOSDENOISE), enable);
				EnableWindow(GetDlgItem(hwnd, ID_DOWNPOSDENOISE), enable);
              //  ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_AUDIOLIST, CBN_SELCHANGE), (LPARAM)hwndAudioList);

				//==============================================
				float fVol = 1.0f; 

				if (!configData->data["volume"].isNull())
					fVol = configData->data["volume"].asDouble();

                SetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME), fVol);
                //------------------------------------------

                int pos = configData->data["soundTimeOffset"].asInt();

                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETRANGE32, -3000, 20000);
                SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_SETPOS32, 0, pos);

				LocalizeWindow(hwnd, pluginLocale);

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

                return TRUE;
            }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_VOLUME:
                    if(HIWORD(wParam) == VOLN_ADJUSTING || HIWORD(wParam) == VOLN_FINALVALUE)
                    {
                        if(IsWindowEnabled((HWND)lParam))
                        {
                            float fVol = GetVolumeControlValue((HWND)lParam);

                            ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;

							if (Video)
								Video->SetFloat(TEXT("volume"), fVol);
                        }
                    }
                    break;

                case IDC_TIMEOFFSET_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
                        ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(configData)
                        {
							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;

							if (Video)
                            {
                                HWND hwndVal = NULL;
                                switch(LOWORD(wParam))
                                {
                                    case IDC_TIMEOFFSET_EDIT:       hwndVal = GetDlgItem(hwnd, IDC_TIMEOFFSET); break;
                                }

                                int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
                                switch(LOWORD(wParam))
                                {
								case IDC_TIMEOFFSET_EDIT:       Video->SetInt(TEXT("timeOffset"), val); break;
                                }
                            }
                        }
                    }
                    break;

                case IDC_AUDIOLIST:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hwndDevices = (HWND)lParam;
                        UINT id = (UINT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);

                        if (id == CB_ERR || id == 0) {
							bool enable = 1;
							EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), enable);
							EnableWindow(GetDlgItem(hwnd, ID_DENOISESLIDER), enable);
							EnableWindow(GetDlgItem(hwnd, ID_CHECKDENOISE), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_DENOISEEDIT), enable);
							EnableWindow(GetDlgItem(hwnd, ID_UPPOSDENOISE), enable);
							EnableWindow(GetDlgItem(hwnd, ID_DOWNPOSDENOISE), enable);
                        }
						else 
						{
							bool enable = TRUE;
							EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_TIMEOFFSET_EDIT), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_VOLUME), enable);
							EnableWindow(GetDlgItem(hwnd, ID_DENOISESLIDER), enable);
							EnableWindow(GetDlgItem(hwnd, ID_CHECKDENOISE), enable);
							EnableWindow(GetDlgItem(hwnd, IDC_DENOISEEDIT), enable);
							EnableWindow(GetDlgItem(hwnd, ID_UPPOSDENOISE), enable);
							EnableWindow(GetDlgItem(hwnd, ID_DOWNPOSDENOISE), enable);
                        }
                    }
                    break;

				case IDOK:
				{
							 ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);

							// int nPos = SendMessage(GetDlgItem(hwnd, IDC_DENOISE), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
							 configData->data["audioDenoise"] = configData->nDB;
							 configData->data["audioDenoisePos"] = configData->nPos;
							 //bool isDenoiseChecked = SendMessage(GetDlgItem(hwnd, ID_CHECKDENOISE), BM_GETCHECK, 0, 0) == BST_CHECKED;
							 configData->data["audioDenoiseCheck"] = configData->bDenoiseCheckFlag;

							 UINT audioDeviceID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_AUDIOLIST), CB_GETCURSEL, 0, 0);

							 if (audioDeviceID < 10000) {
								 String strAudioDevice = configData->audioNameList[audioDeviceID];
								 String strAudioID = configData->audioIDList[audioDeviceID];
								 configData->data["audioDeviceName"] =  WcharToAnsi(strAudioDevice.Array()).c_str();
								 if (!strAudioID.IsEmpty())
									configData->data["audioDeviceId"] =  WcharToAnsi(strAudioID.Array()).c_str();
							 }
							 else
							 {
								 configData->data["audioDeviceName"] =  "";
								 configData->data["audioDeviceID"] =  "";
							 }

							 int soundTimeOffset = (int)SendMessage(GetDlgItem(hwnd, IDC_TIMEOFFSET), UDM_GETPOS32, 0, 0);
							 configData->data["soundTimeOffset"] =  soundTimeOffset;

							 float fVol = GetVolumeControlValue(GetDlgItem(hwnd, IDC_VOLUME));
							 configData->data["volume"] = fVol;

							 DeleteObject(configData->HBrushGreen);
							 DeleteObject(configData->HBrushBlack);
							 DeleteObject(configData->HBrushBackGround);
							 delete configData->m_pSilderImage;
							 delete configData->m_pDenoiseCheckedImage;
							 delete configData->m_pDenoiseUncheckedImage;
							 delete configData->m_pDenoiseUpposImage;
							 delete configData->m_pDenoiseDownposImage;
							 delete configData->m_pDenoiseOkImage;
							 delete configData->m_pDenoiseCancelImage;

							 EndDialog(hwnd, LOWORD(wParam));
							 break;
				}
                case IDCANCEL:
                    if(LOWORD(wParam) == IDCANCEL)
                    {
                       ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
					   DeleteObject(configData->HBrushGreen);
					   DeleteObject(configData->HBrushBlack);
					   DeleteObject(configData->HBrushBackGround);
					   delete configData->m_pSilderImage;
					   delete configData->m_pDenoiseCheckedImage;
					   delete configData->m_pDenoiseUncheckedImage;
					   delete configData->m_pDenoiseUpposImage;
					   delete configData->m_pDenoiseDownposImage;
					   delete configData->m_pDenoiseOkImage;
					   delete configData->m_pDenoiseCancelImage;
					   delete configData->m_pDenoiseOkHoverImage;
					   delete configData->m_pDenoiseCancelHoverImage;
					   std::stringstream SourceId;
					   uint64_t VideoId = 0;
					   if (!configData->data["SourceID"].isNull())
					   {
						   SourceId << configData->data["SourceID"].asString().c_str();
						   SourceId >> VideoId;
					   }

					   IBaseVideo *Video = (IBaseVideo*)VideoId;

					   if (Video)
                        {
						   Video->SetInt(TEXT("timeOffset"), configData->data["soundTimeOffset"].asInt());

						   if (!configData->data["volume"].isNull())
							   Video->SetFloat(TEXT("volume"), configData->data["volume"].asDouble());
						   else
						   {
							   Video->SetFloat(TEXT("volume"), 1.0f);
						   }
						 }
                    }
					

                    EndDialog(hwnd, LOWORD(wParam));
					break;
            }
    }

    return FALSE;
}

bool bFirst = true;
bool STDCALL ConfigureDShowSource(Value &element, bool bCreating)
{
	if (bFirst)
	{
		InitColorControl(hinstMain);
		InitVolumeControl(hinstMain);
		InitVolumeMeter(hinstMain);

		pluginLocale = new LocaleStringLookup;

		if (!pluginLocale->LoadStringFile(AUDIOLOCALPATH))
			Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AUDIOLOCALPATH).c_str());

		bFirst = false;
	}

    ConfigDialogData configData(element);
    configData.bGlobalSource = true;
    configData.bCreating = bCreating;

    if(BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG), GetMainWindow(), ConfigureDialogProc, (LPARAM)&configData) == IDOK)
    {
        return true;
    }

    return false;
}

REGINST_CONFIGFUNC(DSource, ConfigureDShowSource)

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#if defined _M_X64 && _MSC_VER == 1800
        //workaround AVX2 bug in VS2013, http://connect.microsoft.com/VisualStudio/feedback/details/811093
        _set_FMA3_enable(0);
#endif
        hinstMain = hInst;
    }
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		if (pluginLocale)
			delete pluginLocale;
	}

    return TRUE;
}
