#include "BaseAfx.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

//COLORREF CLOROBRUSH = 0x4080ff;
//COLORREF CLOROBRUSH = 0xff9933;
COLORREF CLOROBRUSH = 0x2c80fe;

struct VolumeControlData
{
    float curVolume;
    float lastUnmutedVol;

    DWORD drawColor;
    bool  bDisabled, bHasCapture;
    long  cx,cy;

    HICON hiconPlay, hiconMute;
    bool bDrawIcon;

    void DrawVolumeControl(HDC hDC);
};

inline VolumeControlData* GetVolumeControlData(HWND hwnd)
{
    return (VolumeControlData*)GetWindowLongPtr(hwnd, 0);
}

float ToggleVolumeControlMute(HWND hwnd)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        CrashError(TEXT("ToggleVolumeControlMute called on a control that's not a volume control"));

    if(control->curVolume < VOLN_MUTELEVEL)
    {
        if(control->lastUnmutedVol < VOLN_MUTELEVEL)
            control->lastUnmutedVol = 1.0f;
        control->curVolume = control->lastUnmutedVol;
    }
    else
    {
        control->lastUnmutedVol = control->curVolume;
        control->curVolume = 0.0f;
    }

    HDC hDC = GetDC(hwnd);
    control->DrawVolumeControl(hDC);
    ReleaseDC(hwnd, hDC);

    return control->curVolume;
}

float SetVolumeControlValue(HWND hwnd, float fVal)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        CrashError(TEXT("SetVolumeControlValue called on a control that's not a volume control"));

    float lastVal = control->curVolume;

    control->curVolume = fVal;

    HDC hDC = GetDC(hwnd);
    control->DrawVolumeControl(hDC);
    ReleaseDC(hwnd, hDC);

    return lastVal;
}

float GetVolumeControlValue(HWND hwnd)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        CrashError(TEXT("GetVolumeControlValue called on a control that's not a volume control"));

    return control->curVolume;
}

void SetVolumeControlIcons(HWND hwnd, HICON hiconPlay, HICON hiconMute)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        CrashError(TEXT("GetVolumeControlValue called on a control that's not a volume control"));

    control->hiconPlay = hiconPlay;
    control->hiconMute = hiconMute;
    control->bDrawIcon = (control->cy == 16) && control->hiconPlay;

    HDC hDC = GetDC(hwnd);
    control->DrawVolumeControl(hDC);
    ReleaseDC(hwnd, hDC);
}


LRESULT CALLBACK VolumeControlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    VolumeControlData *control;

    switch(message)
    {
        case WM_NCCREATE:
            {
                CREATESTRUCT *pCreateData = (CREATESTRUCT*)lParam;

                control = (VolumeControlData*)malloc(sizeof(VolumeControlData));
                zero(control, sizeof(VolumeControlData));
                SetWindowLongPtr(hwnd, 0, (LONG_PTR)control);

                control->curVolume = 1.0f;
                control->bDisabled = ((pCreateData->style & WS_DISABLED) != 0);

                control->cx = pCreateData->cx;
                control->cy = pCreateData->cy;

                return TRUE;
            }

        case WM_DESTROY:
            {
                control = GetVolumeControlData(hwnd);
                if(control)
                    free(control);

                break;
            }

        case WM_PAINT:
            {
                control = GetVolumeControlData(hwnd);

                PAINTSTRUCT ps;

                HDC hDC = BeginPaint(hwnd, &ps);
                control->DrawVolumeControl(hDC);
                EndPaint(hwnd, &ps);

                break;
            }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            {
                control = GetVolumeControlData(hwnd);

                short x = short(LOWORD(lParam));
                short y = short(HIWORD(lParam));

                UINT id = (UINT)GetWindowLongPtr(hwnd, GWLP_ID);

                if(message == WM_LBUTTONDOWN && !control->bDisabled)
                {
                    if(control->cy == 32 && x <= 24 /*(control->cx-16)*/)
                    {
                        if(control->curVolume < VOLN_MUTELEVEL)
                        {
                            if(control->lastUnmutedVol < VOLN_MUTELEVEL)
                                control->lastUnmutedVol = 1.0f;
                            control->curVolume = control->lastUnmutedVol;
                        }
                        else
                        {
                            control->lastUnmutedVol = control->curVolume;
                            control->curVolume = 0.0f;
                        }

                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_FINALVALUE), (LPARAM)hwnd);
                    }
                    else
                    {
                        SetCapture(hwnd);
                        control->bHasCapture = true;

                        if(control->curVolume > VOLN_MUTELEVEL)
                            control->lastUnmutedVol = control->curVolume;

                        int cxAdjust = control->cx;
                        //if(control->bDrawIcon) cxAdjust -= 16;
                        control->curVolume = float(x) / cxAdjust;

                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_ADJUSTING), (LPARAM)hwnd);
                    }

                    HDC hDC = GetDC(hwnd);
                    control->DrawVolumeControl(hDC);
                    ReleaseDC(hwnd, hDC);
                }
                else if(control->bHasCapture)
                {
                    UINT id = (UINT)GetWindowLongPtr(hwnd, GWLP_ID);
                    SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_FINALVALUE), (LPARAM)hwnd);

                    ReleaseCapture();
                    control->bHasCapture = false;

                    HDC hDC = GetDC(hwnd);
                    control->DrawVolumeControl(hDC);
                    ReleaseDC(hwnd, hDC);
                }
                
                break;
            }

        case WM_MOUSEMOVE:
            {
                control = GetVolumeControlData(hwnd);
                if(control->bHasCapture)
                {
                    int cxAdjust = control->cx;
                    //if(control->bDrawIcon) cxAdjust -= 16;
                    control->curVolume = float(short(LOWORD(lParam))) / cxAdjust;

                    if(control->curVolume < 0.0f)
                        control->curVolume = 0.0f;
                    else if(control->curVolume > 1.0f)
                        control->curVolume = 1.0f;

                    HDC hDC = GetDC(hwnd);
                    control->DrawVolumeControl(hDC);
                    ReleaseDC(hwnd, hDC);

                    UINT id = (UINT)GetWindowLongPtr(hwnd, GWLP_ID);
                    SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_ADJUSTING), (LPARAM)hwnd);
                }
                break;
            }

        case WM_ENABLE:
            {
                UINT id = (UINT)GetWindowLongPtr(hwnd, GWLP_ID);
                control = GetVolumeControlData(hwnd);

                if(control->bDisabled == !wParam)
                    break;

                control->bDisabled = !control->bDisabled;

                if(control->bDisabled)
                {
                    if(control->curVolume > VOLN_MUTELEVEL)
                    {
                        control->lastUnmutedVol = control->curVolume;
                        control->curVolume = 0.0f;
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_ADJUSTING), (LPARAM)hwnd);
                    }
                }
                else
                {
                    if(control->curVolume < VOLN_MUTELEVEL)
                    {
                        control->curVolume = control->lastUnmutedVol;
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, VOLN_ADJUSTING), (LPARAM)hwnd);
                    }
                }

                HDC hDC = GetDC(hwnd);
                control->DrawVolumeControl(hDC);
                ReleaseDC(hwnd, hDC);
                break;
            }

        case WM_SIZE:
            {
                control = GetVolumeControlData(hwnd);
                control->cx = LOWORD(lParam);
				control->cy = HIWORD(lParam);

                control->bDrawIcon = (control->cy == 32) && control->hiconPlay;

                HDC hDC = GetDC(hwnd);
                control->DrawVolumeControl(hDC);
                ReleaseDC(hwnd, hDC);
                break;
            }

        case WM_COMMAND:
            {
                switch(HIWORD(wParam))
                {
                    /* 
                       Handle API Volume Adjustments 
                       Updates control values and sends message to BLiveproc to update
                       application volume.
                     */
                    case VOLN_FINALVALUE:
                    case VOLN_ADJUSTING:
                        {
							/* float in lParam hack */
                            float val = *((float*)&lParam);
                            SetVolumeControlValue(hwnd, val);
                            
                            /* send message to BLiveproc to update volumes */
                            UINT id = (UINT)GetWindowLongPtr(hwnd, GWLP_ID);
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, HIWORD(wParam)), (LPARAM)hwnd);
                            break;
                        }
                    case VOLN_TOGGLEMUTE:
                        {
                            ToggleVolumeControlMute(hwnd);
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), VOLN_FINALVALUE), (LPARAM)hwnd);
                        }    
                }
                    
            }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

void VolumeControlData::DrawVolumeControl(HDC hDC)
{
    HDC hdcTemp = CreateCompatibleDC(hDC);
    HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, cx, cy);
    SelectObject(hdcTemp, hbmpTemp);

	HBRUSH hBK_153 = CreateSolidBrush(RGB(153,153,153));
    RECT clientRect = {0, 0, cx, cy};
	FillRect(hdcTemp, &clientRect, hBK_153);
	DeleteObject(hBK_153);
	//Color_White
    HBRUSH hGray = CreateSolidBrush(REVERSE_COLOR(Color_Gray));
	HBRUSH hRed = CreateSolidBrush(CLOROBRUSH);//(0x2020bf);//0xff4040);

    float visualVolume = (bDisabled) ? 0.0f : curVolume;

    int cxAdjust = cx;
    if(bDrawIcon) cxAdjust -= 16;

    const int padding = 1;
    const float volSliceSize = float(cxAdjust-(padding*9))/10.0f;

    int volPixelPos = int(visualVolume*float(cxAdjust));

    if(volSliceSize > 1.0f)
    {
		SelectObject(hdcTemp, hGray);
		POINT p01 = { clientRect.left + 2, clientRect.bottom - 4 };
		POINT p02 = { clientRect.right - 2, clientRect.top + 12 };
		POINT p03 = {clientRect.right - 2, clientRect.bottom - 4 };
		POINT gPoint0[3] = { p01, p02, p03 };
		Polygon(hdcTemp, gPoint0, 3);
		
		float h = clientRect.bottom - 4 - (clientRect.top + 12);
		float w = clientRect.right - 2 - (clientRect.left + 2);
		int x = clientRect.left + 2 + visualVolume*w;
		int y2 = clientRect.bottom - 4 - visualVolume*h;
	
		HBRUSH bBrush = CreateSolidBrush(CLOROBRUSH);
		SelectObject(hdcTemp, bBrush);
		POINT p1 = { clientRect.left + 2, clientRect.bottom - 4};
		POINT p2 = { x, y2};
		POINT p3 = { x, clientRect.bottom - 4};
		POINT gPoint[3] = { p1, p2, p3};
		Polygon(hdcTemp, gPoint, 3);
		DeleteObject(bBrush);

		//RECT rect = { x - visualVolume*8, y2 - visualVolume * 8, x + 1, clientRect.bottom - 2 };
		//FillRect(hdcTemp, &rect, hGray);
		HBRUSH hWhite = CreateSolidBrush(REVERSE_COLOR(Color_White));
		SelectObject(hdcTemp, hWhite);
		HRGN hrectRGN = CreateRoundRectRgn(x - visualVolume * 8, y2 - visualVolume * 8, x + 2, clientRect.bottom, 2, 2);
		FillRgn(hdcTemp, hrectRGN, hWhite);
		DeleteObject(hWhite);

		if (bDrawIcon)
		{
			::DrawIconEx(hdcTemp, 0, 0, (visualVolume > VOLN_MUTELEVEL) ? hiconPlay : hiconMute, 24, 24, 0, NULL, DI_NORMAL);
			//DrawIcon(hdcTemp, /*cx-16*/0, 0, (visualVolume > VOLN_MUTELEVEL) ? hiconPlay : hiconMute);
		}

        if(visualVolume > VOLN_MUTELEVEL && bHasCapture)
        {
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HFONT hfontOld = (HFONT)SelectObject(hdcTemp, hFont);

            SetTextColor(hdcTemp, GetSysColor(COLOR_WINDOWTEXT));
            SetBkMode(hdcTemp, TRANSPARENT);
            String volText = FormattedString(TEXT("%d%%"), int(curVolume*100));
			TextOut(hdcTemp, clientRect.left + 40 /*(LONG)volSliceSize //cx/2*/, 6 /*clientRect.top*/, volText, slen(volText));     
            SelectObject(hdcTemp, hfontOld);
        }
    }

    BitBlt(hDC, 0, 0, cx, cy, hdcTemp, 0, 0, SRCCOPY);

    DeleteObject(hdcTemp);
    DeleteObject(hbmpTemp);

    DeleteObject(hGray);
    DeleteObject(hRed);
}

void InitVolumeControl(HINSTANCE hInst)
{
    WNDCLASS wnd;

    wnd.cbClsExtra = 0;
    wnd.cbWndExtra = sizeof(LPVOID);
    wnd.hbrBackground = NULL;
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
    wnd.hIcon = NULL;
    wnd.hInstance = hInst;
    wnd.lpfnWndProc = VolumeControlProc;
    wnd.lpszClassName = VOLUME_CONTROL_CLASS;
    wnd.lpszMenuName = NULL;
    wnd.style = CS_PARENTDC | CS_VREDRAW | CS_HREDRAW;

    if(!RegisterClass(&wnd))
        CrashError(TEXT("Could not register volume control class"));
}

void SetVolumeControlMutedVal(HWND hwnd, float val)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        return;

    control->lastUnmutedVol = val;
}

float GetVolumeControlMutedVal(HWND hwnd)
{
    VolumeControlData *control = GetVolumeControlData(hwnd);
    if(!control)
        return 0.0f;

    return control->lastUnmutedVol;
}
