// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "libDateTime.h"
#include "resource.h"
#include <sstream>


HINSTANCE g_histMian = NULL;

WNDPROC OKProc = NULL;
WNDPROC TimeOKProc = NULL;
WNDPROC TimeCancelProc = NULL;
WNDPROC TimeHelpProc = NULL;

static HWND Hwnd = NULL;
static HWND TimeHwnd = NULL;

struct CCStruct
{
	DWORD structSize;       //size of this structure
	DWORD curColor;         //current color stored
	BOOL  bDisabled;        //whether the control is disabled or not

	long  cx, cy;            //size of control

	DWORD custColors[16];   //custom colors in the colors dialog box
};


INT_PTR CALLBACK WindowButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
		WndProcTem = OKProc;
	}
	else if (hwnd == GetDlgItem(TimeHwnd, IDOK))
	{
		WndProcTem = TimeOKProc;
	}
	else if (hwnd == GetDlgItem(TimeHwnd, IDCANCEL))
	{
		WndProcTem = TimeCancelProc;
	}
	else if (hwnd == GetDlgItem(TimeHwnd, IDC_BUTTON_HELP))
	{
		WndProcTem = TimeHelpProc;
	}

	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}


void DoCancelStuff(HWND hwnd)
{
	ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
	std::stringstream SourceId;
	uint64_t VideoId = 0;
	if (!configInfo->data["SourceID"].isNull())
	{
		SourceId << configInfo->data["SourceID"].asString();
		SourceId >> VideoId;
	}

	IBaseVideo *Video = (IBaseVideo*)VideoId;

	if (Video)
		Video->UpdateSettings(configInfo->data);
}
int CALLBACK FontEnumProcThingy(ENUMLOGFONTEX *logicalData, NEWTEXTMETRICEX *physicalData, DWORD fontType, ConfigDateTimeSourceInfo *configInfo)
{
	if (fontType == TRUETYPE_FONTTYPE) //HomeWorld - GDI+ doesn't like anything other than truetype
	{
		configInfo->fontNames << logicalData->elfFullName;
		configInfo->fontFaces << logicalData->elfLogFont.lfFaceName;
	}

	return 1;
}

UINT FindFontFace(ConfigDateTimeSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
	UINT id = configInfo->fontFaces.FindValueIndexI(lpFontFace);
	if (id == INVALID)
		return INVALID;
	else
	{
		for (UINT i = 0; i < configInfo->fontFaces.Num(); i++)
		{
			UINT targetID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, i, 0);
			if (targetID == id)
				return i;
		}
	}

	return INVALID;
}
CTSTR GetFontFace(ConfigDateTimeSourceInfo *configInfo, HWND hwndFontList)
{
	UINT id = (UINT)SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
	if (id == CB_ERR)
		return NULL;

	UINT actualID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, id, 0);
	return configInfo->fontFaces[actualID];
}

UINT FindFontName(ConfigDateTimeSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
	return configInfo->fontNames.FindValueIndexI(lpFontFace);
}
String FormatInfo = 
"用 \"H\" 代表0到23的24小时制格式. \n\r"
"\"HH\" 代表00到23的24小时制格式. \n\r"
"\"h\" 代表0到12的12小时制格式. \n\r"
"\"hh\" 代表00到12的12小时制格式. \n\r"
"\"m\" 代表0到59的分钟显示格式. \n\r"
"\"mm\" 代表00到59的分钟显示格式. \n\r"
"\n\r"
"\n\r"
"\"d\" 代表1到31的日期显示格式. \n\r"
"\"dd\" 代表01到31的日期显示格式. \n\r"
"\"M\" 代表1到12的月份显示格式. \n\r"
"\"MM\" 代表01到12的月份显示格式. \n\r"
"\"yyyy\" 代表年份. \n\r";

INT_PTR CALLBACK HelpProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool bInitializedDialog = false;
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;

	switch (message)
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
									   DrawFrame(hwnd,GetListTitle()[i].Type, true);
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
			break;
		case WM_DESTROY:
			if (HBrush)
				DeleteObject(HBrush);
			break;
	case WM_INITDIALOG:
	{
	//	SetWindowText(GetDlgItem(hwnd, IDC_STATIC_HELP), FormatInfo);

						  OKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);

		RECT ClientR;
		GetWindowRect(hwnd, &ClientR);
		ClientR.left += 103;
		ClientR.right += 103;
		MoveWindow(hwnd, ClientR.left, ClientR.top, ClientR.right - ClientR.left, ClientR.bottom - ClientR.top,FALSE);
		InvalidateRect(hwnd, NULL, FALSE);
		SendMessage(hwnd, WM_NCPAINT, 0, 0);
	}
			break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
			case IDOK:
				EndDialog(hwnd, LOWORD(wParam));
				break;
			default:
				break;
			 }
	}
		break;
	case WM_CLOSE:
		EndDialog(hwnd, LOWORD(wParam));
		break;
	default:

		return 0;
	}

	return 0;
}


INT_PTR CALLBACK ConfigureDateTimeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool bInitializedDialog = false;
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	TimeHwnd = hwnd;

	switch (message)
	{
		case WM_NCACTIVATE:
		{
							  if (wParam == 0)
								  return FALSE;
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
									   DrawFrame(hwnd,GetListTitle()[i].Type, true);
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
			break;
		case WM_DESTROY:
			if (HBrush)
				DeleteObject(HBrush);
			bInitializedDialog = false;
				break;
	case WM_INITDIALOG:
	{
						  TimeOKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  TimeCancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
						  TimeHelpProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_BUTTON_HELP), GWLP_WNDPROC);

						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_BUTTON_HELP), GWLP_WNDPROC, (LONG_PTR)WindowButtonProc);

						  ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)lParam;
						  SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);

						  Value &data = configInfo->data;

						  //设置字体-----------------------------------------

						  HDC hDCtest = GetDC(hwnd);

						  LOGFONT lf;
						  zero(&lf, sizeof(lf));
						  EnumFontFamiliesEx(hDCtest, &lf, (FONTENUMPROC)FontEnumProcThingy, (LPARAM)configInfo, 0);

						  HWND hwndFonts = GetDlgItem(hwnd, IDC_COMBO_FONT);
						  for (UINT i = 0; i < configInfo->fontNames.Num(); i++)
						  {
							  int id = (int)SendMessage(hwndFonts, CB_ADDSTRING, 0, (LPARAM)configInfo->fontNames[i].Array());
							  SendMessage(hwndFonts, CB_SETITEMDATA, id, (LPARAM)i);
						  }
						  String lpFont = L"";
						  if (!data["Font"].isNull())
						  {
							  lpFont = Asic2WChar(data["Font"].asString()).c_str();
						  }
						  UINT id = FindFontFace(configInfo, hwndFonts, lpFont);
						  if (id == INVALID)
							  id = (UINT)SendMessage(hwndFonts, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT("Arial"));

						  SendMessage(hwndFonts, CB_SETCURSEL, id, 0);

						  //-----------------------------------------

						  DWORD Color = 0xFFFFFFFF;
						  if (!data["FontColor"].isNull())
						  {
							  Color = data["FontColor"].asUInt();
						  }
						  CCSetColor(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR), Color);

						  //OutlineInfo-----------------------------------------

						  bool bChecked = data["useOutline"].asInt() != 0;
						  SendMessage(GetDlgItem(hwnd, IDC_CHECK_OUTLINE), BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED, 0);

						  EnableWindow(GetDlgItem(hwnd, IDC_EDIT_OUTTHCKNESS), bChecked);
						  EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR2), bChecked); 
				

						  String Title = L"2";

						  if (!data["outlineSize"].isNull())
						  {
							  int Size = data["outlineSize"].asInt();
							  TCHAR TSize[10] = { 0 };
							  wsprintf(TSize, L"%d", Size);
							  Title = TSize;
						  }
						  
						  SetWindowText(GetDlgItem(hwnd, IDC_EDIT_OUTTHCKNESS), Title.Array());


						  Color = 0xFF000000;

						  if (!data["outlineColor"].isNull())
						  {
							  Color = data["outlineColor"].asUInt();
						  }

						  CCSetColor(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR2), Color);
	

						  //-----------------------------------------

						   bChecked = data["Update"].asInt() == 1;
						  SendMessage(GetDlgItem(hwnd, IDC_CHECK_UPDATE), BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED, 0);
						  //-----------------------------------------

						  Title = L"HH:mm:ss";

						  if (!data["FormatString"].isNull())
						  {
							  Title = Asic2WChar(data["FormatString"].asString()).c_str();
						  }

						  SetWindowText(GetDlgItem(hwnd, IDC_EDIT_FORMAT), Title.Array());
						  //-----------------------------------------
						  bInitializedDialog = true;

						  InvalidateRect(hwnd, NULL, FALSE);
						  SendMessage(hwnd, WM_NCPAINT, 0, 0);

						  return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_COMBO_FONT:
			if (bInitializedDialog)
			{
				if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
				{
					ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
					if (!configInfo) break;

					String strFont;
					if (HIWORD(wParam) == CBN_SELCHANGE)
						strFont = GetFontFace(configInfo, (HWND)lParam);
					else
					{
						UINT id = FindFontName(configInfo, (HWND)lParam, GetEditText((HWND)lParam));
						if (id != INVALID)
							strFont = configInfo->fontFaces[id];
					}

					std::stringstream SourceId;
					uint64_t VideoId = 0;
					if (!configInfo->data["SourceID"].isNull())
					{
						SourceId << configInfo->data["SourceID"].asString();
						SourceId >> VideoId;
					}

					IBaseVideo *Video = (IBaseVideo*)VideoId;
					if (Video && strFont.IsValid())
						Video->SetString(TEXT("font"), strFont);
				}
			}
			break;

		case IDC_CUSTOM_FONTCOLOR:
		case IDC_CUSTOM_FONTCOLOR2:
			if (bInitializedDialog)
			{
				DWORD color = CCGetColor((HWND)lParam);

				ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
				if (!configInfo) break;

				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configInfo->data["SourceID"].isNull())
				{
					SourceId << configInfo->data["SourceID"].asString();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;
				if (Video)
				{
					switch (LOWORD(wParam))
					{
					case IDC_CUSTOM_FONTCOLOR:    Video->SetInt(TEXT("FontColor"), color); break;
					case IDC_CUSTOM_FONTCOLOR2:   Video->SetInt(TEXT("OutlineColor"), color); break;
					}
				}
			}
			break;
		case IDC_CHECK_OUTLINE:
		case IDC_CHECK_UPDATE:
			if (HIWORD(wParam) == BN_CLICKED && bInitializedDialog)
			{
				BOOL bChecked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

				ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
				if (!configInfo) break;
				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configInfo->data["SourceID"].isNull())
				{
					SourceId << configInfo->data["SourceID"].asString();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;
				if (Video)
				{
					switch (LOWORD(wParam))
					{
						case IDC_CHECK_OUTLINE:
						{
							EnableWindow(GetDlgItem(hwnd, IDC_EDIT_OUTTHCKNESS), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR2), bChecked);

							Video->SetInt(TEXT("useOutline"), bChecked); break;
						}

						case IDC_CHECK_UPDATE:  Video->SetInt(TEXT("Update"), bChecked); break;
					}
				
				}
				else
				{
					switch (LOWORD(wParam))
					{
					case IDC_CHECK_OUTLINE:
					{
											  EnableWindow(GetDlgItem(hwnd, IDC_EDIT_OUTTHCKNESS), bChecked);
											  EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR2), bChecked);
					}
						break;
					default:
						break;

					}
				}
			}
			break;
		case IDC_EDIT_FORMAT:
		case IDC_EDIT_OUTTHCKNESS:
			if (HIWORD(wParam) == EN_CHANGE && bInitializedDialog)
			{
				String strText = GetEditText((HWND)lParam);

				ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
				if (!configInfo) break;

				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configInfo->data["SourceID"].isNull())
				{
					SourceId << configInfo->data["SourceID"].asString();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;
				if (Video)
				{
					switch (LOWORD(wParam))
					{
						
					case IDC_EDIT_FORMAT:
					{
						if (strText.Length() > 30)
							{
								MessageBox(hwnd, L"长度不能超过30", NULL, MB_OK);
								SetWindowText(GetDlgItem(hwnd, IDC_EDIT_FORMAT), TEXT2("HH:mm:ss"));
								break;
							}
						Video->SetString(TEXT("FormatString"), strText); break;
					}
					case IDC_EDIT_OUTTHCKNESS: Video->SetFloat(TEXT("OutlineSize"), strText.ToInt()); break;
					}

				}
			}
			break;
		case IDC_BUTTON_HELP:
			DialogBox(g_histMian, MAKEINTRESOURCE(IDD_HELP), hwnd, HelpProc);
			break;

		case IDOK:
		{
					 ConfigDateTimeSourceInfo *configInfo = (ConfigDateTimeSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
					 if (!configInfo) break;


					 Value &data = configInfo->data;

					 BOOL bUseOutline = SendMessage(GetDlgItem(hwnd, IDC_CHECK_OUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 String outlineSize = GetEditText(GetDlgItem(hwnd, IDC_EDIT_OUTTHCKNESS));

					 String strText = GetEditText(GetDlgItem(hwnd, IDC_EDIT_FORMAT));
					 
					 if (strText.IsEmpty())
					 {
						 strText = "HH:mm:ss";
					 }
					
					 String strFont = GetFontFace(configInfo, GetDlgItem(hwnd, IDC_COMBO_FONT));

					 BOOL bLeft = SendMessage(GetDlgItem(hwnd, IDC_RADIO_LEFT), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bMid = SendMessage(GetDlgItem(hwnd, IDC_RADIO_MID), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bRight = SendMessage(GetDlgItem(hwnd, IDC_RADIO_RIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED;

					 String strFontDisplayName = GetEditText(GetDlgItem(hwnd, IDC_COMBO_FONT));
					 if (strFont.IsEmpty())
					 {
						 UINT id = FindFontName(configInfo, GetDlgItem(hwnd, IDC_COMBO_FONT), strFontDisplayName);
						 if (id != INVALID)
							 strFont = configInfo->fontFaces[id];
					 }

					 if (strFont.IsEmpty())
					 {
						 //String strError = Str("Sources.TextSource.FontNotFound");
						 //strError.FindReplace(TEXT("$1"), strFontDisplayName);
						 //BLiveMessageBox(hwnd, strError, NULL, 0);
						 break;
					 }

				
					 int FontSize = 48;
						 LOGFONT lf;
						 zero(&lf, sizeof(lf));
						 lf.lfHeight = FontSize;
						 lf.lfWeight = FW_DONTCARE; // FW_BOLD 
						 lf.lfItalic = false;
						 lf.lfQuality = ANTIALIASED_QUALITY;
						 if (strFont.IsValid())
							 scpy_n(lf.lfFaceName, strFont, 31);
						 else
							 scpy_n(lf.lfFaceName, TEXT("Arial"), 31);

						 HDC hDC = CreateCompatibleDC(NULL);

						 Gdiplus::Font font(hDC, &lf);

						 if (!font.IsAvailable())
						 {
							// String strError = Str("Sources.TextSource.FontNotFound");
							 //strError.FindReplace(TEXT("$1"), strFontDisplayName);
							 //BLiveMessageBox(hwnd, strError, NULL, 0);
							 DeleteDC(hDC);
							 break;
						 }

						
						 Gdiplus::Graphics graphics(hDC);
						 Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());

						 UINT formatFlags;

						 formatFlags = Gdiplus::StringFormatFlagsNoFitBlackBox
							 | Gdiplus::StringFormatFlagsMeasureTrailingSpaces;


						 format.SetFormatFlags(formatFlags);
						 format.SetTrimming(Gdiplus::StringTrimmingWord);

						 Gdiplus::RectF rcf;
						 graphics.MeasureString(strText, -1, &font, Gdiplus::PointF(0.0f, 0.0f), &format, &rcf);

						 if (bUseOutline)
						 {
							 rcf.Height += outlineSize.ToFloat();
							 rcf.Width += outlineSize.ToFloat();
						 }


						 if (rcf.Height < FontSize)
							 rcf.Height = (float)FontSize;

				    	 configInfo->cx = MAX(rcf.Width, 32.0f);
						 configInfo->cy = MAX(rcf.Height, 32.0f);

						 data["cx"] =  configInfo->cx;
						 data["cy"] =  configInfo->cy;

						 DeleteDC(hDC);
					
					
					 data["Font"] = WcharToAnsi(strFont.Array()).c_str();
					 data["FontColor"] = (UINT)CCGetColor(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR));
						 
	
					 data["useOutline"] = bUseOutline;
					 data["outlineColor"] = (UINT)CCGetColor(GetDlgItem(hwnd, IDC_CUSTOM_FONTCOLOR2));
					 data["outlineSize"] =  outlineSize.ToInt();

				
					 data["FormatString"] = WcharToAnsi(strText.Array()).c_str();
	
		}

		case IDCANCEL:
			if (LOWORD(wParam) == IDCANCEL)
				DoCancelStuff(hwnd);

			EndDialog(hwnd, LOWORD(wParam));
		}
		break;

	case WM_CLOSE:
		DoCancelStuff(hwnd);
		EndDialog(hwnd, IDCANCEL);
	}
	return 0;
}

void WINAPI DrawColorControl(HDC hDC, CCStruct *pCCData)
{
	HDC hdcTemp;
	HBITMAP hBmp, hbmpOld;
	HBRUSH hBrush;
	RECT rect;

	//Create temp draw data
	hdcTemp = CreateCompatibleDC(hDC);
	hBmp = CreateCompatibleBitmap(hDC, pCCData->cx, pCCData->cy);
	hbmpOld = (HBITMAP)SelectObject(hdcTemp, hBmp);


	//draw the outline
	hBrush = CreateSolidBrush(INVALID);

	rect.top = rect.left = 0;
	rect.right = pCCData->cx;
	rect.bottom = pCCData->cy;
	FillRect(hdcTemp, &rect, hBrush);

	DeleteObject(hBrush);


	//draw the color
	hBrush = CreateSolidBrush(pCCData->bDisabled ? 0x808080 : pCCData->curColor);

	rect.top = rect.left = 1;
	rect.right = pCCData->cx - 1;
	rect.bottom = pCCData->cy - 1;
	FillRect(hdcTemp, &rect, hBrush);

	DeleteObject(hBrush);


	//Copy drawn data back onto the main device context
	BitBlt(hDC, 0, 0, pCCData->cx, pCCData->cy, hdcTemp, 0, 0, SRCCOPY);

	//Delete temp draw data
	SelectObject(hdcTemp, hbmpOld);
	DeleteObject(hBmp);
	DeleteDC(hdcTemp);
}
static DWORD customColors[16] = { 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF };
LRESULT WINAPI ColorControlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CCStruct *pCCData;

	switch (message)
	{
	case WM_NCCREATE:
	{
						CREATESTRUCT *pCreateData = (CREATESTRUCT*)lParam;

						//add custom data to window
						pCCData = (CCStruct*)malloc(sizeof(CCStruct));
						zero(pCCData, sizeof(CCStruct));
						SetWindowLongPtr(hwnd, 0, (LONG_PTR)pCCData);

						pCCData->structSize = sizeof(CCStruct);
						pCCData->curColor = 0xFFFFFF;
						pCCData->bDisabled = ((pCreateData->style & WS_DISABLED) != 0);

						pCCData->cx = pCreateData->cx;
						pCCData->cy = pCreateData->cy;

						for (int i = 0; i < 16; i++) pCCData->custColors[i] = 0xC0C0C0;

						return TRUE;
	}

	case WM_DESTROY:
	{
					   pCCData = (CCStruct*)GetWindowLongPtr(hwnd, 0);

					   if (pCCData)
						   free(pCCData);

					   break;
	}

	case WM_PAINT:
	{
					 pCCData = (CCStruct*)GetWindowLongPtr(hwnd, 0);

					 PAINTSTRUCT ps;

					 HDC hDC = BeginPaint(hwnd, &ps);
					 DrawColorControl(hDC, pCCData);
					 EndPaint(hwnd, &ps);

					 break;
	}

	case WM_ENABLE:
	{
					  pCCData = (CCStruct*)GetWindowLongPtr(hwnd, 0);

					  pCCData->bDisabled = !wParam;

					  //redraw control
					  HDC hDC = GetDC(hwnd);
					  DrawColorControl(hDC, pCCData);
					  ReleaseDC(hwnd, hDC);

					  break;
	}

	case WM_LBUTTONDBLCLK:
	{
							 pCCData = (CCStruct*)GetWindowLongPtr(hwnd, 0);

							 if (pCCData->bDisabled)
								 break;

							 CHOOSECOLOR chooserData;
							 zero(&chooserData, sizeof(chooserData));

							 chooserData.lStructSize = sizeof(chooserData);
							 chooserData.hwndOwner = GetParent(hwnd);
							 chooserData.Flags = CC_RGBINIT | CC_FULLOPEN;
							 chooserData.rgbResult = pCCData->curColor;
							 chooserData.lpCustColors = customColors;

							 if (ChooseColor(&chooserData))
							 {
								 pCCData->curColor = chooserData.rgbResult;

								 HDC hDC = GetDC(hwnd);
								 DrawColorControl(hDC, pCCData);
								 ReleaseDC(hwnd, hDC);

								 HWND hwndParent = GetParent(hwnd);

								 DWORD controlID = (DWORD)GetWindowLongPtr(hwnd, GWLP_ID);
								 SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(controlID, CCN_CHANGED), (LPARAM)hwnd);
							 }

							 break;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}
bool bFirst = true;
bool STDCALL ConfigureDateTimeSource(Value& element, bool bCreating)
{
	if (bFirst)
	{
		bFirst = false;
		InitColorControl(g_histMian);
	}

	ConfigDateTimeSourceInfo configData(element);

	if (BLiveDialogBox(g_histMian, MAKEINTRESOURCE(IDD_DATETIME), GetMainWindow(), ConfigureDateTimeProc, (LPARAM)&configData) == IDOK)
	{
		element["cx"] = configData.cx;
		element["cy"]  = configData.cy;

		return true;
	}
	return false;
}


REGINST_CONFIGFUNC(ProcessDateTime, ConfigureDateTimeSource)

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
#if defined _M_X64 && _MSC_VER == 1800
		//workaround AVX2 bug in VS2013, http://connect.microsoft.com/VisualStudio/feedback/details/811093
		_set_FMA3_enable(0);
#endif
		g_histMian = hInst;
	}
	return TRUE;
}

