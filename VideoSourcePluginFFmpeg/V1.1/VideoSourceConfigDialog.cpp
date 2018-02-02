/**
* John Bradley (jrb@turrettech.com)
*/
#include "VideoSourceConfigDialog.h"
#include "WindowsHelper.h"
#include "VideoSourceConfig.h"
#include "resource.h"
#include <array>
#include "CGdiPlusImage.h"
#define HANDLE_DEFAULT default: return false
#define RGB102 RGB(102, 102, 102)
#define RGB255 RGB(255, 255, 255)
CGdiPlusImage* m_pCancleBtnGrayNor = NULL;
CGdiPlusImage* m_pCancleBtnGrayHover = NULL;
CGdiPlusImage* m_pOKBtnGrayNor = NULL;
CGdiPlusImage* m_pOKBtnGrayHover = NULL;
WNDPROC buttonproc = NULL;
WNDPROC canclebuttonproc = NULL;
WNDPROC resetvideofilterproc = NULL;
extern HINSTANCE HinstanceDLL;
extern LocaleStringLookup *pluginLocale;

HBRUSH HBrush;
HFONT  HFont20;
HFONT  HOldFont;

static INT_PTR CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static INT_PTR CALLBACK ResetVideoFilterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static INT_PTR CALLBACK CancleButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	static bool m_bMouseTrack = FALSE;
	
	if (!m_pCancleBtnGrayHover)
	{
		m_pCancleBtnGrayHover = new CGdiPlusImage;
		m_pCancleBtnGrayHover->LoadFromFile(L".\\img\\btn_gray_hover.png");
	}

	if (!m_pCancleBtnGrayNor)
	{
		m_pCancleBtnGrayNor = new CGdiPlusImage;
		m_pCancleBtnGrayNor->LoadFromFile(L".\\img\\btn_gray_nor.png");
	}
	
	if (!m_pOKBtnGrayHover)
	{
		m_pOKBtnGrayHover = new CGdiPlusImage;
		m_pOKBtnGrayHover->LoadFromFile(L".\\img\\btn_blue_hover.png");
	}

	if (!m_pOKBtnGrayNor)
	{
		m_pOKBtnGrayNor = new CGdiPlusImage;
		m_pOKBtnGrayNor->LoadFromFile(L".\\img\\btn_blue_nor.png");
	}


	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);
					 Graphics graphics(hdc);
					 INT nImageWidth = m_pCancleBtnGrayNor->GetWidth();
					 INT nImageHeight = m_pCancleBtnGrayNor->GetHeight();

					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
					 {
						 graphics.DrawImage(m_pOKBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 }
					 else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
					 {
						 graphics.DrawImage(m_pCancleBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
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
						  if (!m_bMouseTrack)
						  {
							  HDC hDC = GetDC(hwnd);
							  Graphics graphics(hDC);
							  INT nImageWidth = m_pCancleBtnGrayHover->GetWidth();
							  INT nImageHeight = m_pCancleBtnGrayHover->GetHeight();

							  //构造位置
							  RectF rcDrawRect;
							  rcDrawRect.X = (REAL)0;
							  rcDrawRect.Y = (REAL)0;
							  rcDrawRect.Width = (REAL)nImageWidth;
							  rcDrawRect.Height = (REAL)nImageHeight;

							  //绘画图像
							  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
							  {
								  graphics.DrawImage(m_pOKBtnGrayHover->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							  }
							  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
							  {
								  graphics.DrawImage(m_pCancleBtnGrayHover->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
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
						 TRACKMOUSEEVENT csTME;
						 csTME.cbSize = sizeof (csTME);
						 csTME.dwFlags = TME_LEAVE | TME_HOVER;
						 csTME.hwndTrack = hwnd;// 指定要 追踪 的窗口 
						 csTME.dwHoverTime = 10;  // 鼠标在按钮上停留超过 10ms ，才认为状态为 HOVER
						 ::_TrackMouseEvent(&csTME); // 开启 Windows 的 WM_MOUSELEAVE ， WM_MOUSEHOVER 事件支持

						 m_bMouseTrack = FALSE;   // 若已经 追踪 ，则停止 追踪 
						 break;
	}
	case WM_MOUSELEAVE:
	{
						 m_bMouseTrack = TRUE;
						  //改变状态
						  HDC hDC = GetDC(hwnd);
						  RECT meterGray;
						  HBRUSH  HBrushBackGround =  CreateSolidBrush(RGB(42, 42, 44));
						  meterGray.left = 0;
						  meterGray.right = 100;
						  meterGray.bottom = 0;
						  meterGray.top = 36;
						  FillRect(hDC, &meterGray, HBrushBackGround);
						  DeleteObject(HBrushBackGround);

						  Graphics graphics(hDC);
						  INT nImageWidth = m_pCancleBtnGrayNor->GetWidth();
						  INT nImageHeight = m_pCancleBtnGrayNor->GetHeight();

						  //构造位置
						  RectF rcDrawRect;
						  rcDrawRect.X = (REAL)0;
						  rcDrawRect.Y = (REAL)0;
						  rcDrawRect.Width = (REAL)nImageWidth;
						  rcDrawRect.Height = (REAL)nImageHeight;

						  //绘画图像
						  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
						  {
							  graphics.DrawImage(m_pOKBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						  }
						  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
						  {
							  graphics.DrawImage(m_pCancleBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
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
		return CallWindowProc(canclebuttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

INT_PTR CALLBACK Config_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SeniorConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL Config_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL SeniorConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void SeniorConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void Config_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
INT_PTR CALLBACK Config_OnNotify(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void Config_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
void Config_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void Config_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
void SeniorConfig_OnPaint(HWND hwnd);
void Config_OnPaint(HWND hwnd);
HBRUSH Config_OnEditChangeColor(HWND hwnd, HDC hDc, HWND lParam, UINT ID);
HBRUSH Config_OnStaticChangeColor(HWND hwnd, HDC hDc, HWND lParam, UINT ID);

std::array<String, 9> deinterlacing_modes = {TEXT("none"), TEXT("blend"), TEXT("bob"), TEXT("discard"), TEXT("linear"), TEXT("mean"), TEXT("x"), TEXT("yadif"), TEXT("yadif2x")};

bool DrawImagePng(HDC DC, Gdiplus::Image* pImage, INT nXPos, INT nYPos, INT Width, INT Height)
{
	//加载判断
	if (pImage == NULL) return false;

	//创建屏幕
	Graphics graphics(DC);

	//获取属性
	INT nImageWidth = pImage->GetWidth();
	INT nImageHeight = pImage->GetHeight();

	//构造位置
	RectF rcDrawRect;
	rcDrawRect.X = (REAL)nXPos;
	rcDrawRect.Y = (REAL)nYPos;
	rcDrawRect.Width = (REAL)Width;
	rcDrawRect.Height = (REAL)Height;

	//绘画图像
	graphics.DrawImage(pImage, rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
	return true;
}

VideoSourceConfigDialog::VideoSourceConfigDialog(VideoSourceConfig *config)
{
    this->config = config;

    playlistDropListener = new PlaylistDropListener(this);
    isDragging = false;
}

VideoSourceConfigDialog::~VideoSourceConfigDialog()
{
    delete playlistDropListener;
}

bool VideoSourceConfigDialog::Show()
{
	return BLiveDialogBox(HinstanceDLL, MAKEINTRESOURCE(IDD_VIDEOCONFIG), GetMainWindow(), Config_DlgProc, (LPARAM)this) == IDOK;
}

void VideoSourceConfigDialog::PlaylistFilesDropped(StringList &files)
{
    LVITEM item;
    item.mask       = LVIF_TEXT | LVIF_STATE;
    item.stateMask  = 0;
    item.iSubItem   = 0;
    item.state      = 0;

    int insertIndex = ListView_GetItemCount(hwndPlaylist);
    for(unsigned int i = 0; i < files.Num(); i++) {
        item.pszText = files[i];
        item.iItem = insertIndex++;
        ListView_InsertItem(hwndPlaylist, &item);
    }
}

INT_PTR CALLBACK Config_DlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
    switch(msg)
    {
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
		case WM_NCPAINT:
		{
		DrawFrame(hwndDlg, -1, true);
		}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg,GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg,-1, true);
	}
		break;
        HANDLE_MSG      (hwndDlg,   WM_INITDIALOG,  Config_OnInitDialog);
        HANDLE_MSG      (hwndDlg,   WM_COMMAND,     Config_OnCommand);
        HANDLE_MSG      (hwndDlg,   WM_LBUTTONUP,   Config_OnLButtonUp);
        HANDLE_MSG      (hwndDlg,   WM_MOUSEMOVE,   Config_OnMouseMove);
        HANDLE_MSG      (hwndDlg,   WM_ACTIVATE,    Config_OnActivate);
		HANDLE_MSG      (hwndDlg,   WM_PAINT,       Config_OnPaint);
		HANDLE_MSG      (hwndDlg, WM_CTLCOLOREDIT, Config_OnEditChangeColor);
		HANDLE_MSG		(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);
		
	
    case WM_NOTIFY: return Config_OnNotify(hwndDlg, msg, wParam, lParam);

        HANDLE_DEFAULT;	
    }
}

INT_PTR CALLBACK SeniorConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	static bool _bMouseTrack = true;
	switch (msg)
	{
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwndDlg,-1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg,GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg,-1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg, -1, true);
	}
		break;
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, SeniorConfig_OnInitDialog);
		HANDLE_MSG(hwndDlg, WM_COMMAND, SeniorConfig_OnCommand);
		HANDLE_MSG(hwndDlg, WM_PAINT, SeniorConfig_OnPaint);
		HANDLE_MSG(hwndDlg, WM_LBUTTONUP, Config_OnLButtonUp);
		HANDLE_MSG(hwndDlg, WM_MOUSEMOVE, Config_OnMouseMove);
		HANDLE_MSG(hwndDlg, WM_ACTIVATE, Config_OnActivate);
		HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);

	case WM_NOTIFY: return Config_OnNotify(hwndDlg, msg, wParam, lParam);
	case WM_CLOSE:
	{
					 EndDialog(hwndDlg, IDOK);
					 break;
	}

		HANDLE_DEFAULT;
	}
}

BOOL Config_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
    VideoSourceConfigDialog *_this = reinterpret_cast<VideoSourceConfigDialog *>(lParam);
    VideoSourceConfig *config = _this->GetConfig();

	LocalizeWindow(hwnd, pluginLocale);

    _this->hwndWidth                    = GetDlgItem(hwnd, IDC_WIDTH);
    _this->hwndHeight                   = GetDlgItem(hwnd, IDC_HEIGHT);
    _this->hwndVolume                   = GetDlgItem(hwnd, IDC_VOLUME);
    _this->hwndStretch                  = GetDlgItem(hwnd, IDC_STRETCH);
    _this->hwndIsAudioOutputToStream    = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TO_STREAM);
    _this->hwndIsAudioOutputToDevice    = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TO_DEVICE);
    _this->hwndAudioOutputType          = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TYPE);
    _this->hwndAudioOutputDevice        = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_DEVICE);
    _this->hwndMediaFileOrUrl           = GetDlgItem(hwnd, IDC_MEDIA_FILE_OR_URL);
    _this->hwndPlaylist                 = GetDlgItem(hwnd, IDC_PLAYLIST); 
    _this->hwndAddMedia                 = GetDlgItem(hwnd, IDC_ADD_MEDIA);
    _this->hwndRemoveMedia              = GetDlgItem(hwnd, IDC_REMOVE_MEDIA);
    _this->hwndPlaylistLooping          = GetDlgItem(hwnd, IDC_PLAYLIST_LOOP);
	_this->hwndSenceChangeStartPlay     = GetDlgItem(hwnd, ID_SENCECHANGEBEGINPLAY);
    _this->hwndDeinterlacing            = GetDlgItem(hwnd, IDC_DEINTERLACING);
    _this->hwndVideoFilter              = GetDlgItem(hwnd, IDC_VIDEOFILTER);
    _this->hwndApplyVideoFilter         = GetDlgItem(hwnd, IDC_APPLYVIDEOFILTER);
    _this->hwndVideoGamma               = GetDlgItem(hwnd, IDC_VIDEOGAMMA); 
    _this->hwndVideoContrast            = GetDlgItem(hwnd, IDC_VIDEOCONTRAST);
    _this->hwndVideoBrightness          = GetDlgItem(hwnd, IDC_VIDEOBRIGHTNESS);
    _this->hwndResetVideoFilter         = GetDlgItem(hwnd, IDC_RESETVIDEOFILTER);
	_this->hwndFileLoop = GetDlgItem(hwnd, IDC_RADIOFILE);
	_this->hwndPlayListLoopRadio = GetDlgItem(hwnd, IDC_RADIOLIST);
	_this->hwndHardwareDecode = GetDlgItem(hwnd, ID_HARDWAREDECODE);
	_this->hwndScanProgressive = GetDlgItem(hwnd, IDC_PROGRESSIVE);
	_this->hwndScanInterlace =   GetDlgItem(hwnd, IDC_INTERLACE);

	ListView_SetBkColor(_this->hwndPlaylist, RGB(35, 35, 37));
	ListView_SetTextBkColor(_this->hwndPlaylist, RGB(35, 35, 37));
	ListView_SetTextColor(_this->hwndPlaylist, RGB(255, 255, 255));

	SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, 353, 337, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, 200, 337, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	SetWindowPos(GetDlgItem(hwnd, IDC_ADD_MEDIA), NULL, 40, 17, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDC_REMOVE_MEDIA), NULL, 150, 17, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDC_REMOVEALLVIDEO), NULL, 260, 17, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDC_UPITEM), NULL, 370, 17, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDC_DOWNITEM), NULL, 480, 17, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	/*buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADD_MEDIA), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVE_MEDIA), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_BUTTON2), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_BUTTON3), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, 1), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, 2), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVEALLVIDEO), GWLP_WNDPROC, (LONG_PTR)ButtonProc);*/
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADD_MEDIA), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVE_MEDIA), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_REMOVEALLVIDEO), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_UPITEM), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_DOWNITEM), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);

	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);

	resetvideofilterproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_RESETVIDEOFILTER), GWLP_WNDPROC, (LONG_PTR)ResetVideoFilterProc);
	
    _this->playlistDropTarget           = DropTarget::RegisterDropWindow(_this->hwndPlaylist, _this->playlistDropListener);

    Edit_SetText(_this->hwndWidth,      IntString(config->width).Array());
    Edit_SetText(_this->hwndHeight,     IntString(config->height).Array());

    Button_SetCheck(_this->hwndStretch, config->isStretching);
 
	EnableWindow(_this->hwndVideoGamma, false);
	EnableWindow(_this->hwndVideoContrast, false);
	EnableWindow(_this->hwndVideoBrightness, false);
	EnableWindow(_this->hwndResetVideoFilter, true);

    Button_SetCheck(_this->hwndApplyVideoFilter, config->isApplyingVideoFilter);

    int index = -1;

    auto audioOutputTypes = config->GetAudioOutputTypes();

    for(auto i = audioOutputTypes.begin(); i < audioOutputTypes.end(); i++) {
        AudioOutputType &audioOutputType = *i;
        ComboBox_AddString(_this->hwndAudioOutputType, audioOutputType.GetDescription().Array());
        if (audioOutputType.GetName() == config->audioOutputType) {
            index = (int)(i - audioOutputTypes.begin());
        }
    }

    if (index < 0) {
        index = 0;                
    }

    ComboBox_SetCurSel(_this->hwndAudioOutputType, index);
    Button_SetCheck(_this->hwndIsAudioOutputToStream, config->isAudioOutputToStream);
    Button_SetCheck(_this->hwndIsAudioOutputToDevice, !config->isAudioOutputToStream);

	if (!config->isPlaylistLooping)
	{
		EnableWindow(_this->hwndFileLoop, false);
		EnableWindow(_this->hwndPlayListLoopRadio, false);
	}
	else
	{
		EnableWindow(_this->hwndFileLoop, true);
		EnableWindow(_this->hwndPlayListLoopRadio, true);
		Button_SetCheck(_this->hwndFileLoop, config->isFileLoop);
		Button_SetCheck(_this->hwndPlayListLoopRadio, config->isListLoop);
	}
	Button_SetCheck(_this->hwndPlaylistLooping, config->isPlaylistLooping);
	Button_SetCheck(_this->hwndSenceChangeStartPlay, config->isSenceChangeStartPlay);

	Button_SetCheck(_this->hwndHardwareDecode, config->isHardwareDecode);
	Button_SetCheck(_this->hwndScanProgressive, !config->bIsScanInterlace);
	Button_SetCheck(_this->hwndScanInterlace, config->bIsScanInterlace);

	HANDLE HMediaProcess = mp_create(0);
	if (mp_get_hardwaredecode(HMediaProcess))
	{
		Button_Enable(_this->hwndHardwareDecode, true);
	}
	else
	{
		Button_Enable(_this->hwndHardwareDecode, false);
		Static_SetText(GetDlgItem(hwnd, IDC_STATIC13), TEXT("您的显卡配置太低，不支持硬解码!"));
	}
	mp_destroy(HMediaProcess);

    _this->PlaylistFilesDropped(config->playlist);

    index = 0;
    for(auto i = deinterlacing_modes.begin(); i < deinterlacing_modes.end(); ++i) {
        ComboBox_AddString(_this->hwndDeinterlacing, *i);
        if(*i == config->deinterlacing) {
            index = i - deinterlacing_modes.begin();
        }
    }
    ComboBox_SetCurSel(_this->hwndDeinterlacing, index);

    FORWARD_WM_COMMAND(
        hwnd, 
        config->isAudioOutputToStream ? IDC_AUDIO_OUTPUT_TO_STREAM : IDC_AUDIO_OUTPUT_TO_DEVICE,
        config->isAudioOutputToStream ? _this->hwndIsAudioOutputToStream : _this->hwndIsAudioOutputToDevice,
        BN_CLICKED,
        SendMessage);
	//int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);//返回当前索引
	ListView_SetItemState(_this->hwndPlaylist, config->CurrentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	InvalidateRect(hwnd, NULL, FALSE);
	SendMessage(hwnd, WM_NCPAINT, 0, 0);

    return true;
}

BOOL SeniorConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	CSeniorVideoSourceConfigDialog *_this = reinterpret_cast<CSeniorVideoSourceConfigDialog *>(lParam);
	VideoSourceConfig *config = _this->GetConfig();

	
	_this->hwndVolume = GetDlgItem(hwnd, IDC_VOLUMESENIOR);
	_this->hwndVideoGamma = GetDlgItem(hwnd, IDC_VIDEOGAMMASENIOR);
	_this->hwndVideoContrast = GetDlgItem(hwnd, IDC_VIDEOCONTRASTSENIOR);
	_this->hwndVideoBrightness = GetDlgItem(hwnd, IDC_VIDEOBRIGHTNESSSENIOR);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDSENIOROK), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	Slider_SetRange(_this->hwndVolume, 0, 100);
	Slider_SetPos(_this->hwndVolume, config->volume);

	EnableWindow(_this->hwndVideoGamma, true);
	EnableWindow(_this->hwndVideoContrast, true);
	EnableWindow(_this->hwndVideoBrightness, true);


	Slider_SetRange(_this->hwndVideoGamma, 1, 1000);
	Slider_SetPos(_this->hwndVideoGamma, config->videoGamma);

	Slider_SetRange(_this->hwndVideoContrast, 0, 200);
	Slider_SetPos(_this->hwndVideoContrast, config->videoContrast);

	Slider_SetRange(_this->hwndVideoBrightness, 0, 200);
	Slider_SetPos(_this->hwndVideoBrightness, config->videoBrightness);

	return true;
}

void SeniorConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{ 
	CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	VideoSourceConfig *config = _this->GetConfig();
	switch (id)
	{
	
		case IDSENIOROK:
		{
						   config->volume = Slider_GetPos(_this->hwndVolume);
						   config->videoGamma = Slider_GetPos(_this->hwndVideoGamma);
						   config->videoContrast = Slider_GetPos(_this->hwndVideoContrast);
						   config->videoBrightness = Slider_GetPos(_this->hwndVideoBrightness);
					   EndDialog(hwnd, IDOK);
					   break;
		}
	}
}

void Config_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
    case IDC_AUDIO_OUTPUT_TO_DEVICE:
    case IDC_AUDIO_OUTPUT_TO_STREAM:
        {
            if (codeNotify == BN_CLICKED) {
                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                bool isAudioOutputToStream = id == IDC_AUDIO_OUTPUT_TO_STREAM;

                EnableWindow(_this->hwndAudioOutputType, !isAudioOutputToStream);
                EnableWindow(_this->hwndAudioOutputDevice, !isAudioOutputToStream);

                if (!isAudioOutputToStream) {
                    int index = ComboBox_GetCurSel(_this->hwndAudioOutputType);
                    if (index > 0) {
                        AudioOutputType &type = _this->GetConfig()->GetAudioOutputTypes()[index];
                        EnableWindow(_this->hwndAudioOutputDevice, (type.GetAudioOutputDevices().size()) ? true : false);
                    }
                }
            }
            break;
        }
    case IDC_AUDIO_OUTPUT_TYPE:
        {
            if (codeNotify == CBN_SELCHANGE) {

                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                int index = ComboBox_GetCurSel(_this->hwndAudioOutputType);

                ComboBox_ResetContent(_this->hwndAudioOutputDevice);

				EnableWindow(_this->hwndAudioOutputDevice, false);
            }
            break;
        }
    case IDC_ADD_MEDIA:
        {
			 bool bBreak =  true;
            if (codeNotify == BN_CLICKED) {				

                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                String newItem(GetEditText(_this->hwndMediaFileOrUrl));

                while(newItem.Length() && newItem[0] == L' ') {
                    newItem.RemoveChar(0);
                }

                if (newItem.Length()) {
                    //TODO: remove when fix is released
                    newItem.Array()[newItem.Length()] = 0;
                    StringList media;
                    media.Add(String(newItem));
                    _this->PlaylistFilesDropped(media);
                }
				else
				{
					bBreak = false;
				}
            }
			if (bBreak)
			{
				break;
			}            
        }
		case IDC_BROWSE_MEDIA:
        {
            if (codeNotify == BN_CLICKED) {
                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);


                //lololololol

                TSTR lpFile = (TSTR)Allocate_Bak(32*1024*sizeof(TCHAR));
                zero(lpFile, 32*1024*sizeof(TCHAR));

                OPENFILENAME ofn;
                zero(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFile = lpFile;
                ofn.hwndOwner = hwnd;
                ofn.nMaxFile = 32*1024*sizeof(TCHAR);

				ofn.lpstrFilter = TEXT("VSP Supported Files (*.anm;*.asf;*.avi;*.bik;*.dts;*.dxa;*.flv;*.fli;*.flc;*.flx;*.h261;*.h263;*.h264;*.m4v;*.mkv;*.mjp;*.mlp;*.mov;*.mp4;*.mp3;*.3gp;*.3g2;*.mj2;*.mvi;*.pmp;*.rm;*.rmvb;*.rpl;*.smk;*.swf;*.vc1;*.wmv;*.ts;*.vob;*.mts;*.m2ts;*.m2t;*.mpg;*.mxf;*.ogm;*.qt;*.tp;*.dvr-ms;*.amv)\0*.anm;*.asf;*.avi;*.bik;*.dts;*.dxa;*.flv;*.fli;*.flc;*.flx;*.h261;*.h263;*.h264;*.m4v;*.mkv;*.mjp;*.mlp;*.mov;*.mp4;*.mp3;*.3gp;*.3g2;*.mj2;*.mvi;*.pmp;*.rm;*.rmvb;*.rpl;*.smk;*.swf;*.vc1;*.wmv;*.ts;*.vob;*.mts;*.m2ts;*.m2t;*.mpg;*.mxf;*.ogm;*.qt;*.tp;*.dvr-ms;*.amv\0");
				ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

                TCHAR curDirectory[MAX_PATH+1];
                GetCurrentDirectory(MAX_PATH, curDirectory);

                BOOL bOpenFile = GetOpenFileName(&ofn);

                TCHAR newDirectory[MAX_PATH+1];
                GetCurrentDirectory(MAX_PATH, newDirectory);

                SetCurrentDirectory(curDirectory);
                StringList files;
                if(bOpenFile)
                {
                    TSTR lpCurFile = lpFile+ofn.nFileOffset;

                    while(lpCurFile && *lpCurFile)
                    {
                        String strPath;
                        strPath << newDirectory << TEXT("\\") << lpCurFile;
                        files.Add(strPath);
                        lpCurFile += slen(lpCurFile)+1;
                    }
                }

                _this->PlaylistFilesDropped(files);

                Free(lpFile);
            }
            break;
        }
    case IDC_REMOVE_MEDIA:
        {
            if (codeNotify == BN_CLICKED) {

                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                // Dropped item is selected?
                LVITEM lvi;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_STATE;
                lvi.stateMask = LVIS_SELECTED;

                int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
                while (position != -1) {
                    // Delete from original position
                    ListView_DeleteItem(_this->hwndPlaylist, position);
                    position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
                }
            }
            break;
        }
	case IDC_REMOVEALLVIDEO:
	{
							   if (codeNotify == BN_CLICKED) {

								   VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
								   ListView_DeleteAllItems(_this->hwndPlaylist);
							   }
							   break;
	}
	case IDC_UPITEM:
	{
						if (codeNotify == BN_CLICKED) {

							VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
							int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);//返回当前索引
							if (position == 0 || position == -1)
							{
								break;
							}
							LVITEM Upitem = { 0 };
							Upitem.iSubItem = 0;
							Upitem.mask = LVIF_TEXT;
							TCHAR UPbuf[MAX_PATH];
							Upitem.iItem = position;
							Upitem.iSubItem = 0;
							Upitem.cchTextMax = MAX_PATH;
							Upitem.pszText = UPbuf;
							ListView_GetItem(_this->hwndPlaylist, &Upitem);


							LVITEM Downitem = { 0 };
							Downitem.iSubItem = 0;
							Downitem.mask = LVIF_TEXT;
							TCHAR Downbuf[MAX_PATH];
							Downitem.iItem = position - 1;
							Downitem.iSubItem = 0;
							Downitem.cchTextMax = MAX_PATH;
							Downitem.pszText = Downbuf;
							ListView_GetItem(_this->hwndPlaylist, &Downitem);
							Downitem.iItem = position;
							ListView_SetItem(_this->hwndPlaylist, &Downitem);
							Upitem.iItem = position - 1;
							ListView_SetItem(_this->hwndPlaylist, &Upitem);
							ListView_SetItemState(_this->hwndPlaylist, position - 1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
							//PostMessage(hwnd, WM_NOTIFY, MAKEWPARAM(IDC_PLAYLIST, NM_CLICK), (LPARAM)GetDlgItem(hwnd, IDC_PLAYLIST));
						}
						break;
	}

	case IDC_DOWNITEM:
	{
						if (codeNotify == BN_CLICKED) {

							VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
							int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
							int itemCount = ListView_GetItemCount(_this->hwndPlaylist);
							if (position == itemCount - 1 || position == -1)
							{
								break;
							}
							LVITEM Upitem = { 0 };
							Upitem.iSubItem = 0;
							Upitem.mask = LVIF_TEXT;
							TCHAR UPbuf[MAX_PATH];
							Upitem.iItem = position + 1;
							Upitem.iSubItem = 0;
							Upitem.cchTextMax = MAX_PATH;
							Upitem.pszText = UPbuf;
							ListView_GetItem(_this->hwndPlaylist, &Upitem);


							LVITEM Downitem = { 0 };
							Downitem.iSubItem = 0;
							Downitem.mask = LVIF_TEXT;
							TCHAR Downbuf[MAX_PATH];
							Downitem.iItem = position;
							Downitem.iSubItem = 0;
							Downitem.cchTextMax = MAX_PATH;
							Downitem.pszText = Downbuf;
							ListView_GetItem(_this->hwndPlaylist, &Downitem);
							Downitem.iItem = position + 1;
							ListView_SetItem(_this->hwndPlaylist, &Downitem);
							Upitem.iItem = position;
							ListView_SetItem(_this->hwndPlaylist, &Upitem);

							ListView_SetItemState(_this->hwndPlaylist, position+1,   LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
							
						}
						break;
	}
    case IDC_APPLYVIDEOFILTER:
        {
            if (codeNotify == BN_CLICKED) {
                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
                bool isChecked = Button_GetCheck(_this->hwndApplyVideoFilter) == 1;
                EnableWindow(_this->hwndVideoGamma, isChecked);
                EnableWindow(_this->hwndVideoContrast, isChecked);
                EnableWindow(_this->hwndVideoBrightness, isChecked);
                EnableWindow(_this->hwndResetVideoFilter, isChecked);
            }
            break;
        }
	case IDC_PLAYLIST_LOOP:
	{
							  if (codeNotify == BN_CLICKED) {
								  VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
								  VideoSourceConfig *config = _this->GetConfig();
								  bool isChecked = Button_GetCheck(_this->hwndPlaylistLooping) == 1;
								  if (isChecked)
								  {
									  EnableWindow(_this->hwndFileLoop, true);
									  EnableWindow(_this->hwndPlayListLoopRadio, true);
									  Button_SetCheck(_this->hwndFileLoop, config->isFileLoop);
									  Button_SetCheck(_this->hwndPlayListLoopRadio, config->isListLoop);
								  }else {
									  Button_SetCheck(_this->hwndFileLoop, false);
									  Button_SetCheck(_this->hwndPlayListLoopRadio, false);
									  EnableWindow(_this->hwndFileLoop, false);
									  EnableWindow(_this->hwndPlayListLoopRadio, false);
								  }

							  }
							  break;
	}
    case IDC_RESETVIDEOFILTER:
        {
				CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
				BLiveDialogBox(HinstanceDLL, MAKEINTRESOURCE(IDD_DIALOGSENIOR), GetMainWindow(), SeniorConfig_DlgProc, (LPARAM)_this);
				/* Slider_SetPos(_this->hwndVideoGamma, 100);
				 Slider_SetPos(_this->hwndVideoContrast, 100);
				 Slider_SetPos(_this->hwndVideoBrightness, 100);*/
            break;
        }
    case IDOK:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
            VideoSourceConfig *config = _this->GetConfig();

// 			ConfigFile  *AppConfig = BLiveGetProfileConfig();
// 			config->height = AppConfig->GetInt(TEXT("Video"), TEXT("BaseHeight"), 0);
// 			config->width = AppConfig->GetInt(TEXT("Video"), TEXT("BaseWidth"), 0);
            config->deinterlacing = deinterlacing_modes[ComboBox_GetCurSel(_this->hwndDeinterlacing)];
            //config->volume = Slider_GetPos(_this->hwndVolume);
            config->isStretching = Button_IsChecked(_this->hwndStretch);
            config->isAudioOutputToStream = Button_IsChecked(_this->hwndIsAudioOutputToStream);

			config->isApplyingVideoFilter = true;//Button_IsChecked(_this->hwndApplyVideoFilter);
           /* config->videoGamma = Slider_GetPos(_this->hwndVideoGamma);
            config->videoContrast = Slider_GetPos(_this->hwndVideoContrast);
            config->videoBrightness = Slider_GetPos(_this->hwndVideoBrightness);*/

            int audioOutputTypeIndex = ComboBox_GetCurSel(_this->hwndAudioOutputType);
            int audioOutputDeviceIndex = ComboBox_GetCurSel(_this->hwndAudioOutputDevice);

            AudioOutputType &type = config->GetAudioOutputTypes()[audioOutputTypeIndex];
            //config->audioOutputType = type.GetName();
            //config->audioOutputTypeDevice = type.GetDeviceSetName();
            if (audioOutputDeviceIndex >= 0) {
                AudioOutputDevice &device = type.GetAudioOutputDevices()[audioOutputDeviceIndex];
                config->audioOutputDevice = device.GetLongName();
            }

            config->isPlaylistLooping = Button_IsChecked(_this->hwndPlaylistLooping);
			if (config->isPlaylistLooping)
			{
				config->isFileLoop = Button_IsChecked(_this->hwndFileLoop);
				config->isListLoop = Button_IsChecked(_this->hwndPlayListLoopRadio); 
			}
			config->isSenceChangeStartPlay = Button_IsChecked(_this->hwndSenceChangeStartPlay);
            config->playlist.Clear();
			config->isHardwareDecode = Button_IsChecked(_this->hwndHardwareDecode);

			config->bIsScanInterlace = Button_IsChecked(_this->hwndScanInterlace);

            int itemCount = ListView_GetItemCount(_this->hwndPlaylist);
            LVITEM item = { 0 };
            item.iSubItem = 0;
            item.mask = LVIF_TEXT;
            TCHAR buf[MAX_PATH];
			bool isSelect = false;
            for (int i = 0; i < itemCount; i++) {
                item.iItem = i;
                item.iSubItem = 0;
                item.cchTextMax = MAX_PATH;
                item.pszText = buf;
                ListView_GetItem(_this->hwndPlaylist, &item);
                config->playlist.Add(item.pszText);
				if (ListView_GetItemState(GetDlgItem(hwnd, IDC_PLAYLIST), i, LVIS_SELECTED) > 0)
				{
					config->CurrentIndex = i;
					config->isHaveSelect = true;
					isSelect = true;
				}
            }
			if (!isSelect)
			{
				config->CurrentIndex = 0;
				config->isHaveSelect = false;
			}

            DropTarget::UnregisterDropWindow(hwnd, _this->playlistDropTarget);
			if (m_pCancleBtnGrayHover)
			{
				delete m_pCancleBtnGrayHover;
				m_pCancleBtnGrayHover = NULL;
			}

			if (m_pCancleBtnGrayNor)
			{
				delete m_pCancleBtnGrayNor;
				m_pCancleBtnGrayNor = NULL;
			}

			if (m_pOKBtnGrayHover)
			{
				delete m_pOKBtnGrayHover;
				m_pOKBtnGrayHover = NULL;
			}

			if (m_pOKBtnGrayNor)
			{
				delete m_pOKBtnGrayNor;
				m_pOKBtnGrayNor = NULL;
			}
            EndDialog(hwnd, IDOK);
            break;
        }
    case IDCANCEL:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

            DropTarget::UnregisterDropWindow(hwnd, _this->playlistDropTarget);
			if (m_pCancleBtnGrayHover)
			{
				delete m_pCancleBtnGrayHover;
				m_pCancleBtnGrayHover = NULL;
			}

			if (m_pCancleBtnGrayNor)
			{
				delete m_pCancleBtnGrayNor;
				m_pCancleBtnGrayNor = NULL;
			}

			if (m_pOKBtnGrayHover)
			{
				delete m_pOKBtnGrayHover;
				m_pOKBtnGrayHover = NULL;
			}

			if (m_pOKBtnGrayNor)
			{
				delete m_pOKBtnGrayNor;
				m_pOKBtnGrayNor = NULL;
			}
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
}

void Config_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (!_this->isDragging) {
        return;
    }

    // End the drag-and-drop process
    _this->isDragging = false;

    ImageList_DragLeave(_this->hwndPlaylist);
    ImageList_EndDrag();
    ImageList_Destroy(_this->hDragImageList);

    ReleaseCapture();

    // Determine the dropped item
    LVHITTESTINFO lvhti;
    lvhti.pt.x = x;
    lvhti.pt.y = y;
    ClientToScreen(hwnd, &lvhti.pt);
    ScreenToClient(_this->hwndPlaylist, &lvhti.pt);
    ListView_HitTest(_this->hwndPlaylist, &lvhti);

    // Out of the ListView?
    if (lvhti.iItem == -1) {
        return;
    }

    // Not in an item?
    if (((lvhti.flags & LVHT_ONITEMLABEL) == 0) && 
        ((lvhti.flags & LVHT_ONITEMSTATEICON) == 0)) {
            return;
    }

    // Dropped item is selected?
    LVITEM lvi;
    lvi.iItem = lvhti.iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    ListView_GetItem(_this->hwndPlaylist, &lvi);

    if (lvi.state & LVIS_SELECTED) {
        return;
    }

    int dropIndex = lvi.iItem;

    // Rearrange the items
    int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
    TCHAR buf[MAX_PATH];
    while (position != -1) {
        // First, copy one item
        lvi.iItem = position;
        lvi.iSubItem = 0;
        lvi.cchTextMax = MAX_PATH;
        lvi.pszText = buf;
        lvi.stateMask = ~LVIS_SELECTED;
        lvi.mask = LVIF_STATE | LVIF_IMAGE 
            | LVIF_INDENT | LVIF_PARAM | LVIF_TEXT;

        ListView_GetItem(_this->hwndPlaylist, &lvi);

        lvi.iItem = lvhti.iItem;
        if (lvhti.iItem < position) {
            lvi.iItem = lvhti.iItem;
        } else {
            lvi.iItem = lvhti.iItem + 1;
        }

        // Insert the main item
        int insertedItemIdex = ListView_InsertItem(_this->hwndPlaylist, &lvi);
        if (lvi.iItem < position) {
            lvhti.iItem++;
        }
        if (insertedItemIdex <= position) {
            position++;
        }

        // Delete from original position
        ListView_DeleteItem(_this->hwndPlaylist, position);
        position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
    }

}

void Config_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (!_this->isDragging) {
        return;
    }

    POINT p;
    p.x = x;
    p.y = y;

    ClientToScreen(hwnd, &p);
    ImageList_DragMove(p.x, p.y);
}

INT_PTR CALLBACK Config_OnNotify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_BEGINDRAG:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

            // You can set your customized cursor here
            POINT p;
            p.x = 8;
            p.y = 8;

            // Ok, now we create a drag-image for all selected items
            bool isFirst = true;
            int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
            int totalHeight;

            IMAGEINFO imf;
            HIMAGELIST hOneImageList;
            HIMAGELIST hTempImageList;

            while (position != -1) {
                if (isFirst) {
                    // For the first selected item,
                    // we simply create a single-line drag image
                    _this->hDragImageList = ListView_CreateDragImage(_this->hwndPlaylist, position, &p);
                    ImageList_GetImageInfo(_this->hDragImageList, 0, &imf);
                    totalHeight = imf.rcImage.bottom;
                    isFirst = false;
                } else {
                    // For the rest selected items,
                    // we create a single-line drag image, then
                    // append it to the bottom of the complete drag image
                    hOneImageList = ListView_CreateDragImage(_this->hwndPlaylist, position, &p);
                    hTempImageList = ImageList_Merge(_this->hDragImageList, 
                        0, hOneImageList, 0, 0, totalHeight);
                    ImageList_Destroy(_this->hDragImageList);
                    ImageList_Destroy(hOneImageList);
                    _this->hDragImageList = hTempImageList;
                    ImageList_GetImageInfo(_this->hDragImageList, 0, &imf);
                    totalHeight = imf.rcImage.bottom;
                }
                position = ListView_GetNextItem(_this->hwndPlaylist, position, LVNI_SELECTED);
            }

            // Now we can initialize then start the drag action
            ImageList_BeginDrag(_this->hDragImageList, 0, 0, 0);

            POINT pt;
            pt = ((NM_LISTVIEW*) ((LPNMHDR)lParam))->ptAction;
            ClientToScreen(_this->hwndPlaylist, &pt);

            ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);

            _this->isDragging = true;

            // Don't forget to capture the mouse
            SetCapture(hwnd);

            break;
        }
    }

    return FALSE;
}

void Config_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (state == WA_INACTIVE) {
        if (_this && _this->isDragging) {
            _this->isDragging = false;
            ImageList_DragLeave(_this->hwndPlaylist);
            ImageList_EndDrag();
            ImageList_Destroy(_this->hDragImageList);
            ReleaseCapture();
        }
    }
}

void Config_OnPaint(HWND hwnd) {

	VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;

	HBRUSH hBK_42 = CreateSolidBrush(RGB(42, 42, 44));
	RECT rect;
	
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_42);

	HPEN hPen;
	HPEN hPenOld;
	hPen = CreatePen(PS_SOLID, 1, (COLORREF)0x5f5f61);
	hPenOld = (HPEN)SelectObject(hDC, hPen);
	MoveToEx(hDC, 160, 287, NULL);
	LineTo(hDC, 160, 330);
	MoveToEx(hDC, 160, 287, NULL);
	LineTo(hDC, 175, 287);
	MoveToEx(hDC, 160, 330, NULL);
	LineTo(hDC, 360, 330);
	MoveToEx(hDC, 360, 330, NULL);
	LineTo(hDC, 360, 287);
	MoveToEx(hDC, 360, 287, NULL);
	LineTo(hDC, 200, 287);

	MoveToEx(hDC, 160 + 215, 287, NULL);
	LineTo(hDC, 160 + 215, 330);
	MoveToEx(hDC, 160 + 215, 287, NULL);
	LineTo(hDC, 175 + 215, 287);
	MoveToEx(hDC, 160 + 215, 330, NULL);
	LineTo(hDC, 360 + 215, 330);
	MoveToEx(hDC, 360 + 215, 330, NULL);
	LineTo(hDC, 360 + 215, 287);
	MoveToEx(hDC, 360 + 215, 287, NULL);
	LineTo(hDC, 200 + 215, 287);

	SelectObject(hDC, hPenOld);
	DeleteObject(hPen);

	EndPaint(hwnd, &ps);

	DeleteObject(hBK_42);
	
}

void SeniorConfig_OnPaint(HWND hwnd) {

	CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;

	HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
	RECT rect;
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_153);
	EndPaint(hwnd, &ps);
	DeleteObject(hBK_153);
	
}

HBRUSH Config_OnEditChangeColor(HWND hwnd, HDC wParam, HWND lParam, UINT ID) {
	
	HDC hdc = (HDC)wParam;
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkColor(hdc, RGB(121, 121, 121));
	//HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	//HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
	if (HBrush)
	{
		DeleteObject(HBrush);
	}
	HBrush = CreateSolidBrush(RGB(121, 121, 121));
	return HBrush;

}

HBRUSH Config_OnStaticChangeColor(HWND hwnd, HDC wParam, HWND lParam, UINT ID){

	if (HWND(lParam) == GetDlgItem(hwnd, IDC_STATICLOOP))
	{
		HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
		HFONT hfontOld = (HFONT)SelectObject((HDC)wParam, hFont);

		SetTextColor((HDC)wParam, RGB(0, 0, 0));
		SetBkColor((HDC)wParam, RGB(42, 42, 44));
		HBrush = CreateSolidBrush(RGB(42, 42, 44));
	}
	else
	{
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, RGB(42, 42, 44));
		//HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
								OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
		//HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
		if (HBrush)
		{
			DeleteObject(HBrush);
		}
		HBrush = CreateSolidBrush(RGB(42, 42, 44));
	}
	return HBrush;

}

static INT_PTR CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	
	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
					 HBRUSH hBK_57 = CreateSolidBrush(RGB(57, 57, 59));
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(54, 54, 57));
					 HFONT  m_hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(255, 255, 255));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);

					 String DB_Tianjia = WndTitle;
					
					 if (DB_Tianjia.Length() > 2)
					 {
						 TextOut(hDC, 16, 6, DB_Tianjia, DB_Tianjia.Length());
					 }
					 else
					 {
						 TextOut(hDC, 28, 6, DB_Tianjia, DB_Tianjia.Length());
					 }
					 
					 
					 
					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 DeleteObject(hBK_153);
					 DeleteObject(hBK_102);
					 DeleteObject(hBK_57);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(buttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

static INT_PTR CALLBACK ResetVideoFilterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
					 HBRUSH hBK_57 = CreateSolidBrush(RGB(57, 57, 59));
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(102, 102, 102));
					 HFONT  m_hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(255, 255, 255));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);

					 String DB_Tianjia = WndTitle;
					 TextOut(hDC, 16, 6, DB_Tianjia, DB_Tianjia.Length());
					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 DeleteObject(hBK_153);
					 DeleteObject(hBK_102);
					 DeleteObject(hBK_57);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(resetvideofilterproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

//DirectPlayConfigDialog
INT_PTR CALLBACK DircetPlay_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL DirectPlay_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void DirectPlay_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

INT_PTR CALLBACK DircetPlay_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
	switch (msg)
	{
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwndDlg, -1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg,GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg,-1, true);
	}
		break;
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, DirectPlay_OnInitDialog);
		HANDLE_MSG(hwndDlg, WM_COMMAND, DirectPlay_OnCommand);
		HANDLE_MSG(hwndDlg, WM_LBUTTONUP, Config_OnLButtonUp);
		HANDLE_MSG(hwndDlg, WM_MOUSEMOVE, Config_OnMouseMove);
		HANDLE_MSG(hwndDlg, WM_ACTIVATE, Config_OnActivate);
		HANDLE_MSG(hwndDlg, WM_PAINT, Config_OnPaint);
		HANDLE_MSG(hwndDlg, WM_CTLCOLOREDIT, Config_OnEditChangeColor);
		HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);


	case WM_NOTIFY: 

		if (IDC_PLAYLIST == wParam)
		{
			
			NMHDR* pHeader = (NMHDR*)lParam;
			if (NM_DBLCLK == pHeader->code)
			{
				DirectPlayConfigDialog *_this = (DirectPlayConfigDialog *)GetWindowLongPtr(hwndDlg, DWLP_USER);
				VideoSourceConfig *config = _this->GetConfig();
				int itemCount = ListView_GetItemCount(_this->hwndPlaylist);
				LVITEM item = { 0 };
				item.iSubItem = 0;
				item.mask = LVIF_TEXT;
				TCHAR buf[MAX_PATH];
				config->playlist.Clear();
				bool hasSelect = false;
				for (int i = 0; i < itemCount; i++) {
					item.iItem = i;
					item.iSubItem = 0;
					item.cchTextMax = MAX_PATH;
					item.pszText = buf;
					ListView_GetItem(_this->hwndPlaylist, &item);
					config->playlist.Add(item.pszText);
					if (ListView_GetItemState(_this->hwndPlaylist, i, LVIS_SELECTED) > 0)
					{
						config->CurrentIndex = i;
						config->isHaveSelect = true;
						hasSelect = true;
					}
				}
				if (!hasSelect)
				{
					config->isHaveSelect = false;
					break;
				}
				EndDialog(hwndDlg, LOWORD(wParam));
				break;
			}
		}
		break;

		HANDLE_DEFAULT;
	}
}

BOOL DirectPlay_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	DirectPlayConfigDialog *_this = reinterpret_cast<DirectPlayConfigDialog *>(lParam);
	VideoSourceConfig *config = _this->GetConfig();
	_this->hwndPlaylist = GetDlgItem(hwnd, IDC_PLAYLIST);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	canclebuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)CancleButtonProc);
	ListView_SetBkColor(_this->hwndPlaylist, RGB(102, 102, 102));
	ListView_SetTextBkColor(_this->hwndPlaylist, RGB(102, 102, 102));
	ListView_SetTextColor(_this->hwndPlaylist, RGB(255, 255, 255));
	_this->PlaylistFilesDropped(config->playlist);
	ListView_SetItemState(_this->hwndPlaylist, config->CurrentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	return TRUE;
}

void DirectPlay_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{

	case IDOK:
	{
				 DirectPlayConfigDialog *_this = (DirectPlayConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
				 VideoSourceConfig *config = _this->GetConfig();
				 int itemCount = ListView_GetItemCount(_this->hwndPlaylist);
				 LVITEM item = { 0 };
				 item.iSubItem = 0;
				 item.mask = LVIF_TEXT;
				 TCHAR buf[MAX_PATH];
				 config->playlist.Clear();
				 bool hasSelect = false;
				 for (int i = 0; i < itemCount; i++) {
					 item.iItem = i;
					 item.iSubItem = 0;
					 item.cchTextMax = MAX_PATH;
					 item.pszText = buf;
					 ListView_GetItem(_this->hwndPlaylist, &item);
					 config->playlist.Add(item.pszText);
					 if (ListView_GetItemState(GetDlgItem(hwnd, IDC_PLAYLIST), i, LVIS_SELECTED) > 0)
					 {
						 config->CurrentIndex = i;
						 config->isHaveSelect = true;
						 hasSelect = true;
					 }
				 }
				 if (!hasSelect)
				 {
					 config->isHaveSelect = false;
				 }

				 EndDialog(hwnd, IDOK);
				 break;
	}
	case IDCANCEL:
	{
					 EndDialog(hwnd, IDCANCEL);
					 break;
	}
	}
}

DirectPlayConfigDialog::DirectPlayConfigDialog(VideoSourceConfig *config)
{
	this->config = config;
}

DirectPlayConfigDialog::~DirectPlayConfigDialog()
{}

bool DirectPlayConfigDialog::Show()
{
	BLiveDialogBox(HinstanceDLL, MAKEINTRESOURCE(IDD_DIRECTPLAY), GetMainWindow(), DircetPlay_DlgProc, (LPARAM)this);
	return  true;
}

void DirectPlayConfigDialog::PlaylistFilesDropped(StringList &files)
{
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_STATE;
	item.stateMask = 0;
	item.iSubItem = 0;
	item.state = 0;

	int insertIndex = ListView_GetItemCount(hwndPlaylist);
	for (unsigned int i = 0; i < files.Num(); i++) {
		item.pszText = files[i];
		item.iItem = insertIndex++;
		ListView_InsertItem(hwndPlaylist, &item);
	}
}

//文件不支持类型提示
WNDPROC filehintbuttonproc = NULL;
INT_PTR CALLBACK FileHint_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FileHint_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void FileHint_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static INT_PTR CALLBACK FileHintButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void FileHint_OnPaint(HWND hwnd);


INT_PTR CALLBACK FileHint_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwndDlg, -1, true);
	}
		return TRUE;
	case WM_CLOSE:
	{
					 EndDialog(hwndDlg,0);
					 break;
	}
	case WM_TIMER:
	{
					// KillTimer(hwndDlg,1);
					 EndDialog(hwndDlg, 0);
					 break;
	}
	
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, FileHint_OnInitDialog);
		HANDLE_MSG(hwndDlg, WM_COMMAND, FileHint_OnCommand);
		HANDLE_MSG(hwndDlg, WM_PAINT, FileHint_OnPaint);
		HANDLE_MSG(hwndDlg, WM_CTLCOLOREDIT, Config_OnEditChangeColor);
		HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);

		HANDLE_DEFAULT;
	}
	//return false;
}

BOOL FileHint_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	CFileNotSupportHintDialog *_this = reinterpret_cast<CFileNotSupportHintDialog *>(lParam);
	_this->m_hwnd = hwnd;
	SetWindowPos(hwnd, NULL, 5,  + 260 * _this->index, 400, 250, 0);
	filehintbuttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)FileHintButtonProc);
	SetTimer(hwnd, 1, 3000,NULL);
	return TRUE;
}

void FileHint_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{

		case IDOK:
		{
					 EndDialog(hwnd, IDOK);
					 break;
		}
	}
}

static INT_PTR CALLBACK FileHintButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {


	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(2, 120, 147));
					 HFONT  m_hFont = CreateFont(25, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(255, 255, 255));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);

					 String DB_Tianjia = WndTitle;
					 TextOut(hDC, 40,10, DB_Tianjia, DB_Tianjia.Length());

					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 
					 DeleteObject(hBK_102);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(filehintbuttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

void FileHint_OnPaint(HWND hwnd) {
	CFileNotSupportHintDialog *_this = (CFileNotSupportHintDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;

	HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
	RECT rect;
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_153);


	HFONT  m_hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkMode(hDC, TRANSPARENT);
	HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

	rect.bottom = rect.bottom / 2;
	String DB_Tianjia = L"文件打开失败";
	DB_Tianjia = DB_Tianjia + _this->FileName + L"为您自动播放下一个文件";
	DrawText(hDC, DB_Tianjia, DB_Tianjia.Length(), &rect, DT_WORDBREAK | DT_LEFT);
	

	SelectObject(hDC, m_hOldFont);
	EndPaint(hwnd, &ps);

	EndPaint(hwnd, &ps);
	DeleteObject(hBK_153);
	DeleteObject(m_hFont);
	DeleteObject(m_hOldFont);


}


CFileNotSupportHintDialog::CFileNotSupportHintDialog()
{
	;
}

CFileNotSupportHintDialog::~CFileNotSupportHintDialog()
{
}

bool CFileNotSupportHintDialog::Show()
{
	return BLiveDialogBox(HinstanceDLL, MAKEINTRESOURCE(IDD_FILEHINT), GetMainWindow(), FileHint_DlgProc, (LPARAM)this);
}
