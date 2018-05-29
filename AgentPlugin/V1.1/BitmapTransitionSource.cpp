#include <time.h>
#include "BitmapImage.h"
#include "BitmapTransitionSource.h"
#include "resource.h"

extern HINSTANCE hinstMain;
extern LocaleStringLookup *pluginLocale;

WNDPROC AddBtnProc = NULL;
WNDPROC RemoveBtnProc = NULL;
WNDPROC MoveUpBtnProc = NULL;
WNDPROC MoveDownBtnProc = NULL;
WNDPROC OKProc = NULL;
WNDPROC CancelProc = NULL;
static HWND Hwnd = NULL;


IMPLEMENT_DYNIC(BitmapTransitionSource, "投影片放映", "V1.0.0.1")

struct ConfigBitmapTransitionInfo
{
	Value& data;
	ConfigBitmapTransitionInfo(Value &Data) :data(Data){}
};

BitmapTransitionSource::BitmapTransitionSource()
{
	D3DRender = GetD3DRender();
	bUpdate = true;
}

BitmapTransitionSource::~BitmapTransitionSource()
{
	for (UINT i = 0; i < bitmapImages.Num(); i++)
		delete bitmapImages[i];
}

bool BitmapTransitionSource::Init(Value &JsonParam)
{
	UpdateSettings(JsonParam);
	return true;
}

void BitmapTransitionSource::UpdateSettings(Value &JsonParam)
{
	this->data = JsonParam;
	bUpdate = true;
}


void BitmapTransitionSource::Preprocess()
{
	if (bUpdate)
	{
		Update();
		bUpdate = false;
	}

}

void BitmapTransitionSource::Update()
{
	for (UINT i = 0; i < bitmapImages.Num(); i++)
		delete bitmapImages[i];
	bitmapImages.Clear();

	//------------------------------------

	bool bFirst = true;
	StringList bitmapList;
	Json::Value &ArryList = data["BitmapList"];
	bitmapList.SetSize(ArryList.size());

	for (int i = 0; i < ArryList.size(); ++i)
	{
		bitmapList[i] = Asic2WChar(ArryList[i].asString().c_str()).c_str();
	}


	for (UINT i = 0; i < bitmapList.Num(); i++)
	{
		String &strBitmap = bitmapList[i];
		if (strBitmap.IsEmpty())
		{
			Log::writeError(LOG_RTSPSERV, 1, "BitmapTransitionSource::UpdateSettings: Empty path");
			continue;
		}

		if (!OSFileExists(strBitmap)) // 文件不存在
		{
			String info(strBitmap);
			info += L" file do not exist";
			Log::writeError(LOG_RTSPSERV, 1, "%s %s", __FUNCTION__, WcharToAnsi(info.Array()).c_str());
		}

		BitmapImage *bitmapImage = new BitmapImage;
		bitmapImage->SetPath(strBitmap);
		bitmapImage->EnableFileMonitor(false);
		bitmapImage->Init();

		if (bFirst)
		{
			fullSize = bitmapImage->GetSize();
			baseAspect = double(fullSize.x) / double(fullSize.y);
			bFirst = false;
		}

		bitmapImages << bitmapImage;
	}

	//------------------------------------

	transitionTime = data["transitionTime"].asDouble();
	if (transitionTime < MIN_TRANSITION_TIME)
		transitionTime = MIN_TRANSITION_TIME;
	else if (transitionTime > MAX_TRANSITION_TIME)
		transitionTime = MAX_TRANSITION_TIME;

	//------------------------------------

	if (data["fadeInOnly"].isNull())
	{
		data["fadeInOnly"] = 1;
	}

	bFadeInOnly = data["fadeInOnly"].asInt() != 0;
	bDisableFading = data["disableFading"].asInt() != 0;
	bRandomize = data["randomize"].asInt() != 0;

	//------------------------------------

	curTransitionTime = 0.0f;
	curTexture = 0;

	if (bRandomize)
	{
		srand((unsigned)time(NULL));
		if (bitmapImages.Num() > 1)
		{
			curTexture = lrand(bitmapImages.Num());
			while ((nextTexture = lrand(bitmapImages.Num())) == curTexture);
		}
	}
	else
		nextTexture = (curTexture == bitmapImages.Num() - 1) ? 0 : curTexture + 1;

	bTransitioning = false;
	curFadeValue = 0.0f;
}


void BitmapTransitionSource::Tick(float fSeconds)
{
	for (UINT i = 0; i<bitmapImages.Num(); i++)
		bitmapImages[i]->Tick(fSeconds);

	if (bTransitioning && bitmapImages.Num() > 1)
	{
		if (bDisableFading)
			curFadeValue = fadeTime;
		else
			curFadeValue += fSeconds;

		if (curFadeValue >= fadeTime)
		{
			curFadeValue = 0.0f;
			bTransitioning = false;

			if (bRandomize)
			{
				curTexture = nextTexture;
				while ((nextTexture = lrand(bitmapImages.Num())) == curTexture);
			}
			else
			{
				if (++curTexture == bitmapImages.Num())
					curTexture = 0;

				nextTexture = (curTexture == bitmapImages.Num() - 1) ? 0 : curTexture + 1;
			}
		}
	}

	curTransitionTime += fSeconds;
	if (curTransitionTime >= transitionTime)
	{
		curTransitionTime = 0.0f;

		curFadeValue = 0.0f;
		bTransitioning = true;
	}
}

void BitmapTransitionSource::DrawBitmap(UINT texID, float alpha, const Vect2 &startPos, const Vect2 &startSize, bool bScaleFull, Texture *FilterTexture)
{
	DWORD curAlpha = DWORD(alpha*255.0f);

	Vect2 pos = Vect2(0.0f, 0.0f);
	Vect2 size = fullSize;

	Vect2 itemSize = bitmapImages[texID]->GetSize();

	double sourceAspect = double(itemSize.x) / double(itemSize.y);
	if (!CloseDouble(baseAspect, sourceAspect))
	{
		if (baseAspect < sourceAspect)
			size.y = float(double(size.x) / sourceAspect);
		else
			size.x = float(double(size.y) * sourceAspect);

		pos = (fullSize - size)*0.5f;

		pos.x = (float)round(pos.x);
		pos.y = (float)round(pos.y);

		size.x = (float)round(size.x);
		size.y = (float)round(size.y);
	}

	pos /= fullSize;
	pos *= startSize;
	pos += startPos;
	Vect2 lr;
	lr = pos + (size / fullSize*startSize);

	if (bScaleFull)
	{
		if (FilterTexture)
		{
			D3DRender->DrawSprite(FilterTexture, (curAlpha << 24) | 0xFFFFFF, pos.x, pos.y, lr.x, lr.y);
		}
		else
			D3DRender->DrawSprite(bitmapImages[texID]->GetTexture(), (curAlpha << 24) | 0xFFFFFF, pos.x, pos.y, lr.x, lr.y);
	}
	else
	{
		Vect2 size;
		size.x = lr.x - pos.x;
		size.y = lr.y - pos.y;
		//等比例缩放
		float aspect = (float)size.x / size.y;
		int Width = itemSize.x;
		int Height = itemSize.y;

		float x, x2;
		float y, y2;

		if (aspect > 1.0f)
		{
			y = pos.y;
			y2 = y + size.y;

			float NewWidth = Width * size.y / Height;

			x = pos.x + (size.x - NewWidth) / 2;
			x2 = x + NewWidth;

			if (NewWidth > size.x)
			{
				x = pos.x;
				x2 = pos.x + size.x;

				float NewHeight = Height * size.x / Width;

				y = pos.y + (size.y - NewHeight) / 2;
				y2 = y + NewHeight;
			}
		}
		else
		{
			x = pos.x;
			x2 = pos.x + size.x;

			float NewHeight = Height * size.x / Width;

			y = pos.y + (size.y - NewHeight) / 2;
			y2 = y + NewHeight;

			if (NewHeight > size.y)
			{
				y = pos.y;
				y2 = y + size.y;

				float NewWidth = Width * size.y / Height;

				x = pos.x + (size.x - NewWidth) / 2;
				x2 = x + NewWidth;
			}
		}

		if (FilterTexture)
		{
			D3DRender->DrawSprite(FilterTexture, (curAlpha << 24) | 0xFFFFFF, x, y, x2, y2);
		}
		else
		{
			D3DRender->DrawSprite(bitmapImages[texID]->GetTexture(), (curAlpha << 24) | 0xFFFFFF, x, y, x2, y2);
		}
		
	}
}

void BitmapTransitionSource::Render(const Vect2 &pos, const Vect2& size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	if (bitmapImages.Num())
	{
		if (bTransitioning && bitmapImages.Num() > 1)
		{
			float curAlpha = MIN(curFadeValue / fadeTime, 1.0f);
			if (bFadeInOnly)
				DrawBitmap(curTexture, 1.0f, pos, size, bScaleFull,FilterTexture);
			else
				DrawBitmap(curTexture, 1.0f - curAlpha, pos, size, bScaleFull,FilterTexture);

			DrawBitmap(nextTexture, curAlpha, pos, size, bScaleFull,FilterTexture);
		}
		else
			DrawBitmap(curTexture, 1.0f, pos, size, bScaleFull,FilterTexture);
	}
}

Vect2 BitmapTransitionSource::GetSize() const
{
	return fullSize;
}

void BitmapTransitionSource::SetHasPreProcess(bool bHasPre)
{
	bHasPreProcess = bHasPre;
}

bool BitmapTransitionSource::GetHasPreProcess() const
{
	return bHasPreProcess;
}


INT_PTR CALLBACK BitmapTransButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
	else if (hwnd == GetDlgItem(Hwnd, IDC_ADD))
	{
		WndProcTem = AddBtnProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = CancelProc;
	}
	else if (Hwnd == GetDlgItem(Hwnd, IDC_REMOVE))
	{
		WndProcTem = RemoveBtnProc;
	}
	else if (Hwnd == GetDlgItem(Hwnd, IDC_MOVEUPWARD))
	{
		WndProcTem = MoveUpBtnProc;
	}
	else
	{
		WndProcTem = MoveDownBtnProc;
	}
	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigureBitmapTransitionProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;
	switch (message)
	{

	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwnd,-1, true);
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
	case  WM_DESTROY:
		if (HBrush)
			DeleteObject(HBrush);
		break;
	case WM_INITDIALOG:
	{

						  AddBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_ADD), GWLP_WNDPROC);
						  RemoveBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVE), GWLP_WNDPROC);
						  OKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  CancelProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
						  MoveUpBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_MOVEUPWARD), GWLP_WNDPROC);
						  MoveDownBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_MOVEDOWNWARD), GWLP_WNDPROC);

						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADD), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVE), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_MOVEUPWARD), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDC_MOVEDOWNWARD), GWLP_WNDPROC, (LONG_PTR)BitmapTransButtonProc);

						  ConfigBitmapTransitionInfo *configInfo = (ConfigBitmapTransitionInfo*)lParam;
						  SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
						  LocalizeWindow(hwnd, pluginLocale);

						  //--------------------------

						  HWND hwndTemp = GetDlgItem(hwnd, IDC_BITMAPS);

						  StringList bitmapList;

						  Json::Value &ArryList = configInfo->data["BitmapList"];
						  bitmapList.SetSize(ArryList.size());

						  for (int i = 0; i < ArryList.size(); ++i)
						  {
							  bitmapList[i] = Asic2WChar(ArryList[i].asString().c_str()).c_str();
						  }
						  for (UINT i = 0; i < bitmapList.Num(); i++)
						  {
							  CTSTR lpBitmap = bitmapList[i];

							  if (OSFileExists(lpBitmap))
								  SendMessage(hwndTemp, LB_ADDSTRING, 0, (LPARAM)lpBitmap);
						  }

						  //--------------------------

						  hwndTemp = GetDlgItem(hwnd, IDC_TRANSITIONTIME);

						  UINT transitionTime = configInfo->data["transitionTime"].asUInt();
						  SendMessage(hwndTemp, UDM_SETRANGE32, MIN_TRANSITION_TIME, MAX_TRANSITION_TIME);

						  if (!transitionTime)
							  transitionTime = 10;

						  SendMessage(hwndTemp, UDM_SETPOS32, 0, transitionTime);

						  EnableWindow(GetDlgItem(hwnd, IDC_REPLACE), FALSE);
						  EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
						  EnableWindow(GetDlgItem(hwnd, IDC_MOVEUPWARD), FALSE);
						  EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWNWARD), FALSE);

						  //--------------------------

						  if (configInfo->data["fadeInOnly"].isNull())
						  {
							  configInfo->data["fadeInOnly"] = 1;
						  }

						  BOOL bFadeInOnly = configInfo->data["fadeInOnly"].asInt();
						  BOOL bDisableFading = configInfo->data["disableFading"].asInt();
						  BOOL bRandomize = configInfo->data["randomize"].asInt();
						  SendMessage(GetDlgItem(hwnd, IDC_FADEINONLY), BM_SETCHECK, bFadeInOnly ? BST_CHECKED : BST_UNCHECKED, 0);
						  SendMessage(GetDlgItem(hwnd, IDC_DISABLEFADING), BM_SETCHECK, bDisableFading ? BST_CHECKED : BST_UNCHECKED, 0);
						  SendMessage(GetDlgItem(hwnd, IDC_RANDOMIZE), BM_SETCHECK, bRandomize ? BST_CHECKED : BST_UNCHECKED, 0);


						  EnableWindow(GetDlgItem(hwnd, IDC_FADEINONLY), !bDisableFading);

						  return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_ADD:
		{
						TSTR lpFile = (TSTR)Allocate_Bak(32 * 1024 * sizeof(TCHAR));
						zero(lpFile, 32 * 1024 * sizeof(TCHAR));

						OPENFILENAME ofn;
						zero(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.lpstrFile = lpFile;
						ofn.hwndOwner = hwnd;
						ofn.nMaxFile = 32 * 1024 * sizeof(TCHAR);
						ofn.lpstrFilter = TEXT("All Formats (*.bmp;*.dds;*.jpg;*.png;*.gif)\0*.bmp;*.dds;*.jpg;*.png;*.gif\0");
						ofn.nFilterIndex = 1;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

						TCHAR curDirectory[MAX_PATH + 1];
						GetCurrentDirectory(MAX_PATH, curDirectory);

						BOOL bOpenFile = GetOpenFileName(&ofn);

						TCHAR newDirectory[MAX_PATH + 1];
						GetCurrentDirectory(MAX_PATH, newDirectory);

						SetCurrentDirectory(curDirectory);

						if (bOpenFile)
						{
							TSTR lpCurFile = lpFile + ofn.nFileOffset;

							while (lpCurFile && *lpCurFile)
							{
								String strPath;
								strPath << newDirectory << TEXT("\\") << lpCurFile;

								SendMessage(GetDlgItem(hwnd, IDC_BITMAPS), LB_ADDSTRING, 0, (LPARAM)strPath.Array());

								lpCurFile += slen(lpCurFile) + 1;
							}
						}

						Free(lpFile);

						break;
		}

		case IDC_BITMAPS:
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				EnableWindow(GetDlgItem(hwnd, IDC_REPLACE), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_MOVEUPWARD), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWNWARD), TRUE);
			}
			break;

		case IDC_REMOVE:
		{
						   UINT curSel = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BITMAPS), LB_GETCURSEL, 0, 0);
						   if (curSel != LB_ERR)
						   {
							   SendMessage(GetDlgItem(hwnd, IDC_BITMAPS), LB_DELETESTRING, curSel, 0);
							   EnableWindow(GetDlgItem(hwnd, IDC_REPLACE), FALSE);
							   EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
							   EnableWindow(GetDlgItem(hwnd, IDC_MOVEUPWARD), FALSE);
							   EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWNWARD), FALSE);
						   }
		}
			break;

		case IDC_MOVEUPWARD:
		{
							   HWND hwndBitmaps = GetDlgItem(hwnd, IDC_BITMAPS);
							   UINT curSel = (UINT)SendMessage(hwndBitmaps, LB_GETCURSEL, 0, 0);
							   if (curSel != LB_ERR)
							   {
								   if (curSel > 0)
								   {
									   String strText = GetLBText(hwndBitmaps, curSel);

									   SendMessage(hwndBitmaps, LB_DELETESTRING, curSel, 0);
									   SendMessage(hwndBitmaps, LB_INSERTSTRING, --curSel, (LPARAM)strText.Array());
									   PostMessage(hwndBitmaps, LB_SETCURSEL, curSel, 0);
								   }
							   }
		}
			break;

		case IDC_MOVEDOWNWARD:
		{
								 HWND hwndBitmaps = GetDlgItem(hwnd, IDC_BITMAPS);

								 UINT numBitmaps = (UINT)SendMessage(hwndBitmaps, LB_GETCOUNT, 0, 0);
								 UINT curSel = (UINT)SendMessage(hwndBitmaps, LB_GETCURSEL, 0, 0);
								 if (curSel != LB_ERR)
								 {
									 if (curSel < (numBitmaps - 1))
									 {
										 String strText = GetLBText(hwndBitmaps, curSel);

										 SendMessage(hwndBitmaps, LB_DELETESTRING, curSel, 0);
										 SendMessage(hwndBitmaps, LB_INSERTSTRING, ++curSel, (LPARAM)strText.Array());
										 PostMessage(hwndBitmaps, LB_SETCURSEL, curSel, 0);
									 }
								 }
		}
			break;

		case IDC_DISABLEFADING:
		{
								  BOOL bDisableFading = SendMessage(GetDlgItem(hwnd, IDC_DISABLEFADING), BM_GETCHECK, 0, 0) == BST_CHECKED;
								  EnableWindow(GetDlgItem(hwnd, IDC_FADEINONLY), !bDisableFading);
		}
			break;

		case IDOK:
		{
					 HWND hwndBitmaps = GetDlgItem(hwnd, IDC_BITMAPS);

					 UINT numBitmaps = (UINT)SendMessage(hwndBitmaps, LB_GETCOUNT, 0, 0);
					 if (!numBitmaps)
					 {
						 BLiveMessageBox(hwnd, pluginLocale->LookupString(L"Sources.TransitionSource.Empty"), NULL, 0);
						 break;
					 }

					 //---------------------------

					 StringList bitmapList;
					 for (UINT i = 0; i < numBitmaps; i++)
						 bitmapList << GetLBText(hwndBitmaps, i);

					 ConfigBitmapTransitionInfo *configInfo = (ConfigBitmapTransitionInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

					 Json::Value &ArryList = configInfo->data["BitmapList"];
					 ArryList.resize(bitmapList.Num());

					 for (int i = 0; i < bitmapList.Num(); ++i)
					 {
						 ArryList[i] = WcharToAnsi(bitmapList[i].Array()).c_str();
					 }

					 UINT transitionTime = (UINT)SendMessage(GetDlgItem(hwnd, IDC_TRANSITIONTIME), UDM_GETPOS32, 0, 0);
					 configInfo->data["transitionTime"] = transitionTime;

					 BOOL bFadeInOnly = SendMessage(GetDlgItem(hwnd, IDC_FADEINONLY), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bDisableFading = SendMessage(GetDlgItem(hwnd, IDC_DISABLEFADING), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 BOOL bRandomize = SendMessage(GetDlgItem(hwnd, IDC_RANDOMIZE), BM_GETCHECK, 0, 0) == BST_CHECKED;
					 configInfo->data["fadeInOnly"] = bFadeInOnly;
					 configInfo->data["disableFading"] = bDisableFading;
					 configInfo->data["randomize"] = bRandomize;
		}

		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
		break;
	}

	return 0;
}

static bool bFirst = true;
bool STDCALL ConfigureBitmapTransitionSource(Value& element, bool bCreating)
{

	if (bFirst)
	{
		bFirst = false;
		InitColorControl(hinstMain);

		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(AGENTLOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AGENTLOCALPATH).c_str());
		}
	}

	ConfigBitmapTransitionInfo configInfo(element);

	if (BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIGURETRANSITIONSOURCE), GetMainWindow(), ConfigureBitmapTransitionProc, (LPARAM)&configInfo) == IDOK)
	{
		return true;
	}

	return false;
}

REGINST_CONFIGFUNC(BitmapTransitionSource, ConfigureBitmapTransitionSource)
